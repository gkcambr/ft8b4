/*
 * Copyright (C) 2023 G. Keith Cambron
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
 * Sections of this code are taken from:
 * <https://tldp.org/HOWTO/NCURSES-Programming-HOWTO/windows.html/>
 *  
 * File:   LogView.cpp
 * Author: keithc
 * 
 * Created on March 16, 2023, 8:16 AM
 */

#include <vector>
#include <sstream>
#include <iostream>
#include <string>
#include <iostream>
#include <cstring>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include "logView.h"
#include "proxyChannel.h"

const short PAIR_CQ = 1;
const short PAIR_QSO_OLD = 2;
const short PAIR_QSO_RECENT = 3;
const short PAIR_QSO_OTHER = 4;
const short PAIR_INFO = 5;
const short PAIR_CMD = 6;

LogView *LogView::_instance;
int LogView::_cols;
int LogView::_rows;
int LogView::_buttonSpace;
bool LogView::_showDupes;
WINDOW *LogView::_qsoWin;
WINDOW *LogView::_infoWin;
WINDOW *LogView::_cmdWin;

const int seconds_per_day = 60 * 60 * 24;
const int SIZE_OF_INFO_WIN = 5;
const int SIZE_OF_CMD_WIN = 3;
const int NO_CMDS = 3;
const int SIZE_OF_CMD_BUTTON = 10;
const int DISPLAY_LIST_SIZE = 50;

void destroy_win(WINDOW *local_win) {
  /* box(local_win, ' ', ' '); : This won't produce the desired
   * result of erasing the window. It will leave it's four corners 
   * and so an ugly remnant of window. 
   */
  wborder(local_win, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
  /* The parameters taken are 
   * 1. win: the window on which to operate
   * 2. ls: character to be used for the left side of the window 
   * 3. rs: character to be used for the right side of the window 
   * 4. ts: character to be used for the top side of the window 
   * 5. bs: character to be used for the bottom side of the window 
   * 6. tl: character to be used for the top left corner of the window 
   * 7. tr: character to be used for the top right corner of the window 
   * 8. bl: character to be used for the bottom left corner of the window 
   * 9. br: character to be used for the bottom right corner of the window
   */
  wrefresh(local_win);
  delwin(local_win);
}

WINDOW *create_newwin(int height, int width, int starty, int startx) {
  WINDOW *local_win;

  local_win = newwin(height, width, starty, startx);
  box(local_win, 0, 0); /* 0, 0 gives default characters 
					 * for the vertical and horizontal
					 * lines			*/
  wrefresh(local_win); /* Show that box 		*/

  return local_win;
}

FILE *keyboard = fopen("keyboard.info", "w");

LogView::LogView(ProxyChannel *channel) {

  _channel = channel;
  _showDupes = true;
  _currentMode = "";
  _currentBand = "";
  _infoWin = NULL;
  _qsoWin = NULL;
  _cmdWin = NULL;
  _instance = this;
  pthread_mutex_init(&_listMutex, NULL);
  pthread_mutex_unlock(&_listMutex);
  signal(SIGWINCH, handleResizing);
  initscr();
  cbreak();
  noecho();
  intrflush(stdscr, FALSE);
  keypad(stdscr, TRUE);
  scrollok(stdscr, true);
  idlok(stdscr, true);
  curs_set(0); // hide the cursor
  refresh();
  getmaxyx(stdscr, _rows, _cols);
  _curX = _curY = 0;
  if (!has_colors()) {
    cout << "This terminal is not equipped for color. The Application will terminate.\n";
    exit(1);
  }
  start_color();
  init_pair(PAIR_CQ, COLOR_WHITE, COLOR_GREEN);
  init_pair(PAIR_QSO_OLD, COLOR_BLACK, COLOR_WHITE);
  init_pair(PAIR_QSO_RECENT, COLOR_WHITE, COLOR_RED);
  init_pair(PAIR_QSO_OTHER, COLOR_YELLOW, COLOR_GREEN);
  init_pair(PAIR_INFO, COLOR_BLACK, COLOR_WHITE);
  init_pair(PAIR_CMD, COLOR_WHITE, COLOR_BLUE);

  raise(SIGWINCH);

  // raise(SIGWINCH);
}

LogView::LogView(const LogView& orig) {
}

LogView::~LogView() {
  pthread_mutex_unlock(&_listMutex);
  pthread_mutex_destroy(&_listMutex);
  curs_set(1); // restore the cursor
  endwin();
}

int LogView::displayList() {
  int msgCnt = 0;

  werase(_qsoWin);
  my_wrefresh(_qsoWin);

  pthread_mutex_lock(&_listMutex);
  std::vector<std::pair < std::string, LogMessage*>>::reverse_iterator
  rit = _displayList.rbegin();
  while (rit != _displayList.rend()) {
    std::pair<std::string, LogMessage*> checkEntry = *rit;
    LogMessage *msg = checkEntry.second;
    updateDisplay(msg, false);
    rit++;
    msgCnt += 1;
  }
  pthread_mutex_unlock(&_listMutex);

  return msgCnt;
}

void LogView::updateDisplay(LogMessage *msg, bool updateList) {
  char msgStr[200];
  std::vector<std::string>::iterator it;
  std::vector<std::string> previousQsos;
  bool hasQsos = false;

  // check for previous QSOs
  std::string callsign = msg->getCallSign();
  if (callsign.length() > 0) {
    std::string cs(callsign);
    previousQsos = _sql3Db->checkLog(cs);
    it = previousQsos.begin();
    if (it != previousQsos.end()) {
      hasQsos = true;
    }
  }

  memset(msgStr, 0, 200);
  updateFreqMode(msg);
  if (!hasQsos) {
    wattrset(_qsoWin, COLOR_PAIR(PAIR_CQ));
    snprintf(msgStr, 200, "%s %d\n", msg->getMsgStr().c_str(), msg->getSNR());
    int row, col;
    getyx(stdscr, row, col);
    if ((row - _rows) >= 0) {
      scrl(1);
    }
    my_waddstr(_qsoWin, msgStr);
    wattroff(_qsoWin, COLOR_PAIR(PAIR_CQ));
    my_wrefresh(_qsoWin);
  } else {
    // there are associated qsos
    wattrset(_qsoWin, COLOR_PAIR(PAIR_QSO_OLD));
    snprintf(msgStr, 200, "%s %d\n", msg->getMsgStr().c_str(), msg->getSNR());
    my_waddstr(_qsoWin, msgStr);
    wattroff(_qsoWin, COLOR_PAIR(PAIR_QSO_OLD));
    my_wrefresh(_qsoWin);
    for (it = previousQsos.begin(); it != previousQsos.end(); ++it) {
      strncpy(msgStr, "   QSO: ", 199);
      strcat(msgStr, (*it).c_str());
      strcat(msgStr, "\n");
      std::vector<std::string> list = LogView::parseStr(*it, ' ');
      bool cqRecent = false;
      if (list.size() == 5) {
        std::string mode = list[3];
        std::string band = list[4];
        // compare the mode and bands
        if (mode.compare(_currentMode) == 0 && band.compare(_currentBand) == 0) {
          cqRecent = true;
          // check for skip days
          std::vector<std::string> qsoList = parseStr(*it, ' ');
          if (qsoList.size() == 5) {
            std::vector<std::string> dateList = LogView::parseStr(qsoList[1], '-');
            if (dateList.size() == 3) {
              struct tm qsoTime;
              memset(&qsoTime, 0, sizeof (struct tm));
              qsoTime.tm_year = atoi(dateList[0].c_str()) - 1900;
              qsoTime.tm_mon = atoi(dateList[1].c_str()) - 1;
              qsoTime.tm_mday = atoi(dateList[2].c_str());
              time_t then = mktime(&qsoTime);
              time_t today = time(NULL);
              int days = (today - then) / seconds_per_day;
              if (days > _skipDays) {
                cqRecent = false;
              }
            }
          }
        }
      }
      if (cqRecent) {
        wattrset(_qsoWin, COLOR_PAIR(PAIR_QSO_RECENT));
        my_waddstr(_qsoWin, msgStr);
        wattroff(_qsoWin, COLOR_PAIR(PAIR_QSO_RECENT));
        my_wrefresh(_qsoWin);
      } else {
        wattrset(_qsoWin, COLOR_PAIR(PAIR_QSO_OLD));
        my_waddstr(_qsoWin, msgStr);
        wattroff(_qsoWin, COLOR_PAIR(PAIR_QSO_OLD));
        my_wrefresh(_qsoWin);
      }
    }
  }
  // insert the CQ message in the display list
  if (updateList) {
    push_list(msg);
  }
}

void LogView::processMessage(LogMessage *msg) {
  char msgStr[200];

  memset(msgStr, 0, 200);
  if (msg->getType() == LogMessage::MSG_TYPE_LOGGED_ADIF) {
    _sql3Db->loadRecord(msg);
    wattrset(_infoWin, COLOR_PAIR(PAIR_INFO));
    snprintf(msgStr, 200, "logged: %s\n", msg->getMsgStr().c_str());
    my_waddstr(_infoWin, msgStr);
    my_wrefresh(_infoWin);
    wattroff(_infoWin, COLOR_PAIR(PAIR_INFO));
  } else if (msg->getType() == LogMessage::MSG_TYPE_STATUS) {
    updateFreqMode(msg);
  } else if (msg->getType() == LogMessage::MSG_TYPE_CLEAR) {
    clear();
    my_wrefresh(_qsoWin);
  } else if (msg->getType() == LogMessage::MSG_TYPE_HEARTBEAT) {
    scrubExpiredMessages();
    my_wrefresh(_qsoWin);
    my_wrefresh(_infoWin);
    my_wrefresh(_cmdWin);
  }
  // no need to keep this message
  if (msg != NULL) {
    delete msg;
    msg = NULL;
  }
}

void LogView::show(LogMessage* msg) {

  getmaxyx(stdscr, _rows, _cols);
  if (msg->getType() == LogMessage::MSG_TYPE_DECODE && msg->isCQ() == true) {
    if (_showDupes) {
      updateDisplay(msg, true);
    } else {
      LogMessage *listMsg = getMessageFromList((char*) (msg->getCallSign().c_str()));

      if (listMsg == NULL) {
        // no duplicates
        updateDisplay(msg, true);
      } else {
        // there are duplicates
        long unsigned int startPos = 0;
        deleteDuplicates(msg->getCallSign(), startPos);
        displayList();
        updateDisplay(msg, true);
      }
    }
  } else {
    processMessage(msg);
  }
}

void LogView::updateFreqMode(LogMessage *msg) {
  char msgStr[200];

  // update the mode
  std::string oldMode = _currentMode;
  if ((msg->getMode().length() > 1) && (oldMode.compare(msg->getMode())) != 0) {
    _currentMode = msg->getMode();
    wattrset(_infoWin, COLOR_PAIR(PAIR_INFO));
    snprintf(msgStr, 200, "switching mode to %s\n", _currentMode.c_str());
    my_waddstr(_infoWin, msgStr);
    my_wrefresh(_infoWin);
    wattroff(_infoWin, COLOR_PAIR(PAIR_INFO));
  }

  // update the band
  std::string oldBand = _currentBand;
  unsigned int freq = msg->getDialFreq();
  if (freq > 135700 && freq < 137800) {
    _currentBand = "2190M";
  } else if (freq > 472000 && freq < 479000) {
    _currentBand = "630M";
  } else if (freq > 1800000 && freq < 2000000) {
    _currentBand = "160M";
  } else if (freq > 3500000 && freq < 4000000) {
    _currentBand = "80M";
  } else if (freq > 5332000 && freq < 5403500) {
    _currentBand = "60M";
  } else if (freq > 7000000 && freq < 7300000) {
    _currentBand = "40M";
  } else if (freq > 10100000 && freq < 10150000) {
    _currentBand = "30M";
  } else if (freq > 14000000 && freq < 14350000) {
    _currentBand = "20M";
  } else if (freq > 18068000 && freq < 18168000) {
    _currentBand = "17M";
  } else if (freq > 21000000 && freq < 21450000) {
    _currentBand = "15M";
  } else if (freq > 24890000 && freq < 24990000) {
    _currentBand = "12M";
  } else if (freq > 28000000 && freq < 29700000) {
    _currentBand = "10M";
  } else if (freq > 50100000 && freq < 54000000) {
    _currentBand = "6M";
  } else if (freq > 144100000 && freq < 148000000) {
    _currentBand = "2M";
  } else if (freq > 219000000 && freq < 225000000) {
    _currentBand = "1.25M";
  } else if (freq > 420000000 && freq < 450000000) {
    _currentBand = "70CM";
  } else if (freq > 902000000 && freq < 928000000) {
    _currentBand = "33CM";
  } else if (freq > 1240000000 && freq < 1300000000) {
    _currentBand = "23CM";
  }
  if (oldBand.compare(_currentBand) != 0) {
    wattrset(_infoWin, COLOR_PAIR(PAIR_INFO));
    snprintf(msgStr, 200, "switching band to %s\n", _currentBand.c_str());
    my_waddstr(_infoWin, msgStr);
    my_wrefresh(_infoWin);
    wattroff(_infoWin, COLOR_PAIR(PAIR_INFO));
  }
}

void LogView::setDatabase(char* dBfileName) {
  char msgStr[200];

  _sql3Db = new SqlLiteDb();
  int noRecs = _sql3Db->open(dBfileName);
  wattrset(_infoWin, COLOR_PAIR(PAIR_INFO));
  snprintf(msgStr, 200, "loaded %d database records\n", noRecs);
  my_waddstr(_infoWin, msgStr);
  my_wrefresh(_infoWin);
  wattroff(_infoWin, COLOR_PAIR(PAIR_INFO));
  _sql3Db->close();
}

void LogView::setSkipDays(int days) {
  _skipDays = days;
}

void LogView::setMsgTTL(time_t ttl) {
  _msgTTL = ttl;
}

void LogView::handleResizing(int signo) {

  endwin();
  initscr();
  cbreak();
  noecho();

  intrflush(stdscr, FALSE);
  keypad(stdscr, TRUE);
  scrollok(stdscr, true);
  idlok(stdscr, true);
  refresh();

  _rows = getmaxy(stdscr);
  _cols = getmaxx(stdscr);

  if (_qsoWin != NULL) {
    destroy_win(_qsoWin);
    _qsoWin = NULL;
  }
  _qsoWin = create_newwin(_rows - SIZE_OF_INFO_WIN - SIZE_OF_CMD_WIN,
          _cols, 0, 0);
  intrflush(_qsoWin, FALSE);
  keypad(_qsoWin, TRUE);
  scrollok(_qsoWin, true);
  idlok(_qsoWin, true);

  if (_infoWin != NULL) {
    destroy_win(_infoWin);
    _infoWin = NULL;
  }

  _infoWin = create_newwin(SIZE_OF_INFO_WIN,
          _cols, _rows - SIZE_OF_INFO_WIN - SIZE_OF_CMD_WIN, 0);
  intrflush(_infoWin, FALSE);
  keypad(_infoWin, FALSE);
  scrollok(_infoWin, true);
  idlok(_infoWin, true);

  if (_cmdWin != NULL) {
    destroy_win(_cmdWin);
    _cmdWin = NULL;
  }

  _cmdWin = create_newwin(SIZE_OF_CMD_WIN,
          _cols, _rows - SIZE_OF_CMD_WIN, 0);
  intrflush(_cmdWin, FALSE);
  keypad(_cmdWin, TRUE);
  scrollok(_cmdWin, true);
  idlok(_cmdWin, true);

  resizeterm(_rows, _cols);

  my_wrefresh(_qsoWin);
  my_wrefresh(_infoWin);
  my_wrefresh(_cmdWin);

  drawCmds();

  wmove(_qsoWin, 0, 0);
  wmove(_infoWin, 0, 0);
  wmove(_cmdWin, 0, 0);

  my_wrefresh(_qsoWin);
  my_wrefresh(_infoWin);
  my_wrefresh(_cmdWin);

}

std::vector<std::string> LogView::parseStr(std::string str, char ch) {
  std::vector<std::string> tokens;

  istringstream f(str);
  string s;
  while (getline(f, s, ch)) {
    tokens.push_back(s);
  }

  return tokens;
}

int LogView::getCallSignFromMessage(char *msgStr, char **csHandle, int csLen) {
  char *cs = NULL;
  int ret = 0;
  int tokenCnt = 0;
  char tokens[4][20];
  memset(tokens, 0, 80);

  char ch = *msgStr;
  int chCnt = 0;
  while (ch != '\0') {
    if (ch == ' ') {
      if (chCnt == 0) {
        // successive spaces
        ch = *(++msgStr);
        continue;
      }
      tokens[tokenCnt][chCnt] = '\0';
      fprintf(keyboard, "token[%d]:%s:\n", tokenCnt, tokens[tokenCnt]);
      fflush(keyboard);
      tokenCnt += 1;
      chCnt = 0;
      ch = *(++msgStr);
    } else {
      tokens[tokenCnt][chCnt++] = *msgStr;
      ch = *(++msgStr);
    }
  }
  fprintf(keyboard, "token[%d]:%s:\n", tokenCnt, tokens[tokenCnt]);
  fflush(keyboard);

  tokenCnt += 1;
  if (tokenCnt == 4) {
    cs = tokens[2];
  } else if (tokenCnt == 3 || tokenCnt == 2) {
    cs = tokens[1];
  } else {
    return 0;
  }

  int tokenLen = strnlen(cs, 20);
  if (tokenLen > csLen) {
    strncpy(*csHandle, cs, csLen - 1);
  } else {
    strncpy(*csHandle, cs, 19);
  }
  ret = strnlen(*csHandle, 19);
  
  return ret;
}

void LogView::scanKeyboard() {
  char msgStr[200];
  char chstr[MAX_FT8_MSG_SIZE];

  for (;;) {
    // Set up mouse event throwing
    mousemask(BUTTON1_PRESSED, NULL);

    int c = wgetch(stdscr);
    if (c == ERR) {
      // do nothing
    } else if (c == KEY_MOUSE) {
      MEVENT event;
      if (getmouse(&event) == OK) {
        int selcol = event.x;
        int selrow = event.y;
        if (selrow >= 0 && selrow <
                (_rows - SIZE_OF_CMD_WIN - SIZE_OF_INFO_WIN)) {
          // qso window click
          memset(chstr, 0, MAX_FT8_MSG_SIZE);
          int x = 1;
          for (; x < MAX_FT8_MSG_SIZE - 2; x++) {
            chstr[x - 1] = (char) (mvwinch(_qsoWin, event.y, x) & A_CHARTEXT);
          }


          for (; x > 0; x--) {
            // remove end spaces
            if (chstr[x] == ' ' ||
                    chstr[x] == '\0' ||
                    chstr[x] == 127) {
              chstr[x] = '\0';
            } else {
              break;
            }
          }

          // remove the signal report
          x = strnlen(chstr, MAX_FT8_MSG_SIZE) - 1;
          while ((chstr[x] >= '0' && chstr[x] <= '9') ||
                  chstr[x] == '-') {
            x -= 1;
          }
          while (chstr[x] == ' ') {
            chstr[x] = '\0';
            x -= 1;
          }

          fprintf(keyboard, "  trim chstr:%s:\n", chstr);
          fflush(keyboard);

          if (strnlen(chstr, 40) > 0) {
            char callSign[10];
            memset(callSign, 0, 10);
            char *csPtr = callSign;
            int csLen = getCallSignFromMessage(chstr, &csPtr, 10);
            fprintf(keyboard, "\t call sign:%s:\n", csPtr);
            fflush(keyboard);
            if (csLen > 0 && csLen < 9) {
              LogMessage *listMsg = getMessageFromList(callSign);
              if (listMsg != NULL) {
                if (_channel != NULL) {
                  wattrset(_infoWin, COLOR_PAIR(PAIR_INFO));
                  snprintf(msgStr, 200, "%s selected\n", listMsg->getCallSign().c_str());
                  fprintf(keyboard, "\t%s selected\n\n", listMsg->getCallSign().c_str());
                  fflush(keyboard);
                  my_waddstr(_infoWin, msgStr);
                  my_wrefresh(_infoWin);
                  _channel->xmitResponse(listMsg);
                  wattroff(_infoWin, COLOR_PAIR(PAIR_INFO));
                }
              }
            }
          }
          wmove(_qsoWin, _curY, _curX);
        } else if (selrow > (_rows - SIZE_OF_CMD_WIN) &&
                selrow == (_rows - SIZE_OF_CMD_WIN + 1)) {
          // cmd window click
          if (selcol > _buttonSpace &&
                  selcol < (_buttonSpace + SIZE_OF_CMD_BUTTON)) {
            // exit button
            return;
          } else if (selcol > (2 * _buttonSpace + SIZE_OF_CMD_BUTTON) &&
                  selcol < (2 * _buttonSpace + 2 * SIZE_OF_CMD_BUTTON)) {
            // 'clr ft8b4' button - clear the qso window
            werase(_qsoWin);
            my_wrefresh(_qsoWin);
          } else if (selcol > (3 * _buttonSpace + 2 * SIZE_OF_CMD_BUTTON) &&
                  selcol < (3 * _buttonSpace + 3 * SIZE_OF_CMD_BUTTON)) {
            // dupes button - toggle it
            if (_showDupes) {
              _showDupes = false;
              scrubDuplicateMessages();
              displayList();
            } else {
              _showDupes = true;
            }
            drawCmds();
            my_wrefresh(_cmdWin);
          }
        }
      } else {
        //  bad mouse event
      }
    } else {
      // do nothing
    }
    my_wrefresh(_qsoWin);
    my_wrefresh(_infoWin);
    my_wrefresh(_cmdWin);
  }
}

int LogView::my_waddstr(WINDOW *win, const char *str) {
  int ret = 0;

  if (win == _infoWin) {
    int x = getcurx(_infoWin);
    int y = getcury(_infoWin);
    if (y == 0) y = 1;
    if (y >= SIZE_OF_INFO_WIN) y = SIZE_OF_INFO_WIN - 2;
    ret = mvwaddstr(_infoWin, y, x + 1, str);
  } else if (win == _qsoWin) {
    int x = getcurx(_qsoWin);
    int y = getcury(_qsoWin);
    if (y == 0) y = 1;
    if (y >= _rows - SIZE_OF_INFO_WIN - SIZE_OF_CMD_WIN) y = _rows - SIZE_OF_INFO_WIN - SIZE_OF_CMD_WIN - 2;
    ret = mvwaddstr(_qsoWin, y, x + 1, str);
    _curX = getcurx(_qsoWin);
    _curY = getcury(_qsoWin);
  }
  return ret;
}

int LogView::my_wrefresh(WINDOW * win) {
  int ret = 0;

  if (win == _infoWin) {
    int x = getmaxx(_infoWin);
    int cx = getcurx(_infoWin);
    int cy = getcury(_infoWin);
    wattrset(_infoWin, COLOR_PAIR(PAIR_INFO));
    box(_infoWin, 0, 0);
    mvwaddstr(_infoWin, 0, x - 10, "info");
    wattroff(_infoWin, COLOR_PAIR(PAIR_INFO));
    scrollok(_qsoWin, TRUE);
    wmove(_infoWin, cy, cx);
    ret = wrefresh(_infoWin);
  } else if (win == _cmdWin) {
    int x = getmaxx(_cmdWin);
    int cx = getcurx(_cmdWin);
    int cy = getcury(_cmdWin);
    wattrset(_cmdWin, COLOR_PAIR(PAIR_INFO));
    box(_cmdWin, 0, 0);
    mvwaddstr(_cmdWin, 0, x - 10, "cmds");
    wattroff(_cmdWin, COLOR_PAIR(PAIR_INFO));
    wmove(_cmdWin, cy, cx);
    scrollok(_cmdWin, FALSE);
    ret = wrefresh(_cmdWin);
  } else if (win == _qsoWin) {
    int x = getmaxx(_qsoWin);
    int cx = getcurx(_qsoWin);
    int cy = getcury(_qsoWin);
    wattrset(_qsoWin, COLOR_PAIR(PAIR_INFO));
    box(_qsoWin, 0, 0);
    mvwaddstr(_qsoWin, 0, x - 10, "msgs");
    wattroff(_qsoWin, COLOR_PAIR(PAIR_INFO));
    wmove(_qsoWin, cy, cx);
    scrollok(_qsoWin, TRUE);
    ret = wrefresh(_qsoWin);
  }

  return ret;
}

void LogView::push_list(LogMessage * msg) {
  //  char msgStr[200];

  pthread_mutex_lock(&_listMutex);
  std::vector<std::pair < std::string, LogMessage*>>::iterator it = _displayList.begin();
  std::pair<std::string, LogMessage*> entry(msg->getMsgStr(), msg);
  _displayList.insert(it, entry);
  while (_displayList.size() > DISPLAY_LIST_SIZE) {
    std::pair<std::string, LogMessage*> oldEntry = _displayList.back();
    delete oldEntry.second;
    _displayList.pop_back();
  }
  pthread_mutex_unlock(&_listMutex);
}

LogMessage * LogView::getMessageFromList(char* callSign) {
  LogMessage* msg = NULL;

  if (callSign == NULL) {
    return msg;
  }

  std::string str(callSign);
  pthread_mutex_lock(&_listMutex);
  std::vector<std::pair < std::string, LogMessage*>>::iterator it = _displayList.begin();
  while (it != _displayList.end()) {
    std::pair<std::string, LogMessage*> entry = *it;
    if (str.compare(entry.second->getCallSign()) == 0) {
      // create a copy
      msg = new LogMessage(*(entry.second));
      break;
    }
    it++;
  }
  pthread_mutex_unlock(&_listMutex);
  return msg;
}

void LogView::drawCmds() {

  // draw the cmd window
  _buttonSpace = (_cols - (NO_CMDS * SIZE_OF_CMD_BUTTON)) / (NO_CMDS + 1);
  std::string spacer;
  for (int s = 0; s < _buttonSpace; s++) {
    spacer += " ";
  }

  wattrset(_cmdWin, COLOR_PAIR(PAIR_INFO));
  mvwaddstr(_cmdWin, 1, 1, spacer.c_str());
  wattroff(_cmdWin, COLOR_PAIR(PAIR_INFO));
  wattrset(_cmdWin, COLOR_PAIR(PAIR_CMD));
  waddstr(_cmdWin, "   exit   ");
  wattroff(_cmdWin, COLOR_PAIR(PAIR_CMD));
  wattrset(_cmdWin, COLOR_PAIR(PAIR_INFO));
  waddstr(_cmdWin, spacer.c_str());
  wattroff(_cmdWin, COLOR_PAIR(PAIR_INFO));
  wattrset(_cmdWin, COLOR_PAIR(PAIR_CMD));
  waddstr(_cmdWin, "clr  ft8b4");
  wattroff(_cmdWin, COLOR_PAIR(PAIR_CMD));
  wattrset(_cmdWin, COLOR_PAIR(PAIR_INFO));
  waddstr(_cmdWin, spacer.c_str());
  wattroff(_cmdWin, COLOR_PAIR(PAIR_INFO));
  wattrset(_cmdWin, COLOR_PAIR(PAIR_CMD));
  if (_showDupes == true) {
    waddstr(_cmdWin, "hide  dupes");
  } else {
    waddstr(_cmdWin, "show  dupes");
  }
  wattroff(_cmdWin, COLOR_PAIR(PAIR_CMD));
  wattrset(_cmdWin, COLOR_PAIR(PAIR_INFO));
  waddstr(_cmdWin, spacer.c_str());
  wattroff(_cmdWin, COLOR_PAIR(PAIR_INFO));
  my_wrefresh(_cmdWin);
}

int LogView::deleteDuplicates(std::string callSign, long unsigned int startPos) {
  int noDeletes = 0;

  // delete all duplicates of this callSign AFTER the starting position
  bool deletedEntry = true;
  pthread_mutex_lock(&_listMutex);
  while (deletedEntry) {
    // reset the iterator
    std::vector<std::pair < std::string, LogMessage*>>::iterator
    itCheck = _displayList.begin();
    // advance past the call sign position
    for (long unsigned int i = 0; i <= startPos; i++) {
      itCheck++;
    }
    int delPos = startPos + 1;
    deletedEntry = false;
    while (itCheck != _displayList.end()) {
      std::pair<std::string, LogMessage*> checkEntry = *itCheck;
      std::string cs = checkEntry.second->getCallSign();
      if (cs.compare(callSign) == 0) {
        _displayList.erase(itCheck);
        delete checkEntry.second;
        deletedEntry = true;
        // we need to reset the iterator
        noDeletes += 1;
        break;
      }
      itCheck++;
      delPos += 1;
    }
  }
  pthread_mutex_unlock(&_listMutex);
  return noDeletes;
}

void LogView::scrubDuplicateMessages() {
  long unsigned pos = 0;

  // remove all duplicates from the list
  pthread_mutex_lock(&_listMutex);
  while (pos < _displayList.size()) {
    std::vector<std::pair < std::string, LogMessage*>>::iterator it =
            _displayList.begin();
    // advance to pos and get the call sign
    for (long unsigned int i = 0; i < pos; i++) {
      it++;
    }
    std::pair<std::string, LogMessage*> entry = *it;
    std::string callSign = entry.second->getCallSign();

    pthread_mutex_unlock(&_listMutex);
    deleteDuplicates(callSign, pos);
    pthread_mutex_lock(&_listMutex);
    pos += 1;
  }
  pthread_mutex_unlock(&_listMutex);
}

void LogView::scrubExpiredMessages() {
  time_t now = time(NULL);

  while (_displayList.size() > 0) {
    std::pair<std::string, LogMessage*> entry = _displayList.back();
    time_t postTime = entry.second->getTime();
    if ((now - postTime) > _msgTTL) {
      delete entry.second;
      _displayList.pop_back();
    } else {
      break;
    }
  }
}
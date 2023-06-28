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
 * File:   LogView.h
 * Author: keithc
 *
 * Created on March 16, 2023, 8:16 AM
 */

#ifndef LOGVIEW_H
#define LOGVIEW_H

#include <vector>
#include <ncurses.h>
#include <pthread.h>
#include "logMessage.h"
#include "sqlLiteDb.h"

using namespace std;

class ProxyChannel;

class LogView {
public:
    LogView(ProxyChannel *channel);
    LogView(const LogView& orig);
    virtual ~LogView();
    void show(LogMessage *msg);
    void updateDisplay(LogMessage *msg, bool updateList);
    void processMessage(LogMessage *msg);
    int displayList();
    void setDatabase(char *dBfileName);
    void setSkipDays(int days);
    void setMsgTTL(time_t ttl);
    int deleteDuplicates(std::string callSign, long unsigned int startPos);
    void scrubDuplicateMessages();
    void scrubExpiredMessages();
    void updateFreqMode(LogMessage *msg);
    std::vector<std::string> parseStr(std::string str, char ch);
    void push_list(LogMessage *msg);
    void scanKeyboard();
    static void drawCmds();
    int my_waddstr(WINDOW *win, const char *str);
    static void handleResizing(int signo);
    static int my_wrefresh(WINDOW *win);
    LogMessage* getMessageFromList(char *msgStr);
    int getCallSignFromMessage(char *msgStr, char **cs, int csLen);
    
private:
    static LogView *_instance;
    ProxyChannel *_channel;
    static WINDOW *_qsoWin;
    static WINDOW *_infoWin;
    static WINDOW *_cmdWin;
    int _curX;
    int _curY;
    static int _buttonSpace;
    SqlLiteDb* _sql3Db;
    std::string _currentBand;
    std::string _currentMode;
    static int _rows, _cols;
    static bool _showDupes;
    int _skipDays;
    time_t _msgTTL;
    pthread_mutex_t _listMutex;
    std::vector<std::pair<std::string, LogMessage*>> _displayList;
};

#endif /* LOGVIEW_H */


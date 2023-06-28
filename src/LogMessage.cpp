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
 * File:   LogMessage.cpp
 * Author: keithc
 * 
 * Created on March 15, 2023, 11:23 AM
 */
#include <iostream>
#include <unistd.h>

#include <string.h>
#include <arpa/inet.h>
#include "logMessage.h"
#include "main.h"

const int MAX_TOKEN_SIZE = 300;
const int MIN_MESSAGE_SIZE = 32;

const unsigned char MAGIC_NUMBER[] = {0xad, 0xbc, 0xcb, 0xda};
long LogMessage::MAGIC_INT;

const unsigned int SCHEMA_VERSION_NO = 2;

LogMessage::LogMessage() {
  union INT_CAST mn;
  memcpy(mn.bytes, MAGIC_NUMBER, 4);
  LogMessage::MAGIC_INT = mn.num;
}

LogMessage::LogMessage(unsigned char* data, int len) {
  union INT_CAST mn;
  memcpy(mn.bytes, MAGIC_NUMBER, 4);
  LogMessage::MAGIC_INT = mn.num;
  parse(data, len);
}

LogMessage::LogMessage(const LogMessage& orig) {
  // perform a deep copy

  _dataLen = orig._dataLen;
  memcpy(_data, orig._data, _dataLen);

  _msgType = orig._msgType;
  _Id.assign(orig._Id);
  _msgStr.assign(orig._msgStr);
  _callSign.assign(orig._callSign);
  _grid.assign(orig._grid);
  _time = orig._time;
  _date.assign(orig._date);
  _mode.assign(orig._mode);
  _band.assign(orig._band);
  memcpy(&_mn, &orig._mn, sizeof (union INT_CAST));
  memcpy(&_schemaVersionNo, &orig._schemaVersionNo, sizeof (union INT_CAST));
  memcpy(&_messageType, &orig._messageType, sizeof (union INT_CAST));
  memcpy(&_todMs, &orig._todMs, sizeof (union INT_CAST));
  memcpy(&_snr, &orig._snr, sizeof (union INT_CAST));
  memcpy(&_deltaFreq, &orig._deltaFreq, sizeof (union INT_CAST));
  memcpy(&_dialFreq, &orig._dialFreq, sizeof (union LONG_CAST));
  _newMsg = orig._newMsg;
  _isCQ = orig._isCQ;
  _lowConfidence = orig._lowConfidence;
  _offAir = orig._offAir;
}

// FILE *tfile = fopen("/tmp/tfile", "w");

LogMessage::~LogMessage() {
}

LogMessage *LogMessage::createMessage(int msgType) {
  LogMessage *msg = NULL;
  unsigned char *dataPtr;
  const char *ID = "WSJT-X";
  
  if(msgType == LogMessage::MSG_TYPE_CLEAR) {
    msg = new LogMessage();
    msg->_msgType = LogMessage::MSG_TYPE_CLEAR;
    msg->_data[0] = 0xAD;
    msg->_data[1] = 0xBC;
    msg->_data[2] = 0xCB;
    msg->_data[3] = 0xDA;
    dataPtr = &(msg->_data[4]);
    union INT_CAST version, type, IDlen;
    version.num = htonl(2);
    memcpy(dataPtr, version.bytes, 4);
    dataPtr += 4;
    type.num = htonl(LogMessage::MSG_TYPE_CLEAR);
    memcpy(dataPtr, type.bytes, 4);
    dataPtr += 4;
    IDlen.num = htonl(6);
    memcpy(dataPtr, IDlen.bytes, 4);
    dataPtr += 4;
    memcpy(dataPtr, ID, 6);
    dataPtr += 6;
    *dataPtr = 0x01;
    dataPtr += 2;
    msg->_dataLen = (dataPtr - msg->_data);
  }
  
  return msg;
}

int msgNo = 0;

int LogMessage::parse(unsigned char* data, int len) {
  int ret = LogMessage::MSG_TYPE_UNDEFINED;
  unsigned char* dataPtr = data;
  char msg[Clogger::MAX_LOG_MSG_SIZE];
  
  _isCQ = false;
  
  _msgType = LogMessage::MSG_TYPE_UNDEFINED;
  if (len < MIN_MESSAGE_SIZE) {
    LOG_MSG(theLog, Clogger::LOG_MINOR, "undersized message received");
    return ret;
  }

  // fill the data field
  _dataLen = len;
  memcpy(_data, data, len);

  // check the magic number
  memcpy(_mn.bytes, dataPtr, 4);
  if (_mn.num != LogMessage::MAGIC_INT) {
    LOG_MSG(theLog, Clogger::LOG_MINOR, "magic number not found");
    return ret;
  }
  dataPtr += 4;

  // get the schema version
  memcpy(_schemaVersionNo.bytes, dataPtr, 4);
  _schemaVersionNo.num = ntohl(_schemaVersionNo.num);
  if (_schemaVersionNo.num != SCHEMA_VERSION_NO) {
    sprintf(msg, "unsupported schema version: %d\n", _schemaVersionNo.num);
    LOG_MSG(theLog, Clogger::LOG_MINOR, msg);
    return ret;
  }
  dataPtr += 4;

  // get the message type
  memcpy(_messageType.bytes, dataPtr, 4);
  _messageType.num = ntohl(_messageType.num);
  dataPtr += 4;
  _msgType = _messageType.num;

  if (_msgType == LogMessage::MSG_TYPE_DECODE) {

    // get the ID banner
    int strlen = 0;
    _Id = getTextStr(dataPtr, &strlen);
    dataPtr += (4 + strlen);

    // new message?
    if (*dataPtr == 0) {
      _newMsg = false;
    } else {
      _newMsg = true;
    }
    dataPtr += 1;

    // get _todMs
    union INT_CAST _todMs;
    memcpy(_todMs.bytes, dataPtr, 4);
    _todMs.num = ntohl(_todMs.num);
    dataPtr += 4;

    // get _snr
    memcpy(_snr.bytes, dataPtr, 4);
    _snr.num = ntohl(_snr.num);
    dataPtr += 4;

    // skip delta time
    // use the parse time as the
    _time = time(NULL);
    dataPtr += 8;

    // get _freq
    union INT_CAST _freq;
    memcpy(_freq.bytes, dataPtr, 4);
    _freq.num = ntohl(_freq.num);
    dataPtr += 4;

    // get mode
    strlen = 0;
    _mode = getTextStr(dataPtr, &strlen);
    dataPtr += (4 + strlen);

    // get message
    strlen = 0;
    _msgStr = getTextStr(dataPtr, &strlen);
    dataPtr += (4 + strlen);

    if (_msgStr.size() > 6 && _msgStr.find("CQ") == 0) {
      char tokenStr[60];
      strcpy(tokenStr, _msgStr.c_str());
      const char delim = ' ';
      int noTokens = 0;
      char *tokenPtr;
      char tokens[6][20];
      tokenPtr = strtok(tokenStr, &delim);
      while (tokenPtr != NULL) {
        strcpy(tokens[noTokens], tokenPtr);
        ++noTokens;
        tokenPtr = strtok(NULL, &delim);
      }
      if (noTokens == 2) {
        _callSign = tokens[1];
      } else if (noTokens == 3) {
        _callSign = tokens[1];
        _grid = tokens[2];
      } else if (noTokens == 4) {
        _callSign = tokens[2];
        _grid = tokens[3];
      }
      _isCQ = true;
    }

    // get low confidence
    if (*dataPtr++ == 0) {
      _lowConfidence = false;
    } else {
      _lowConfidence = true;
    }

    // get off air
    if (*dataPtr++ == 0) {
      _offAir = false;
    } else {
      _offAir = true;
    }

  } else if (_msgType == LogMessage::MSG_TYPE_LOGGED_ADIF) {
    std::string time;

    std::string dataStr(&data[0], &data[len]);
    size_t startPos = dataStr.find("<call:");
    startPos = dataStr.find('>', startPos);
    size_t endPos = dataStr.find(" <", startPos);
    if (endPos != std::string::npos && startPos < endPos) {
      _callSign = dataStr.substr(startPos + 1, (endPos - startPos - 1));
    }
    startPos = dataStr.find("<gridsquare:");
    startPos = dataStr.find('>', startPos);
    endPos = dataStr.find(" <", startPos);
    if (endPos != std::string::npos && startPos < endPos) {
      _grid = dataStr.substr(startPos + 1, (endPos - startPos - 1));
    }
    startPos = dataStr.find("<mode:");
    startPos = dataStr.find('>', startPos);
    endPos = dataStr.find(" <", startPos);
    if (endPos != std::string::npos && startPos < endPos) {
      _mode = dataStr.substr(startPos + 1, (endPos - startPos - 1));
    }
    startPos = dataStr.find("<qso_date:");
    startPos = dataStr.find('>', startPos);
    endPos = dataStr.find(" <", startPos);
    if (endPos != std::string::npos && startPos < endPos) {
      _date = dataStr.substr(startPos + 1, (endPos - startPos - 1));
    }
    startPos = dataStr.find("<time_on:");
    startPos = dataStr.find('>', startPos);
    endPos = dataStr.find(" <", startPos);
    if (endPos != std::string::npos && startPos < endPos) {
      time = dataStr.substr(startPos + 1, (endPos - startPos - 1));
    }
    startPos = dataStr.find("<band:");
    startPos = dataStr.find('>', startPos);
    endPos = dataStr.find(" <", startPos);
    if (endPos != std::string::npos && startPos < endPos) {
      _band = dataStr.substr(startPos + 1, (endPos - startPos - 1));
      char* bandchars = (char*) _band.c_str();
      if (bandchars[strlen(bandchars) - 1] == 'm') {
        bandchars[strlen(bandchars) - 1] = 'M';
        _band = bandchars;
      }
    }
    std::string dateStr = _date.substr((size_t) 0, (size_t) 4) + "-" + _date.substr((size_t) 4, (size_t) 2) + "-" + _date.substr((size_t) 6, (size_t) 2);
    std::string timeStr = time.substr((size_t) 0, (size_t) 2) + ":" + time.substr((size_t) 2, (size_t) 2) + ":" + time.substr((size_t) 4, (size_t) 2);
    _msgStr = _callSign + " " + dateStr + " " + timeStr + " " + _mode + " " + _band + " ";
  } else if (_msgType == LogMessage::MSG_TYPE_STATUS) {
    // frequency and mode update

    // get the ID banner
    int strlen = 0;
    _Id = getTextStr(dataPtr, &strlen);
    dataPtr += (4 + strlen);

    // get _dialFreq
    for (int i = 0; i < 8; i++) {
      _dialFreq.bytes[i] = *(dataPtr + 7 - i);
    }
    dataPtr += 8;

    // find the mode
    int modeLen = 0;
    _mode = getTextStr(dataPtr, &modeLen);
  } else {
    _msgType = _messageType.num;
  }
  return ret;
}

int LogMessage::prepareResponse(unsigned char **buf, int buflen) {

  if (getType() != LogMessage::MSG_TYPE_DECODE && isCQ() == true) {
    return 0;
  }
  if (buflen < (getDataLen() + 4)) {
    return 0;
  }
  int tmplen = 0;
  unsigned char *tmpPtr = *buf;
  unsigned char *dataPtr = _data;
  memcpy(tmpPtr, dataPtr, 4); // magic number
  tmplen += 4;
  tmpPtr += 4;
  dataPtr += 4;
  memcpy(tmpPtr, dataPtr, 4); // version no.
  tmplen += 4;
  tmpPtr += 4;
  dataPtr += 4;
  memcpy(tmpPtr, dataPtr, 4); // message type
  tmplen += 4;
  tmpPtr += 3;
  dataPtr += 4;
  *tmpPtr = 4; // Reply - call select
  tmpPtr += 1;
  // get ID
  int strlen = 0;
  getTextStr(dataPtr, &strlen);
  memcpy(tmpPtr, dataPtr, strlen + 4);
  tmplen += strlen + 4;
  tmpPtr += strlen + 4;
  dataPtr += strlen + 4;
  // boolean new - skip for reply
  dataPtr += 1;
  memcpy(tmpPtr, dataPtr, 4); // time
  tmplen += 4;
  tmpPtr += 4;
  dataPtr += 4;
  memcpy(tmpPtr, dataPtr, 4); // snr
  tmplen += 4;
  tmpPtr += 4;
  dataPtr += 4;
  memcpy(tmpPtr, dataPtr, 8); // delta time 
  tmplen += 8;
  tmpPtr += 8;
  dataPtr += 8;
  memcpy(tmpPtr, dataPtr, 4); // delta freq
  tmplen += 4;
  tmpPtr += 4;
  dataPtr += 4;
  // get mode
  strlen = 0;
  getTextStr(dataPtr, &strlen);
  memcpy(tmpPtr, dataPtr, strlen + 4);
  tmplen += strlen + 4;
  tmpPtr += strlen + 4;
  dataPtr += strlen + 4;
  // get message
  strlen = 0;
  getTextStr(dataPtr, &strlen);
  memcpy(tmpPtr, dataPtr, strlen + 4);
  tmplen += strlen + 4;
  tmpPtr += strlen + 4;
  dataPtr += strlen + 4;
  memcpy(tmpPtr, dataPtr, 1); // low confidence
  tmplen += 1;
  tmpPtr += 1;
  *tmpPtr = 0x00; // modifiers
  tmplen += 1;

  //  FILE *orig = fopen("origmsg.info", "w");
  //  fwrite(_data, 1, _dataLen, orig);
  //  fclose(orig);

  return tmplen;
}

std::string LogMessage::getTextStr(unsigned char* data, int* len) {
  std::string str;

  // find the length of the text field
  union INT_CAST strLen;
  memcpy(strLen.bytes, data, 4);
  strLen.num = ntohl(strLen.num);

  if (strLen.num < 1 || strLen.num > 20) {
    return str;
  }

  std::string s((const char*) (data + 4), (size_t) strLen.num);
  str = s;
  *len = strLen.num;

  return str;
}

int LogMessage::getDataLen() {
  return _dataLen;
}

unsigned char* LogMessage::getData() {
  return _data;
}

int LogMessage::getType() {
  return _msgType;
}

std::string LogMessage::getMsgStr() {
  return _msgStr;
}

std::string LogMessage::getCallSign() {
  return _callSign;
}

std::string LogMessage::getGrid() {
  return _grid;
}

std::string LogMessage::getMode() {
  return _mode;
}

unsigned int LogMessage::getDeltaFreq() {
  return _deltaFreq.num;
}

unsigned int LogMessage::getDialFreq() {
  return _dialFreq.num;
}

std::string LogMessage::getDate() {
  return _date;
}

time_t LogMessage::getTime() {
  return _time;
}

int LogMessage::getSNR() {
  return _snr.num;
}

bool LogMessage::isCQ() {
  return _isCQ;
}
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
 * File:   LogMessage.h
 * Author: keithc
 *
 * Created on March 15, 2023, 11:23 AM
 */

#ifndef LOGMESSAGE_H
#define LOGMESSAGE_H

using namespace std;

#include <string>
#include <time.h>

union INT_CAST {
  unsigned char bytes[4];
  int num;
};

union LONG_CAST {
  unsigned char bytes[8];
  unsigned long long num;
};

const int MAX_WSJTX_MSG_SIZE = 1000;
const int MAX_FT8_MSG_SIZE = 32;

class LogMessage {
public:
    LogMessage();
    LogMessage(unsigned char* data, int len);
    LogMessage(const LogMessage& orig);
    virtual ~LogMessage();
    static LogMessage *createMessage(int msgType);
    int parse(unsigned char* data, int len);
    int prepareResponse(unsigned char **buf, int buflen);
    int getDataLen();
    int getType();
    unsigned char* getData();
    std::string getTextStr(unsigned char* data, int* len);
    std::string getMsgStr();
    std::string getCallSign();
    std::string getGrid();
    std::string getMode();
    std::string getDate();
    time_t getTime();
    unsigned int getDeltaFreq();
    unsigned int getDialFreq();
    int getSNR();
    bool isCQ();
    
    const static int MSG_TYPE_UNDEFINED     = -1;
    const static int MSG_TYPE_HEARTBEAT     = 0;
    const static int MSG_TYPE_STATUS        = 1;
    const static int MSG_TYPE_DECODE        = 2;
    const static int MSG_TYPE_CLEAR         = 3;
    const static int MSG_TYPE_REPLY         = 4;
    const static int MSG_TYPE_QSO_LOGGED    = 5;
    const static int MSG_TYPE_CLOSE         = 6;
    const static int MSG_TYPE_REPLAY        = 7;
    const static int MSG_TYPE_HALT          = 8;
    const static int MSG_TYPE_FREE_TEXT     = 9;
    const static int MSG_TYPE_WSPR_DECODE   = 10;
    const static int MSG_TYPE_LOCATION      = 11;
    const static int MSG_TYPE_LOGGED_ADIF   = 12;
    const static int MSG_TYPE_HIGHLIGHT_CS  = 13;
    const static int MSG_TYPE_SWITCH_CFG    = 14;
    const static int MSG_TYPE_CONFIGURE     = 15;
    
private:
    static long MAGIC_INT;
    int _msgType;
    int _dataLen;
    unsigned char _data[MAX_WSJTX_MSG_SIZE];
    std::string _Id;
    std::string _msgStr;
    std::string _callSign;
    std::string _grid;
    time_t _time;
    std::string _date;
    std::string _mode;
    std::string _band;
    union INT_CAST _mn;
    union INT_CAST _schemaVersionNo;
    union INT_CAST _messageType;
    union INT_CAST _todMs;
    union INT_CAST _snr;
    union INT_CAST _deltaFreq;
    union LONG_CAST _dialFreq;
    bool _isCQ;
    bool _newMsg;
    bool _lowConfidence;
    bool _offAir;
};

#endif /* LOGMESSAGE_H */


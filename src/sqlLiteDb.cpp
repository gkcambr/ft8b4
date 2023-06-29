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
 * File:   sqlLiteDb.cpp
 * Author: keithc
 * 
 * Created on March 17, 2023, 6:16 AM
 */

#include <map>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "main.h"
#include "sqlLiteDb.h"
#include "logger.h"

const int MAX_CALLSIGN = 40;
const int MAX_LINE = 120;
 
static const char* modes[] = { "0", "ARDOP", "AMTORFEC", "ASCI", "ATV", "C4FM", "CHIP", "CHIP64", "CHIP128", "CLO", "CONTESTI", "CW", "DIGITALVOICE", "C4FM", "DMR", 
    "DSTAR", "VARA HF", "VARA SATELLITE", "VARA FM 1200", "VARA FM 9600", "DOMINO", "DOMINOEX", "DOMINOF", "FAX", "FM", "FMHELL", "FT4", "FST4", "FST4W", 
    "FT8", "FSK31", "FSK441", "FSKHELL", "FSQCALL", "GTOR", "HELL", "HELL80", "HFSK", "ISCAT", "ISCAT-A", "ISCAT-B", "JS8", "JT4", "JT4A", "JT4B", "JT4C", 
    "JT4D", "JT4E", "JT4F", "JT4G", "JT6M", "JT9", "JT9-1", "JT9-2", "JT9-5", "JT9-10", "JT9-30", "JT9A", "JT9B", "JT9C", "JT9D", "JT9E", "JT9E FAST", "JT9F", 
    "JT9F FAST", "JT9G", "JT9G FAST", "JT9H", "JT9H FAST", "JT44", "JT65", "JT65A", "JT65B", "JT65B2", "JT65C", "JT65C2", "MFSK", "MFSK4", "MFSK8", "MFSK11", 
    "MFSK16", "MFSK22", "MFSK31", "MFSK32", "MFSK64", "MFSK128", "MSK144", "MT63", "OLIVIA", "OLIVIA 4/125", "OLIVIA 4/250", "OLIVIA 8/250", "OLIVIA 8/500", 
    "OLIVIA 16/500", "OLIVIA 16/1000", "OLIVIA 32/1000", "OPERA", "OPERA-BEACON", "OPERA-QSO", "PAC", "PAC2", "PAC3", "PAC4", "PAX", "PAX2", "PCW", "PKT", 
    "PSK", "PSK10", "PSK31", "PSK63", "PSK63F", "PSK125", "PSK250", "PSK500", "PSK1000", "PSKAM10", "PSKAM31", "PSKAM50", "PSKFEC31", "PSK2K", "PSKHELL", "Q15", 
    "Q65", "QPSK31", "QPSK63", "QPSK125", "QPSK250", "QPSK500", "QRA64", "QRA64A", "QRA64B", "QRA64C", "QRA64D", "QRA64E", "ROS", "ROS-EME", "ROS-HF", "ROS-MF", 
    "RTTY", "RTTYM", "SSB", "LSB", "USB", "SIM31", "SSTV", "T10", "THRB", "THRBX", "THOR", "TOR", "V4", "VOI", "WINMOR", "WSPR"};

const char* bands[] = {"0", "0", "1mm", "2mm", "2.5mm", "4mm", "6mm", "1.25CM", "3CM", "6CM", "9CM", "13CM", "23CM", "33CM", "70CM", "1.25M", "2M", "4M", "5M", "6M", 
"8M", "10M", "12M", "15M", "17M", "20M", "30M", "40M", "60M", "80M", "160M", "560M", "630M", "2190M"};

multimap<std::string, std::string> _hash;

SqlLiteDb::SqlLiteDb() {
}

SqlLiteDb::SqlLiteDb(const SqlLiteDb& orig) {
}

SqlLiteDb::~SqlLiteDb() {
}

static int callback(void *v, int argc, char **argv, char **colName) {
  int ret = 0;
  char line[MAX_LINE];
  char callSign[MAX_CALLSIGN];
  
  memset(line, 0, MAX_LINE);
  int mod, bnd;
  const char *mdStr, *bndStr;
  for(int i = 0; i < argc; i++) {
    if(*argv != NULL) {
      switch (i) {
        case 0:
          strncpy(callSign, *argv, MAX_CALLSIGN - 1);
          strncat(line, *argv++, MAX_LINE - 2);
          strncat(line, " ", 2);
          break;
        case 1:
          strncat(line, *argv++, MAX_LINE - 2);
          strncat(line, " ", 2);
          break;
        case 2:
          mod = atoi(*argv++);
          mdStr = modes[mod];
          strncat(line, mdStr, MAX_LINE - 2);
          strncat(line, " ", 2);
          break;
        case 3:
          bnd = atoi(*argv++);
          bndStr = bands[bnd];
          strncat(line, bndStr, MAX_LINE - 2);
          strncat(line, " ", 2);
          break;
        default:
          break;
      }
    }
  }
  string cs(callSign);
  string ln(line);
  std::multimap<std::string, std::string>::iterator it = _hash.find(callSign);
  if(it != _hash.end()) {
    _hash.insert(it, std::make_pair(cs, ln));
  }
  else {
    _hash.insert(std::make_pair(cs, ln));
  }
  return ret;
}

int SqlLiteDb::open(char* dbFileName) {
  int ret = 0;
  std::string dbName = "file://";
  char msg[Clogger::MAX_LOG_MSG_SIZE];
  
  dbName = dbName + dbFileName;
  ret = sqlite3_open(dbFileName, &_sqliteDb);
  if(ret != SQLITE_OK) {
    const char* errStr = sqlite3_errstr(ret);
    snprintf(msg, Clogger::MAX_LOG_MSG_SIZE, "cannot open db file %s error:%s", dbFileName, errStr);
    LOG_MSG(theLog, Clogger::LOG_MAJOR, msg);
  }
  else {
    requestRecords();
    ret = _hash.size();
  }
  
  return ret;
}

void SqlLiteDb::requestRecords() {
  char *errMsg = 0;
  
  char* sql = (char*)"SELECT call,qso_date,modeid,bandid FROM log";
  sqlite3_exec(_sqliteDb, sql, callback, 0, &errMsg);
}

void SqlLiteDb::loadRecord(LogMessage* msg) {
  std::multimap<std::string, std::string>::iterator it = _hash.find(msg->getCallSign());
  if(it != _hash.end()) {
    if(it != _hash.begin()) {
      --it;
    }
    _hash.insert(it, std::make_pair(msg->getCallSign(), msg->getMsgStr()));
  }
  else {
    _hash.insert(std::make_pair(msg->getCallSign(), msg->getMsgStr()));
  }
}

std::vector<std::string> SqlLiteDb::checkLog(std::string callSign) {
  std::vector<std::string> duplicates;
  
  std::pair <std::multimap<std::string, std::string>::iterator, std::multimap<std::string, std::string>::iterator> ret;
  ret = _hash.equal_range(callSign);
  for (std::multimap<string, string>::iterator it=ret.first; it!=ret.second; ++it) {
    duplicates.push_back(it->second);
  }
  
  return duplicates;
}

void SqlLiteDb::close() {
  sqlite3_close(_sqliteDb);
}

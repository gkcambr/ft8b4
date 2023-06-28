/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ConfigFile.cpp
 * Author: keithc
 * 
 * Created on March 16, 2023, 9:33 AM
 */

#include <cstring>
#include <string.h>
#include "configFile.h"

char* trim(char* token) {
  char* ptr = NULL;
  char* endptr = NULL;

  if (token != NULL && strnlen(token, (size_t) ConfigFile::MAX_PARAM_SIZE) < ConfigFile::MAX_PARAM_SIZE) {
    // remove trailing white space
    int len = strnlen(token, (size_t) ConfigFile::MAX_PARAM_SIZE) - 1;
    endptr = token + len;
    while (*endptr == '\n' || *endptr == ' ' || *endptr == '\t') {
      *endptr = 0;
      endptr = token + --len;
      if (len == 0) {
        break;
      }
    }
    // remove leading white space
    ptr = token;
    int n = 0;
    while (*ptr == '\n' || *ptr == ' ' || *ptr == '\t') {
      ptr = token + ++n;
      if (n > len - 1) {
        break;
      }
    }
  }
  return ptr;
}

ConfigFile::ConfigFile() {
  _configFile = NULL;
  _noParams = 0;
}

ConfigFile::ConfigFile(const ConfigFile& orig) {
}

ConfigFile::~ConfigFile() {
  finis();
}

void ConfigFile::finis() {

  if (_configFile != NULL) {
    fclose(_configFile);
    _configFile = NULL;
  }

  _params.erase(_params.begin(), _params.end());
  _values.erase(_values.begin(), _values.end());
}

int ConfigFile::open(char* filename) {
  int ret = 0;

  _configFile = fopen(filename, "r");
  if (_configFile == NULL) {
    ret = 1;
  } else {
    parse();
    fclose(_configFile);
    _configFile = NULL;
  }

  return ret;
}

void ConfigFile::close() {
  finis();
}

int ConfigFile::parse() {
  int cnt = 0;
  char cfgLine[MAX_PARAM_SIZE * 2];
  char *cfgStr = cfgLine;
  char *token;
  char *value;
  size_t lineSize = MAX_PARAM_SIZE;

  _noParams = 0;
  while (getline(&cfgStr, &lineSize, _configFile) > 0) {
    if (strlen(cfgStr) < 4) {
      continue;
    }
    if (*cfgStr == '#') {
      continue;
    }
    // remove any newline at the end of string
    if (cfgLine[strlen(cfgStr) - 1 ] == '\n') {
      cfgLine[strlen(cfgStr) - 1] = 0;
    }
    token = strtok(cfgStr, "=");
    value = strtok(NULL, "=");
    token = trim(token);
    value = trim(value);
    struct paramPair* tuple = new paramPair();
    strncpy(tuple->param, token, MAX_PARAM_SIZE);
    strncpy(tuple->value, value, MAX_PARAM_SIZE);
    std::set<std::pair<int, struct paramPair*>>::iterator pit = _params.end();
    _params.insert(pit, std::pair<int, struct paramPair*>{_noParams++, tuple});
    _values.insert(std::pair<char*, char*>(tuple->param, tuple->value));
  }

  return cnt;
}

int ConfigFile::getParamCnt() {
  return _noParams;
}

int ConfigFile::getParam(int no, char** param, char** value) {
  int ret = 0;

  if (no < getParamCnt()) {
    std::set<std::pair<int, struct paramPair*>>::iterator it = _params.begin();
    for (; _params.size() > 0; --it) {
      if(it->first == no) {
        struct paramPair* pair = it->second;
        *param = pair->param;
        *value = pair->value;
        break;
      }
    }
  }
  return ret;
}

char* ConfigFile::getValue(char* param) {
  char* v = NULL;

  std::map<char*, char*>::iterator it = _values.find(param);
  if (it != _values.end()) {
    v = it->second;
  }
  return v;
}
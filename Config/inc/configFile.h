/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ConfigFile.h
 * Author: keithc
 *
 * Created on March 16, 2023, 9:33 AM
 */

#ifndef CONFIGFILE_H
#define CONFIGFILE_H

#include <stdio.h>
#include <string.h>
#include <set>
#include <map>

struct cmp_str
{
   bool operator()(char const *a, char const *b) const
   {
      return strcmp(a, b) < 0;
   }
};

class ConfigFile {
public:
    ConfigFile();
    ConfigFile(const ConfigFile& orig);
    virtual ~ConfigFile();
    int open(char* filename);
    void close();
    int parse();
    int getParam(int no, char** param, char**value);
    char* getValue(char* param);
    int getParamCnt();
    
    static const int MAX_PARAM_SIZE = 120;
    
private:
    void finis();
    
    std::set<std::pair<int, struct paramPair*>> _params;
    std::map<char*, char*, cmp_str> _values;
    FILE* _configFile;
    int _noParams;
};

struct paramPair {
    char param[ConfigFile::MAX_PARAM_SIZE];
    char value[ConfigFile::MAX_PARAM_SIZE];
};

#endif /* CONFIGFILE_H */


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


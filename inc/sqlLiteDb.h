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
 * File:   sqlLiteDb.h
 * Author: keithc
 *
 * Created on March 17, 2023, 6:16 AM
 */

#ifndef SQLLITEDB_H
#define SQLLITEDB_H

#include <vector>
#include <string>
#include <sqlite3.h>
#include "logMessage.h"

using namespace std;

class SqlLiteDb {
public:
    SqlLiteDb();
    SqlLiteDb(const SqlLiteDb& orig);
    virtual ~SqlLiteDb();
    int open(char* dbFileName);
    void requestRecords();
    void loadRecord(LogMessage* msg);
    std::vector<std::string> checkLog(std::string callSign);
    void close();

private:
    sqlite3 *_sqliteDb;
};

#endif /* SQLLITEDB_H */


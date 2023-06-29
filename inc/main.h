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
 * File:   main.h
 * Author: keithc
 *
 * Created on March 14, 2023, 9:17 AM
 */

#include "logger.h"

#ifndef MAIN_H
#define MAIN_H

const int MAX_PARAM_SIZE=120;
const int PARAM_UNASSIGNED = -1;

#ifndef MAIN_CPP
extern Clogger* theLog;
#else
Clogger* theLog;
#endif

typedef struct Params {
    
} Param;

#endif /* MAIN_H */


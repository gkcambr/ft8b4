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
 * File:   messageBuffer.h
 * Author: keithc
 *
 * Created on July 31, 2022, 11:51 AM
 */

#ifndef MESSAGEBUFFER_H
#define MESSAGEBUFFER_H

#include <vector>
#include <pthread.h>
#include "logMessage.h"

class MessageBuffer {

public:
    MessageBuffer(int maxSize);
    virtual ~MessageBuffer();
    void put(LogMessage* msg);
    int get_front(LogMessage** msgHandle);
    int get_back(LogMessage** msgHandle);
    void clear();
    unsigned long size();
private:
    
  std::vector<LogMessage*> _buffer;
  MessageBuffer* _instance = NULL;
  pthread_mutex_t _bufferMutex;
  int _maxBufferSize;
};

#endif /* MESSAGEBUFFER_H */


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
 * File:   MessageBuffer.cpp
 * Author: keithc
 * 
 * Created on July 31, 2022, 11:51 AM
 */

#include "messageBuffer.h"

MessageBuffer::MessageBuffer(int maxSize) {
  _maxBufferSize = maxSize;
  pthread_mutex_init(&_bufferMutex, NULL);
  pthread_mutex_unlock(&_bufferMutex);
}

MessageBuffer::~MessageBuffer() {
  pthread_mutex_unlock(&_bufferMutex);
  pthread_mutex_destroy(&_bufferMutex);
}

void MessageBuffer::put(LogMessage* msg) {
  
  pthread_mutex_lock(&_bufferMutex);
  std::vector<LogMessage*>::iterator it;
  it = _buffer.begin();
  if((int)_buffer.size() >= _maxBufferSize) {
    pthread_mutex_unlock(&_bufferMutex);
    return;
  }
  _buffer.insert(it, msg);
  pthread_mutex_unlock(&_bufferMutex);
}

int MessageBuffer::get_front(LogMessage** msgHandle) {
  int ret = 0;

  pthread_mutex_lock(&_bufferMutex);
  if(_buffer.empty()) {
    pthread_mutex_unlock(&_bufferMutex);
    return ret;
  }
  
  std::vector<LogMessage*>::iterator it;
  it = _buffer.begin();
  LogMessage* msg = *it;
  if(msg != NULL) {
    LogMessage* msgCopy = msg;
    _buffer.erase(it);
    *msgHandle = msgCopy;
    ret = msgCopy->getDataLen();
  }
  pthread_mutex_unlock(&_bufferMutex);
  return ret;
}

int MessageBuffer::get_back(LogMessage** msgHandle) {
  int ret = 0;
  
  pthread_mutex_lock(&_bufferMutex);
  if(_buffer.empty()) {
    pthread_mutex_unlock(&_bufferMutex);
    return ret;
  }

  std::vector<LogMessage*>::iterator it;
  it = _buffer.end();
  LogMessage* msg = *(--it);
  if(msg != NULL) {
    LogMessage* msgCopy = msg;
    _buffer.erase(it);
    *msgHandle = msgCopy;
    ret = msgCopy->getDataLen();
  }
  pthread_mutex_unlock(&_bufferMutex);
  return ret;
}

void MessageBuffer::clear() {
  pthread_mutex_lock(&_bufferMutex);
  _buffer.clear();
  pthread_mutex_unlock(&_bufferMutex);
}

unsigned long MessageBuffer::size() {
  int ret;
  
  pthread_mutex_lock(&_bufferMutex);
  ret = _buffer.size();
  pthread_mutex_unlock(&_bufferMutex);
  return ret;
}

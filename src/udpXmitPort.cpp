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
 * File:   udpXmitPort.cpp
 * Author: keithc
 * 
 * Created on May 15, 2022, 6:10 PM
 */

#include "logger.h"
#include "proxyChannel.h"

const int WRITE_BUF_SIZE = 1000;

// need to avoid circular header conflicts
ProxyChannel* _xmitParent;

UdpXmitPort::UdpXmitPort(
        char* channelName,
        char* nodeXmitToIpAddr,
        int nodeXmitToPort,
        int socketFD,
        char* proxyIpAddr,
        int proxyPort,
        MessageBuffer* buffer,
        sem_t* channelReadySemiphore) {

  strncpy(_channelName, channelName, MAX_PARAM_SIZE - 1);
  strncpy(_nodeXmitToIpAddr, nodeXmitToIpAddr, MAX_PARAM_SIZE - 1);
  _nodeXmitToPort = nodeXmitToPort;
  _xmitSocketFd = socketFD;
  strncpy(_proxyIpAddr, proxyIpAddr, MAX_PARAM_SIZE - 1);
  if(proxyPort ==  PARAM_UNASSIGNED) {
    _proxyXmitPort = 0;
  }
  else {
    _proxyXmitPort = proxyPort;
  }
  _channelBuffer = buffer;
  _channelReadySemiphore = channelReadySemiphore;
  _halt = false;

  memset(&_params, 0, sizeof (Params));
  configureXmitSocket();
}

UdpXmitPort::~UdpXmitPort() {
}

void UdpXmitPort::run() {
  process();
}

void UdpXmitPort::setParent(void* parent) {
  _xmitParent = (ProxyChannel*) parent;
}

bool UdpXmitPort::configureXmitSocket() {
  char msg[Clogger::MAX_LOG_MSG_SIZE];

  //prepare the proxy address and port
  memset((char *) &_proxyAddr, 0, sizeof (_proxyAddr));
  _proxyAddr.sin_family = AF_INET;
  _proxyAddr.sin_addr.s_addr = inet_addr(_proxyIpAddr);
  if (_proxyXmitPort != PARAM_UNASSIGNED) {
    _proxyAddr.sin_port = htons(_proxyXmitPort);
  } else {
    _proxyAddr.sin_port = htons(0);
  }

  // prepare the node address
  memset((char *) &_nodeAddr, 0, sizeof (_nodeAddr));
  _nodeAddr.sin_family = AF_INET;
  _nodeAddr.sin_port = htons(_nodeXmitToPort);
  _nodeAddr.sin_addr.s_addr = inet_addr(_nodeXmitToIpAddr);

  // log the true proxy address and port
  snprintf(msg, Clogger::MAX_LOG_MSG_SIZE, "prepared proxy xmit at address %s and port %d\n",
          _proxyIpAddr, _proxyXmitPort);
  LOG_MSG(theLog, Clogger::LOG_INFO, msg);

  return true;
}

void UdpXmitPort::copyXmitMsg(struct XMIT_MSG* dest, struct XMIT_MSG* src) {
}

void UdpXmitPort::initXmitMsg(struct XMIT_MSG* msg) {
}

int UdpXmitPort::process() {
  char msg[Clogger::MAX_LOG_MSG_SIZE];
  int ret = 0;

  snprintf(msg, Clogger::MAX_LOG_MSG_SIZE, "ready to send to node %s address %s port %d\n",
          _channelName, _nodeXmitToIpAddr, _nodeXmitToPort);
  LOG_MSG(theLog, Clogger::LOG_INFO, msg);

  for (;;) {

    int v;
    sem_getvalue(_channelReadySemiphore, &v);
    sem_wait(_channelReadySemiphore);
    if (_halt) {
      break;
    }
    while (_channelBuffer->size() > 0) {
      LogMessage* msgPtr;
      _channelBuffer->get_back(&msgPtr);

      // create a copy for viewing
      LogMessage* viewMsg = new LogMessage(*msgPtr);
      _logView->show(viewMsg);

      sendto(_xmitSocketFd, msgPtr->getData(),
              msgPtr->getDataLen(), 0,
              (const struct sockaddr *) &_nodeAddr,
              sizeof (_nodeAddr));

      // the original LogMessage was constructed by the receive port.
      // it needs to be deleted now.
      delete(msgPtr);
    }
  }
  return ret;
}

int UdpXmitPort::getPortNo() {
  return _nodeXmitToPort;
}

void UdpXmitPort::setLogView(LogView* view) {
  _logView = view;
}

void UdpXmitPort::halt() {
  _halt = true;
  sem_post(_channelReadySemiphore);
}

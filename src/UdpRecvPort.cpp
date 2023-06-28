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
 * File:   UdpRecvPort.cpp
 * Author: keithc
 * 
 * Created on May 15, 2022, 6:12 PM
 */

#include <iostream>
#include "main.h"
#include "logger.h"
#include "udpRecvPort.h"
#include "proxyChannel.h"

UdpRecvPort::UdpRecvPort(
        char* channelName,
        char* nodeRecvFromIpAddr,
        int nodeRecvFromPort,
        int socketFD,
        char* proxyAddr,
        int proxyPort,
        MessageBuffer* buffer,
        sem_t* channelReadySemiphore) {

  strncpy(_channelName, channelName, MAX_PARAM_SIZE - 1);
  strncpy(_nodeRecvFromIpAddr, nodeRecvFromIpAddr, MAX_PARAM_SIZE - 1);
  if(nodeRecvFromPort == PARAM_UNASSIGNED) {
    _nodeRecvFromPort = 0;
  }
  else {
    _nodeRecvFromPort = nodeRecvFromPort;
  }
  _proxyRecvPort = proxyPort;
  _recvSocketFd = socketFD;
  strncpy(_proxyIpAddr, proxyAddr, MAX_PARAM_SIZE - 1);
  _channelBuffer = buffer;
  _udpResponseXmitPort = NULL;
  _channelReadySemiphore = channelReadySemiphore;
  _xmitReponseSocketFd = 0;
  _halt = false;

  configureRecvSocket();
}

UdpRecvPort::~UdpRecvPort() {
}

void UdpRecvPort::run() {
  process();
}

bool UdpRecvPort::configureRecvSocket() {
  char msg[Clogger::MAX_LOG_MSG_SIZE];

  //prepare and bind the proxy address to the receive port
  memset((char *) &_proxyAddr, 0, sizeof (_proxyAddr));
  _proxyAddr.sin_family = AF_INET;
  _proxyAddr.sin_addr.s_addr = INADDR_ANY;
  _proxyAddr.sin_port = htons(_proxyRecvPort);

  // bind the socket with the proxy address 
  if (bind(_recvSocketFd, (const struct sockaddr *) &_proxyAddr,
          sizeof (_proxyAddr)) < 0) {
    snprintf(msg, Clogger::MAX_LOG_MSG_SIZE, "proxy receive port bind for %s failed to address %s port %d: %s\n",
            _channelName, _proxyIpAddr, _proxyRecvPort, strerror(errno));
    LOG_MSG(theLog, Clogger::LOG_MAJOR, msg);
    return false;
  }
  snprintf(msg, Clogger::MAX_LOG_MSG_SIZE, "proxy receive port bind for %s succeded to address %s port %d\n",
          _channelName, _proxyIpAddr, _proxyRecvPort);
  LOG_MSG(theLog, Clogger::LOG_INFO, msg);

  // prepare the receive node address
  memset((char *) &_nodeAddr, 0, sizeof (_nodeAddr));
  _nodeAddr.sin_family = AF_INET;
  _nodeAddr.sin_addr.s_addr = inet_addr(_nodeRecvFromIpAddr);
  if (_nodeRecvFromPort != PARAM_UNASSIGNED) {
    _nodeAddr.sin_port = htons(_nodeRecvFromPort);
  }

  return true;
}

bool UdpRecvPort::configureResponseSocket() {
  char msg[Clogger::MAX_LOG_MSG_SIZE];

  //prepare the proxy address and port
  memset((char *) &_proxyAddr, 0, sizeof (_proxyAddr));
  _proxyAddr.sin_family = AF_INET;
  _proxyAddr.sin_addr.s_addr = inet_addr(_proxyIpAddr);
  // use any available port
  _proxyResponsePort = 0;
  _proxyAddr.sin_port = htons(_proxyResponsePort);

  // prepare the node address
  memset((char *) &_nodeAddr, 0, sizeof (_nodeAddr));
  _nodeAddr.sin_family = AF_INET;
  _nodeAddr.sin_port = htons(_proxyResponsePort);
  _nodeAddr.sin_addr.s_addr = inet_addr(_nodeRecvFromIpAddr);

  // declare the SOCK_DGRAM transmit response socket
  if ((_xmitReponseSocketFd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) <= 0) {
    LOG_MSG(theLog, Clogger::LOG_CRITICAL, "failed to create xmit response socket");
  }

  snprintf(msg, Clogger::MAX_LOG_MSG_SIZE, "prepared response xmit to address %s and port %d\n",
          _nodeRecvFromIpAddr, _nodeRecvFromPort);
  LOG_MSG(theLog, Clogger::LOG_INFO, msg);

  return true;
}

int UdpRecvPort::process() {
  char msg[Clogger::MAX_LOG_MSG_SIZE];
  unsigned char readBuf[MAX_WSJTX_MSG_SIZE];
  int ret = 0;

  int nodeAddrLen = sizeof (_nodeAddr);

  snprintf(msg, Clogger::MAX_LOG_MSG_SIZE, "listening to node %s address %s node port %d\n", 
          _channelName, _nodeRecvFromIpAddr, _nodeRecvFromPort);
  LOG_MSG(theLog, Clogger::LOG_INFO, msg);

  for (;;) {

    if (_halt) {
      break;
    }

    int readCnt = recvfrom(_recvSocketFd, readBuf, MAX_WSJTX_MSG_SIZE, MSG_WAITALL,
            (struct sockaddr *) &_nodeAddr, (socklen_t*) & nodeAddrLen);

    char* senderAddr = inet_ntoa(_nodeAddr.sin_addr);
    int lastPort = _nodeRecvFromPort;
    _nodeRecvFromPort = ntohs(_nodeAddr.sin_port);
    if (lastPort != _nodeRecvFromPort || _xmitReponseSocketFd == 0) {
      configureResponseSocket();
    }

    if (readCnt < 0) {
      char strerr[200];
      strncpy(strerr, strerror(errno), 199);
      snprintf(msg, Clogger::MAX_LOG_MSG_SIZE, "receive failure from addr %s port %d : err msg %s",
              senderAddr, _nodeRecvFromPort, strerr);
      LOG_MSG(theLog, Clogger::LOG_CRITICAL, msg);
      continue;
    } else if (readCnt == 0) {
      continue;
    } else if (readCnt > 0) {
      LogMessage* recvMsg = new LogMessage(readBuf, readCnt);
      if (recvMsg->getType() != LogMessage::MSG_TYPE_CLEAR &&
              recvMsg->getType() != LogMessage::MSG_TYPE_UNDEFINED) {
        _channelBuffer->put(recvMsg);
        sem_post(_channelReadySemiphore);
        int v;
        sem_getvalue(_channelReadySemiphore, &v);
      }
    }
  }
  return ret;
}

int UdpRecvPort::xmitResponse(unsigned char *buf, int buflen) {
  int ret = 0;

  ret = sendto(_xmitReponseSocketFd, buf,
          buflen, 0,
          (const struct sockaddr *) &_nodeAddr,
          sizeof (_nodeAddr));

  return ret;
}

int UdpRecvPort::getPortNo() {
  return _proxyRecvPort;
}

void UdpRecvPort::halt() {
  _halt = true;
  shutdown(_recvSocketFd, SHUT_RDWR);
}

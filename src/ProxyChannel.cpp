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
 * File:   RecvPort.cpp
 * Author: keithc
 * 
 * Created on May 15, 2022, 5:44 PM
 */

#include <iostream>
#include <string.h>
#include "main.h"
#include "proxyChannel.h"

const int MAX_BUFFER_SIZE = 10;

void *startXmitThread(void *p_param) {
  char msg[Clogger::MAX_LOG_MSG_SIZE];
  UdpXmitPort* xmitPort = (UdpXmitPort*) p_param;
  xmitPort->run();
  snprintf(msg, Clogger::MAX_LOG_MSG_SIZE,
          "xmit thread failed on xmit port %d", xmitPort->getPortNo());
  LOG_MSG(theLog, Clogger::LOG_CRITICAL, msg);
  return NULL;
}

void *startRecvThread(void *p_param) {
  char msg[Clogger::MAX_LOG_MSG_SIZE];
  UdpRecvPort* recvPort = (UdpRecvPort*) p_param;
  recvPort->run();
  snprintf(msg, Clogger::MAX_LOG_MSG_SIZE,
          "xmit thread failed on recv port %d", recvPort->getPortNo());
  LOG_MSG(theLog, Clogger::LOG_CRITICAL, msg);
  return NULL;
}

ProxyChannel::ProxyChannel(
        // xmit to - node info
        char* xmitTolName,
        char* xmitToIpAddr,
        int xmitToPort,
        int xmitToSocketFD,
        // recv from - node info
        char* recvFromName,
        char* recvFromIpAddr,
        int recvFromPort,
        int recvFromSocketFD,
        // proxy info
        char* proxyIpAddr,
        int proxyXmitPort,
        int proxyRecvPort) {

  // xmit to - node info
  strncpy(_xmitToName, xmitTolName, MAX_PARAM_SIZE - 1);
  strncpy(_nodeXmitToIpAddr, xmitToIpAddr, MAX_PARAM_SIZE - 1);
  _nodeXmitToPort = xmitToPort;
  _xmitToSocketFD = xmitToSocketFD;
  // recv from - node info
  strncpy(_recvFromName, recvFromName, MAX_PARAM_SIZE - 1);
  strncpy(_nodeRecvFromIpAddr, recvFromIpAddr, MAX_PARAM_SIZE - 1);
  _nodeRecvFromPort = recvFromPort;
  _recvFromSocketFD = recvFromSocketFD;
  // proxy info
  strncpy(_proxyAddr, proxyIpAddr, MAX_PARAM_SIZE - 1);
  _proxyXmitPort = proxyXmitPort;
  _proxyRecvPort = proxyRecvPort;

  _buffer = new MessageBuffer(MAX_BUFFER_SIZE);
  sem_init(&_channelReadySemiphore, 0, 1);

  _udpRecvPort = new UdpRecvPort(
          _recvFromName,
          _nodeRecvFromIpAddr,
          _nodeRecvFromPort,
          _recvFromSocketFD,
          _proxyAddr,
          _proxyRecvPort,
          _buffer,
          &_channelReadySemiphore);

  _udpXmitPort = new UdpXmitPort(
          _xmitToName,
          _nodeXmitToIpAddr,
          _nodeXmitToPort,
          _xmitToSocketFD,
          _proxyAddr,
          _proxyXmitPort,
          _buffer,
          &_channelReadySemiphore);

  memset(&_xmitTh, 0, sizeof (pthread_t));
  memset(&_recvTh, 0, sizeof (pthread_t));
}

int ProxyChannel::openXmit() {
  int ret = 0;

  // start the transmit thread
  int xth = pthread_create(&_xmitTh, NULL, startXmitThread, _udpXmitPort);
  if (xth != 0) {
    LOG_MSG(theLog, Clogger::LOG_CRITICAL, "failed to start udp transmit thread");
    ret = 1;
  }
  return ret;
}

int ProxyChannel::openRecv() {
  int ret = 0;

  // start the receive thread
  int rth = pthread_create(&_recvTh, NULL, startRecvThread, _udpRecvPort);
  if (rth != 0) {
    LOG_MSG(theLog, Clogger::LOG_CRITICAL, "failed to start udp receive thread");
    ret = 1;
  }
  return ret;
}

ProxyChannel::~ProxyChannel() {
  delete _buffer;
  delete _udpXmitPort;
  delete _udpRecvPort;
}

void ProxyChannel::setView(LogView *view) {
  _view = view;
  _udpXmitPort->setLogView(_view);
}

void ProxyChannel::xmitResponse(LogMessage *msg) {

  unsigned char buf[msg->getDataLen() + 10];
  unsigned char *bufPtr = buf;
  
  int buflen = msg->prepareResponse(&bufPtr, msg->getDataLen() + 10);
   _udpRecvPort->xmitResponse(buf, buflen);
  delete msg;
  
//  FILE *newmsg = fopen("newmsg.info", "w");
//  fwrite(buf, 1, buflen, newmsg);
//  fclose(newmsg);
}

int ProxyChannel::process() {
  int ret = 0;
  openRecv();
  openXmit();
  
  _view->scanKeyboard();

  _udpXmitPort->halt();
  _udpRecvPort->halt();
  
  void* ptrRet = NULL;
  pthread_join(_xmitTh, &ptrRet);
  pthread_join(_recvTh, &ptrRet);
  
  return ret;
}
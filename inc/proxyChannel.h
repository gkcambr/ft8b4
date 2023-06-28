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
 * File:   RecvPort.h
 * Author: keithc
 *
 * Created on May 15, 2022, 5:44 PM
 */

#ifndef RECVPORT_H
#define RECVPORT_H

#include <pthread.h>
#include <semaphore.h>
#include "messageBuffer.h"
#include "udpRecvPort.h"
#include "udpXmitPort.h"

class ProxyChannel {
public:
    ProxyChannel(
            // xmit to - node info
            char* xmitToName,
            char* xmitToIpAddr,
            int xmitToPort,
            int xmitToSocketFD,
            // recv from - node info
            char* recvFromName,
            char* recvFromIpAddr,
            int recvFromPort,
            int recvFromSocketFD,
            // proxy info
            char* proxyAddr,
            int proxyXmitPort,
            int proxyRecvPort);
    ~ProxyChannel();

    int openXmit();
    int openRecv();
    void setView(LogView *view);
    void xmitResponse(LogMessage *msg);
    int process();

private:
    /* properties */

    // xmit to - node info
    char _xmitToName[MAX_PARAM_SIZE];
    char _nodeXmitToIpAddr[MAX_PARAM_SIZE];
    int _nodeXmitToPort;
    int _xmitToSocketFD;
    // recv from - node info
    char _recvFromName[MAX_PARAM_SIZE];
    char _nodeRecvFromIpAddr[MAX_PARAM_SIZE];
    int _nodeRecvFromPort;
    int _recvFromSocketFD;
    // proxy info
    char _proxyAddr[MAX_PARAM_SIZE];
    int _proxyXmitPort;
    int _proxyRecvPort;
    // ports
    UdpXmitPort* _udpXmitPort;
    UdpRecvPort* _udpRecvPort;
    // buffers
    MessageBuffer* _buffer;
    // terminal view
    LogView* _view;
    // semiphores
    sem_t _channelReadySemiphore;
    // threads
    pthread_t _recvTh;
    pthread_t _xmitTh;
};

#endif /* RECVPORT_H */

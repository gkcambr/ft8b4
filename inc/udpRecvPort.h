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
 * File:   udpRecvPort.h
 * Author: keithc
 *
 * Created on May 15, 2022, 6:12 PM
 */

#ifndef UDPRECVPORT_H
#define UDPRECVPORT_H

#include <sys/socket.h>
#include <semaphore.h>
#include "main.h"
#include "messageBuffer.h"
#include "udpXmitPort.h"

using namespace std;

class UdpRecvPort {
public:

    UdpRecvPort(
        char* channelName,
        char* nodeIpAddr,
        int nodePort,
        int socketFD,
        char* proxyAddr,
        int proxyPort,
        MessageBuffer* buffer,
        sem_t* channelReadySemiphore);
    ~UdpRecvPort();

    void run();
    bool configureRecvSocket();
    bool configureResponseSocket();
    int xmitResponse(unsigned char *buf, int buflen);
    int process();
    int getPortNo();
    void halt();

    /* properties */
private:
    UdpXmitPort* _udpResponseXmitPort;
    MessageBuffer* _channelBuffer;
    char _channelName[MAX_PARAM_SIZE];
    char _nodeRecvFromIpAddr[MAX_PARAM_SIZE];
    int _nodeRecvFromPort;
    int _recvSocketFd;
    int _xmitReponseSocketFd;
    char _proxyIpAddr[MAX_PARAM_SIZE];
    int _proxyRecvPort;
    int _proxyResponsePort;
    struct sockaddr_in _nodeAddr, _proxyAddr;
    sem_t* _channelReadySemiphore;
    bool _halt;
};

#endif /* UDPRECVPORT_H */

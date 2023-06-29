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
 * File:   udpXmitPort.h
 * Author: keithc
 *
 * Created on May 15, 2022, 6:10 PM
 */

#ifndef UDPXMITPORT_H
#define UDPXMITPORT_H

#include <semaphore.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include "main.h"
#include "messageBuffer.h"
#include "logView.h"

struct XMIT_MSG {
  unsigned char* bufPtr;
  int msgLen;
};

class UdpXmitPort {
public:
    UdpXmitPort(
        char* channelName,
        char* nodeXmitToIpAddr,
        int nodeXmitToPort,
        int socketFD,
        char* proxyIpAddr,
        int proxyXmitPort,
        MessageBuffer* buffer,
        sem_t* channelReadySemiphore);
    ~UdpXmitPort();

    void run();
    void setParent(void* parent);
    void setLogView(LogView* view);
    bool configureXmitSocket();
    int process();
    int getPortNo();
    int setParams(Param* params);
    bool encode(unsigned char** buf, int* len);
    void copyXmitMsg(struct XMIT_MSG* dest, struct XMIT_MSG* src);
    void initXmitMsg(struct XMIT_MSG* msg);
    void halt();

    /* properties */
private:
    MessageBuffer* _channelBuffer;
    LogView* _logView;
    struct sockaddr_in _nodeAddr, _proxyAddr;
    sem_t* _channelReadySemiphore;
    char _channelName[MAX_PARAM_SIZE];
    char _nodeXmitToIpAddr[MAX_PARAM_SIZE];
    int _nodeXmitToPort;
    char _proxyIpAddr[MAX_PARAM_SIZE];
    int _proxyXmitPort;
    int _xmitSocketFd;
    // params
    Params _params;
    bool _halt;
};

#endif /* UDPXMITPORT_H */

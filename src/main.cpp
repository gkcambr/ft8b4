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
 * File:   main.cpp
 * Author: keithc
 *
 * Created on March 14, 2023, 8:52 AM
 */

#define MAIN_CPP

#include <cstdlib>
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <semaphore.h>
#include <time.h>
#include "configFile.h"
#include "main.h"
#include "proxyChannel.h"
#include "sqlLiteDb.h"

using namespace std;

const char* USAGE = "usage: %s configFile";
sem_t _finisSemiphore;

void term(int signum) {
  sem_post(&_finisSemiphore);
}

/*
 * 
 */
int main(int argc, char** argv) {
  struct sigaction action;
  memset(&action, 0, sizeof (action));
  action.sa_handler = term;
  sigaction(SIGTERM, &action, NULL);

  if (argc != 2) {
    fprintf(stdout, "%s\n", USAGE);
    exit(1);
  }

  sem_init(&_finisSemiphore, 0, 0);

  // xmit to - node info
  char xmitToName[MAX_PARAM_SIZE];
  memset(xmitToName, 0, MAX_PARAM_SIZE);
  char xmitToIpAddr[MAX_PARAM_SIZE];
  memset(xmitToIpAddr, 0, MAX_PARAM_SIZE);
  int xmitToPort = PARAM_UNASSIGNED;
  int xmitToSocketFD;

  // recv from - node info
  char recvFromName[MAX_PARAM_SIZE];
  memset(recvFromName, 0, MAX_PARAM_SIZE);
  char recvFromIpAddr[MAX_PARAM_SIZE];
  memset(recvFromIpAddr, 0, MAX_PARAM_SIZE);
  int recvFromPort = PARAM_UNASSIGNED;
  int recvFromSocketFD;

  // proxy info
  char proxyAddr[MAX_PARAM_SIZE];
  int proxyXmitPort;
  int proxyRecvPort = PARAM_UNASSIGNED;

  // database info
  char sqlite3DbFile[MAX_PARAM_SIZE];
  memset(sqlite3DbFile, 0, MAX_PARAM_SIZE);

  // view info
  int skipDays = 365;
  time_t msgTTL = 180;

  // ft8B4 application log
  char* logFile = NULL;

  // default assignments
  strncpy(xmitToName, (char*) "klog", 5);
  strncpy(xmitToIpAddr, (char*) "127.0.0.1", 11);
  xmitToPort = 3333;

  strncpy(recvFromName, (char*) "wsjtx", 6);
  strncpy(recvFromIpAddr, (char*) "127.0.0.1", 11);
  recvFromPort = PARAM_UNASSIGNED;

  strncpy(proxyAddr, (char*) "127.0.0.1", 11);
  proxyXmitPort = PARAM_UNASSIGNED;
  proxyRecvPort = PARAM_UNASSIGNED;

  ConfigFile* cfg = new ConfigFile();
  if (cfg->open(*(++argv)) != 0) {
    fprintf(stdout, "cannot open config file %s\n", *argv);
    getchar();
    exit(1);
  }

  char* v = cfg->getValue((char*) "xmitToName");
  if (v != NULL) {
    strncpy(xmitToName, v, 100);
  }
  v = cfg->getValue((char*) "xmitToIpAddr");
  if (v != NULL) {
    strncpy(xmitToIpAddr, v, 30);
  }
  v = cfg->getValue((char*) "xmitToPort");
  if (v != NULL) {
    xmitToPort = atoi(v);
  }

  v = cfg->getValue((char*) "recvFromName");
  if (v != NULL) {
    strncpy(recvFromName, v, 100);
  }
  v = cfg->getValue((char*) "recvFromIpAddr");
  if (v != NULL) {
    strncpy(recvFromIpAddr, v, 30);
  }
  v = cfg->getValue((char*) "recvFromPort");
  if (v != NULL) {
    recvFromPort = atoi(v);
  }

  v = cfg->getValue((char*) "proxyAddr");
  if (v != NULL) {
    strncpy(proxyAddr, v, 100);
  }
  v = cfg->getValue((char*) "proxyRecvPort");
  if (v != NULL) {
    proxyRecvPort = atoi(v);
  }

  v = cfg->getValue((char*) "sqlite3Db");
  if (v != NULL) {
    strncpy(sqlite3DbFile, v, 100);
  }

  v = cfg->getValue((char*) "skipDays");
  if (v != NULL) {
    skipDays = atoi(v);
  }

  v = cfg->getValue((char*) "logFile");
  if (v != NULL) {
    logFile = v;
  }
  
  v = cfg->getValue((char*) "msgTTL");
  if (v != NULL) {
    msgTTL = atol(v);
  }

  cfg->close();

  // check for a log file
  if (logFile != NULL) {
    theLog = new Clogger();
    theLog->openLogFile((char*) logFile, true);
    theLog->setLevel(Clogger::LOG_DEBUG);
  } else {
    theLog = NULL;
  }

  // check for existence of db file
  FILE* db = NULL;
  if (sqlite3DbFile != NULL) {
    db = fopen(sqlite3DbFile, "r");
  }
  if (db == NULL) {
    fprintf(stderr, "cannot open database file %s\n", sqlite3DbFile);
    getchar();
    exit(1);
  }
  fclose(db);

  // check for mandatory parameters
  if (strnlen(xmitToName, MAX_PARAM_SIZE) < 3) {
    fprintf(stderr, "missing mandatory parameter xmitToName in config file\n");
    getchar();
    exit(1);
  }
  if (strnlen(xmitToIpAddr, MAX_PARAM_SIZE) < 7) {
    fprintf(stderr, "missing mandatory parameter xmitToIpAddr in config file\n");
    getchar();
    exit(1);
  }
  if (xmitToPort == PARAM_UNASSIGNED) {
    fprintf(stderr, "missing mandatory parameter xmitToPort in config file\n");
    getchar();
    exit(1);
  }
  if (strnlen(recvFromName, MAX_PARAM_SIZE) < 3) {
    fprintf(stderr, "missing mandatory parameter recvFromName in config file\n");
    getchar();
    exit(1);
  }
  if (strnlen(recvFromIpAddr, MAX_PARAM_SIZE) < 7) {
    fprintf(stderr, "missing mandatory parameter recvFromIpAddr in config file\n");
    getchar();
    exit(1);
  }
  if (strnlen(proxyAddr, MAX_PARAM_SIZE) < 7) {
    fprintf(stderr, "missing mandatory parameter proxyAddr in config file\n");
    getchar();
    exit(1);
  }
  if (proxyRecvPort == PARAM_UNASSIGNED) {
    fprintf(stderr, "missing mandatory parameter proxyRecvPort in config file\n");
    getchar();
    exit(1);
  }

  // declare the SOCK_DGRAM receive socket
  if ((recvFromSocketFD = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) <= 0) {
    LOG_MSG(theLog, Clogger::LOG_CRITICAL, "failed to create recv socket");
    getchar();
    exit(1);
  }

  // declare the SOCK_DGRAM transmit socket
  if ((xmitToSocketFD = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) <= 0) {
    LOG_MSG(theLog, Clogger::LOG_CRITICAL, "failed to create xmit socket");
    getchar();
    exit(1);
  }
  
  // set the title bar
  printf("%c]0;%s%c", '\033', "ft8b4", '\007');

  ProxyChannel* channel = new ProxyChannel(
          // xmit to - node info
          xmitToName,
          xmitToIpAddr,
          xmitToPort,
          xmitToSocketFD,
          // recv from - node info
          recvFromName,
          recvFromIpAddr,
          recvFromPort,
          recvFromSocketFD,
          // proxy info
          proxyAddr,
          proxyXmitPort,
          proxyRecvPort);
  
  // open the terminal viewport
  LogView* logView = new LogView(channel);
  logView->setDatabase(sqlite3DbFile);
  logView->setSkipDays(skipDays);
  logView->setMsgTTL(msgTTL);

  channel->setView(logView);
  channel->process();

  // clean up
  if (theLog != NULL) {
    theLog->closeLogFile();
    delete theLog;
  }
  delete channel;

  return 0;
}


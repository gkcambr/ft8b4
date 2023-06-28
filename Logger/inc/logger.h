/* 
 * File:   logger.h
 * Author: keithc
 *
 * Created on October 17, 2016, 9:07 AM
 */


#ifndef LOGGER_H
#define	LOGGER_H

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

extern char* mystrncpy(char* dest, const char*src, size_t n);

#define LOG_MSG(myLog, lvl, msg) {if(myLog != NULL) { ((Clogger*)myLog)->log(lvl, (long)pthread_self(), (const char*)__FILE__, __LINE__, (const char*)msg);}}

class Clogger {
public:
    Clogger();
    virtual ~Clogger();
    bool openLogFile(char *fileName, bool logDaily);
    void getDateStr(char *str);
    void setLevel(int level);
    void printMetadata();
    void printHeader();
    void log(int level, long id, char const* src, int line, char const* msg);
    void closeLogFile();
    
    const char* SEV_LABELS[6] = 
    {"Critical", "Major", "Minor", "Warning", "Info", "Debug"};

    static const int LOG_CRITICAL = 0;
    static const int LOG_MAJOR = 1;
    static const int LOG_MINOR = 2;
    static const int LOG_WARN = 3;
    static const int LOG_INFO = 4;
    static const int LOG_DEBUG = 5;
    static const int NO_LOG_LEVELS = 6;
    
    static const int MAX_LOG_MSG_SIZE = 300;
    static const int MAX_FILE_NAME_SIZE = 300;

protected:
private:
    pthread_mutex_t _critSection;
    FILE *_logFile = NULL;
    fpos_t _pos;
    fpos_t *_lastPos;
    char _logFileName[MAX_FILE_NAME_SIZE];
    char _dateLogFileName[MAX_FILE_NAME_SIZE];
    char _dateStr[MAX_LOG_MSG_SIZE];
    int _logLevel;
    int _errCnt[NO_LOG_LEVELS];
    bool _dailyLogs;
};

#endif	/* LOGGER_H */
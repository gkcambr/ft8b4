/* 
 * File:   logger.cpp
 * Author: keithc
 * 
 * Created on October 17, 2016, 9:07 AM
 */

#include <string.h>
#include <sys/time.h>
#include <time.h>
#include "logger.h"

char* mystrncpy(char* dest, const char*src, size_t n) {
  memset(dest, 0, n);
  memcpy(dest, src, strnlen(src, n-1));
  return dest;
}

Clogger::Clogger() {
    _logLevel = LOG_DEBUG;
    getDateStr(_dateStr);
    _lastPos = NULL;
    _dailyLogs = true;
    pthread_mutex_init(&_critSection, NULL);
    pthread_mutex_unlock(&_critSection);
}

Clogger::~Clogger() {
    pthread_mutex_unlock(&_critSection);
    pthread_mutex_destroy(&_critSection);
}

void Clogger::getDateStr(char *str) {
    struct tm date;
    time_t timePtr;

    time(&timePtr);
    localtime_r(&timePtr, &date);
    snprintf(str, MAX_LOG_MSG_SIZE, "%d_%d_%d", (int) (date.tm_mon + 1), (int) (date.tm_mday), (int) (date.tm_year + 1900));
}

bool Clogger::openLogFile(char* fileName, bool logDaily) {
    bool ret = false;
    char extension = 'A';
    int maxOpenAttempts = 26;

    pthread_mutex_lock(&_critSection);
    _dailyLogs = logDaily;
    if (_logFile != NULL) {
        pthread_mutex_unlock(&_critSection);
        closeLogFile();
        pthread_mutex_lock(&_critSection);
    }

    // zero the error counts
    for (int lvl = LOG_CRITICAL; lvl <= LOG_DEBUG; lvl++) {
        _errCnt[lvl] = 0;
    }

    if (_dailyLogs) {
        getDateStr(_dateStr);
        snprintf(_dateLogFileName, MAX_FILE_NAME_SIZE, "%s_%s.xml", fileName, _dateStr);
        _logFile = fopen(_dateLogFileName, "r");
        while (_logFile != NULL && (--maxOpenAttempts > 0)) {
            fclose(_logFile);
            snprintf(_dateLogFileName, MAX_FILE_NAME_SIZE, "%s_%c_%s.xml", fileName, extension++, _dateStr);
            _logFile = fopen(_dateLogFileName, "r");
        }
        if (_logFile != NULL) {
            fclose(_logFile);
        }
        _logFile = fopen(_dateLogFileName, "w");
    } else {
        snprintf(_dateLogFileName, MAX_FILE_NAME_SIZE, "%s.xml", fileName);
        _logFile = fopen(_dateLogFileName, "a");
    }
    strncpy(_logFileName, fileName, strnlen(fileName, MAX_FILE_NAME_SIZE) + 1);
    ret = true;
    char *msg = (char*) "<?xml version='1.0' encoding='us-ascii'?>\n";
    size_t len = strlen(msg);
    fwrite(msg, len, sizeof (char), _logFile);
    msg = (char*) "<?xml-stylesheet type=\"text/css\" href=\"logger.css\" ?>\n";
    len = strlen(msg);
    fwrite(msg, len, sizeof (char), _logFile);
    msg = (char*) "<xmlfile>\n";
    len = strlen(msg);
    fwrite(msg, len, sizeof (char), _logFile);
    fflush(_logFile);
    fgetpos(_logFile, &_pos);
    _lastPos = &_pos;
    printHeader();

    printMetadata();
    ret = true;
    pthread_mutex_unlock(&_critSection);
    return ret;
}

void Clogger::setLevel(int level) {
    pthread_mutex_lock(&_critSection);
    if (level >= LOG_CRITICAL && level <= LOG_DEBUG) {
        _logLevel = level;
    }
    pthread_mutex_unlock(&_critSection);
}

void Clogger::log(int level, long id, char const* src, int line, char const* msg) {
    char printMsg[MAX_LOG_MSG_SIZE];
    char currentDate[MAX_LOG_MSG_SIZE];
    int len;
    struct timeval tsecs;
    struct timezone tzone;

    pthread_mutex_lock(&_critSection);
    if (level > _logLevel) {
        pthread_mutex_unlock(&_critSection);
        return;
    }


    getDateStr(currentDate);
    if (_dailyLogs && strcmp(currentDate, _dateStr) != 0) {
        pthread_mutex_unlock(&_critSection);
        closeLogFile();
        openLogFile(_logFileName, _dailyLogs);
        pthread_mutex_lock(&_critSection);
    }

    if (_lastPos != NULL) {
        fsetpos(_logFile, &_pos);
    }
    gettimeofday(&tsecs, &tzone);
    snprintf(printMsg, MAX_LOG_MSG_SIZE, "<msg millis=\"%ld%ld\">\n", tsecs.tv_sec, tsecs.tv_usec);
    len = strlen(printMsg);
    fwrite(printMsg, len, sizeof (char), _logFile);

    struct tm date;
    time_t timePtr;
    time(&timePtr);
    localtime_r(&timePtr, &date);
    snprintf(printMsg, MAX_LOG_MSG_SIZE, "<date>%d.%d.%d</date>\n", (int)(date.tm_mon + 1), (int)date.tm_mday, (int)(date.tm_year + 1900));
    len = strlen(printMsg);
    fwrite(printMsg, len, sizeof (char), _logFile);

    snprintf(printMsg, MAX_LOG_MSG_SIZE, "%ld", tsecs.tv_usec);
    char msecs[10];
    len = strlen(printMsg);
    strncpy(msecs, printMsg + len - 3, 3);
    msecs[3] = '\0';

    snprintf(printMsg, MAX_LOG_MSG_SIZE, "<time>%d.%d.%d.%s</time>\n", date.tm_hour, date.tm_min, date.tm_sec, msecs);
    len = strlen(printMsg);
    fwrite(printMsg, len, sizeof (char), _logFile);

    snprintf(printMsg, MAX_LOG_MSG_SIZE, "<severity>%s</severity>\n", SEV_LABELS[level]);
    len = strlen(printMsg);
    fwrite(printMsg, len, sizeof (char), _logFile);
    _errCnt[level]++;

    snprintf(printMsg, MAX_LOG_MSG_SIZE, "<ID>%ld</ID>\n", id);
    len = strlen(printMsg);
    fwrite(printMsg, len, sizeof (char), _logFile);

    snprintf(printMsg, MAX_LOG_MSG_SIZE, "<source>%s</source>\n", src);
    len = strlen(printMsg);
    fwrite(printMsg, len, sizeof (char), _logFile);

    snprintf(printMsg, MAX_LOG_MSG_SIZE, "<lineNo>%d</lineNo>\n", line);
    len = strlen(printMsg);
    fwrite(printMsg, len, sizeof (char), _logFile);

    snprintf(printMsg, MAX_LOG_MSG_SIZE, "<text>%s</text>\n", msg);
    len = strlen(printMsg);
    fwrite(printMsg, len, sizeof (char), _logFile);

    snprintf(printMsg, MAX_LOG_MSG_SIZE, "</msg>\n");
    len = strlen(printMsg);
    fwrite(printMsg, len, sizeof (char), _logFile);

    fflush(_logFile);
    fgetpos(_logFile, &_pos);
    _lastPos = &_pos;
    printMetadata();

    fflush(_logFile);

    pthread_mutex_unlock(&_critSection);
}

void Clogger::closeLogFile() {

    pthread_mutex_lock(&_critSection);
    if (_logFile != NULL) {
        fclose(_logFile);
        _logFile = 0;
    }
    _lastPos = NULL;
    pthread_mutex_unlock(&_critSection);
}

void Clogger::printMetadata() {
    char text[MAX_LOG_MSG_SIZE];
    int len;
    
    /* write the meta data header */
    snprintf(text, MAX_LOG_MSG_SIZE, "<metadata>\n");
    len = strlen(text);
    fwrite(text, len, sizeof (char), _logFile);

    /* write the msg counts */
    for (int lvl = 0; lvl <= LOG_DEBUG; lvl++) {
        snprintf(text, MAX_LOG_MSG_SIZE, "<sev%dcnt>%s</sev%dcnt>\n", lvl, SEV_LABELS[lvl], lvl);
        len = strlen(text);
        fwrite(text, len, sizeof (char), _logFile);
    }

    snprintf(text, MAX_LOG_MSG_SIZE, "</metadata>\n");
    len = strlen(text);
    fwrite(text, len, sizeof (char), _logFile);

    /* write the meta data */
    snprintf(text, MAX_LOG_MSG_SIZE, "<metadata>\n");
    len = strlen(text);
    fwrite(text, len, sizeof (char), _logFile);

    /* write the msg counts */
    for (int lvl = 0; lvl <= LOG_DEBUG; lvl++) {
        snprintf(text, MAX_LOG_MSG_SIZE, "<sev%dcnt>%d</sev%dcnt>\n", lvl, _errCnt[lvl], lvl);
        len = strlen(text);
        fwrite(text, len, sizeof (char), _logFile);
    }

    snprintf(text, MAX_LOG_MSG_SIZE, "</metadata>\n");
    len = strlen(text);
    fwrite(text, len, sizeof (char), _logFile);
    
    snprintf(text, MAX_LOG_MSG_SIZE, "<filename>%s</filename>\n", _dateLogFileName);
    len = strlen(text);
    fwrite(text, len, sizeof (char), _logFile);

    char *msg = (char*) "</xmlfile>";
    len = strlen(msg);
    fwrite(msg, len, sizeof (char), _logFile);
    fflush(_logFile);
}

void Clogger::printHeader() {
    char text[MAX_LOG_MSG_SIZE];
    int len;

    snprintf(text, MAX_LOG_MSG_SIZE, "<msg millis=\"0000000000000000\">\n");
    len = strlen(text);
    fwrite(text, len, sizeof (char), _logFile);

    /* write the headers */
    snprintf(text, MAX_LOG_MSG_SIZE, "<date>Date</date>\n");
    len = strlen(text);
    fwrite(text, len, sizeof (char), _logFile);
 
    snprintf(text, MAX_LOG_MSG_SIZE, "<time>Time</time>\n");
    len = strlen(text);
    fwrite(text, len, sizeof (char), _logFile);
 
    snprintf(text, MAX_LOG_MSG_SIZE, "<severity>Severity</severity>\n");
    len = strlen(text);
    fwrite(text, len, sizeof (char), _logFile);
 
    snprintf(text, MAX_LOG_MSG_SIZE, "<ID>ID</ID>\n");
    len = strlen(text);
    fwrite(text, len, sizeof (char), _logFile);
 
    snprintf(text, MAX_LOG_MSG_SIZE, "<source>Source</source>\n");
    len = strlen(text);
    fwrite(text, len, sizeof (char), _logFile);
 
    snprintf(text, MAX_LOG_MSG_SIZE, "<lineNo>LineNo</lineNo>\n");
    len = strlen(text);
    fwrite(text, len, sizeof (char), _logFile);
 
    snprintf(text, MAX_LOG_MSG_SIZE, "<text>Text</text>\n");
    len = strlen(text);
    fwrite(text, len, sizeof (char), _logFile);
    
    snprintf(text, MAX_LOG_MSG_SIZE, "</msg>\n");
    len = strlen(text);
    fwrite(text, len, sizeof (char), _logFile);
    
    fflush(_logFile);
    fgetpos(_logFile, &_pos);
    _lastPos = &_pos;
}

// logMsg.cpp - code to log debug messages

#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <time.h>

#include "logMsg.h"

// log file
static FILE * logFile = NULL;
static const char * LOG_MSG_BASENAME = "/var/log/alarmLog";

// ring buffer for queuing log messages
static const int LOG_RING_SIZE = 128;     // max number of messages to queue
static const int LOG_MSG_LEN = 256;       // max log message size
static char ringBuf[LOG_RING_SIZE][LOG_MSG_LEN];
static int ringIdx = 0;

static bool logAll = false;  // immediately log all messages to log file

bool openLogFile(void);
void initRingBuf(void);

bool initLogMsg(void)
{
    initRingBuf();
    return openLogFile();
}

void finiLogMsg(void)
{
    if (logFile != NULL)
    {
        fclose(logFile);
        logFile = NULL;
    }
}

void logEverything(bool state)
{
    logAll = state;  // set boolean to decide if everything should be logged immediately
}

/// openLogFile. Returns: pointer to opened file
bool openLogFile(void)
{
    // keep the last 3 alarm logs
    char name1[128], name2[128];

    sprintf(name1, "%s.3", LOG_MSG_BASENAME);
    if (access(name1, F_OK) == 0)  // log.3 exists, remove it
    {
        remove(name1);
    }
    sprintf(name2, "%s.2", LOG_MSG_BASENAME);
    if (access(name2, F_OK) == 0)  // log.2 exists, rename it to log.3
    {
        if (rename(name2, name1) != 0)
        {
            fprintf(stderr, "Failed to rename %s to %s\n", name2, name1);
        }
    }
    sprintf(name1, "%s.1", LOG_MSG_BASENAME);
    if (access(name1, F_OK) == 0)  // log.1 exists, rename it to log.2
    {
        if (rename(name1, name2) != 0)
        {
            fprintf(stderr, "Failed to rename %s to %s\n", name1, name2);
        }
    }
    if (access(LOG_MSG_BASENAME, F_OK) == 0)  // log exists, rename it to log.1
    {
        if (rename(LOG_MSG_BASENAME, name1) != 0)
        {
            fprintf(stderr, "Failed to rename %s to %s\n", LOG_MSG_BASENAME, name1);
        }
    }

    logFile = fopen(LOG_MSG_BASENAME, "w");
    if (logFile == NULL)
    {
        fprintf(stderr, "Failed to open logMsg file\n");
        return false;
    }
    return true;
}

// log messages to ring buffer and possibly file
void logMsg(const char *fmt, ...) // vararg format
{
    if (fmt == NULL)
    {
        return;
    }

    static char msg[LOG_MSG_LEN];
    msg[0] = '\0';

    time_t currTime = time(NULL);
    struct tm * pTime = localtime(&currTime);

    int i = sprintf(msg, "%02d:%02d:%02d ", pTime->tm_hour, pTime->tm_min, pTime->tm_sec);

    va_list pArg;
    va_start(pArg, fmt);

    int size = i + vsnprintf(msg+i, LOG_MSG_LEN-i-1, fmt, pArg);
    if (size > LOG_MSG_LEN-1)
    {
        size = LOG_MSG_LEN-1;
    }
    va_end(pArg);

    if (size <= 0)
    {
        return;  // vsnprintf failed, there is no output
    }
    msg[size] = '\0';  // make sure string is terminated properly

    if (logAll)
    {
        if (logFile != NULL)
        {
            fprintf(logFile, "%s", msg);
            fflush(logFile);
        }
    }
    else // otherwise, add msg to ring buffer
    {
        strcpy(ringBuf[ringIdx], msg);
        ringIdx = (ringIdx + 1) % LOG_RING_SIZE;
    }
}

void initRingBuf(void)
{
    ringIdx = 0;
    for (int i=0; i < LOG_RING_SIZE; i++)
    {
        memset(ringBuf[i], 0, LOG_MSG_LEN);
    }
}

// flush messages in ring buffer to log file
void flushMsgRing(void)
{
    if (logFile == NULL)
    {
        fprintf(stderr, "flushMsgRing called with NULL logFile\n");
        return;
    }

    int count = 0;

    // oldest message in ring is at ringIdx, so start there
    for (int i=0; i < LOG_RING_SIZE; i++)
    {
        if (ringBuf[ringIdx] != '\0')
        {
            fprintf(logFile, "%s", ringBuf[ringIdx]);  // print each msg in ring
            memset(ringBuf[ringIdx], 0, LOG_MSG_LEN);  // clear ring buf
            count++;
        }
        ringIdx = (ringIdx + 1) % LOG_RING_SIZE;
    }
    fprintf(stderr, "Sent %d messages to alarmLog\n", count);
    fflush(logFile);
}


// Serial.h - class to manage Serial functions

#pragma once

#include "stdafx.h"
#include "AlarmManager.h"

// values returned by parseMsg
enum {
    SERIAL_CMD_NONE = 0,
    SERIAL_CMD_KEYS,
    SERIAL_CMD_VOLTS,
    SERIAL_CMD_ERROR
};

class Serial
{
public:
    bool init(const char * serialPort, const int baudRate);
    void fini(void);

    uint8_t parseMsg(const char * cmd, const int len);
    void    sendF7msg(AlarmManager * pAlarmManager, int hour, int min);
    int     readBytes(char * buf, int buflen, bool * newLine);

private:
    int fd;

    int baudLookup(int baud);

};

// end of Serial.h

// The file Serial.h is part of RPIalarm.
// Copyright (C) 2018  Thomas Vickers
//
// RPIalarm is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// RPIalarm is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with RPIalarm.  If not, see <http://www.gnu.org/licenses/>.

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

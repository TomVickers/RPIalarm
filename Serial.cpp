// The file Serial.cpp is part of RPIalarm.
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

#include "stdafx.h"
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>

#include "AlarmManager.h"
#include "Serial.h"
#include "logMsg.h"

// open serial port for read/write, raw, non-blocking
bool Serial::init(const char * serialPort, const int baudRate)
{
    int baud = baudLookup(baudRate);
    if (baud <= 0)
    {
        fprintf(stderr, "Improper baud rate specified for serial port: %d\n", baudRate);
        return false;
    }

    fd = open(serialPort, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd < 0)
    {
        fprintf(stderr, "Error opening serial port %s\n", serialPort);
        return false;
    }

    struct termios tty, oldtty;
    memset(&tty, 0, sizeof(tty));
    if (tcgetattr(fd, &oldtty) != 0)
    {
        fprintf(stderr, "Error calling tcgetattr\n");
        close(fd);
        fd = -1;
        return false;
    }

    tty.c_cflag = baud | CRTSCTS | CS8 | CLOCAL | CREAD;
    tty.c_iflag = IGNPAR | ICRNL;   // raw input
    tty.c_oflag = 0;                // raw output
    tty.c_lflag = 0;                // non-canonical, raw mode

    tty.c_cc[VTIME] = 0;     // don't block
    tty.c_cc[VMIN] = 0;      // no min char read

    tcflush(fd, TCIFLUSH);  // flush the serial line

    if (tcsetattr(fd, TCSANOW, &tty) != 0)
    {
        fprintf(stderr, "Error calling tcsetattr\n");
        close(fd);
        fd = -1;
        return false;
    }
    return true;
}

void Serial::fini(void)
{
    if (fd > 0)
    {
        close(fd);
        fd = -1;
    }
}

// parse serial message from Arduino
uint8_t Serial::parseMsg(const char * cmd, const int len)
{
    if (strncmp(cmd, "KEYS_", 5) == 0)  // recv cmd is a KEYS msg
    {
        return SERIAL_CMD_KEYS;
    }
    else if (strncmp(cmd, "ERR_", 4) == 0)  // recv error msg, probably buf overflow
    {
        logMsg("Err mesg: '%s'\n", cmd);
        return SERIAL_CMD_ERROR;
    }
// FIXME - need VOLTS message here
    else
    {
        logMsg("Unexpected mesg: '%s'\n", cmd);
    }
    return SERIAL_CMD_NONE;
}

// send an F7 mesg via the serial port
void Serial::sendF7msg(AlarmManager * pAlarmManager, int hour, int min)
{
    if (fd > 0)
    {
        char buf[128];
        pAlarmManager->makeF7msg(buf, hour, min, false);
        int writeBytes = write(fd, (void *)buf, strlen(buf));
    
        if (writeBytes == (int)strlen(buf))
        {
            logMsg("Sent F7 msg to keypad: %s", buf);
        }
        else
        {
            logMsg("ERR: Short write of F7 msg to keypad: %s", buf);
        }

        if (pAlarmManager->getAltTextActive())
        {
            pAlarmManager->makeF7msg(buf, hour, min, true);
            writeBytes = write(fd, (void *)buf, strlen(buf));
    
            if (writeBytes == (int)strlen(buf))
            {
                logMsg("Sent F7A msg to keypad: %s", buf);
            }
            else
            {
                logMsg("ERR: Short write of F7A msg to keypad: %s", buf);
            }
        }
        pAlarmManager->setLastMsgTime();
    }
}

// read bytes from serial.  Return bytes read
int Serial::readBytes(char * buf, int buflen, bool * newLine)
{
    if (fd < 0)
        return 0;

    int readStat = read(fd, (void *)buf, buflen);

    if (readStat > 0)
    {
        for (int i=0; i < readStat; i++)
        {
            if (*(buf + i) == '\n')
            {
                *(buf + i) = '\0';
                *newLine = true;
            }
        }
        *(buf + readStat) = '\0'; // null terminate what has been read
        return readStat;
    }
    else if (readStat < 0 && errno != EAGAIN)  // read returned an error other than "no data"
    {
        perror("read ");
    }
    return 0;
}

// converts integer baud to termios define
int Serial::baudLookup(int baud)
{
    switch (baud)
    {
    case 9600:
        return B9600;
    case 19200:
        return B19200;
    case 38400:
        return B38400;
    case 57600:
        return B57600;
    case 115200:
        return B115200;
    case 230400:
        return B230400;
    case 460800:
        return B460800;
    case 500000:
        return B500000;
    case 576000:
        return B576000;
    case 921600:
        return B921600;
    case 1000000:
        return B1000000;
    case 1152000:
        return B1152000;
#if 0
    case 1500000:
        return B1500000;
    case 2000000:
        return B2000000;
    case 2500000:
        return B2500000;
    case 3000000:
        return B3000000;
    case 3500000:
        return B3500000;
    case 4000000:
        return B4000000;
#endif
    default:
        return -1;
    }
}

// end of Serial.cpp

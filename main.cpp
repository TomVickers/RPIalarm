// The file main.cpp is part of RPIalarm.
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
#include <string.h>
#include <unistd.h>
#include <signal.h>

#ifdef _HAVE_WIRING_PI
#include <wiringPi.h>
#endif

#include "Config.h"        // user/zone structs & defines
#include "AlarmManager.h"  // alarm manager class definition
#include "Serial.h"        // serial manager class definition
#include "SockClient.h"    // socket manager class definition
#include "logMsg.h"

static volatile uint8_t done = 0;  // flag used to shut down main while loop

#define MIL_TO_12HR(x)  ((x) % 12 == 0 ? 12 : (x) % 12)

void handleVoltMsg(AlarmManager * pAlarmManager, const char * msg, int msgLen);

// interrupt handler to capture kill/interrupt signal
void intHandler(int notUsed)
{
    done = 1;  // shut down main loop
}

// main 
int main(int argc, char *argv[])
{
    Config config;             // reads pin/loop definitions from config file
    AlarmManager alarmManager; // handles alarm functions
    Serial serial;             // handles serial communication
    SockClient sock;           // handles socket communication with web app
    initLogMsg();

    if (!config.readFile(ALARM_CONFIG_FILE))
    {
        fprintf(stderr, "Failed to read config file\n");
        return -1;
    }

    if (!serial.init(config.getSerial(), config.getBaudRate()))
    {
        fprintf(stderr, "Failed to open serial port\n");
        return -1;
    }
    else  // suceeded in opening the serial port
    {
        // My Raspberry PI 3 serial port is squirrely on the first open after boot.
        // Since I run this app as a service at boot time, this open-close-reopen 
        // sequence fixes the issue.  A simply delay won't fix it.
        
        sleep(1);       // appears to be needed on my setup
        serial.fini();  // close the serial port
        sleep(1);       // appears to be needed on my setup

        // then re-open
        if (!serial.init(config.getSerial(), config.getBaudRate()))
        {
            fprintf(stderr, "Failed to open serial port\n");
            return -1;
        }
    }

    if (!sock.init())
    {
        logMsg("Failed to open socket\n");
    }

    Loop * pLoop = config.getLoop();   // get pointer to array of sense loops

#ifdef _HAVE_WIRING_PI
    wiringPiSetupGpio(); // Initialize wiringPi - Broadcom pin numbering (MUST BE RUN AS ROOT)

    for (int i=0; i < config.getLoopCount(); i++)  // configure pin io for sense loops
    {
        pinMode((pLoop+i)->gpio, INPUT);           // enable as input
        pullUpDnControl((pLoop+i)->gpio, PUD_UP);  // enable pull-up resistor on input
    }
    for (int i=0; i < config.getOutputCount(); i++) // config pin out for outputs
    {
        int pin = (config.getOutputs()+i)->gpio;
        pinMode(pin, OUTPUT);  // set as output
    }
#endif // _HAVE_WIRING_PI

    alarmManager.init(&config); // init AlarmManager class (pass in pointer to config class)

    // check for data from serial port each time around.  Build up messages
    //   in buffer until a complete message is received, then process mesg

    uint32_t prevLoopMask = 0xFF;   // force an update on first pass
    char     readBuf[READ_BUF_SIZE];
    int      readBufIdx = 0;
    uint8_t  sendF7msgNow = true;   // send F7 mesg at the next available time slot
    char     sockBuf[SOCK_BUF_SIZE];
    int      sockRecvBytes = 0;

    signal(SIGINT,  intHandler);  // catch and handle interrupt signals
    signal(SIGTERM, intHandler);
    signal(SIGKILL, intHandler);
    signal(SIGQUIT, intHandler);
    signal(SIGHUP,  intHandler);

    fprintf(stdout, "alarm app started\n");
    fflush(stdout);
    alarmManager.sendAlertMsg("alarm app started");

    while (!done)  // main alarm loop
    {
        if (READ_BUF_SIZE-readBufIdx-1 < 1)
        {
            logMsg("read buffer overflow, discarding read buffer\n");
            readBufIdx = 0;
        }
        bool newLine = false;
        readBufIdx += serial.readBytes(readBuf+readBufIdx, READ_BUF_SIZE-readBufIdx-1, &newLine);

        if (newLine && strlen(readBuf) > 0)  // complete serial command in readBuf
        {
            switch (serial.parseMsg(readBuf, readBufIdx)) // what type of message was received?
            {
                case SERIAL_CMD_KEYS:
                    alarmManager.processKeyMsg(readBuf, readBufIdx);
                    sendF7msgNow = true;
                    break;

                case SERIAL_CMD_VOLTS:
                    handleVoltMsg(&alarmManager, readBuf, readBufIdx);
                    break;

                case SERIAL_CMD_ERROR:
                default:
                    logMsg("parseMsg error, resend F7 msg\n");
                    sendF7msgNow = true;  // resend msg that failed
                    break;
            }
            readBufIdx = 0;  // reset buffer index for new command
        }

        memset(sockBuf, 0, SOCK_BUF_SIZE);
        if ((sockRecvBytes = sock.recvMsg(sockBuf, SOCK_BUF_SIZE)) > 0)  // check socket from web app
        {
            if (strncmp(sockBuf, "KEYS_", 5) == 0)
            {
                alarmManager.processKeyMsg(sockBuf, sockRecvBytes);
                sendF7msgNow = true;
            }
            else
            {
                logMsg("Recv unexpected msg on socket '%s'\n", sockBuf);
            }
        }
        
        if (alarmManager.checkLoops(&prevLoopMask))  // check sense loops for change
        {
            sendF7msgNow = true;
        }

        if (alarmManager.checkTimeouts())  // check timeout conditions
        {
            sendF7msgNow = true;
        }

        // check to see if it is time to send a new F7 msg (don't send faster than every 200ms)
        if (sendF7msgNow && AlarmManager::getTimestamp() - alarmManager.getLastMsgTime() > 200)
        {
            time_t currTime = time(NULL);
            struct tm * pTime = localtime(&currTime);
            serial.sendF7msg(&alarmManager, MIL_TO_12HR(pTime->tm_hour), pTime->tm_min);
            sock.sendMsg(alarmManager.getSockMsg(MIL_TO_12HR(pTime->tm_hour), pTime->tm_min));
            sendF7msgNow = false;
        }
        usleep(100 * 1000);  // sleep 100ms
    }
    sock.fini();
    serial.fini();
    alarmManager.sendAlertMsg("alarm app shutdown");
    fprintf(stdout, "alarm app shutdown\n");
    fflush(stdout);
    finiLogMsg();
    return 0;
}

// --------------------------------------------- support funcs ------------------------------------------

void handleVoltMsg(AlarmManager * pAlarmManager, const char * msg, int msgLen)
{
    // FIXME - Handle voltage message from arduino
    // If no AC power, look at what should be done differently to reduce power consumption
    // If batt is getting low, shut down raspberry pi
    //   on shutdown, gpio output keeping power connected to pi will be lost, turning off input power
    //   when AC power resumes, input power to pi will be restored
}

// end of main.cpp

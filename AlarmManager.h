// The file AlarmManager.h is part of RPIalarm.
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
#include "Config.h"
#include <time.h>

static const int ALERT_MSG_SIZE = 256;   // max size of alert text sent as email/sms

// pin functions returned by validatePin
enum {
    PIN_FUNC_NONE    = 0,
    PIN_FUNC_DISARM  = 1,
    PIN_FUNC_AWAY    = 2,
    PIN_FUNC_STAY    = 3,
    PIN_FUNC_MAX     = 4, // ?
    PIN_FUNC_TEST    = 5,
    PIN_FUNC_BYPASS  = 6,
    PIN_FUNC_INSTANT = 7,
    PIN_FUNC_CODE    = 8,
    PIN_FUNC_CHIME   = 9
};

// modes for armed 
enum {
    DISARMED = 0,
    ARMED_STAY,
    ARMED_AWAY
};

class AlarmManager
{
public:
    AlarmManager(void) {};   // constructor
    ~AlarmManager(void) {};  // destructor

    void init(Config * pConfig);

    bool checkLoops(uint32_t * prevLoopMask);
    bool checkTimeouts(void);

    void processKeyMsg(const char * buf, int bufLen);
    void makeF7msg(char * buf, int hour, int min, bool altText);
    void sendAlertMsg(const char * msg);

    const char * getSockMsg(int hour, int min);

    void     setLastMsgTime(void);
    uint32_t getLastMsgTime(void)
    {
        return lastMsgTime;
    }

    bool getAltTextActive(void)
    {
        return altTextActive;
    }

    static uint32_t getTimestamp(void);

private:
    uint8_t getPinDigits(int kp);
    uint8_t getPinDigits(void);

    bool checkPinTimeout(uint32_t ms);
    bool processTimeouts(uint32_t ms);
    bool setPinDigit(int keypad, uint8_t digit, uint32_t ms);

    void clearPin(void);
    bool pinPrompt(void);
    void setAlarm(void);
    void setDisarm(void);
    void setTone(uint8_t val);
    void setTempMsg(const char * s1, const char * s2);
    void updateState(void);
    void turnOnBacklight(void);
    void displayOpenLoops(void);

    int  validatePin(uint8_t * func);
    int  loopIndex(uint32_t mask);


    uint8_t  zone;                                  // byte 0-FF
    uint8_t  tone;                                  // (nibble) 0-7 valid
    bool     chime;                                 // true - chime active
    bool     ready;                                 // true - ready LED lit
    uint8_t  armed;                                 // false - disarmed, othewise armed
    bool     power;                                 // true - AC power present
    bool     backlight;                             // true - LCD backlight lit
    bool     alarm;                                 // if true, alarm is sounding
    bool     armTimeoutActive;                      // arm delay while leaving active
    bool     disarmTimeoutActive;                   // disarm delay while entering active
    bool     altTextActive;                         // add alt F7 message
    bool     tmpTextActive;                         // momentary message is active
    uint8_t  pinDigits[MAX_KEYPADS];                // number of pin digits entered
    uint8_t  timeoutRemain;                         // number of seconds that remain in timeout
    uint32_t timeoutStart;                          // ms timestamp of timeout start
    uint32_t lastMsgTime;                           // ms timestamp of last F7 message sent
    uint32_t backlightOnTime;                       // ms timestamp of turning on backlight
    uint32_t lastPinRecv;                           // ms timestamp of last keypad key recv
    uint32_t chimeMsgTime;                          // ms timestamp of msg with chime tone set
    uint32_t tmpMsgTime;                            // ms timestamp when temp msg set
    uint32_t loopMask;                              // sensor loop mask
    uint8_t  pinCode[MAX_KEYPADS][MAX_PIN_DIGITS];  // current pin code entered
    char     line1[17];                             // line1 text (16 chars + NULL)
    char     line2[17];                             // line2 text (16 chars + NULL)
    char     altLine1[17];                          // alt line1 text (16 chars + NULL)
    char     altLine2[17];                          // alt line2 text (16 chars + NULL)
    char     tempLine1[17];                         // momentary message line1
    char     tempLine2[17];                         // momentary message line2

    Config * pConfig;                               // pointer to config class

    Loop   * pLoop;                                 // pointer to sense loop class array
    int      loopCount;                             // number of sense loops
    Pin    * pPin;                                  // pointer to array of pin number classes
    int      userCount;                             // number of user pin numbers

    int      sirenGpioPin;                          // gpio pin for siren output

    time_t   startTime;                             // alarm init time

    char     lastAlertMsg[ALERT_MSG_SIZE];          // last alert msg sent
    uint32_t lastAlertTime;
};

// file AlarmManager.cpp - manage alarm functions

#include "AlarmManager.h"
#include <string.h>
#include "sendEmail.h"
#include "logMsg.h"

#ifdef _HAVE_WIRING_PI
#include <wiringPi.h>
#endif

// returns a timestamp (number of milliseconds since power-on)
uint32_t AlarmManager::getTimestamp(void)
{
    struct timespec spec;
    clock_gettime(CLOCK_MONOTONIC, &spec);
    return spec.tv_sec * 1000 + spec.tv_nsec/1000000;
}

// initialize the alarm manager class.  
void AlarmManager::init(
    Config * pConfig)    // pointer to config class
{
    zone = 0;
    ready = false;
    power = false;
    timeoutRemain = 0;
    timeoutStart = 0;
    lastMsgTime = getTimestamp();
    lastPinRecv = 0;
    chimeMsgTime = 0;
    tmpMsgTime = 0;
    loopMask = 0;

    setDisarm();

    sprintf(line1, "Power on        ");
    sprintf(line2, "Alarm init done ");

    altLine1[0] = '\0';
    altLine2[0] = '\0';
    altTextActive = false;   // enables F7A msg when true

    tempLine1[0] = '\0';
    tempLine2[0] = '\0';
    tmpTextActive = false;

    for (int i=0; i < MAX_KEYPADS; i++)
    {
        pinDigits[i] = 0;
        for (int j=0; j < MAX_PIN_DIGITS; j++)
        {
            pinCode[i][j] = 0;
        }
    }

    this->pConfig = pConfig;

    chime     = pConfig->getChimeDefault(); // initial state of chime mode
    pLoop     = pConfig->getLoop();         // pointer to array of sense loops
    loopCount = pConfig->getLoopCount();    // number of sense loops in array
    pPin      = pConfig->getPin();          // pointer to array of user pin entries
    userCount = pConfig->getPinCount();     // number of user pin entries

    sirenGpioPin = pConfig->getOutput("SIREN");   // gpio pin for siren output

    time(&startTime);

    lastAlertMsg[0] = '\0';
    lastAlertTime = 0;

    turnOnBacklight();
}

void AlarmManager::turnOnBacklight(void)
{
    backlight = true;
    backlightOnTime = getTimestamp();
}

void AlarmManager::setLastMsgTime(void)
{
    lastMsgTime = getTimestamp();
}

// add a digit to the received pin code from keypad. Return: true if pin digit received
bool AlarmManager::setPinDigit(int keypad, uint8_t digit, uint32_t ms)
{
    lastPinRecv = ms;   // set timestamp of last pin received

    if (digit == 0x0b)  // '#' key hit, force timeout on pin code
    {
        clearPin();
    }
    else if (keypad < MAX_KEYPADS)
    {
        pinCode[keypad][pinDigits[keypad]] = digit;
        pinDigits[keypad] += 1;
        return true;
    }
    return false;
}

// check to see if it is time to timeout pin code.  Return: true if pin is timed out
bool AlarmManager::checkPinTimeout(uint32_t ms)
{
    if (getPinDigits() > 0 && ms - lastPinRecv > PIN_TIMEOUT)  // if no pin entered for N secs, timeout pin
    {
        clearPin();
        return true;
    }
    return false;
}

// return the number of pin digits received from keypad kp
uint8_t AlarmManager::getPinDigits(int kp)
{
    return pinDigits[kp];
}

// return the number of pin digits from first keypad that has pin digits (overloaded func)
//   note - this won't work if multiple keypads are receiving key presses at the same time
uint8_t AlarmManager::getPinDigits(void)
{
    for (int kp=0; kp < MAX_KEYPADS; kp++)
    {
        if (getPinDigits(kp))
        {
            return getPinDigits(kp);
        }
    }
    return 0;
}

// display pin prompt (as stars) on line2 of alarm panel LCD. Returns true if pin stars displayed
bool AlarmManager::pinPrompt(void)
{
    int dig = min(getPinDigits(), 9);

    if (dig == 0)
    {
        if (armTimeoutActive)
            sprintf(line2, "Leave now %02d sec", timeoutRemain);
        else if (disarmTimeoutActive)
            sprintf(line2, "Enter pin %02d sec", timeoutRemain);
        else
            sprintf(line2, "Enter pin       ");
    }

    if (dig > 0)
    {
        if (armTimeoutActive || disarmTimeoutActive)
            sprintf(line2, "          %02d sec", timeoutRemain);
        else
            sprintf(line2, "                ");

        for (int i=0; i < dig; i++)
        {
            line2[i] = '*';  // character to show number of pin digits entered
        }
        return true;
    }
    return false;
}

// set alarm condition
void AlarmManager::setAlarm(void)
{
    alarm = true;
    ready = false;
    setTone(TONE_ALARM);
    sprintf(line1, "Alarm!          ");
    if (!pinPrompt())
        sprintf(line2, "Enter pin       ");
}

// set disarmed condition
void AlarmManager::setDisarm(void)
{
    alarm = false;
    armed = DISARMED;
    armTimeoutActive = false;
    disarmTimeoutActive = false;
    setTone(TONE_NONE);
}

// convert loop bitmask value into integer loop index
int AlarmManager::loopIndex(uint32_t mask)
{
    for (int i=0; i < loopCount; i++)
    {
        if ((mask >> i) & 0x1)
        {
            return i;
        }
    }
    return 0;
}

// send alert message to email/sms address about alarm state
void AlarmManager::sendAlertMsg(const char * msg)
{
    uint32_t ms = getTimestamp();

    // only send alert mesg if it is unique or timeout has passed since last repeat of message
    if (strcmp(lastAlertMsg, msg) != 0 || ms - lastAlertTime > ALERT_MSG_REPEAT_TIMEOUT)
    {
        sendEmail(pConfig->getSendEmailAccnt(), pConfig->getAlertEmail(), pConfig->getSendEmailPasswd(),
            "Alarm update", msg);
        strcpy(lastAlertMsg, msg);
        lastAlertTime = ms;
    }
}

// update keypad display with message about open loops
void AlarmManager::displayOpenLoops(void)
{
    int firstLoop  = -1;
    int secondLoop = -1;

    for (int i=0; i < loopCount; i++)
    {
        if (loopMask & (1 << i))
        {
            if (firstLoop == -1)   // first loop not set, set it
            {
                firstLoop = i;
            }
            else  // first loop set, set second loop and break
            {
                secondLoop = i;
                break;
            }
        }
    }
    if (firstLoop > -1)  // first open loop set
    {
        sprintf(line2, "x %-14s", (pLoop+firstLoop)->name);
    }
    if (secondLoop > -1) // two or more open loops
    {
        strcpy(altLine1, line1);
        sprintf(altLine2, "x %-14s", (pLoop+secondLoop)->name);
        altTextActive = true;
    }
}

// update the alarm state
void AlarmManager::updateState(void)
{
    char msg[ALERT_MSG_SIZE];
    altTextActive = false;     // default alternate F7 msg to off

    ready = (loopMask == 0); // all loops are closed, ready for arming

    if (alarm)                                 // alarm is going off!
    {
        setAlarm();  // set alarm state 
    }
    else if (armed == ARMED_STAY && loopMask)  // alarm should be going off, loop opened while in ARMED_STAY
    {
        int openLoop = loopIndex(loopMask);
        sprintf(msg, "Alarm! %s opened while armed-stay set", (pLoop+openLoop)->name);
        sendAlertMsg(msg);
        setAlarm();  // set alarm state 
    }
    else if (armed != DISARMED) // system is armed (or arm delay)
    {
        if (armTimeoutActive)   // system is in arm delay mode
        {
            sprintf(line1, "Arming          ");
            if (!pinPrompt())
                sprintf(line2, "Leave now %02d sec", timeoutRemain);
        }
        else // armed, not in leave delay 
        {
            sprintf(line1, "Armed %s      ", armed == ARMED_STAY ? "Stay" : "Away");

            if (loopMask && armed == ARMED_AWAY && !disarmTimeoutActive)
            {
                // a loop opened while in armedAway mode and disarmTimeout not yet active
                int openLoop = loopIndex(loopMask);

                if ((pLoop+openLoop)->entryAllowed)  // this zone permitted for delayed alarm entry
                {
                    disarmTimeoutActive = true;
                    timeoutRemain = DEFAULT_TIMEOUT_MS / 1000;
                    timeoutStart = getTimestamp();
                }
                else  // a zone unauthorized for entry opened, immediate alarm
                {
                    sprintf(msg, "Alarm! %s opened while armed-away set", (pLoop+openLoop)->name);
                    sendAlertMsg(msg);
                    setAlarm();
                }
            }
            if (!pinPrompt())
                sprintf(line2, "Enter pin       ");
        }
    }
    else  // not armed
    {
        sprintf(line1, "%-16s", "Disarmed        ");

        if (!pinPrompt())
        {
            if (tmpTextActive)  // a temp message is active, override normal disarmed display
            {
                strcpy(line1, tempLine1);
                strcpy(line2, tempLine2);
            }
            else 
            {
                if (ready)
                {
                    sprintf(line2, "Ready to arm    ");
                }
                else // one or more loops are open
                {
                    displayOpenLoops();
                }
            }
        }
    }
    turnOnBacklight();   // updating F7 message, turn on LCD backlight
}

// check the sense loops and update alarm state if needed.  Returns: true if message should be pushed to keypad
bool AlarmManager::checkLoops(uint32_t * prevLoopMask)
{
    loopMask = 0;  // init all closed (safe)

#ifdef _HAVE_WIRING_PI
    // read the sense loop state by doing a digital read of associated GPIO pin (high => open loop)
    for (int i=0; i < loopCount; i++)
    {
        if (digitalRead((pLoop+i)->gpio))
        {
            loopMask |= (1 << i);
        }
    }
#endif

    uint32_t diffMask = loopMask ^ *prevLoopMask;  // use exclusive or to determine which bits have changed state

    if (diffMask != 0)  // at least one of the loops changed state
    {
        if (tone == TONE_NONE) // no alarm tone currently sounding (don't override timeout or alarm tones)
        {
            int idx = loopIndex(diffMask);
            if ((loopMask >> idx) & 0x1)             // loop that changed was opened
            {
                if (chime)                           // chime mode is enabled
                {
                    setTone((pLoop+idx)->chimeTone); // set chime for opened loop
                    chimeMsgTime = getTimestamp();   // store time of setting chime
                }
            }
        }
        updateState(); // update alarm state
        *prevLoopMask = loopMask;
        return true;   // send update to keypad
    }
    return false;
}

void AlarmManager::setTone(uint8_t toneVal)
{
    tone = toneVal;

#ifdef _HAVE_WIRING_PI
    if (sirenGpioPin >= 0)
    {
        digitalWrite(sirenGpioPin, tone == TONE_ALARM ? 1 : 0);
    }
#endif
}

// check all the timeout conditions.  Returns: true if message should be pushed to keypad
bool AlarmManager::checkTimeouts(void)
{
    uint32_t ms = getTimestamp();
    bool updateKeypad = false;

    if (armTimeoutActive || disarmTimeoutActive) // arm or disarm timeout is active
    {
        if (processTimeouts(ms))
        {
            updateState();
            updateKeypad = true;
        }
    }
    else if (backlight && ms - backlightOnTime > BACKLIGHT_ON_TIME)  // time to turn off backlight
    {
        //logMsg("backlight turned off: curr ms %u, backlightOnTime %u, diff %u\n", ms, 
        //    backlightOnTime, ms - backlightOnTime);
        backlight = false;
        updateKeypad = true;
    }
    else if (ms - lastMsgTime > 60 * 1000)  // it has been more than 1 minute since the last F7 mesg
    {
        updateKeypad = true;
    }
    else if ((tone == TONE_CHIME_1 || tone == TONE_CHIME_2 || tone == TONE_CHIME_3) && 
        ms - chimeMsgTime > CHIME_TIMEOUT)
    {
        setTone(TONE_NONE); // after we set a chime tone, clear it so the keypad does not keep chiming
        updateKeypad = true;
    }
    else if (tmpTextActive && ms - tmpMsgTime > TEMP_MSG_TIMEOUT)  // time to time-out a displayed temp message
    {
        tmpTextActive = false;
        updateState(); // update alarm state
        updateKeypad = true;
    }

    if (checkPinTimeout(ms)) // check if it is time to time-out partial pin code
    {
        //printf("pincode timed out\n");
        updateState(); // update F7 msg
        updateKeypad = true;
    }
    return updateKeypad;
}

// process arm/disarm timeouts.  Returns: true if message should be pushed to keypad
bool AlarmManager::processTimeouts(const uint32_t ms)
{
    int tRemain;
    if (ms - timeoutStart >= DEFAULT_TIMEOUT_MS)  // timeout expired
    {
        tRemain = 0;
    }
    else
    {
        tRemain = (DEFAULT_TIMEOUT_MS - (ms - timeoutStart)) / 1000;  // secs that remain
    }

    if (tRemain != timeoutRemain)  // number of seconds remaining has changed
    {
        if (tRemain > CRITICAL_TIMEOUT_S)
        {
            setTone(TONE_SLOW_RATE);
        }
        else if (tRemain > 0)
        {
            setTone(TONE_FAST_RATE);
        }
        else // tRemain is zero
        {
            if (armTimeoutActive)
            {
                armTimeoutActive = false;
                if (loopMask != 0)  // a loop was open when the arm timeout expired, this is an alarm
                {
                    char msg[ALERT_MSG_SIZE];
                    int openLoop = loopIndex(loopMask);
                    sprintf(msg, "Alarm! %s open when arm timeout expired", (pLoop+openLoop)->name);
                    sendAlertMsg(msg);
                    alarm = true;  // disarm timeout expired without disarm
                }
                else
                {
                    setTone(TONE_NONE);
                }
            }
            else  // disarmTimeout
            {
                sendAlertMsg("Alarm! pin not entered before disarm timeout");
                disarmTimeoutActive = false;
                alarm = true;  // disarm timeout expired without disarm
            }
        }
        timeoutRemain = tRemain;
        return true;
    }
    return false;
}

// clear any previously received pin digits.  Returns: none
void AlarmManager::clearPin(void)
{
    for (int i=0; i < MAX_KEYPADS; i++)
    {
        pinDigits[i] = 0;
        for (int j=0; j < MAX_PIN_DIGITS; j++)
            pinCode[i][j] = 0;
    }
}

// set an temporary keypad LCD message that expires after TEMP_MSG_TIMEOUT ms
void AlarmManager::setTempMsg(const char * s1, const char * s2)
{
    tmpMsgTime = getTimestamp();

    if (s1 != NULL)
        sprintf(tempLine1, "%-16.16s", s1);
    else
        strcpy(tempLine1, line1);  // re-use existing line1

    if (s2 != NULL)
        sprintf(tempLine2, "%-16.16s", s2);
    else
        strcpy(tempLine2, line2);  // re-use existing line2

    tmpTextActive = true;
}

// attempt to validate a provided pin.  If pin verifies, provide index of validated user and desired function
int AlarmManager::validatePin(uint8_t * func)
{
    for (int kp=0; kp < MAX_KEYPADS; kp++)  // look at all keypads
    {
        // must receive MIN_PIN_DIGITS + FUNC key before we consider entered pin
        if (pinDigits[kp] > MIN_PIN_DIGITS)
        {
            for (int user=0; user < userCount; user++)
            {
                // check if length of pin (minus func key at end) matches
                if ((int)strlen((pPin+user)->pin) == pinDigits[kp]-1)
                {
                    bool match = true;  // compare digits up until a mismatch
                    for (int d=0; match && d < pinDigits[kp]-1; d++)
                    {
                        if (pinCode[kp][d] > 9 || pinCode[kp][d] != *((pPin+user)->pin + d) - '0')
                            match = false;  // a digit in the pin failed to match
                    }
                    if (match)  // all tested digits match
                    {
                        *func = pinCode[kp][pinDigits[kp]-1];  // function is final pin digit
                        if (*func > 9)                         // only accept func keys up to 9?
                            *func = 0;
                        clearPin();                            // pin matched, clear all pin digits
                        return user;
                    }
                }
            }
        }
    }
    return -1;  // pin does not validate
}

// process a keys message from keypad
void AlarmManager::processKeyMsg(const char * buf, int bufLen)
{
    uint32_t ms = getTimestamp();
    if (bufLen < 12)
    {
        logMsg("processKeyMsg received short command\n");
        return;
    }

    // grab the key data from the message string
    // format of msg is: KEYS_XX[N] key0 key1 ... keyN-1, where XX is keypad number, N is key count
    //   keys format are two digit hex values of the form 0x00

    int keypad = min(atoi(buf+5) - 16, MAX_KEYPADS-1);
    int keyCount = min(atoi(buf+8), MAX_PIN_DIGITS);
    int offset = 12;  // offset to first key value

    //logMsg("processing KEYS msg from keypad %d with %d keys\n", keypad, keyCount);

    for (int i=0; i < keyCount && offset < bufLen && *(buf+offset) == '0'; i++)  // parse pressed keys
    {
        if (!setPinDigit(keypad, strtol(buf+offset, NULL, 16), ms))
        {
            break;
        }
        offset += 5;  // step to next key
    }

    uint8_t func;
    int user = validatePin(&func);

    if (user >= 0)  // valid pin recv from user
    {
        char msg[ALERT_MSG_SIZE];
        if (func == PIN_FUNC_DISARM)
        {
            sprintf(msg, "Alarm disarmed by %s", (pPin+user)->name);
            sendAlertMsg(msg);
            setDisarm();
        }
        else if ((func == PIN_FUNC_AWAY || func == PIN_FUNC_STAY) && armed == DISARMED)  // not already armed
        {
            if (loopMask == 0)  // all loops are closed, ready for arming
            {
                if (func == PIN_FUNC_AWAY)
                {
                    sprintf(msg, "Alarm armed-away by %s", (pPin+user)->name);
                    armed = ARMED_AWAY;
                    armTimeoutActive = true;
                    timeoutStart = getTimestamp();
                    timeoutRemain = DEFAULT_TIMEOUT_MS / 1000;
                }
                else
                {
                    sprintf(msg, "Alarm armed-stay by %s", (pPin+user)->name);
                    armed = ARMED_STAY;
                }
                sendAlertMsg(msg);
            }
            else
            {
                logMsg("attempt to arm failed. Sense loop fault.  loopmask = 0x%08x\n", loopMask);
                //fprintf(stderr, "Can't arm, sense loop fault\n");
            }
        }
        else if (func == PIN_FUNC_CHIME)
        {
            chime = !chime;  // toggle chime mode
            setTempMsg(NULL, chime ? "Chime enabled" : "Chime disabled");
        }
        else if (func == PIN_FUNC_TEST)  // display alarm uptime
        {
            char msg[17];
            time_t now;
            time(&now);
            int diffSecs = (int)difftime(now, startTime);
            int mins = (diffSecs / 60) % 60;
            int hours = (diffSecs / (60*60)) % 24;
            int days = diffSecs / (24*60*60);
            if (days < 1)
                sprintf(msg, "Uptime  %02d:%02d:%02d", hours, mins, diffSecs % 60);
            else
                sprintf(msg, "Up %6dd %02d:%02d", days, hours, mins);
            setTempMsg(NULL, msg);
        }
        else if (func == PIN_FUNC_CODE)  // write recent logMsgs to log file
        {
            flushMsgRing();
            sprintf(msg, "Msgs sent to log");
            setTempMsg(NULL, msg);
        }
        else  // unsupported func NONE, MAX, BYPASS, INSTANT
        {
            setTempMsg(NULL, "Not supported");
        }
    }
    updateState();  // update alarm state based on received keypad message
}

// create an F7 message for the keypad (via Arduino) and store it in buf. Returns: none
void AlarmManager::makeF7msg(char * buf, int hour, int min, bool altText)
{
    if (altText)
    {
#if 0
        // send full parameter F7A msg
        sprintf(buf, "F7A z=%02x t=%d c=%c r=%c a=%c s=%c p=%c b=%c 1=%-11.11s%2d:%02d 2=%-16.16s\n",
            zone, tone, chime ? '1' : '0', ready ? '1' : '0', armed == ARMED_AWAY ? '1' : '0',
            armed == ARMED_STAY ? '1' : '0', power ? '1' : '0', backlight ? '1' : '0',
            altLine1, hour, tm_min, altLine2);
#else
        // send abbreviated F7A msg
        sprintf(buf, "F7A t=%d c=%c r=%c a=%c b=%c 1=%-11.11s%2d:%02d 2=%-16.16s\n",
            tone, chime ? '1' : '0', ready ? '1' : '0', armed == DISARMED ? '0' : '1',
            backlight ? '1' : '0', altLine1, hour, min, altLine2);
#endif
    }
    else
    {
#if 0
        // send full parameter F7 msg
        sprintf(buf, "F7 z=%02x t=%d c=%c r=%c a=%c s=%c p=%c b=%c 1=%-11.11s%2d:%02d 2=%-16.16s\n",
            zone, s->tone, 
            chime ? '1' : '0', ready ? '1' : '0', armed == ARMED_AWAY ? '1' : '0',
            armed == ARMED_STAY ? '1' : '0', power ? '1' : '0', backlight ? '1' : '0',
            line1, hour, min, line2);
#else
        // send abbreviated F7 msg
        sprintf(buf, "F7 t=%d c=%c r=%c a=%c b=%c 1=%-11.11s%2d:%02d 2=%-16.16s\n",
            tone, chime ? '1' : '0', ready ? '1' : '0', armed == DISARMED ? '0' : '1',
            backlight ? '1' : '0', line1, hour, min, line2);
#endif
    }
}

const char * AlarmManager::getSockMsg(int hour, int min)
{
    static char buf[64];
    sprintf(buf, "%c%c%-11.11s%2d:%02d~%-16.16s", ready ? 'R' : 'r', armed == DISARMED ? 'a' : 'A', 
        line1, hour, min, line2);
    return buf;
}


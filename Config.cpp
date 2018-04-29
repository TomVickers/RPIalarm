// The file Config.cpp is part of RPIalarm.
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
#include "Config.h"

// init pin line from config file.  Return true if success
bool Pin::init(const char * line)
{
    const char * p = line;
    p += Config::getTextParm(p, name, sizeof(name));
    return (p > line && Config::getTextParm(p, pin, sizeof(pin)) > 0);
}

// init loop line from config file.  Return true if success
bool Loop::init(const char * line)
{
    int  len = strlen(line);
    bool success = true;
    char * endptr;
    const char * p;

    gpio = strtol(line, &endptr, 10);  // grab gpio pin number

    if (endptr != line && endptr != NULL && endptr - line < len)
    {
        p = (const char *)endptr;
        p += Config::getTextParm(p, name, sizeof(name));  // grab loop name
        p = Config::nextParm(p);                          // jump over spaces after loop name

        // get chime tone for loop
        if (strncmp(p, "TONE_CHIME_1", 12) == 0)
            chimeTone = TONE_CHIME_1;
        else if (strncmp(p, "TONE_CHIME_2", 12) == 0)
            chimeTone = TONE_CHIME_2;
        else if (strncmp(p, "TONE_CHIME_3", 12) == 0)
            chimeTone = TONE_CHIME_3;
        else
        {
            chimeTone = TONE_NONE;
            success = false;
        }

        if (p + 12 - line < len)
        {
            p = Config::nextParm(p+12);
            entryAllowed = (strncmp(p, "true", 4) == 0);
        }
        else
            success = false;
    }
    else
        success = false;

    return success;
}

// init gpio output line from config file.  Return true if success
bool GpioOutput::init(const char * line)
{
    char * endptr;

    gpio = strtol(line, &endptr, 10);  // grab gpio pin number
    const char * p = (const char *)endptr;

    return (p > line && Config::getTextParm(p, funcName, sizeof(funcName)) > 0);
}

// get pointer to next parm on config line, skipping over any white-space chars
const char * Config::nextParm(const char * buf)
{
    while (*buf == ' ' || *buf == '\t')
        buf++;
    return buf;
}

// copy next text parm to parm (any non-whitespace text chars allowed)
int Config::getTextParm(const char * buf, char * parm, int parmSize)
{
    const char * b2 = nextParm(buf);  // skip past any blank chars

    int i = 0;
    while (i < parmSize-1 && *b2 != ' ' && *b2 != '\t' && *b2 != '\0')
    {
        *parm++ = *b2++;
        i++;
    }
    *parm = '\0';
    return b2 - buf;  // return number of consumed chars (blanks + parm)
}

// copy next num parm to parm (only 0-9 chars allowed)
int Config::getNumParm(const char * buf, char * parm, int parmSize)
{
    const char * b2 = nextParm(buf);  // skip past any blank chars

    int i=0;
    while (i < parmSize-1 && *b2 >= '0' && *b2 <= '9')
    {
        *parm++ = *b2++;
        i++;
    }
    *parm = '\0';
    return b2 - buf;  // return number of consumed chars (blanks + parm)
}

bool Config::otherConfig(const char * line)
{
    const char * p = nextParm(line);  // skip any leading white space

    if (strncmp(p, "SERIAL_PORT", strlen("SERIAL_PORT")) == 0)
    {
        p = nextParm(p + strlen("SERIAL_PORT"));  // point to arg
        strncpy(serialPort, p, MAX_PARM_LENGTH-1);
        return true;
    }
    else if (strncmp(p, "SERIAL_BAUD_RATE", strlen("SERIAL_BAUD_RATE")) == 0)
    {
        p = nextParm(p + strlen("SERIAL_BAUD_RATE"));  // point to arg
        baudRate = atoi(p);
        return true;
    }
    else if (strncmp(p, "CHIME_DEFAULT", strlen("CHIME_DEFAULT")) == 0)
    {
        p = nextParm(p + strlen("CHIME_DEFAULT"));  // point to arg
        chimeDefault = (*p == 't' || *p == 'T' || *p == '1');  // true if arg begins with t,T or is 1
        return true;
    }
    else if (strncmp(p, "SEND_EMAIL_ACCNT", strlen("SEND_EMAIL_ACCNT")) == 0)
    {
        p = nextParm(p + strlen("SEND_EMAIL_ACCNT"));  // point to arg
        strncpy(sendEmailAccnt, p, MAX_PARM_LENGTH-1);
        return true;
    }
    else if (strncmp(p, "SEND_EMAIL_PASSWD", strlen("SEND_EMAIL_PASSWD")) == 0)
    {
        p = nextParm(p + strlen("SEND_EMAIL_PASSWD"));  // point to arg
        strncpy(sendEmailPasswd, p, MAX_PARM_LENGTH-1);
        return true;
    }
    else if (strncmp(p, "ALERT_EMAIL", strlen("ALERT_EMAIL")) == 0)
    {
        p = nextParm(p + strlen("ALERT_EMAIL"));  // point to arg
        strncpy(alertEmail, p, MAX_PARM_LENGTH-1);
        return true;
    }
    else
    {
        // do nothing?
    }
    return false;
}

// return the gpio pin associated with the provided function name
int Config::getOutput(const char * func)
{
    for (int i=0; i < outputCount; i++)
    {
        if (strcmp(func, Output[i].funcName) == 0)
            return Output[i].gpio;
    }
    return -1;
}

// read alarm config file.  Returns true on success
bool Config::readFile(const char * pathAndFilename)
{
    enum {
        SECTION_NONE = 0,
        SECTION_PIN,
        SECTION_IO_INPUT,
        SECTION_IO_OUTPUT,
        SECTION_CONFIG
    } eSection = SECTION_NONE;

    bool success = true;
    FILE * fp = fopen(pathAndFilename, "r");
    int lineCount = 0;

    if (fp == NULL)
    {
        fprintf(stderr, "Failed to open config file '%s'\n", pathAndFilename);
        return false;
    }

    while (success && fgets(lineBuf, LINE_BUF_SIZE-1, fp) == lineBuf)  // get one line from config file
    {
        for (int i=0; i < (int)strlen(lineBuf); i++)
        {
            if (lineBuf[i] == '\n')   // strip newline 
                lineBuf[i] = '\0';
        }

        const char * pBuf = nextParm((const char *)lineBuf);  // get pointer to first non-blank char

        if (*pBuf == '\0' || *pBuf == '#')
        {
            // blank line or comment, skip it
        }
        else if (*pBuf == '_')  // section header
        {
            if (strncmp(pBuf, "_START_PIN_SECTION", 18) == 0)
            {
                eSection = SECTION_PIN;
            }
            else if (strncmp(pBuf, "_START_IO_INPUT_SECTION", 23) == 0)
            {
                eSection = SECTION_IO_INPUT;
            }
            else if (strncmp(pBuf, "_START_IO_OUTPUT_SECTION", 24) == 0)
            {
                eSection = SECTION_IO_OUTPUT;
            }
            else if (strncmp(pBuf, "_START_CONFIG_SECTION", 21) == 0)
            {
                eSection = SECTION_CONFIG;
            }
            else
            {
                eSection = SECTION_NONE;
            }
        }
        else if (eSection == SECTION_PIN)
        {
            if (pinCount < MAX_PIN_CODES && AlarmPin[pinCount].init(pBuf))
            {
                pinCount++;
            }
            else 
            {
                if (pinCount >= MAX_PIN_CODES)
                    fprintf(stderr, "Too many PIN lines in config file\n");
                else
                    fprintf(stderr, "Error processing pin line '%s'\n", lineBuf);
                success = false;
            }
        }
        else if (eSection == SECTION_IO_INPUT)
        {
            if (strncmp(pBuf, "LOOP", 4) == 0)  // line begins with loop
            {
                pBuf += strlen("LOOP");
                if (loopCount < MAX_SENSE_LOOPS && SenseLoop[loopCount].init(pBuf))
                {
                    loopCount++;
                }
                else 
                {
                    if (loopCount >= MAX_SENSE_LOOPS)
                        fprintf(stderr, "Too many LOOP lines in config file\n");
                    else
                        fprintf(stderr, "Error processing loop line '%s'\n", lineBuf);
                    success = false;
                }
            }
            else 
            {
                fprintf(stderr, "Unrecognized IO_INPUT section line '%s'\n", lineBuf);
            }
        }
        else if (eSection == SECTION_IO_OUTPUT)
        {
            if (outputCount < MAX_GPIO_OUTPUTS && Output[outputCount].init(pBuf))
            {
                outputCount++;
            }
            else
            {
                if (outputCount >= MAX_GPIO_OUTPUTS)
                    fprintf(stderr, "Too many gpio output lines in config file\n");
                else
                    fprintf(stderr, "Error processing gpio output line '%s'\n", lineBuf);
                success = false;
            }
        }
        else if (eSection == SECTION_CONFIG)
        {
            if (!otherConfig(pBuf))
                success = false;
        }
        else  // eSection == SECTION_NONE
        {
            fprintf(stderr, "readFile error line %d: line outside section\n", lineCount);
            success = false;
        }
        lineCount++;
    }
    fclose(fp);
    //fprintf(stderr, "Config::readFile processed %d lines\n", lineCount);
    return success;
}

// end of config.cpp

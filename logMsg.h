// logMsg.h - code to handle logging of debug messages

#pragma once

#include "stdafx.h"

bool initLogMsg(void);
void finiLogMsg(void);
void logEverything(bool state);
void logMsg(const char *fmt, ...);
void flushMsgRing(void);

// end of logMsg.h

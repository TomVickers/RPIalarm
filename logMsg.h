// The file logMsg.h is part of RPIalarm.
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

#define MAX_DEBUG_LOGS  (4)  // should be last LOG_DEBUG_N enum + 1

enum t_eLogMsgType
{
    LOG_DEFAULT,  // log to default (ring buffer)
    LOG_DEBUG_1,  // immediately write to dbg1 file
    LOG_DEBUG_2,  // immediately write to dbg2 file
    LOG_DEBUG_3   // immediately write to dbg3 file
};

bool initLogMsg(void);
void finiLogMsg(void);
void logMsg(uint8_t logType, const char *fmt, ...);
void flushMsgRing(void);

// end of logMsg.h

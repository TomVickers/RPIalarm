// The file SockClient.h is part of RPIalarm.
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

class SockClient
{
public:
    bool init(void);
    void fini(void);

    bool sendMsg(const char * msg);
    int  recvMsg(char * buf, int bufSize);

private:
    int clientSock;
    int failCount;
};

// end of SockClient.h

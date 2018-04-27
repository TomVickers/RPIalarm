// SockClient.h - class to manage socket connection to web app

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

};

// end of SockClient.h

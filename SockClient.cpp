// SockClient.cpp - alarm socket client class
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>

#include "SockClient.h"

static const uint16_t port = YOUR_SERVER_PORT;

bool SockClient::init(void)
{
    clientSock = socket(PF_INET, SOCK_STREAM, 0);

    if (clientSock >= 0)
    {
  	    struct sockaddr_in serverAddr;
        socklen_t addrSize = sizeof(serverAddr);

  	    serverAddr.sin_family = AF_INET;
  	    serverAddr.sin_port = htons(port);
  	    serverAddr.sin_addr.s_addr = inet_addr("YOUR_SERVER_IP_ADDRESS");
	    memset(serverAddr.sin_zero, 0, sizeof(serverAddr.sin_zero));

        if (connect(clientSock, (struct sockaddr *)&serverAddr, addrSize) >= 0)
        {
            // change socket to non-blocking
            fcntl(clientSock, F_SETFL, fcntl(clientSock, F_GETFL, 0) | O_NONBLOCK);
            return true;
        }
    }
    return false;
}

void SockClient::fini(void)
{
    if (clientSock >= 0)
    {
        close(clientSock);
        clientSock = -1;
    }
}

int SockClient::recvMsg(char * buf, int bufSize)
{
    int readBytes;

    if (clientSock < 0) // socket is not open
    {
        init();  // try and open socket
    }
    else  // socket is open
    {
        readBytes = recv(clientSock, buf, bufSize-1, 0);
        if (readBytes > 0)
        {
            //fprintf(stderr, "Recv %d bytes: %s\n", readBytes, buf);
            return readBytes;
        }
        else  // socket closed or failed to read, re-try once
        {
            if (readBytes < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
            {
                // timeout, nothing to read
                return 0;
            }
            else
            {
                fini();
                if (init())
                {
                    readBytes = recv(clientSock, buf, bufSize-1, 0);
                    if (readBytes > 0)
                    {
                        //fprintf(stderr, "2nd try: Recv %d bytes: %s\n", readBytes, buf);
                        return readBytes;
                    }
                }
            }
        }
    }
    return 0;
}

bool SockClient::sendMsg(const char * msg)
{
    if (clientSock < 0)
    {
        init();  // try and open socket
    }
    if (clientSock >= 0)
    {
        if (send(clientSock, msg, strlen(msg), 0) >= 0)
        {
            return true;
        }
        else  // try and re-init the socket once
        {
            fini();
            if (init())
            {
                if (send(clientSock, msg, strlen(msg), 0) >= 0)
                {
                    return true;
                }
            }
        }
            
    }
    return false;
}


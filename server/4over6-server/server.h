#pragma once
#include "common.h"
#include "clientConnection.h"

#define SERVER_PORT 5678 
#define MAX_LISTEN_COUNT 5 
    
class Server
{
public:
    sockaddr_in6 serverAddr ;
    int serverFd, epollFd ;
    void startServer();
};


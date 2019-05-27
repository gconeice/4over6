
#include "server.h"
#include <unordered_map>

std::unordered_map<int, ClientConnection*> clientConnections ;

void Server::startServer() 
{
    printf("Starting Server...\n") ;

    printf("Generating socket...") ;
    serverFd = socket(AF_INET6, SOCK_STREAM, 0); 
    if (serverFd == -1) 
    {
        printf("error! can not create server socket\n") ;
        exit(1) ;
    }
    printf("done. Server socket is %d\n", serverFd) ;

    int reuse = 1;
    setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &reuse, (socklen_t ) sizeof(int));        

    printf("Binding socket...") ;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin6_family = AF_INET6;
    serverAddr.sin6_port = htons(SERVER_PORT);
    if(bind(serverFd, (const struct sockaddr *) &serverAddr, (socklen_t) sizeof(struct sockaddr_in6)) == -1) 
    {
        printf("error! can not bind address\n") ;
        exit(-1) ;
    }
    printf("done. The used port is %d\n", SERVER_PORT) ;

    if(listen(serverFd, MAX_LISTEN_COUNT) == -1) 
    {
        printf("error! can not start listening\n") ;
        exit(-1) ;
    }
 
    int epollFd = -1;
    struct epoll_event event ;

    epollFd = epoll_create(MAX_LISTEN_COUNT);
    if (epollFd == -1) 
    {
        printf("error! can not create epoll\n") ;
        exit(-1) ;
    }

    memset(&event, 0, sizeof(struct epoll_event));
    event.data.fd = serverFd;
    event.events = EPOLLIN ;
    epoll_ctl(epollFd, EPOLL_CTL_ADD, serverFd, &event);

    int epollRet = 0;
    Msg readedMsg;
    socklen_t ipv6AddrSize = sizeof(struct sockaddr_in6);

    sockaddr_in6 clientAddr;

    printf("starting server done. Now listening to the socket...\n") ;

    while (true) 
    {
            epollRet = epoll_wait(epollFd, &event, MAX_LISTEN_COUNT, -1);
            printf("A new epoll event happened!\n") ;
            if (epollRet == -1) 
            {
                    printf("error occurs\n");
                    continue;
            } 
            else if (epollRet == 0) 
                 printf("timeout\n");
            else if (event.data.fd == serverFd) // a new client
            { 
                int connectionFd = accept(serverFd, (sockaddr*)&clientAddr, &ipv6AddrSize);
                if (connectionFd == -1) // || fcntl(connectionFd, F_SETFL, fcntl(connectionFd, F_GETFD, 0)|O_NONBLOCK)) 
                {
                    printf("error: invalid connection. retry...\n") ;
                    continue;
                }
                ClientConnection* conn = new ClientConnection(clientAddr, connectionFd) ;
                conn->run() ;
                printf("new connect\n");
            } 
    }
}

 
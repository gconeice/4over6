#pragma once
#include "common.h"
#include "msg_queue.h"
#include <unordered_map>
#include <atomic>   

inline int ipNum(std::string addr) ;

class ClientConnection
{
public:

    ClientConnection(const sockaddr_in6& address, int fd) : socketFd(fd) 
    {
        char addrStr[105] ;
        heartBeatCount.store(0) ;
        inet_ntop(AF_INET6, &address.sin6_addr, addrStr, sizeof(addrStr)) ;
        ipv6Address = std::string(addrStr) ;
    }
    int getSocketFd() {return socketFd;}


    void run();

    void keepLiveThread() ;
    void writeThread();
    void readThread() ;
    
    int sendMessage(const Msg& message) ;
    void push_to_write_queue(int len, char type, std::string &data);

    static bool status[1024];
    static std::unordered_map<int, ClientConnection*> connections ;
    //static std::mutex ipMutex ;

    static void setIpStatus(const std::string& addr, bool s) ;
    static ClientConnection* getConnectionByIp(const std::string& addr) ;
    static void setConnectionWithIp(const std::string& addr, ClientConnection* conn) ;
    static std::string getFreeIP() ;

private:

    int socketFd ;
    std::atomic<int> heartBeatCount;
    std::mutex sendMutex ;
    std::string ipv6Address, ipv4Address ;
    MsgQueue<Msg> wtSktQue;
};
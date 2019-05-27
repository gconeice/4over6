#pragma once
#include "common.h"
#include "msg_queue.cpp"
#include "clientConnection.h"

class Tun {
public:
    Tun(std::string device);
    void pushQueue(std::string packet);
    void tunWriter();
    void tunReader();
    void start();
    std::string tunRead();
    int tunWrite(const char *buf, int len);
    int tunWrite(std::string &buf);
    
    
private:
    MsgQueue<std::string> waitingQuene;
    int fd;
};


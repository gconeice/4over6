#include "common.h"
#include "clientConnection.h"
#include "tun.h"
#include "server.h"

Tun *tun;

int main()
{
    tun = new Tun(std::string("tun5"));
    tun->start();

    Server server;
    server.startServer() ;

    return 0;
}
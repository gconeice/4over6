#include "clientConnection.h"
#include "tun.h"

extern Tun *tun;

bool ClientConnection::status[1024] ;
std::unordered_map<int, ClientConnection*> ClientConnection::connections ;
std::mutex ipMutex ;

inline int getIpNum(std::string addr) 
{
    std::istringstream f(addr);
    std::string s;
    while (getline(f, s, '.'));
    return std::stoi(s);
}

void ClientConnection::setIpStatus(const std::string& addr, bool s) 
{
    ipMutex.lock() ;
    int num = getIpNum(addr) ;
    assert(status[num] != s) ;
    status[num] = s;
    ipMutex.unlock() ;
}

ClientConnection* ClientConnection::getConnectionByIp(const std::string& addr) 
{
    ipMutex.lock() ;
    ClientConnection* ret = status[getIpNum(addr)]?connections[getIpNum(addr)]:NULL ;
    ipMutex.unlock() ;
    return ret ;
}

void ClientConnection::setConnectionWithIp(const std::string& addr, ClientConnection* connection) 
{
    ipMutex.lock() ;
    int num = getIpNum(addr);
    if(!status[num]) setIpStatus(addr, true) ;
    printf("setting connection map with ip %s\n", addr.c_str()) ;
    connections[num] = connection;
    ipMutex.unlock() ;
}

std::string ClientConnection::getFreeIP() {
    ipMutex.lock() ;
    for (int i = 2; i < 1024; i++) 
        if (!status[i])
        {
            ipMutex.unlock() ;
            return std::string("10.128.0.") + std::to_string(i);
        }
    ipMutex.unlock() ;
    printf("Error: no avaliable ip left.\n") ;
    exit(1) ;
}

int ClientConnection::sendMessage(const Msg& message) 
{
    sendMutex.lock() ;
    printf("Sending an message of type %d to socket %d\n", message.type, socketFd) ;
    int currentLength = 0, totalLength = 0 ;
    while(totalLength < 5) 
    {
        //currentLength = write(socketFd, &message, sizeof(Msg)) ;
        currentLength = write(socketFd, ((char*)&message)+totalLength, 5-totalLength) ;
        if(currentLength == 0 || currentLength == -1 && errno == EAGAIN) continue ;
        if(currentLength == -1) 
        {
            sendMutex.unlock() ;
            return -1 ;
        }
        printf("%d %d\n", currentLength, errno) ;
        totalLength += currentLength ;
    }
    if(message.type == 101 || message.type == 103)
    {
        totalLength = 0 ;
        while(totalLength < message.length-5) 
        {
            //currentLength = write(socketFd, &message, sizeof(Msg)) ;
            currentLength = write(socketFd, ((char*)&message.data)+totalLength, message.length-5-totalLength) ;
            if(currentLength == 0 || currentLength == -1 && errno == EAGAIN) continue ;
            if(currentLength == -1) 
            {
                sendMutex.unlock() ;
                return -1 ;
            }
            printf("%d %d\n", currentLength, errno) ;
            totalLength += currentLength ;
        }
    }
    sendMutex.unlock() ;
    printf("Sending Complete.\n") ;
    return 0 ;
}

void ClientConnection::keepLiveThread()
{
    Msg heartBeatMsg ;
    while (heartBeatCount.load() < 60) 
    {
        sleep(1) ;
        heartBeatCount ++;
        printf("Heartbeat time of socket %d: %d\n", socketFd, heartBeatCount.load()) ;
        heartBeatMsg.type = 104 ;
        heartBeatMsg.length = 0 ;
        if(heartBeatCount%20 == 0)
        {
            if(sendMessage(heartBeatMsg) == -1) break ;
            printf("Sent heart beat signal to socket %d\n", socketFd) ;
        }
    }

    close(socketFd) ;
    printf("No heartbeat signal: Socket %d has been closed.\n", socketFd) ;

    printf("Ending keeplive thread of socket %d, ipv6 address %s\n", socketFd, ipv6Address.c_str()) ;
};


void ClientConnection::writeThread() {
    while(true) {
        Msg message ; 
        wtSktQue.pop(message);
        if(sendMessage(message) == -1) break ;
        //std::cout << "Writed!" << std::endl;
    }
    printf("Ending write thread of socket %d, ipv6 address %s\n", socketFd, ipv6Address.c_str()) ;
}

void ClientConnection::push_to_write_queue(int len, char type, std::string &data) 
{
    Msg tmp ;
    tmp.length = len ;
    tmp.type = type ;
    memcpy(tmp.data, data.c_str(), len-5) ;
    wtSktQue.push(std::move(tmp));
}

void ClientConnection::readThread()
{
    Msg message ;
    while (heartBeatCount.load() < 60) {
        int currentLength = 0, totalLength = 0 ;
        while(totalLength < 5) 
        {
            //currentLength = read(socketFd, &message, sizeof(Msg)) ;
            currentLength = read(socketFd, ((char*)&message)+totalLength, 5-totalLength) ;
            if(currentLength == 0 || currentLength == -1 && errno == EAGAIN) continue ;
            if(currentLength == -1) break ;
            printf("reading: %d %d\n", currentLength, errno) ;
            totalLength += currentLength ;
        }
        if(message.type == 102)
        {
            totalLength = 0 ;
            while(totalLength < message.length-5) 
            {
                //currentLength = read(socketFd, &message, sizeof(Msg)) ;
                currentLength = read(socketFd, ((char*)&message.data)+totalLength, message.length-5-totalLength) ;
                if(currentLength == 0 || currentLength == -1 && errno == EAGAIN) continue ;
                if(currentLength == -1) goto endReading ;
                printf("reading: %d %d\n", currentLength, errno) ;
                totalLength += currentLength ;
            }
        }

        printf("Readed an message from socket %d with type %d, length %d, ipv6 address %s\n", socketFd, message.type, message.length, ipv6Address.c_str()) ;
        switch(message.type)
        {
            case 100:
            {
                assert(!ipv4Address.length() || getConnectionByIp(ipv4Address) == NULL) ;
                if (!ipv4Address.length()) 
                    ipv4Address = getFreeIP();
                std::string ipInfoStr = ipv4Address+ " 0.0.0.0 202.38.120.242 8.8.8.8 202.106.0.20";

                memset(message.data, 0, sizeof(message.data)) ;
                message.type = 101 ;
                message.length = ipInfoStr.length()+5 ;
                memcpy(message.data, ipInfoStr.c_str(), message.length) ;
                message.data[message.length] = 0 ;

                printf("Sending ip info \"%s\" to socket %d\n", message.data, socketFd) ;
                if(sendMessage(message) == -1) goto endReading ;

                //socket_.write_some(buf);
                //std::cout << "PostSent " + Cliv4Addr.to_string() << std::endl;
                printf("Set ipv4 IP complete.\n") ;
                setIpStatus(ipv4Address, true);
                printf("Set ipv4 IP complete.\n") ;
                setConnectionWithIp(ipv4Address, this);
                break ;
            }
            case 104:
            {
                heartBeatCount.store(0) ;
                break ;
            }
            case 102:
            {
                tun->pushQueue(std::string(message.data, message.length));
                break ;
            }
            default:
            {
                printf("error: invalid message type of connection %d. type %d\n", socketFd, message.type) ;
                break ;
            }
        }
    }
endReading:
    setIpStatus(ipv4Address, false);
    printf("Ending read thread of socket %d, ipv6 address %s\n", socketFd, ipv6Address.c_str()) ;
}

void ClientConnection::run() 
{
    printf("Creating a connection thread group on socket %d, ipv6 address %s\n", socketFd, ipv6Address.c_str()) ;
    std::thread(&ClientConnection::writeThread, this).detach();
    std::thread(&ClientConnection::keepLiveThread, this).detach();
    std::thread(&ClientConnection::readThread, this).detach() ;
}

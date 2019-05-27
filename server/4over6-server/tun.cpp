#include "tun.h"

std::string mkAddr(const std::string &str) {

    return  std::to_string(int(str[0]) < 0 ? int(str[0]) + 256 : int(str[0])) + "."
            + std::to_string(int(str[1]) < 0 ? int(str[1]) + 256 : int(str[1])) + "."
            + std::to_string(int(str[2]) < 0 ? int(str[2]) + 256 : int(str[2])) + "."
            + std::to_string(int(str[3]) < 0 ? int(str[3]) + 256 : int(str[3]));
}

Tun::Tun(std::string device) {
    ifreq ifr;
    int err;
    if ((fd = open("/dev/net/tun", O_RDWR)) == -1) {
        perror("error: cannot open tun");
        exit(1);
    }
    memset(&ifr, 0, sizeof(ifr));
    device.copy(ifr.ifr_name, IFNAMSIZ);
    ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
    if ((err = ioctl(fd, TUNSETIFF, (void *) &ifr)) == -1) {
        perror("error: ioctl TUNSETIFF");
        close(fd);
        exit(1);
    }
    std::cout << device << " opened" << std::endl;
}

void Tun::pushQueue(std::string packet) {
    waitingQuene.push(packet);
}

void Tun::tunWriter() {
    std::string buf;
    while(1) {
        waitingQuene.pop(buf);
        //printf("Push a message to the tun.\n") ;
        write(fd, buf.c_str(), buf.length());
    }
}

std::string Tun::tunRead() {
    static char read_buf[1600];
    auto nbytes = read(fd, read_buf, sizeof(read_buf));
    //printf("readed from tun with length %d: %s\n", nbytes, read_buf) ;
    //printf("readed from tun with length %d: %s\n", nbytes, read_buf) ;
    return std::string(read_buf, nbytes);
}

void Tun::tunReader() {
    std::string buf;
    while (true) {
        buf = tunRead();
        std::string dst_ip = buf.substr(16, 4);
        printf("readed from tun with ip %s\n", mkAddr(dst_ip).c_str()) ;
        auto conn = ClientConnection::getConnectionByIp(mkAddr(dst_ip));
        if (conn == NULL) continue;
        printf("readed from tun: ip %s, socket %d\n", mkAddr(dst_ip), conn->getSocketFd()) ;
        int len = buf.length();
        char type = 103;
        conn->push_to_write_queue(len + 5, type, buf);
    }
}

void Tun::start() {
    auto writeThread = std::thread(&Tun::tunWriter, this);
    writeThread.detach();
    auto readThread = std::thread(&Tun::tunReader, this);
    readThread.detach();
}


int Tun::tunWrite(const char *buf, int len) {
    std::string buffer(buf, len);
    std::cout << "Write to TUN data: " << std::string(buf, len) << std::endl;
    auto dst_ip = buffer.substr(16, 4);
    auto src_ip = buffer.substr(12, 4);
    std::cout << "src: " + mkAddr(src_ip) << std::endl;
    std::cout << "dst: " + mkAddr(dst_ip) << std::endl;
    auto nbytes = write(fd, buf, len);
    return nbytes;
}

int Tun::tunWrite(std::string &buf) {
    return tunWrite(buf.c_str(), buf.length());
}







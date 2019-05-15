#include "4o6.h"
#include "server_util.h"

using namespace std;

static struct IPADDR ipaddr[CLIENT_NUM]; //全局变量的地址池
struct User_Info_Table users[CLIENT_NUM];
struct buffer_entry bufs[MAX_FDS];
static struct sockaddr_in6 server_addr, client_addr;
int server_fd = -1, tun_fd = -1, epoll_fd = -1;

// socket 初始化
int ServerInit()
{
	int fd;
	if ((fd = socket(AF_INET6, SOCK_STREAM, 0)) < 0)
	{
		printf("Error: open server socket failed!\n");
		return -1;
	}

	int on = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const void *)&on, sizeof(on)) < 0)
	{
        printf("Error: enable address use failed\n");
        return -1;
    }

	// bind
	memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin6_family = AF_INET6;
    server_addr.sin6_port = htons(SERV_PORT);
    server_addr.sin6_addr = in6addr_any;
	if (bind(fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	{
        printf("Error: bind server socket failed\n");
        return -1;
    }

	// listen
	if (listen(fd, MAX_FDS) < 0) {
        printf("Error: listen on socket failed\n");
        return -1;
    }

    return fd;
}

// tun 初始化

int TunInit() {
    int fd;

    // open tun
    if ((fd = open("/dev/net/tun", O_RDWR)) < 0) {
        printf("Error: open /dev/net/tun failed\n");
        return -1;
    }

    // issue name change request
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
    strncpy(ifr.ifr_name, "over6", IFNAMSIZ);

    if (ioctl(fd, TUNSETIFF, (void *)&ifr) == -1) {
        printf("Error: set tun interface name failed\n");
        return -1;
    }

    // enable interface and nat
    system("ifconfig over6 " ADDRESS_PREFIX ".1 netmask 255.255.255.0 up");
    system("iptables -t nat -A POSTROUTING -s " ADDRESS_PREFIX ".0/24 -j MASQUERADE");

    return fd;
}

int epoll_add_fd(int epoll_fd, int fd, int events) {
    // add socket to epoll
    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events = events;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) < 0) {
        printf("Error: failed to add fd %d to epoll\n", fd);
        return -1;
    }
    return 0;
}

int MainThread()
{
	// 创建IPV6套接字，把该套接字加入Select模型字符集
	if ((server_fd = ServerInit()) == -1) return -1;
	printf("server_fd = %d\n", server_fd);

	// 创建tun虚接口
	if ((tun_fd = TunInit()) == -1) return -1;
	printf("tun_fd = %d\n", tun_fd);

	// 初始化epoll
    if ((epoll_fd = epoll_create(EPOLL_MAX_EVENTS)) < 0) {
        printf("Error: create epoll fd failed\n");
		return -1;
    }

	// 加入server和tun
	if (epoll_add_fd(epoll_fd, server_fd, EPOLL_EVENTS) < 0) {
        printf("Error: failed to add server socket to epoll\n");
		return -1;
    }
    if (epoll_add_fd(epoll_fd, tun_fd, EPOLL_EVENTS) < 0) {
        printf("Error: failed to add tun to epoll\n");
		return -1;
    }

	// 初始化user table
    for (int i = 0; i < CLIENT_NUM; i++) {
        sprintf(users[i].addr, ADDRESS_PREFIX ".%d", i);
        users[i].status = 1;
        users[i].fd = -1;
    }
    users[0].status = 0; // x.x.x.0 is invalid
    users[1].status = 0; // x.x.x.1 is invalid
    users[255].status = 0; // x.x.x.255 is invalid

	// 初始化buffer pool
    for (int i = 0; i < MAX_FDS; i++) {
        bufs[i].used = 0;
    }

	return 0;
}

int main()
{
	if (MainThread() == -1) goto clean;
	while (1)
	{
		cout << "cnm" << endl;
	}

clean:
    if (server_fd != -1) close(server_fd);
	if (tun_fd != -1) close(tun_fd);
    for(int i = 0; i < CLIENT_NUM; i ++)
        if (users[i].fd >= 0)
            close(users[i].fd);
	return 0;
}

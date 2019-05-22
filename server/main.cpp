#include "4o6.h"
#include "server_util.h"

using namespace std;

static struct User_Info_Table users[CLIENT_NUM];
static struct buffer_entry bufs[MAX_FDS];
static struct sockaddr_in6 server_addr, client_addr;
int server_fd = -1, tun_fd = -1, epoll_fd = -1;

// search for user info entry given ipv4 address
int search_user_info_by_addr(uint32_t addr)
{
    int i;
    for(i = 0; i < CLIENT_NUM; i ++)
        if (users[i].v4addr.s_addr == addr && !users[i].status)
            return i;
    return -1;
}

// search for user info entry given fd
int search_user_info_by_fd(int client_fd)
{
	int i;
	for (i = 0; i < CLIENT_NUM; i ++)
		if (users[i].fd == client_fd && !users[i].status)
			return i;	
	return -1;
}

int epoll_add_fd(int epoll_fd, int fd, int events)
{
    // add socket to epoll
    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events = events;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) < 0)
	{
        printf("Error: failed to add fd %d to epoll\n", fd);
        return -1;
    }
    return 0;
}

// remove fd from epoll
int epoll_remove_fd(int epoll_fd, int fd)
{
    if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL) < 0)
	{
        printf("Error: failed to remove fd %d from epoll\n", fd);
        return -1;
    }
    return 0;
}

// allocate ip address from pool
int allocate_ip_addr(int client_fd)
{
    int i;
    for(i = 0; i < CLIENT_NUM; i ++)
        if (users[i].status)
		{
            users[i].status = 0;

            users[i].fd = client_fd;
            users[i].last_heartbeat_sent_secs = time(0);
            users[i].last_heartbeat_recved_secs = time(0);
            inet_pton(AF_INET, users[i].addr, (void *)&users[i].v4addr);

            struct sockaddr_in6 clientaddr;
            socklen_t addrsize = sizeof(clientaddr);
            getpeername(client_fd, (struct sockaddr *)&clientaddr, &addrsize);
            users[i].v6addr = clientaddr.sin6_addr;
            
            return i;
        }
    return -1;
}

// deallocate ip address from pool
int deallocate_ip_addr(int client_fd)
{
    int i;
    for(i = 0; i < CLIENT_NUM; i ++)
        if (users[i].fd == client_fd && !users[i].status)
		{
            users[i].status = 1;
            users[i].fd = -1;
            return i;
        }
    return -1;
}

void free_client_fd(int client_fd, int epoll_fd)
{
    if (epoll_remove_fd(epoll_fd, client_fd) < 0)
	{
        printf("Error: failed to remove client fd %d from epoll\n", client_fd);
        return;
    }
    deallocate_ip_addr(client_fd);
    close(client_fd);
}

int write_all(int fd, void* buf, int size)
{
    int offset = 0;
    uint8_t* w = (uint8_t*)buf;
    while(offset < size)
	{
        int ret;
        if ((ret = write(fd, w+offset, size-offset)) < 0)
		{
            printf("Error: fail to write data to fd = %d: %s\n", fd, strerror(errno));
            return ret;
        }
        offset += ret;
    }
    return offset;
}

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
    if ((fd = open("/dev/net/tun", O_RDWR)) < 0)
	{
        printf("Error: open /dev/net/tun failed\n");
        return -1;
    }

    // issue name change request
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
    strncpy(ifr.ifr_name, "over6", IFNAMSIZ);

    if (ioctl(fd, TUNSETIFF, (void *)&ifr) == -1)
	{
        printf("Error: set tun interface name failed\n");
        return -1;
    }

    // enable interface and nat
    system("ifconfig over6 " ADDRESS_PREFIX ".1 netmask 255.255.255.0 up");
    system("iptables -t nat -A POSTROUTING -s " ADDRESS_PREFIX ".0/24 -j MASQUERADE");

    return fd;
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
    if ((epoll_fd = epoll_create(EPOLL_MAX_EVENTS)) < 0)
	{
        printf("Error: create epoll fd failed\n");
		return -1;
    }

	// 加入server和tun
	if (epoll_add_fd(epoll_fd, server_fd, EPOLL_EVENTS) < 0)
	{
        printf("Error: failed to add server socket to epoll\n");
		return -1;
    }
    if (epoll_add_fd(epoll_fd, tun_fd, EPOLL_EVENTS) < 0)
	{
        printf("Error: failed to add tun to epoll\n");
		return -1;
    }

	// 初始化user table
    for (int i = 0; i < CLIENT_NUM; i++)
	{
        sprintf(users[i].addr, ADDRESS_PREFIX ".%d", i);
        users[i].status = 1;
        users[i].fd = -1;
    }
    users[0].status = 0; // x.x.x.0 is invalid
    users[1].status = 0; // x.x.x.1 is invalid
    users[255].status = 0; // x.x.x.255 is invalid

	// 初始化buffer pool
    for (int i = 0; i < MAX_FDS; i++)
	{
        bufs[i].used = 0;
    }

	return 0;
}

void process_server(int server_fd, int epoll_fd)
{
    struct sockaddr_in6 clientaddr;
    memset(&clientaddr, 0, sizeof(clientaddr));
    socklen_t addrsize = sizeof(clientaddr);

    // accept new connection from a client
    int client_fd;
    if ((client_fd = accept(server_fd, (struct sockaddr *)&clientaddr, &addrsize)) < 0)
	{
        printf("Error: accept connection failed\n");
        return;
    }

    // setup buffer
    bufs[client_fd].used = 0;

    // get and display peer info
    char addrpeer6[BUF_SIZE];
    getpeername(client_fd, (struct sockaddr *)&clientaddr, &addrsize);
    inet_ntop(AF_INET6, &clientaddr.sin6_addr, addrpeer6, sizeof(addrpeer6));
    printf("New connection - addr: %s\n", addrpeer6);
    printf("New connection - port: %d\n", ntohs(clientaddr.sin6_port));

    // add new client to epoll list
    if (epoll_add_fd(epoll_fd, client_fd, EPOLL_EVENTS) < 0)
	{
        printf("Error: failed to add client fd %d to epoll\n", client_fd);
        return;
    }
}

void process_tun(int tun_fd)
{
    char* buf = bufs[tun_fd].buf;
    int& nread = bufs[tun_fd].used;

    int size;
    // read data from tun
    if ((size = read(tun_fd, buf+nread, BUF_SIZE-nread)) < 0)
	{
        printf("Error: can not read from tun\n");
        return;
    }
    nread += size;

    while(true)
	{

        if (nread < (int)sizeof(struct iphdr)) // linux
            break;
        
        // extract ip address from ip header
        struct iphdr hdr = *(struct iphdr *)buf;
        int iplen = nread;
        // if (nread < iplen) break; ??		

		int id = search_user_info_by_addr(hdr.daddr);
		if (id == -1)
		{
			printf("Warning: can not locate ip packet dest\n");
			continue;
		}
		int client_fd = users[id].fd;

		message reply;
		reply.type = MSG_DATA_RSB;
		reply.length = iplen + sizeof(reply);

		int ret;
		if ((ret = write_all(client_fd, &reply, sizeof(reply))) < (int)sizeof(reply))
		{
			printf("Error: send reply header failed\n");
			return;
		}
		if ((ret = write_all(client_fd, buf, iplen)) < 0)
		{
			printf("Error: send reply data failed\n");
			return;
		}

		printf("Package tunnel - length: %u\n", iplen + sizeof(message));

        memmove(buf, buf + iplen, nread - iplen);
        nread -= iplen;
    }
}

void process_client(int client_fd, int tun_fd, int epoll_fd)
{
    printf("process_client\n");

    char* buf = bufs[client_fd].buf;
    int& nread = bufs[client_fd].used;
    
    ssize_t size;
    // read data from client
    size = read(client_fd, buf+nread, BUF_SIZE-nread);

    if (size <= 0)
	{
        if (errno == ECONNRESET || size == 0) printf("Connection ternimated\n");
        else printf("Error: error at read client:%s\n", strerror(errno));
        free_client_fd(client_fd, epoll_fd);
        return;
    }
    printf("[debug] size = %d\n", size);
    nread += size;

    // process incoming data from client
    while(true)
	{
        if (nread < sizeof(message)) break;

        message msg = *(message *)buf;
        int msglen = msg.length;
        if (nread < msg.length) break;

        // process data
        if (msg.type == MSG_IP_REQ)
		{
            // allocate ip address
            int id;
            if ((id = allocate_ip_addr(client_fd)) < 0)
			{
                printf("Error: ip address pool is full\n");
                return;
            }

            // send ip response
            char bufreply[BUF_SIZE];
            sprintf(bufreply, "%s 0.0.0.0 %s %s %s", users[id].addr, DNS1, DNS2, DNS3);
            int buflen = strlen(bufreply);

            message reply;
            reply.type = MSG_IP_RSB;
            reply.length = buflen + sizeof(message);
            if (write_all(client_fd, &reply, sizeof(reply)) < (int)sizeof(reply))
			{
                printf("Error: send reply header failed\n");
                return;
            }
            if (write_all(client_fd, bufreply, buflen) < buflen)
			{
                printf("Error: send reply data failed\n");
                return;
            }
        }
        else if (msg.type == MSG_DATA_REQ)
		{
            // send data to tun
            int datalen = msglen - sizeof(message);
            if (write_all(tun_fd, buf+sizeof(message), datalen) < datalen)
			{
                printf("Error: send data to tun failed\n");
                return;
            }
        }
        else if (msg.type == MSG_HEARTBEAT)
		{
            printf("client keep-alive\n");

            int id = search_user_info_by_fd(client_fd);
            users[id].last_heartbeat_recved_secs = time(0);
        }
        else
		{
            printf("Warning: unknown type of data from client fd %d\n", client_fd);
        }

        printf("Package client - length: %d\n", msglen);
        memmove(buf, buf + msglen, nread - msglen);
        nread -= msglen;
    }
}

void process_heartbeat(int epoll_fd)
{
    for(int i = 0; i < CLIENT_NUM; i ++)
        if (users[i].fd >= 0 && !users[i].status)
		{
            time_t now = time(0);
            if (now - users[i].last_heartbeat_sent_secs > HEARTBEAT_INTERVAL)
			{
                message heartbeat;
                heartbeat.type = MSG_HEARTBEAT;
                heartbeat.length = sizeof(message);
                if (write_all(users[i].fd, &heartbeat, sizeof(message)) != sizeof(message))
				{
                    printf("Error: fail to send heartbeat packet\n");
                }
                users[i].last_heartbeat_sent_secs = now;
            }
            if (now - users[i].last_heartbeat_recved_secs > HEARTBEAT_TIMEOUT)
			{
                free_client_fd(users[i].fd, epoll_fd);
            }
        }
}

int main()
{	
	if (MainThread() == -1) goto clean;
	for (;;)
	{
		// wait for all epoll events
		int num_events;
		struct epoll_event events[EPOLL_MAX_EVENTS];
		if ((num_events = epoll_wait(epoll_fd, events, EPOLL_MAX_EVENTS, 1)) < 0)
		{
			printf("Error: failed to wait for epoll events\n");
			goto clean;
		}

		// handle each event
		int i;
        for (i = 0; i < num_events; i++)
		{
            // event from server socket
            if (events[i].data.fd == server_fd) process_server(server_fd, epoll_fd);
            // event from tun
            else if (events[i].data.fd == tun_fd) process_tun(tun_fd);
            // event from client socket
            else process_client(events[i].data.fd, tun_fd, epoll_fd);
        }

        process_heartbeat(epoll_fd);

        fflush(stdout);
	}

clean:
    if (server_fd != -1) close(server_fd);
	if (tun_fd != -1) close(tun_fd);
    for(int i = 0; i < CLIENT_NUM; i ++)
        if (users[i].fd >= 0)
            close(users[i].fd);
	return 0;
}

#ifndef SERVERUTIL
#define SERVERUTIL

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <bits/ioctls.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <errno.h>

#define SERV_PORT           6666
#define MAX_FDS             2048
#define ADDRESS_PREFIX      "13.8.0"
#define NETMASK             "255.255.255.0"
#define DNS1                "166.111.8.28"
#define DNS2                "166.111.8.29"
#define DNS3                "8.8.8.8"
#define CLIENT_NUM          256
#define EPOLL_MAX_EVENTS    2048
#define BUF_SIZE            65536
#define EPOLL_EVENTS        (EPOLLIN|EPOLLERR|EPOLLHUP)
#define HEARTBEAT_INTERVAL  20
#define HEARTBEAT_TIMEOUT   60

typedef struct User_Info_Table 		//客户信息表
{
	char addr[32];		            //IP地址
	int status;			            //标志位
	int fd; 						//套接字描述符

	time_t last_heartbeat_sent_secs;
    time_t last_heartbeat_recved_secs;
	
	struct in_addr v4addr;			//服务器给客户端分配的IPV4地址
	struct in6_addr v6addr;			//客户端的IPV6地址
	User_Info_Table() {
		fd = -1;
	}
}User_Info_Table;

typedef struct buffer_entry {
    char buf[BUF_SIZE];
    int used;
} buffer_entry;

// message between server and client
typedef struct message {
    int length;
    char type;
} __attribute__((packed)) message;

#endif

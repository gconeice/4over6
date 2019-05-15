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
#define CLIENT_NUM          256
#define EPOLL_MAX_EVENTS    2048
#define BUF_SIZE            65536
#define EPOLL_EVENTS        (EPOLLIN|EPOLLERR|EPOLLHUP)

typedef struct User_Info_Table 		//客户信息表
{
	char addr[32];		            //IP地址
	int status;			            //标志位
	int fd; 						//套接字描述符
	int count;						//标志位
	unsigned long int secs;			//上次收到keeplive时间
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

#endif

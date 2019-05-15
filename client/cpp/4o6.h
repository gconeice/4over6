#ifndef IP4O6
#define IP4O6

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <errno.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_MSG_SIZE 4096

struct Msg
{
	int length;      // 长度
	char type;       // 类型
	char data[MAX_MSG_SIZE]; // 数据段
};

#define MSG_IP_REQ      100
#define MSG_IP_RSB      101
#define MSG_DATA_REQ    102
#define MSG_DATA_RSB    103
#define MSG_HEARTBEAT   104

#endif

#ifndef APP_PROTOCOL_H
#define APP_PROTOCOL_H

#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <android/log.h>

#define HEARTBEAT_TIMEOUT_SECS 60
#define HEARTBEAT_INTERVAL_SECS 20

#define IPC_COMMAND_EXIT 1
#define IPC_COMMAND_FETCH_CONFIG 2 // 获取IP配置信息
#define IPC_COMMAND_SET_TUN 3
#define IPC_COMMAND_FETCH_STATE 4

#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, "vpn_backend", __VA_ARGS__);
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "vpn_backend", __VA_ARGS__);
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, "vpn_backend", __VA_ARGS__);
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, "vpn_backend", __VA_ARGS__);
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "vpn_backend", __VA_ARGS__);

#define ERROR_CHECK(code, exit_label) \
{ \
    int __ret = (code); \
    if (__ret < 0) { \
        LOGE("error at %s:%d error=%s", __FILE__, __LINE__, strerror(errno)); \
        goto exit_label; \
    } \
}
#define ASSERT(cond, exit_label) \
{ \
    if (!(cond)) { \
        LOGE("error at %s:%d [%s]", __FILE__, __LINE__, #cond);\
        goto exit_label; \
    } \
}

int protocol_init(int _socketFd, int _commandReadFd, int _responseWriteFd);

int get_tun_fd();

int handle_tunnel();

int handle_socket();

int handle_command();

int handle_heartbeat();

const char* READ_IP(const char* ptr, uint8_t ip[4]);

#endif //APP_PROTOCOL_H

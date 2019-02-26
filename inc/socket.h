
#ifndef __SOCKET__
#define __SOCKET__   1

#include <sys/socket.h>
#include <arpa/inet.h>

#define LISTEN_MAX 1024

// 获取并设置 服务器套接字
int setServerSocket(char *ip, int port);

// 设置 文件描述符 为 非阻塞模式
void setNonblockingMode(int fd);

// 关闭 连接
void _close(int _fd);

// 发送 数据
ssize_t _send(int t_fd, char *buffer, ssize_t buf_len);

#endif
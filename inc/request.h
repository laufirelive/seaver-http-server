#ifndef __REQUEST__
#define __REQUEST__     1

#define CR  '\r'
#define LF  '\n'

#define REQUEST_BUF_LEN 8196

#include "../inc/list.h"

// 请求行
struct http_request_line {
    char *method;   // 方法
    char *url;      // URL
    char *url_para; // URL 后附带的参数
    char *version;  // 版本
};

// 请求体
struct http_request_body {

};

// 请求结构体
struct http_request {
    // fd 和 管理epoll
    int fd;
    int ep_fd;

    // 报文
    char message[REQUEST_BUF_LEN + 1];

    // 请求行 与 请求头
    struct http_request_line          request_line;
    struct __http_request_head       *request_head;
    struct http_request_body          request_body;
};

struct http_request_method {
    char *key;
    int (*val)(struct http_request *);
};

struct http_request *request_init(int _fd);
void request_handle(struct http_request *header);
void request_del(struct http_request *header);

#endif
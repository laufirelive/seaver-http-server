#ifndef __REQUEST__
#define __REQUEST__     1

#define CR  '\r'
#define LF  '\n'

#define REQUEST_BUF_LEN 8000

#include "../inc/list.h"
#include "../inc/timer.h"
// 请求行
struct http_request_line {
    char *method;   // 方法
    char *url;      // URL
    char *url_para; // URL 后附带的参数
    char *version;  // 版本
};

struct http_request_head_data {
    char *field;
    char *value;
};

// 请求头部
typedef struct __http_request_head {
    struct http_request_head_data          data;
    list_head head;
} http_request_head;

// 请求体
struct http_request_body {
    char *s;
};

// 请求结构体
struct http_request {
    // fd 和 管理epoll
    int fd;
    int ep_fd;
    timer *in_slot;

    // 报文
    char message[REQUEST_BUF_LEN + 1];


    // 请求行 与 请求头
    struct http_request_line          request_line;
    struct __http_request_head        request_head;
    struct http_request_body          request_body;
};

struct http_request_method {
    char *key;
    int (*val)(struct http_request *);
};


struct http_request *request_init(int _fd);
void *request_handle(void *args);
void request_del(struct http_request *header);
void *request_shutdown(void *args);

#endif
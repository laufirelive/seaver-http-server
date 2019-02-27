#ifndef __LIST__
#define __LIST__    1

struct http_request_head_data {
    char *field;
    char *value;
};

// 请求头部
typedef struct __http_request_head {
    struct http_request_head_data          data;
    struct __http_request_head            *next;
} http_request_head;

// 请求链表

http_request_head *request_head_initStack();

int request_head_isEmpty(http_request_head *S);

int request_head_push(http_request_head *S, char *f, char *v);

int request_head_pop(http_request_head *S, struct http_request_head_data *backup);

#endif
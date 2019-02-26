#ifndef __LIST__
#define __LIST__    1

#include "request.h"

// 请求链表

struct http_request_head *
    http_request_head_init();

int 
    http_request_head_add(struct http_request_head *L, char *fields, char *val);

int 
    http_request_head_del(struct http_request_head *target);

int 
    http_request_head_mod(struct http_request_head *L, char *val);

struct http_request_head *
    http_request_head_get(struct http_request_head *L, char *fields);

struct http_request_head *
    http_request_head_get_first(struct http_request_head *L);

void
    http_request_head_destroy(struct http_request_head *L);

#endif
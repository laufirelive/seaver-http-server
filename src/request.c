#include <stdlib.h>
#include <unistd.h>
#include "../inc/request.h"
#include "../inc/response.h"
#include "../inc/epoll.h"
#include "../inc/debug.h"
#include "../inc/list.h"


struct http_request_method Method[] = {
    {"GET", request_GET},
    {"POST", request_POST},
    {"HEAD", request_HEAD},
};

struct http_request *request_init(int _fd)
{
    struct http_request *header;
    header = (struct http_request *)malloc(sizeof(struct http_request));
    if (!header)
    {
        log_err("malloc(), errno: %d\t%s", errno, strerror(errno));
        return NULL;
    }
    
    memset(header, 0, sizeof(header));
    header->fd = _fd;

    return header;
}

void request_handle(struct http_request *header)
{
    int _fd = header->fd;
    int epoll_fd = header->ep_fd;
    char *message_buf = header->message;

    int str_len;
    int recv_now = 0;
    int recv_len = REQUEST_BUF_LEN - recv_now;

    // 接收 请求报文
    do
    {
        str_len = read(_fd, message_buf + recv_now, recv_len);
        if (str_len < 0)
        {
            if (errno == EAGAIN)
                break;
                
            continue;
        }
        else if (str_len == 0)
            break;
        
        recv_now += str_len;
        recv_len = REQUEST_BUF_LEN - recv_now;

    } while (!str_len && recv_len > 0);

    recv_len = strlen(message_buf);
    
    log("recv_len %d", recv_len);
    puts(message_buf);


    if (recv_len < 4)
    {
        log_err("Client Message is too short.");
        goto ERROR;
    }
    
/*     // 最后是否 \r\n\r\n 结尾
    if (
        message_buf[recv_len - 4] != CR ||
        message_buf[recv_len - 2] != CR ||
        message_buf[recv_len - 3] != LF ||
        message_buf[recv_len - 1] != LF
    )
    {
        log_err("Client Message Format is wrong. %d", _fd);
        goto ERROR;
    }
 */


    // HTTP 请求报文分割
    header->request_line.method = strtok(message_buf, " /");
    header->request_line.url = strtok(NULL, " ");
    header->request_line.version = strtok(NULL, " \r\n");
    // header->request_head = http_request_head_init();

    // HTTP 请求报文 请求首部加入 链表
    char *fields, *value;
    while (fields = strtok(NULL, ": ") + 1)
    {
        if ((value = strtok(NULL, "\r\n")) == NULL)
            break;
        value[strlen(value)] = '\0';

        // http_request_head_add(header->request_head, fields, value);
    }


    // 解析 HTTP 请求行
    int loop_len = sizeof(Method) / sizeof(*Method);
    for (int i = 0; i < loop_len; i++)
    {
        if (!strcmp(header->request_line.method, Method[i].key))
        {
            Method[i].val(header);
            break;
        }
    }    

ERROR:
    // 释放 回调 请求结构体
    // 其 申请在 server.c 
    // request_del(header);
    log("Connection Disconnected : %d", _fd);

    return ;
}

void request_del(struct http_request *header)
{
    // http_request_head_destroy(header->request_head);
    free(header);
}

int request_GET(struct http_request *header)
{
    log("GET Method");
    response_handle_static(header);    
    return 0;
}

int request_POST(struct http_request *header)
{
    log("POST Method");
    return -1;
}

int request_HEAD(struct http_request *header)
{
    log("HEAD Method");
    return -1;
}
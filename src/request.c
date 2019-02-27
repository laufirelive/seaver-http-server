#include <stdlib.h>
#include <unistd.h>
#include "../inc/request.h"
#include "../inc/response.h"
#include "../inc/epoll.h"
#include "../inc/debug.h"
#include "../inc/list.h"

char *request_header_parse(char *rest);
char *request_get_urlpara(char *url);
void request_backward_space(char *val, int len);
void request_forward_space(char *val);

int request_GET(struct http_request *header);
int request_POST(struct http_request *header);
int request_HEAD(struct http_request *header);

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

int request_legal_check(char *message)
{
    int islegal;
    int len;
    char *target = "\r\n\r\n";
    
    islegal = 0;
    len = 0;

    char *p, *q;
    while (*message)
    {
        p = message; q = target;
        while (*p == *q && *q)
            p++, q++;

        if (*q == '\0')
            islegal = 1;

        len++;
        message++;
    }

    if (islegal)
        return len;
    else
        return 0;
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

    recv_len = request_legal_check(message_buf);

#if (DBG)
    // 打印报文
    puts(message_buf);
    log("recv_len %d", recv_len);
#endif

    if (recv_len == 0)
    {
        log_err("Client Request Format Error.");
    }

    if (recv_len < 4)
    {
        log_err("Client Request is too short.");
        goto ERROR;
    }

    // HTTP 请求报文分割
    header->request_line.method = strtok(message_buf, " /");
    header->request_line.url = strtok(NULL, " ");
    header->request_line.version = strtok(NULL, "\n");
    header->request_line.url_para = request_get_urlpara(header->request_line.url);
#if (DBG)
    log("Method : %s\n", header->request_line.method);
    log("Url : %s\n", header->request_line.url);
    log("Url_Para : %s\n", header->request_line.url_para);
    log("Version : %s\n", header->request_line.version);
#endif

    char *rest;
    rest = strtok(NULL, "\0");

    // 请求头
    rest = request_header_parse(rest);
    // 请求体
    if (rest && *rest)
    {
#if (DBG)
        printf("\nThe Rest : \n%s\n", rest);
#endif

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

    request_del(header);
    log("Connection Disconnected : %d", _fd);

    return ;
}


char *request_header_parse(char *rest)
{
    char *key, *val;
    char *p, *q;
    int key_mode, val_mode;

    key = rest;
    key_mode = 1;
    val_mode = 0;
    while (*rest)
    {
        switch (*rest)
        {
          case ':' :
            if (key_mode)
            {
                *rest = '\0';
                val = rest + 1;
                key_mode = 0; val_mode = 1;     // 第一个 冒号 为准
            }
            break;
          case '\n' :
            *rest = '\0';
            if (*key != '\0')
            {
                int i;
                request_backward_space(val, strlen(key));
                request_forward_space(val);
                // TODO : add to list
#if (DBG)
                log("%s:%s\n", key, val);
#endif
                // ----------------------
                if (val_mode)
                {
                    key = rest + 1;
                    key_mode = 1; val_mode = 0;
                }
            }
            else
                return rest + 1;

            break;
          case '\r' :
            *rest = '\0';
            break;
          default:
            break;
        }
        rest++;
    }

    return NULL;
}

void request_backward_space(char *val, int len)
{
    int i = 2;
    while (len)
    {
        if (*(val - i) == ' ')
            *(val - i) = '\0';
        else
            break;
        i++;
        len--;
    }
}

void request_forward_space(char *val)
{
    int space_count = 0;
    int i = 0;
    while (val[i])
    {
        if (val[i] == ' ')
            space_count++;
        else
            break;
        i++;
    }
    i = 0;
    while (val[i])
    {
        val[i] = val[i + space_count];
        i++;
    }
}

char *request_get_urlpara(char *url)
{
    while (*url)
    {
        if (*url == '?')
        {
            *url = '\0';
            return url + 1;
        }
        url++;
    }

    return NULL;
}

void request_del(struct http_request *header)
{
    // http_request_head_destroy(header->request_head);
    memset(header, 0, sizeof(header));
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
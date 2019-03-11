#include <stdlib.h>
#include <unistd.h>
#include "../inc/request.h"
#include "../inc/response.h"
#include "../inc/epoll.h"
#include "../inc/socket.h"
#include "../inc/debug.h"

char *request_header_parse(char *rest, http_request_head *S);
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
    memset(header->message, 0, REQUEST_BUF_LEN);
    
    header->fd = _fd;
    LIST_HEAD_INIT(&header->request_head.head);

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

void *request_handle(void *args)
{
    struct http_request *header = (struct http_request *)args;
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
    printf("\n");
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

    // 线程安全 strtok， 巨坑
    char *save;

    // HTTP 请求报文分割
    header->request_line.method = __strtok_r(message_buf, " /", &save);
    header->request_line.url = __strtok_r(NULL, " ", &save);
    header->request_line.version = __strtok_r(NULL, "\n", &save);
    header->request_line.url_para = request_get_urlpara(header->request_line.url);

#if (DBG)
    log("Method : %s", header->request_line.method);
    log("Url : %s", header->request_line.url);
    log("Url_Para : %s", header->request_line.url_para);
    log("Version : %s", header->request_line.version);
#endif

    char *rest;
    rest = __strtok_r(NULL, "\0", &save);
    // 请求头
    rest = request_header_parse(rest, &header->request_head);

    // 请求体
    if (rest && *rest)
    {

#if (DBG)
        printf("\nThe Body : \n%s\n", rest);
#endif
    }

#if (DBG)
    printf("\nThe Request Head : \n");
#endif

    struct http_request_head_data temp_data;
    http_request_head *r_head;
    list_head *index;
    list_head *L = &header->request_head.head;
    list_for_each(index, L)
    {
        r_head = list_entry(index, http_request_head, head);
        
        temp_data.field = r_head->data.field;
        temp_data.value = r_head->data.value;
        
        list_del(&r_head->head);
        free(r_head);
#if (DBG)
        printf("%s:%s\n", temp_data.field, temp_data.value);
#endif
    }
    putchar('\n');

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

    request_del(header);
    epoll_del(epoll_fd, _fd);
    _close(_fd);

#if (DBG)
    log("Connection Disconnected : %d", _fd);
#endif
    return NULL;
}

char *request_header_parse(char *rest, http_request_head *S)
{
    char *key, *val;
    char *p, *q;
    int key_mode, val_mode;
    http_request_head *temp;

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
          case LF :
            *rest = '\0';
            if (*key != '\0')
            {
                int i;
                request_backward_space(val, strlen(key));
                request_forward_space(val);
                
                // 加入链表
                temp = (http_request_head *)malloc(sizeof(http_request_head));
                temp->data.field = key;
                temp->data.value = val;
                list_add_tail(&S->head, &temp->head);

                if (val_mode)
                {
                    key = rest + 1;
                    key_mode = 1; val_mode = 0;
                }
            }
            else
                return rest + 1;

            break;
          case CR :
            *rest = '\0';
            break;
          default:
            break;
        }
        rest++;
    }

    return NULL;
}

inline void request_backward_space(char *val, int len)
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

inline void request_forward_space(char *val)
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

// Get 参数处理
inline char *request_get_urlpara(char *url)
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

// 删除连接
inline void request_del(struct http_request *header)
{
    memset(header->message, 0, REQUEST_BUF_LEN);
    free(header);
}

inline int request_GET(struct http_request *header)
{
#if (DBG)
    log("GET Method");
#endif
    response_handle_static(header);    
    return 0;
}

inline int request_POST(struct http_request *header)
{
#if (DBG)
    log("POST Method");
#endif
    return -1;
}

inline int request_HEAD(struct http_request *header)
{
#if (DBG)
    log("HEAD Method");
#endif
    return -1;
}
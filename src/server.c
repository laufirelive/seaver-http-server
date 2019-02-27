#include <stdio.h>
#include <stdlib.h>
#include "../inc/conf.h"
#include "../inc/socket.h"
#include "../inc/epoll.h"
#include "../inc/response.h"
#include "../inc/debug.h"
#include "../inc/cJSON.h"

static void connection_handle(int serv_fd, int ep_fd);
static void connection_wait(int serv_fd, int epoll_fd);

struct conf Configuration;

int conf_read()
{
    int i, ch;
    char error_message[30] = {0}; 

    int conf_len = 0;
    FILE *conf_file;

    char *docment;
    cJSON *root;
    cJSON *server;
    cJSON *bind;
    cJSON *http;
    cJSON *epoll;
    cJSON *location;
    cJSON *default_page;

    conf_file = fopen(CONF_FILE, "r");
    if (!conf_file)
    {
        log_err("fopen(\"" CONF_FILE "\"), errno: %d\t%s", errno, strerror(errno));
         return -1;
    }
    
    fseek(conf_file, 0, SEEK_END);
    conf_len = ftell(conf_file);
    if (conf_len == 0)
    {
        log_err("Configuration file [ " CONF_FILE " ] is empty.");
        fclose(conf_file);
        return -1;
    }
    fseek(conf_file, 0, 0);
    
    docment = (char *)malloc(sizeof(char) * conf_len);
    if (!docment)
    {
        log_err("malloc(), errno: %d\t%s", errno, strerror(errno));
        return -1;
    }
    
    i = 0;
    while (!feof(conf_file))
    {
        ch = fgetc(conf_file);
        switch (ch)
        {
            case EOF  :
            case ' '  :
            case '\n' :
            case '\r' :
            case '\t' :
                continue;
        }
        docment[i++] = ch;
    }
    docment[i] = '\0';
    fclose(conf_file);

    root = cJSON_Parse(docment);
    if (!root)
    {
        strncpy(error_message, "format error", sizeof(error_message)); 
        goto ERROR;
    }
    
    server = cJSON_GetObjectItem(root, "server");
    if (!server)
    {
        strncpy(error_message, "must have [server] block", sizeof(error_message)); 
        goto ERROR;
    }

    bind = cJSON_GetObjectItem(server, "bind");
    if (!bind)
    {
        strncpy(error_message, "must have [bind] block", sizeof(error_message)); 
        goto ERROR;
    }

    // bind 块解析
    if (cJSON_GetObjectItem(bind, "ipv4") && cJSON_GetObjectItem(bind, "port"))
    {
        strncpy(Configuration.ipv4, cJSON_GetObjectItem(bind, "ipv4")->valuestring, CONF_STR_LEN);
        Configuration.port = cJSON_GetObjectItem(bind, "port")->valueint;
    }
    else
    {
        strncpy(error_message, "must have [ipv4] and [port] block", sizeof(error_message)); 
        goto ERROR;
    }

    // http 块解析
    http = cJSON_GetObjectItem(server, "http");
    if (http)
    {
        cJSON *temp;

        temp = cJSON_GetObjectItem(http, "keep-alive");
        if (temp)   
            Configuration.keep_alive = temp->valueint;
        else
            Configuration.keep_alive = 0;

        temp = cJSON_GetObjectItem(http, "default-page");
        if (temp)
            strncpy(Configuration.default_page, temp->valuestring, CONF_LOC_LEN);
        else
            strncpy(Configuration.default_page, DEFAULT_PAGE, CONF_LOC_LEN);

        // and on
    }

    location = cJSON_GetObjectItem(server, "location");
    if (!location)
    {
        strncpy(error_message, "must have [location] block", sizeof(error_message)); 
        goto ERROR;
    }
    strncpy(Configuration.loc, location->valuestring, CONF_LOC_LEN);

    // epoll 块解析
    epoll = cJSON_GetObjectItem(server, "epoll");
    if (epoll && cJSON_GetObjectItem(epoll, "events_max"))
    {
        Configuration.max_events = cJSON_GetObjectItem(epoll, "events_max")->valueint;
        epoll_set_maxevents(Configuration.max_events);
    }
    // and on

    goto DONE;
    
// 错误出口
ERROR :
    log_err("[" CONF_FILE "] %s.", error_message);

    cJSON_Delete(root);
    free(docment);

    return -1;

// 成功出口
DONE :
    log("IP : %s\t PORT : %d", Configuration.ipv4, Configuration.port);
    log("keep-alive : %d\t Location : %s", Configuration.keep_alive, Configuration.loc);
    
    cJSON_Delete(root);
    free(docment);

    return 0;
}

void conf_init()
{
    memset(&Configuration, 0, sizeof(Configuration));
}

int main(int argc, char *argv[])
{
    int serv_fd;    // 服务器套接字
    int epoll_fd;   // epoll 描述符
    
    struct epoll_event event;
    struct http_request *header;

    conf_init();
    if (conf_read() == -1)
    {
        log_err("Configuration deadly error.");
        return -1;
    }
    
    serv_fd = setServerSocket(Configuration.ipv4, Configuration.port); 
    setNonblockingMode(serv_fd);

    // epoll 初始化
    epoll_fd = epoll_init(0);
    memset(&event, 0, sizeof(event));
    
    // epoll 注册
    header = request_init(serv_fd);
    header->ep_fd = epoll_fd;
    event.data.ptr = (void *)header;
    event.events = EPOLLIN | EPOLLET;
    epoll_sign(epoll_fd, serv_fd, &event);    
    log("epoll Initialization Complete.");

    // TODO ： 线程池
    
    // 等待连接
    printf("\n\nServer Is Working... \n\n");
    connection_wait(serv_fd, epoll_fd); 

    return 0;
}

void connection_wait(int serv_fd, int epoll_fd)
{
    int ep_count;   // epoll 活跃连接数

    int i, _fd;
    int events_max = epoll_get_maxevents();
    struct http_request *header;

 while (1)
 {
    // 服务器开始处理连接
    ep_count = epoll_wait(epoll_fd, Events, events_max, -1);
    if (ep_count == -1)
    {
        log_err("epoll_wait(), errno: %d\t%s", errno, strerror(errno));
        break;
    }

    // 处理 活跃连接
    for (i = 0; i < ep_count; i++)
    {
        log("epoll Alive: %d event(s)", ep_count);
        header = (struct http_request *)Events[i].data.ptr;
        _fd = header->fd;
        
        // 新连接请求
        if (_fd == serv_fd)
        {
            // 处理 连接请求
            log("New Connection");
            connection_handle(_fd, epoll_fd);
            log("Wait...");
        }
        else // TODO : 加入线程池队列
        {
            // 处理请求
            request_handle(header);

            // TODO : 长连接管理
            memset(&Events[i], 0, sizeof(Events[i]));
            epoll_del(epoll_fd, _fd);
            _close(_fd);

            log("Wait...");
        }
    }

 }
    return ;
}

void connection_handle(int serv_fd, int _ep_fd)
{
    int clnt_fd;    // 客户端套接字
    socklen_t clnt_len;
    struct epoll_event event;
    struct sockaddr_in clnt_addr; 
    struct http_request *header;

    clnt_len = sizeof(clnt_addr);
    
    while (1)
    {
        memset(&clnt_addr, 0, clnt_len);
        clnt_fd = accept(serv_fd, (struct sockaddr *)&clnt_addr, &clnt_len);
        if (clnt_fd < 0)
        {
            if (errno != EAGAIN && errno != EWOULDBLOCK)
                log_err("accept(), errno: %d\t%s", errno, strerror(errno));
            
            return ;
        }
        // 设置为 非阻塞 并 配置 header
        setNonblockingMode(clnt_fd);
        header = request_init(clnt_fd);
        header->ep_fd = _ep_fd;
        
        // epoll 注册信息
        event.data.ptr = (void *)header;
        event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;

        // 注册
        epoll_sign(_ep_fd, clnt_fd, &event);
        log("Connection Accept : %d", clnt_fd);
    }

    return ;
}
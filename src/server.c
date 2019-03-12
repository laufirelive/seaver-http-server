#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "../inc/conf.h"
#include "../inc/socket.h"
#include "../inc/epoll.h"
#include "../inc/response.h"
#include "../inc/debug.h"
#include "../inc/cJSON.h"
#include "../inc/timer.h"

#if (TPOOL)
#include "../inc/threadpool.h"
static threadpool_t *pool;
#endif

static void connection_handle(int serv_fd, int ep_fd);
static void connection_wait(int serv_fd, int epoll_fd);

struct conf Configuration;
timer_manager *Timer;

int conf_load()
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
#if (TPOOL)
    cJSON *pool_size;
#endif
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

        temp = cJSON_GetObjectItem(http, "keep-alive-timeout");
        if (temp) 
            Configuration.keep_alive_timeout = temp->valueint;
        else
            Configuration.keep_alive_timeout = DEFAULT_TIMEOUT;
            
        temp = cJSON_GetObjectItem(http, "default-page");
        if (temp)
            strncpy(Configuration.default_page, temp->valuestring, CONF_LOC_LEN);
        else
            strncpy(Configuration.default_page, DEFAULT_PAGE, CONF_LOC_LEN);

        temp = cJSON_GetObjectItem(http, "error-page");
        if (temp)
            strncpy(Configuration.error_page, temp->valuestring, CONF_LOC_LEN);
        else
            memset(Configuration.error_page, 0, CONF_LOC_LEN);

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
#if (TPOOL)
    pool_size = cJSON_GetObjectItem(server, "pool-size");
    if (!pool_size)
    {
        strncpy(error_message, "must have [pool-size] block", sizeof(error_message)); 
        goto ERROR;
    }
    Configuration.pool_size = pool_size->valueint;
    if (Configuration.pool_size <= 0)
    {
        strncpy(error_message, "[pool-size] must >= 0", sizeof(error_message)); 
        goto ERROR;
    }
#endif

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

int no_signal()
{
    struct sigaction sign_sa;
    memset(&sign_sa, 0, sizeof(sign_sa));
    sign_sa.sa_handler = SIG_IGN;
    sign_sa.sa_flags = 0;
    if (sigaction(SIGPIPE, &sign_sa, NULL))
        return 0;
    else
        return 1;
}

int main(int argc, char *argv[])
{
    int serv_fd;    // 服务器套接字
    int epoll_fd;   // epoll 描述符
    
    struct epoll_event event;
    struct http_request *header;

    // 配置文件初始化与读取
    conf_init();
    if (conf_load() == -1)
    {
        log_err("Configuration deadly error.");
        return -1;
    }

    if (!no_signal())
    {
        log_err("sigaction(), errno: %d\t%s", errno, strerror(errno));
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

    if (Configuration.keep_alive)
    {
        Timer = timer_init(Configuration.keep_alive_timeout, 1);
        if (!Timer)
        {
            log_err("timer_init(), errno: %d\t%s", errno, strerror(errno));
            return -1;
        }
        log("Timer Initialization Complete.");
    }


#if (TPOOL)
    // 线程池
    pool = threadpool_create(Configuration.pool_size);
    if (!pool)
    {
        log_err("threadpool_create(), errno: %d\t%s", errno, strerror(errno));
        return -1;
    }
    log("Threadpool Initialization Complete.");
#endif

    // 等待连接
    printf("\n\nServer Is Working... \n\n");
    connection_wait(serv_fd, epoll_fd); 

#if (TPOOL)
    threadpool_destroy(pool);
#endif

    if (Configuration.keep_alive)
        timer_destory(Timer);

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

#if (DBG)
    if (ep_count > 0)
        log("epoll Alive: %d event(s)", ep_count);
#endif

    // 处理 活跃连接
    for (i = 0; i < ep_count; i++)
    {
        header = (struct http_request *)Events[i].data.ptr;
        _fd = header->fd;
        
        // 新连接请求
        if (_fd == serv_fd)
        {
            // 处理 连接请求
            connection_handle(_fd, epoll_fd);
        }
        else 
        {
#if (TPOOL)
            // 加入线程池队列
            threadpool_sign(pool, request_handle, header);
#else
            request_handle(header);
#endif
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
#if (DBG)
        log("Connection Accept : %d", clnt_fd);
#endif
    }

    return ;
}
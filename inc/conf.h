#ifndef __CONF__
#define __CONF__    1

#define CONF_STR_LEN    16
#define CONF_LOC_LEN    256
#define DEFAULT_PAGE    "index.html"
#define DEFAULT_TIMEOUT 60
#include "../inc/timer.h"

struct conf{
    // bind
    char ipv4[CONF_STR_LEN];
    int port;
    
    // http
    int keep_alive;
    int keep_alive_timeout;
    char default_page[CONF_LOC_LEN];
    char error_page[CONF_LOC_LEN];

    // epoll
    int max_events;

    // location
    char loc[CONF_LOC_LEN];
#if (TPOOL)
    int pool_size;
#endif
};

extern struct conf Configuration;
extern timer_manager *Timer;
#endif
#ifndef __CONF__
#define __CONF__    1

#define CONF_STR_LEN    16
#define CONF_LOC_LEN    256
#define CONF_FILE       "seaver.json"
#define DEFAULT_PAGE    "default.html"

struct conf{
    // bind
    char ipv4[CONF_STR_LEN];
    int port;
    
    // http
    int keep_alive;
    char default_page[CONF_LOC_LEN];
    // epoll
    int max_events;

    // location
    char loc[CONF_LOC_LEN];

};

extern struct conf Configuration;

#endif
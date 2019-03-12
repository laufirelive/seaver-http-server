#include <stdlib.h>
#include "../inc/debug.h"
#include "../inc/epoll.h"

struct epoll_event *Events;

static int EVENTS_MAX = 2048;

int epoll_init(int flag)
{
    int fd;
    if ((fd = epoll_create1(0)) == -1)
    {
        log_err("epoll_create1(), errno: %d\t%s", errno, strerror(errno));
        exit(-1);
    }
    
    Events = (struct epoll_event *)malloc(sizeof(struct epoll_event) * EVENTS_MAX);
    if (!Events)
    {
        log_err("malloc(), errno: %d\t%s", errno, strerror(errno));
        exit(-1);
    }
    memset(Events, 0, sizeof(struct epoll_event) * EVENTS_MAX);

    return fd;
}

int epoll_sign(int epfd, int fd, struct epoll_event *event)
{
    int result;
    result = epoll_ctl(epfd, EPOLL_CTL_ADD, fd, event);
    if (result == -1)
    {
        log_err("epoll_ctl(), errno: %d\t%s", errno, strerror(errno));
        return -1;
    }

    return result;
}

void epoll_reset_oneshot(int epfd, int fd, void *header)
{
    struct epoll_event event;

    event.data.ptr = header;
    event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;

    epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &event); 
}

int epoll_del(int epfd, int fd)
{
    int result;
    result = epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
    if (result == -1)
    {
        log_err("epoll_ctl(), errno: %d\t%s", errno, strerror(errno));
        return -1;
    }
    return result;
}

void epoll_set_maxevents(int maxevents)
{
    if (maxevents < 10)
        maxevents = 10;
    
    EVENTS_MAX = maxevents;
}

int epoll_get_maxevents()
{
    return EVENTS_MAX;
}
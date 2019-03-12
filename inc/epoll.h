#ifndef __EPOLL__ 
#define __EPOLL__   1

#include <sys/epoll.h>

extern struct epoll_event *Events;

int epoll_init(int flag);
int epoll_sign(int epfd, int fd, struct epoll_event *event);
void epoll_reset_oneshot(int epfd, int fd, void *header);
int epoll_del(int epfd, int fd);
void epoll_set_maxevents(int maxevents);
int epoll_get_maxevents();

#endif
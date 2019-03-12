#ifndef __TIMER__
#define __TIMER__   1

#include <pthread.h>
#include "list.h"

#define MAXSLOT 128

typedef struct __timer {
    int rotation;    // 当前圈数
    int slot;    // 槽位
    
    // 超时回调
    void *(*task)(void *);
    void *args;

    list_head list;
} timer;

typedef struct __timer_manager {
    int slot_now;       // 当前槽
    int slot_num;       // 总槽数
    int slot_interval;  // 间隔

    pthread_cond_t cond;
    pthread_mutex_t mutex;

    list_head slot[MAXSLOT];    // timer 链表
} timer_manager;

// 定时器初始化
timer_manager *
    timer_init(int _slot_num, int _slot_interval);

// 添加定时器
timer *
    timer_add(timer_manager *T, int timeout, void *(_task)(void *), void *_arg);

// 重置定时器
void
    timer_reinsert(timer_manager *T, timer *t, int timeout);

// 删除定时器
void 
    timer_del(timer_manager *T, timer *t);

// 摧毁定时器
void
    timer_destory(timer_manager *T);

#endif
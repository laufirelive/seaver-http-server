#ifndef __threadpool__
#define __threadpool__  1

#include <pthread.h>

// 线程池 任务队列
typedef struct threadpool_task {
    void                    *(*routine)(void *);
    void                    *args;
    struct threadpool_task  *next;
} threadpool_task_t;


// 线程池 数据结构
typedef struct threadpool {
    size_t              shutdown;           // 是否关闭线程池
    size_t              thread_max_num;     // 线程池线程数
    pthread_t           *thread_arr;
    threadpool_task_t   *task_queue;        // 线程池队列
    pthread_mutex_t     queue_lock;         // 队列 互斥锁
    pthread_cond_t      queue_ready;        // 条件变量
} threadpool_t;

// 线程池初始化
threadpool_t *
    threadpool_create(size_t thread_num);

// 线程池销毁
void 
    threadpool_destroy(threadpool_t *P);

// 注册任务到线程池
int 
    threadpool_sign(threadpool_t *P, void *(*task)(void *), void *args);

#endif
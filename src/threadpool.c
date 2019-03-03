#include "../inc/threadpool.h"
#include <stdio.h>
#include <stdlib.h>

static void *work_routine(void *args)
{
    threadpool_t      *P    = (threadpool_t *)args;
    threadpool_task_t *task = NULL;

    while (1)
    {
        // 线程池加锁 进入临界区
        pthread_mutex_lock(&(P->queue_lock));

        // 如果 没有任务 而且 也没有被关闭 则 等待条件变量的通知
        while (!P->task_queue && !P->shutdown)
        {
            pthread_cond_wait(&P->queue_ready, &P->queue_lock);
        }

        // 如果 被关闭 则 释放当前线程
        if (P->shutdown)
        {
            pthread_mutex_unlock(&P->queue_lock);
            pthread_exit(NULL);
        }

        // 有任务 且 没有被关闭
        task = P->task_queue;   // 工作队列 出队
        P->task_queue = P->task_queue->next;
        pthread_mutex_unlock(&P->queue_lock);

        // 执行任务并释放
        task->routine(task->args);
        free(task);
    }

    return NULL;
}

threadpool_t *threadpool_create(size_t thread_num)
{
    threadpool_t *P = (threadpool_t *)malloc(sizeof(threadpool_t));
    if (!P)
        return NULL;
    
    P->thread_max_num = thread_num;
    P->task_queue = NULL;
    P->shutdown = 0;

    // 互斥锁初始化
    if (pthread_mutex_init(&P->queue_lock, NULL) != 0)
        return NULL;
    
    // 条件变量初始化
    if (pthread_cond_init(&P->queue_ready, NULL) != 0)
        return NULL;
    
        // 线程id数组初始化
    P->thread_arr = (pthread_t *)malloc(sizeof(pthread_t) * thread_num);
    if (!(P->thread_arr))
        return NULL;
    

    // 消费者线程创建
    for (int i = 0; i < thread_num; i++)
    {
        if (pthread_create(&(P->thread_arr[i]), NULL, work_routine, (void *)P))
            return NULL;
    }

    return P;
}

void threadpool_destroy(threadpool_t *P)
{
    threadpool_task_t *temp_task;

    if (P->shutdown)
        return ;
    P->shutdown = 1;

    // 广播 所有等待任务的线程 释放自己
    pthread_mutex_lock(&(P->queue_lock));
    pthread_cond_broadcast(&(P->queue_ready));
    pthread_mutex_unlock(&(P->queue_lock));

    // 等待线程释放
    for (int i = 0; i < P->thread_max_num; i++)
    {
        pthread_join(P->thread_arr[i], NULL);
    }
    free(P->thread_arr);

    // 清空工作队列
    while (P->task_queue)
    {
        temp_task = P->task_queue;
        P->task_queue = P->task_queue->next;
        free(temp_task);
    }

    // 销毁 互斥锁 与 条件变量
    pthread_mutex_destroy(&(P->queue_lock));
    pthread_cond_destroy(&(P->queue_ready));

    // 摧毁 线程池数据结构
    free(P);
    return ;
}

int threadpool_sign(threadpool_t *P, void *(*task)(void *), void *args)
{
    threadpool_task_t *T;
    threadpool_task_t *queue;

    if (!task)
        return -1;

    T = (threadpool_task_t *)malloc(sizeof(threadpool_task_t));
    if (!T)
        return -2;

    T->routine = task;
    T->args = args;
    T->next = NULL;

    // 入队操作
    pthread_mutex_lock(&(P->queue_lock));
    queue = P->task_queue;
    if (!queue)
        P->task_queue = T;
    else
    {
        while (queue->next)
        {
            queue = queue->next;
        }
        queue->next = T;
    }
    // 设置条件变量 
    pthread_cond_signal(&(P->queue_ready));
    pthread_mutex_unlock(&(P->queue_lock));
    
    return 0;
}
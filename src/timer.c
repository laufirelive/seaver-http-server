#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include "../inc/timer.h"

static void *tick_move(void *args);
static void *on_time(void *args);

timer_manager *timer_init(int _slot_num, int _slot_interval)
{
    pthread_t t_id;
    timer_manager *newTM;
    newTM = (timer_manager *)malloc(sizeof(timer_manager));
    if (!newTM)
        return NULL;
    
    if (_slot_num <= 1 || _slot_interval <= 0)
    {
        free(newTM);
        return NULL;
    }

    memset(newTM, 0, sizeof(timer_manager));

    newTM->slot_now = 0;
    newTM->slot_num = _slot_num;
    newTM->slot_interval = _slot_interval;

    for (int i = 0; i < _slot_num; i++)
        LIST_HEAD_INIT(&newTM->slot[i]);

    pthread_create(&t_id, NULL, tick_move, (void *)newTM);
    return newTM;
}

timer *timer_add(timer_manager *T, int timeout, void *(_task)(void *), void *_arg)
{
    int _rotation = 0;
    int _slot = 0;
    timer *newTimer;

    if (timeout < 0 || !T)
        return NULL;

    _rotation = timeout / T->slot_num;
    _slot = (timeout % T->slot_num) + T->slot_now;
    
   newTimer = (timer *)malloc(sizeof(timer));
   if (!newTimer) return 0;

   newTimer->slot = _slot;
   newTimer->rotation = _rotation;
   newTimer->task = _task;
   newTimer->args = _arg;

   pthread_mutex_lock(&T->mutex);
   list_add(&T->slot[_slot], &newTimer->list);
   pthread_mutex_unlock(&T->mutex);

   return newTimer;
}

static void *tick_move(void *args)
{   
    timer_manager *TM = (timer_manager *)args;
    pthread_t t_id;

    while (1)
    {
        struct timeval time_now;
        struct timespec time_out;

        gettimeofday(&time_now, NULL);
        time_out.tv_sec = time_now.tv_sec + TM->slot_interval;
        time_out.tv_nsec = time_now.tv_usec;
        
        pthread_mutex_lock(&TM->mutex);
        pthread_cond_timedwait(&TM->cond, &TM->mutex, &time_out);
        pthread_mutex_unlock(&TM->mutex);
        
        TM->slot_now = (TM->slot_now + 1) % TM->slot_num;
        if (!list_is_empty(&TM->slot[TM->slot_now]))
            pthread_create(&t_id, NULL, on_time, (void *)TM);
    }
    
    return NULL;
}

static void *on_time(void *args)
{
    timer_manager *TM = (timer_manager *)args;
    list_head *index;
    timer *timer_ontime;

    pthread_mutex_lock(&TM->mutex);
    list_for_each(index, &TM->slot[TM->slot_now])
    {
        timer_ontime = list_entry(index, timer, list);

        if (--(timer_ontime->rotation) <= 0)
        {
            timer_ontime->task(timer_ontime->args);
            list_del(index);
            free(timer_ontime);
        }
    }
    pthread_mutex_unlock(&TM->mutex);

    return NULL;
}

void timer_reinsert(timer_manager *T, timer *t, int timeout)
{
    int _rotation = 0;
    int _slot = 0;

    if (!t || !T)
        return ;
    
    _rotation = timeout / T->slot_num;
    _slot = (timeout % T->slot_num) + T->slot_now;

    pthread_mutex_lock(&T->mutex);
    list_del(&t->list);
    t->slot = _slot;
    t->rotation = _rotation;
    list_add(&T->slot[_slot], &t->list);
    pthread_mutex_unlock(&T->mutex);
    
    return ;
}

void timer_del(timer_manager *T, timer *t)
{
    if (!t || !T)
        return ;

    pthread_mutex_lock(&T->mutex);

    list_del(&t->list);
    free(t);
    t = NULL;

    pthread_mutex_unlock(&T->mutex);
}

void timer_destory(timer_manager *T)
{
    list_head *index;
    timer *t;

    for (int i = 0; i < T->slot_num; i++)
    {
        if (list_is_empty(&T->slot[i]))
            continue;
        
        list_for_each(index, &T->slot[i])
        {
            t = list_entry(index, timer, list);
            list_del(index);
            free(t);
        }
    }

    free(T);
}
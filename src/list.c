#include <stdlib.h>
#include <sys/errno.h>
#include "../inc/debug.h"
#include "../inc/list.h"


struct http_request_head *http_request_head_init()
{
    struct http_request_head *L;

    L = (struct http_request_head *)
        malloc(sizeof(struct http_request_head));

    if (!L)
    {
        log_err("malloc(), errno: %d\t%s", errno, strerror(errno));
        return NULL;
    }

    L->fields  =  NULL;
    L->value   =  NULL;
    L->next    =  NULL;
    L->prev    =  NULL;
    
    return L;
}

int http_request_head_add(struct http_request_head *L, char *fields, char *val)
{
    struct http_request_head *newNode;
    
    if (!L)
        return -1;

    newNode = (struct http_request_head *)malloc(sizeof(struct http_request_head));    
    if (!newNode)
    {
        log_err("malloc(), errno: %d\t%s", errno, strerror(errno));
        return -1;
    }
    
    newNode->fields = fields;
    newNode->value  = val;

    newNode->next = L->next;
    newNode->prev = L;

    if (L->next)
        L->next->prev = newNode;

    L->next = newNode;
    
    return 0;
}

int http_request_head_del(struct http_request_head *target)
{
    if (!target)
        return -1;

    if (target->next)
        target->next->prev = target->prev;

    target->prev->next = target->next;

    free(target);
    return 0;
}

int http_request_head_mod(struct http_request_head *target, char *val)
{
    if (!target)
        return -1;

    target->value = val;

    return 0;    
}

struct http_request_head *
    http_request_head_get(struct http_request_head *L, char *fields)
{
    if (!L)
        return NULL;
    
    while (L = L->next)
    {
        if (!strcmp(fields, L->fields))
            return L;
    }

    return NULL;
}

struct http_request_head *
    http_request_head_get_first(struct http_request_head *L)
{
    if (!L)
        return NULL;
    
    return L->next;
}

void http_request_head_destroy(struct http_request_head *L)
{
    struct http_request_head *fir = http_request_head_get_first(L);
    struct http_request_head *del;

    while (fir)
    {   
        log("%s : %s", fir->fields, fir->value);
        del = fir;
        fir = fir->next;
        http_request_head_del(del);
    }

    if (L)
        free(L);

    return ;
}
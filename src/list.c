#include <stdlib.h>
#include <sys/errno.h>
#include "../inc/debug.h"
#include "../inc/list.h"


http_request_head *request_head_initStack()
{
    http_request_head *headNode;
    headNode = (http_request_head *)malloc(sizeof(http_request_head));
    if (!headNode)
        return NULL;
    
    headNode->data.field = NULL;
    headNode->data.value = NULL;
    headNode->next = NULL;

    return headNode;
}

int request_head_isEmpty(http_request_head *S)
{
    if (S && S->next)
        return 0;
    else
        return 1;
}

int request_head_push(http_request_head *S, char *f, char *v)
{
    http_request_head *headNode;
    headNode = (http_request_head *)malloc(sizeof(http_request_head));
    if (!headNode)
        return 0;

    if (!S)
    {
        free(headNode);
        return 0;
    }

    headNode->data.field = f;
    headNode->data.value = v;
    headNode->next = S->next;
    S->next = headNode;   

    return 1;
}

int request_head_pop(http_request_head *S, struct http_request_head_data *backup)
{
    http_request_head *deleteMe;
    if (!S || !(S->next))
        return 0;
    
    deleteMe = S->next;
    backup->field = deleteMe->data.field;
    backup->value = deleteMe->data.value;
    S->next = deleteMe->next;
    free(deleteMe);

    return 1;
}
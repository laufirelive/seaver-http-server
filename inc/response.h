#ifndef __RESPONSE__
#define __RESPONSE__    1

#include "../inc/request.h"

#define RESPONSE_BUF_LEN    8196
#define LINE_BUF_LEN        1024
#define SHORT_BUF_LEN       35
#define PYTHON_LOC          "/usr/bin/python"
#define QUSTR               "QUERY_STRING="
#define QUSTR_LEN           strlen(QUSTR)

extern char ** environ;

struct mime_type {
    char *key;
    char *value;
};

struct http_response {
    int     status;

    char    *file_type;
    size_t  file_length;

    char    context_type[SHORT_BUF_LEN];
    
    char    message[RESPONSE_BUF_LEN];
};

int response_handle_static(struct http_request *header);
int response_handle_dynamic(struct http_request *header);
int response_handle_HEAD(struct http_request *header);

#endif
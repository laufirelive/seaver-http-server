#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include "../inc/response.h"
#include "../inc/request.h"
#include "../inc/socket.h"
#include "../inc/debug.h"
#include "../inc/conf.h"

char *url_parse(struct http_response *response, char *file);

struct mime_type Mime[] = {
    {".html", "text/html"},
    {".htm", "text/html"},
    {".pdf", "application/pdf"},
    {".rtf", "application/rtf"},
    {".jpg", "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".gif", "image/gif"},
    {".png", "image/png"},
    {".avi", "video/x-msvideo"},
    {".mpg", "video/mpeg"},
    {".mpeg", "video/mpeg"},
    {".tgz", "application/x-compressed"},
    {".gz", "application/x-gzip"},
    {".js", "application/x-javascript"},
    {".css", "text/css"},
    {".txt", "text/plain"},
};

const char *response_msg_code(int stat_code)
{
    switch (stat_code)
    {
        case 200:
            return "OK";
            break;
        case 403:
            return "Not Modified";
            break;
        case 404:
            return "Not Found";
            break;
        default:
            return "Unknown";
            break;
    }
}

const char *get_mime_type(char *filetype)
{
    int MimeLen = sizeof(Mime) / sizeof(*Mime);

    if (filetype == NULL)
        return "text/plain";

    for (int i = 0; i < MimeLen; i++)
    {
        if (!(Mime[i].key))
            return Mime[i].value;
            
        if (!strcmp(Mime[i].key + 1, filetype))
            return Mime[i].value;
    }

    return "text/plain";
}

char *get_file_type(char *find)
{
    char *head = find;

    while (*find)
        find++;
    
    for (; find > head; find--)
    {
        if (*find == '.')
            break;
    }

    if (find == head)
        return NULL;

    return find + 1;
}

int response_handle_static(struct http_request *header)
{
    char        *filename;
    char        *file_mem;
    char        buf[LINE_BUF_LEN];
    int         file_fd;
    ssize_t     message_len;

    struct http_response response;
    memset(&response, 0, sizeof(response));
    memset(&buf, 0, sizeof(buf));

    // 处理 URI, 查询 其 URI 是否 合法 存在 占用
    filename = url_parse(&response, header->request_line.url);

    // 响应头 拼接
    sprintf(buf, "HTTP/1.1 %d %s\r\n", response.status, response_msg_code(response.status));
    strncat(response.message, buf, RESPONSE_BUF_LEN - strlen(response.message));

    sprintf(buf, "Content-type: %s\r\n", get_mime_type(response.file_type));
    strncat(response.message, buf, RESPONSE_BUF_LEN - strlen(response.message));

    sprintf(buf, "Content-length: %ld\r\n", response.file_length);
    strncat(response.message, buf, RESPONSE_BUF_LEN - strlen(response.message));

    sprintf(buf, "Server: %s\r\n", "Seaver");
    strncat(response.message, buf, RESPONSE_BUF_LEN - strlen(response.message));
    
    sprintf(buf, "\r\n");
    strncat(response.message, buf, RESPONSE_BUF_LEN - strlen(response.message));

    message_len = strlen(response.message);

    // 发送 响应头
    if (_send(header->fd, response.message, message_len) != message_len)
    {
        log_err("sent failed");
        goto ERROR;
    }

    // 发送 响应体
    if (response.status == 200)
    {
        // 打开文件
        if ((file_fd = open(filename, O_RDONLY)) == -1)
        {
            log_err("open(%s), errno: %d\t%s", filename, errno, strerror(errno));
            goto ERROR;
        }
        // 映射至内存
        if ((file_mem = mmap(NULL, response.file_length, PROT_READ, MAP_PRIVATE, file_fd, 0)) == (void *)-1)
        {
            log_err("mmap(), errno: %d\t%s", errno, strerror(errno));

            close(file_fd);
            goto ERROR;
        }
        // 关闭文件
        close(file_fd);

        // 发送内存中的文件数据
        if (_send(header->fd, file_mem, response.file_length) != response.file_length)
        {
            log_err("sent failed");
            goto ERROR;
        }
        
        // 释放
        munmap(file_mem, response.file_length);
        log("Request has been Responsed.");
    }

ERROR:

    memset(filename, 0, strlen(filename));
    free(filename);
    return 0;
}

char *url_parse(struct http_response *response, char *file)
{
    struct stat file_stat;
    char *r_file;

    int r_file_len;
    r_file_len = sizeof(char) * (strlen(Configuration.loc) + strlen(file) + strlen(Configuration.default_page) + 1);
    r_file = (char *)malloc(r_file_len);

    memset(r_file, 0, r_file_len);
    memset(&file_stat, 0, sizeof(file_stat));

    sprintf(r_file, "%s%s", Configuration.loc, file);
    
TRY_AGAIN :

    if (stat(r_file, &file_stat) == -1)
    {
        log_err("[%s] is not exist, errno: %d\t%s", r_file, errno, strerror(errno));
        response->status = 404;
    }
    else if (S_ISDIR(file_stat.st_mode))
    {
        sprintf(r_file, "%s%s", r_file, Configuration.default_page);
        goto TRY_AGAIN;
    }
    else if (!(S_ISREG(file_stat.st_mode)) || !(S_IRUSR & file_stat.st_mode))
    {
        log_err("[%s] cannot be read, errno: %d\t%s", r_file, errno, strerror(errno));
        response->status = 403;
    }
    else
    {
        response->file_length = file_stat.st_size;
        response->status = 200;
    }
    
    response->file_type = get_file_type(r_file);

    log("file_name : %s", r_file);
    log("file_type : %s", response->file_type);
    log("file_leng : %lu", response->file_length);

    return r_file;
}


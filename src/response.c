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

#define ERROR_MESSAGE \
            "<html>"\
            "<head>"\
            "<meta charset=\"utf-8\">"\
            "<title>Seaver Server Error</title>"\
            "</head>"\
            "<body style=\"text-align: center\">"\
            "<h1>ERROR %d %s</h1>"\
            "<hr>"\
            "<p>This is the error page of Seaver Server.</p>"\
            "</body>"\
            "</html>"

char *url_parse(struct http_response *response, char *file);
void cgi_handle(char *para, struct http_request *header, char *filename);

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

int reqponse_set_header(int fd, int status, char *filetype, size_t filelen, char *message, char *cgi)
{
    char        buf[LINE_BUF_LEN];
    ssize_t     message_len;
    memset(&buf, 0, sizeof(buf));
    
    // 响应头 拼接
    sprintf(buf, "HTTP/1.1 %d %s\r\n", status, response_msg_code(status));
    strncat(message, buf, RESPONSE_BUF_LEN - strlen(message));

    sprintf(buf, "Server: %s\r\n", "Seaver");
    strncat(message, buf, RESPONSE_BUF_LEN - strlen(message));

if (!cgi)
{

    sprintf(buf, "Content-length: %ld\r\n", filelen);
    strncat(message, buf, RESPONSE_BUF_LEN - strlen(message));

    sprintf(buf, "Content-type: %s\r\n", get_mime_type(filetype));
    strncat(message, buf, RESPONSE_BUF_LEN - strlen(message));

    sprintf(buf, "\r\n");
    strncat(message, buf, RESPONSE_BUF_LEN - strlen(message));

}

    message_len = strlen(message);

#if (DBG)
    printf("\nResponse_head : \n%s\n", message);
#endif

    // 发送 响应头
    if (_send(fd, message, message_len) != message_len)
    {
        log_warn("sent failed");
        return -1;
    }

    return 0;
}

int response_handle_static(struct http_request *header)
{
    char        *filename;
    char        *file_mem;
    char        *para;
    int         file_fd;
    char        error_message[LINE_BUF_LEN];
    struct http_response response;
    memset(&response, 0, sizeof(response));

    // 处理 URI, 查询 其 URI 是否 合法 存在 占用
    filename = url_parse(&response, header->request_line.url);

    para = header->request_line.url_para;

    if (response.status != 200 && Configuration.error_page[0] == '\0')
    {
        sprintf(error_message,
            ERROR_MESSAGE
            , response.status, response_msg_code(response.status));

        response.file_length = strlen(error_message);
        response.file_type = "html";
    }

    if (reqponse_set_header
            (
                header->fd, 
                response.status, 
                response.file_type, 
                response.file_length, 
                response.message, 
                para
            )
        )
        goto ERROR;

    // 发送 响应体
    if (response.status == 200 || Configuration.error_page[0] != '\0')
    {
        if (para)
        {
            cgi_handle(para, header, filename);
#if (DBG)
            log("Request has been Responsed, fd : %d", header->fd);
#endif
            goto OUT;
        }
        // 打开文件
        if ((file_fd = open(filename, O_RDONLY)) == -1)
        {
            log_warn("open(%s), errno: %d\t%s", filename, errno, strerror(errno));
            goto ERROR;
        }
        // 映射至内存
        if ((file_mem = mmap(NULL, response.file_length, PROT_READ, MAP_PRIVATE, file_fd, 0)) == (void *)-1)
        {
            log_warn("mmap(), errno: %d\t%s", errno, strerror(errno));

            close(file_fd);
            goto ERROR;
        }
        // 关闭文件
        close(file_fd);

        // 发送内存中的文件数据
        if (_send(header->fd, file_mem, response.file_length) != response.file_length)
        {
            munmap(file_mem, response.file_length);
            log_warn("sent failed");
            goto ERROR;
        }
        
        // 释放
        munmap(file_mem, response.file_length);
#if (DBG)
        log("Request has been Responsed, fd : %d", header->fd);
#endif
    }
    else
    {
        puts(error_message);
        if (_send(header->fd, error_message, response.file_length) != response.file_length)
        {
            log_warn("sent failed");
            goto ERROR;
        }
#if (DBG)
        log("Request has been Responsed, fd : %d", header->fd);
#endif
    }
    

ERROR:
OUT :

    memset(filename, 0, strlen(filename));
    free(filename);
    return 0;
}

int response_handle_dynamic(struct http_request *header)
{
    char        *filename;
    char        error_message[LINE_BUF_LEN];
    struct http_response response;
    memset(&response, 0, sizeof(response));

    // 处理 URI, 查询 其 URI 是否 合法 存在 占用
    filename = url_parse(&response, header->request_line.url);

    if (reqponse_set_header
            (
                header->fd, 
                response.status, 
                response.file_type, 
                response.file_length, 
                response.message, 
                (char *)1
            )
        )
        goto ERROR;

    if (response.status == 200 || Configuration.error_page[0] != '\0')
    {
        cgi_handle(header->request_body.s, header, filename);
#if (DBG)
        log("Request has been Responsed, fd : %d", header->fd);
#endif
        goto OUT;
    }
    else
    {
        puts(error_message);
        if (_send(header->fd, error_message, response.file_length) != response.file_length)
        {
            log_warn("sent failed");
            goto ERROR;
        }
#if (DBG)
        log("Request has been Responsed, fd : %d", header->fd);
#endif
    }
    

ERROR:
OUT :

    memset(filename, 0, strlen(filename));
    free(filename);
    return 0;

}

void cgi_handle(char *para, struct http_request *header, char *filename)
{
    __pid_t pid;
    int pipe_fd[2];

    int str_len = sizeof(char *) * (strlen(para) + QUSTR_LEN) + 1;
    char *str = (char *)malloc(str_len);
    
    memset(str, '\0', str_len);
    sprintf(str, "%s%s", QUSTR, para);

    char *arg[] = {PYTHON_LOC, filename, NULL};   
    char *envp[] = {str, NULL};    

    pipe(pipe_fd);

 #if (DBG)
    log("%s", str);
 #endif

    if ((pid = fork()) == 0)
    {
        dup2(header->fd, STDOUT_FILENO);
        //dup2()
        execve(arg[0], arg, envp);
    }
    
    free(str);
}

char *url_parse(struct http_response *response, char *file)
{
    struct stat file_stat;
    char *r_file;

    int r_file_len;
    // 文件名长度 = 配置 项目路径 的字符串长度 + url 长度 + 默认页面 长度
    r_file_len = sizeof(char) * (strlen(Configuration.loc) + strlen(file) + strlen(Configuration.default_page) + 1);
    r_file = (char *)malloc(r_file_len);
    memset(r_file, 0, r_file_len);
    memset(&file_stat, 0, sizeof(file_stat));

    // 拼接文件名
    sprintf(r_file, "%s/%s", Configuration.loc, file);
    
 while (1)
 {
    if (stat(r_file, &file_stat) == -1)
    {
        log_warn("[%s] is not exist, errno: %d\t%s", r_file, errno, strerror(errno));

        sprintf(r_file, "%s/%s", Configuration.loc, Configuration.error_page);
        // printf("\n%s\n", Configuration.error_page);
        if (stat(r_file, &file_stat) == -1 || S_ISDIR(file_stat.st_mode))
            memset(Configuration.error_page, 0, CONF_LOC_LEN);

        response->status = 404;
        response->file_length = file_stat.st_size;
    }
    else if (S_ISDIR(file_stat.st_mode))
    {
        sprintf(r_file, "%s/%s", r_file, Configuration.default_page);
        continue;
    }
    else if (!(S_ISREG(file_stat.st_mode)) || !(S_IRUSR & file_stat.st_mode))
    {
        log_warn("[%s] cannot be read, errno: %d\t%s", r_file, errno, strerror(errno));

        sprintf(r_file, "%s/%s", Configuration.loc, Configuration.error_page);
        // printf("\n%s\n", Configuration.error_page);
        if (stat(r_file, &file_stat) == -1 || S_ISDIR(file_stat.st_mode))
            memset(Configuration.error_page, 0, CONF_LOC_LEN);

        response->status = 403;
    }
    else
    {
        response->file_length = file_stat.st_size;
        response->status = 200;
    }
    
    response->file_type = get_file_type(r_file);
    break;
 }

 #if (DBG)
     log("file_name : %s", r_file);
     log("file_type : %s", response->file_type);
     log("file_leng : %lu", response->file_length);
 #endif

    return r_file;
}
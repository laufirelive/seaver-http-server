#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include "../inc/socket.h"
#include "../inc/debug.h"

int setServerSocket(char *ip, int port)
{
    int fd;
    struct sockaddr_in fd_addr; 
    int opt_val = 1;

    fd = socket(PF_INET, SOCK_STREAM, 0);
    if (fd == -1)
    {
        log_err("socket(), errno: %d\t%s", errno, strerror(errno));
        exit(-1);
    }

    memset(&fd_addr, 0, sizeof(fd_addr));
    fd_addr.sin_family = AF_INET;
    fd_addr.sin_addr.s_addr = inet_addr(ip);
    fd_addr.sin_port = htons((uint16_t)port);

    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const void *)&opt_val , sizeof(int)) == -1)
    {
        log_err("setsockopt(), errno: %d\t%s", errno, strerror(errno));
	    return -1;
    }
    
    if (bind(fd, (struct sockaddr *)&fd_addr, sizeof(fd_addr)) == -1)
    {
        log_err("bind(), errno: %d\t%s", errno, strerror(errno));
        exit(-1);
    }

    if (listen(fd, LISTEN_MAX) == -1)
    {
        log_err("listen(), errno: %d\t%s", errno, strerror(errno));
        exit(-1);
    }

    log("Server Socket Initialization Complete.");
    return fd;
}

void setNonblockingMode(int fd)
{
    int flag = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flag | O_NONBLOCK);
}


void _close(int _fd)
{
    close(_fd);
}

ssize_t _send(int t_fd, char *buffer, ssize_t buf_len)
{
    size_t    buf_left = buf_len;      // 剩余未传输
    ssize_t   buf_sent = 0;            // 已传输  

    while (buf_left > 0)
    {
        if ((buf_sent = write(t_fd, buffer, buf_left)) == -1)
        {
            if (errno == EINTR)
                buf_sent = 0;   // 重写
            else
            {
                log_err("write(), errno: %d\t%s", errno, strerror(errno));
                return -1;
            }
        }

        buf_left -= buf_sent;
        buffer   += buf_sent;
    }

    return buf_len;
}

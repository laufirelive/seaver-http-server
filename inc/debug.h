#ifndef __DEBUG__ 
#define __DEBUG__   1

#include <stdio.h>
#include <string.h>
#include <string.h>
#include <sys/errno.h>

#define log_err(INFO, ...) fprintf(stderr, "[\033[;31mERROR\033[0m] -> [FILE: %s, FUNC: %s, LINE: %d]: \n           " "\033[;31m" INFO "\033[0m" "\n", \
                                   __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__)

#define log_warn(INFO, ...) fprintf(stderr, "[\033[;33mWARN\033[0m] -> [FILE: %s, FUNC: %s, LINE: %d]: \n           " "\033[;33m" INFO "\033[0m" "\n", \
                                   __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__)

#define log(INFO, ...) fprintf(stderr, "[\033[;32mINFO \033[0m] -> [FILE: %s, FUNC: %s, LINE: %d]: \n           " "\033[;32m" INFO "\033[0m" "\n", \
                                   __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__)

#endif
#ifndef _LOG_H_
#define _LOG_H_

#include <stdio.h>
#include <stdarg.h>

typedef enum{
    LOG_DEBUG = 0,
    LOG_TRACE,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR
}acoral_log_enum;

#define ACORAL_LOG_DEBUG(format , ...)  { \
                                          printf("[\033[0;36mLOG_DEBUG\033[0m] %s:%d -> "format, __FILE__, __LINE__ , ##__VA_ARGS__); \
                                          printf("\n"); \
                                        }

#define ACORAL_LOG_TRACE(format , ...)  { \
                                          printf("[\033[0;32mLOG_TRACE\033[0m] %s:%d -> "format, __FILE__, __LINE__ , ##__VA_ARGS__); \
                                          printf("\n"); \
                                        }

#define ACORAL_LOG_INFO(format , ...)   { \
                                          printf("[\033[0;35mLOG_INFO\033[0m] %s:%d -> "format, __FILE__, __LINE__ , ##__VA_ARGS__); \
                                          printf("\n"); \
                                        }

#define ACORAL_LOG_WARN(format , ...)   { \
                                          printf("[\033[0;33mLOG_WARN\033[0m] %s:%d -> "format, __FILE__, __LINE__ , ##__VA_ARGS__); \
                                          printf("\n"); \
                                        }

#define ACORAL_LOG_ERROR(format , ...)  { \
                                          printf("[\033[0;31mLOG_ERROR\033[0m] %s:%d -> "format, __FILE__, __LINE__ , ##__VA_ARGS__); \
                                          printf("\n"); \
                                        }


#endif
#pragma once

#include <3ds.h>
#include <citro2d.h>

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <malloc.h>
#include <time.h>
#include <pthread.h>
#include <fcntl.h>

#define LOG_LINE_MAX 256
#define LOG_LOGS_MAX 256

typedef enum 
{
    LOG_FATAL,
    LOG_ERROR,
    LOG_WARN,
    LOG_INFO,
    LOG_DEBUG
} log_level_t;

typedef struct _log_ctx_t 
{
    log_level_t level = LOG_INFO;
    char line[LOG_LINE_MAX] = {0};
} log_ctx_t;

const char *log_level_str(log_level_t level);

typedef struct _logs_ctx_t
{
    log_ctx_t logs[LOG_LOGS_MAX];
    int num = 0;
} logs_ctx_t;

void logs_ctx_add_log(logs_ctx_t *ctx, log_level_t level, const char *line);
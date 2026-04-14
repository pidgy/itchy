#include "log.hpp"

#define LOG_LEVEL_FATAL_STR "FATAL"
#define LOG_LEVEL_ERROR_STR "ERROR"
#define LOG_LEVEL_WARN_STR "WARN"
#define LOG_LEVEL_INFO_STR "INFO"
#define LOG_LEVEL_DEBUG_STR "DEBUG"
#define LOG_LEVEL_UNK_STR "UNK"

const char *log_level_str(log_level_t level)
{
    switch (level)
    {
        case LOG_FATAL:
            return LOG_LEVEL_FATAL_STR;
        case LOG_ERROR:
            return LOG_LEVEL_ERROR_STR;
        case LOG_WARN:
            return LOG_LEVEL_WARN_STR;
        case LOG_INFO:
            return LOG_LEVEL_INFO_STR;
        case LOG_DEBUG:
            return LOG_LEVEL_DEBUG_STR;
        default:
            return LOG_LEVEL_UNK_STR;
    }
}

void logs_ctx_add_log(logs_ctx_t *ctx, log_level_t level, const char *line)
{
    if (ctx->num == LOG_LOGS_MAX)
    {
        return;
    }

    ctx->logs[ctx->num].level = level;
    snprintf(ctx->logs[ctx->num].line, LOG_LINE_MAX, line);

    ctx->num++;
}
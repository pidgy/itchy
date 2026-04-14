#include "irc.hpp"

#define FILE_ITCHY_CACHE "itchy.cache"

typedef struct _itchy_ctx
{
    irc_ctx_t *irc;
    // http_ctx_t *http;
    // mp4_ctx_t *mp4;
    // ... etc.

    Thread thread_irc;

    u32 keys;

    bool debug_mode;
} itchy_ctx_t;

itchy_ctx_t *itchy_ctx_new();
void itchy_ctx_free(itchy_ctx_t *);
int itchy_ctx_thread_start(itchy_ctx_t *);
void itchy_ctx_thread_stop(itchy_ctx_t *);
void itchy_ctx_scan_input(itchy_ctx_t *);
u32 itchy_ctx_key_pressed(itchy_ctx_t *);
double itchy_ctx_uptime(itchy_ctx_t *);
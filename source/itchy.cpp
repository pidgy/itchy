#include "itchy.hpp"

#define THREAD_TOTAL 1
#define THREAD_STACKSIZE (4 * 1024)

time_t itchy_start = time(0);

itchy_ctx_t *itchy_ctx_new()
{
    itchy_ctx_t *ctx = (itchy_ctx_t *)calloc(1, sizeof(itchy_ctx_t));
    ctx->debug_mode = true;
    ctx->irc = irc_ctx_new();
    return ctx;
}

void itchy_ctx_free(itchy_ctx_t *ctx)
{
    ctx->keys = 0;
    ctx->debug_mode = false;
    
    itchy_ctx_thread_stop(ctx);

    irc_ctx_free(ctx->irc);

    free(ctx);

    ctx = NULL;
}

int itchy_ctx_thread_start(itchy_ctx_t *ctx)
{
    s32 thread_priority = 0;
    svcGetThreadPriority(&thread_priority, CUR_THREAD_HANDLE);
    ctx->thread_irc = threadCreate(irc_thread, ctx->irc, THREAD_STACKSIZE, thread_priority - 1, -2, true);
    if (ctx->thread_irc == NULL)
    {
        return -1;
    }
    return 0;
}

void itchy_ctx_thread_stop(itchy_ctx_t *ctx)
{
    ctx->irc->is_running = false;
    threadJoin(ctx->thread_irc, U64_MAX);
    threadFree(ctx->thread_irc);
    ctx->thread_irc = NULL;
}

void itchy_ctx_scan_input(itchy_ctx_t *ctx)
{
    hidScanInput();
    ctx->keys = hidKeysDown();
}

#define NUM_KEYS 23

static const u32 keys[NUM_KEYS] = {
    KEY_A, 
    KEY_B,
    KEY_SELECT,
    KEY_START,
    KEY_DRIGHT,
    KEY_DLEFT,
    KEY_DUP,
    KEY_DDOWN,
    KEY_R,
    KEY_L,
    KEY_X,
    KEY_Y,
    KEY_ZL,
    KEY_ZR,
    KEY_TOUCH,
    KEY_CSTICK_RIGHT,
    KEY_CSTICK_LEFT,
    KEY_CSTICK_UP,
    KEY_CSTICK_DOWN,
    KEY_CPAD_RIGHT,
    KEY_CPAD_LEFT,
    KEY_CPAD_UP,
    KEY_CPAD_DOWN,
};

u32 itchy_ctx_key_pressed(itchy_ctx_t *ctx)
{
    if (ctx->keys > 0)
    {
        for (int i=0; i<NUM_KEYS; i++)
        {
            if (ctx->keys & keys[i])
            {
                return keys[i];
            }
        }
    }

    return 0;
}

double itchy_ctx_uptime(itchy_ctx_t *)
{
    return difftime( time(0), itchy_start);
}
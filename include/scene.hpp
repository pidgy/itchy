#pragma once

#include "itchy.hpp"

typedef enum
{
    SCENE_LOGIN,
    SCENE_TWITCH,
    SCENE_TWITCH_CLOSE,
    SCENE_LOGIN_CLOSE,
    SCENE_NONE,
} scene_state_t;

extern scene_state_t scene_state;

const char *scene_state_str();

void scene_init();
void scene_exit();

void scene_login_render_top(itchy_ctx_t *itchy_ctx);
void scene_login_render_bottom(itchy_ctx_t *itchy_ctx);
void scene_login_input(itchy_ctx_t *itchy_ctx);

void scene_twitch_render_top(itchy_ctx_t *itchy_ctx);
void scene_twitch_render_bottom(itchy_ctx_t *itchy_ctx);
void scene_twitch_input(itchy_ctx_t *itchy_ctx);
void scene_twitch_close_render_top(itchy_ctx_t *itchy_ctx);

void scene_debug_console_render_bottom(itchy_ctx_t*);
void scene_debug_console_input(itchy_ctx_t *);

void scene_toast_error_render_top(itchy_ctx_t *itchy_ctx);
void scene_toast_error_input(itchy_ctx_t *itchy_ctx);

void scene_toast_info_render_top(itchy_ctx_t *itchy_ctx);
void scene_toast_info_input(itchy_ctx_t *itchy_ctx);

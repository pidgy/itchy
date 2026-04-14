#include "scene.hpp"

const u32 COLOR_BG = C2D_Color32(65, 50, 112, 0xFF);
const u32 COLOR_FG = C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF);
const u32 COLOR_ERR_BG = C2D_Color32(0xFF, 0x33, 0x33, 0xFF);
const u32 COLOR_ERR_FG = C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF);

#define POS_X_Y_Z(x, y, z) x, y, z
#define SCALE_X_Y(x, y) x, y
#define RECT_COLOR(tl, tr, bl, br) tl, tr, bl, br

#define SCREEN_WIDTH_BOTTOM 320.0f
#define SCREEN_HEIGHT 240.0f
#define SCREEN_WIDTH_TOP 400

#define UI_NAV_TEXT_SCALE SCALE_X_Y(0.4f, 0.4f)
#define UI_NAV_TEXT_POS_TOP_LEFT POS_X_Y_Z(2.0f, 2.0f, 0.5f)
#define UI_NAV_TEXT_POS_TOP_RIGHT(s) POS_X_Y_Z(float(SCREEN_WIDTH_BOTTOM-(strlen(s)*6)), 1.0f, 0.5f)
#define UI_NAV_RECT_POS POS_X_Y_Z(0.0f, 0.0f, 0.0f) 
#define UI_NAV_RECT_SIZE SCREEN_WIDTH_BOTTOM, 25.0f
#define UI_NAV_RECT_COLOR RECT_COLOR(COLOR_FG, COLOR_FG, COLOR_FG, COLOR_FG)

#define UI_BG_IMG_POS POS_X_Y_Z(0.0f, 0.0f, 0.0f) 

#define UI_CHAT_TEXT_POS(i) POS_X_Y_Z(1, 1 + (i*15), 0.5f)
#define UI_CHAT_TEXT_SCALE SCALE_X_Y(0.4f, 0.4f)

#define UI_ERROR_TEXT_POS POS_X_Y_Z(SCREEN_WIDTH_TOP/2, SCREEN_HEIGHT/3, 0.5f)
#define UI_ERROR_TEXT_SCALE SCALE_X_Y(.7f, .7f)
#define UI_ERROR_TEXT_LINE_LENGTH 225.0f
#define UI_ERROR_IMG_POS POS_X_Y_Z(50, 25, 0.0f) 

#define UI_DEBUG_TEXT_SCALE SCALE_X_Y(0.5f, 0.5f)
#define UI_DEBUG_TEXT_POS_TOP_LEFT POS_X_Y_Z(2.0f, 2.0f, 0.5f)

C3D_RenderTarget *SCREEN_TOP, *SCREEN_BOTTOM;
C2D_SpriteSheet sheet;
C2D_Image scene_login_img, scene_twitch_img, scene_toast_error_img, scene_toast_info_img;

C2D_TextBuf text_buf_static = C2D_TextBufNew(4096);
C2D_TextBuf text_buf_dynamic = C2D_TextBufNew(4096);
C2D_Text    text_appname_static;

scene_state_t scene_state = SCENE_LOGIN;
scene_state_t scene_state_prev = SCENE_NONE;

scene_state_t scene_sub_state = SCENE_NONE;

const char *scene_state_str()
{
    switch (scene_state)
    {
        case SCENE_LOGIN:
            return "SCENE_LOGIN";
        case SCENE_TWITCH:
            return "SCENE_TWITCH";
        case SCENE_TWITCH_CLOSE:
            return "SCENE_TWITCH_CLOSE";
        case SCENE_LOGIN_CLOSE:
            return "SCENE_LOGIN_CLOSE";
        case SCENE_NONE:
            return "SCENE_NONE";
        default:
            return "SCENE_UNKNOWN";
    }
}

static void scene_state_set(scene_state_t s)
{
    scene_state_prev = scene_state;
    scene_state = s;
}

void scene_init()
{
    romfsInit();
	gfxInitDefault();
	gfxSet3D(true); // Activate stereoscopic 3D
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
	C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
	C2D_Prepare();
    
	SCREEN_TOP = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
	SCREEN_BOTTOM = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

	sheet = C2D_SpriteSheetLoad("romfs:/gfx/itchy.t3x");
	scene_login_img = C2D_SpriteSheetGetImage(sheet, 0);
	scene_twitch_img = C2D_SpriteSheetGetImage(sheet, 1);
    scene_toast_error_img = C2D_SpriteSheetGetImage(sheet, 2);
    scene_toast_info_img = C2D_SpriteSheetGetImage(sheet, 3);

    C2D_TextParse(&text_appname_static, text_buf_static, "itchy v0.1.0");
    C2D_TextOptimize(&text_appname_static);
}

void scene_exit()
{
	C2D_SpriteSheetFree(sheet);
    C2D_TextBufDelete(text_buf_static);
    C2D_TextBufDelete(text_buf_dynamic);
    
	C2D_Fini();
	C3D_Fini();
	romfsExit();
	gfxExit();
}

void scene_login_render_top(itchy_ctx_t *itchy_ctx)
{
    C2D_TargetClear(SCREEN_TOP, COLOR_BG);
    C2D_SceneBegin(SCREEN_TOP);
    C2D_DrawImageAt(scene_login_img, UI_BG_IMG_POS);
}

void scene_login_render_bottom(itchy_ctx_t *itchy_ctx)
{
    C2D_TargetClear(SCREEN_BOTTOM, COLOR_BG);
    C2D_SceneBegin(SCREEN_BOTTOM);
}

void scene_login_input(itchy_ctx_t *itchy_ctx)
{
    if (scene_state_prev == SCENE_NONE) // First time.
    {
        if (R_SUCCEEDED(irc_ctx_file_read_credentials(itchy_ctx->irc, FILE_ITCHY_CACHE)))
        {
            scene_state_set(SCENE_TWITCH);
        }
    }

    switch (itchy_ctx_key_pressed(itchy_ctx))
    {
        case KEY_A:
            if (R_SUCCEEDED(irc_ctx_file_read_credentials(itchy_ctx->irc, FILE_ITCHY_CACHE)))
            {
                scene_state_set(SCENE_TWITCH);
            }
            else if (R_SUCCEEDED(irc_ctx_enter_credentials(itchy_ctx->irc)))
            {
                if (R_FAILED(irc_ctx_file_write_credentials(itchy_ctx->irc, FILE_ITCHY_CACHE)))
                {
                    irc_ctx_log_info(itchy_ctx->irc, "Failed to save %s", FILE_ITCHY_CACHE);
                }

                scene_state_set(SCENE_TWITCH);
            }

            break;
        case KEY_START:
            scene_state_set(SCENE_LOGIN_CLOSE);
            
            break;
        case KEY_Y:
            if (irc_ctx_file_delete_credentials(itchy_ctx->irc, FILE_ITCHY_CACHE) != 0 )
            {
                irc_ctx_log_error(itchy_ctx->irc, "Failed to delete %s", FILE_ITCHY_CACHE);
            }
            else
            {
                irc_ctx_log_info(itchy_ctx->irc, "Deleted %s", FILE_ITCHY_CACHE);
            }
            
            break;
        default:
            break;
    }
}

void scene_twitch_render_top(itchy_ctx_t *itchy_ctx)
{
    if (itchy_ctx->irc->thread_irc == NULL)
    {
        if (R_FAILED(irc_ctx_thread_start(itchy_ctx->irc)))
        {
            irc_ctx_log_error(itchy_ctx->irc, "Failed to start IRC thread");
            return;
        }
    }

    C2D_TargetClear(SCREEN_TOP, COLOR_BG);
    C2D_SceneBegin(SCREEN_TOP);
    C2D_DrawImageAt(scene_twitch_img, UI_BG_IMG_POS);
    pthread_mutex_lock(&itchy_ctx->irc->chats_mutex);
    {
        for (size_t i=0; i<itchy_ctx->irc->chats_size; i++)
        {
            C2D_DrawText(
                &itchy_ctx->irc->chats[i].txt, 
                C2D_AlignLeft | C2D_WithColor, 
                UI_CHAT_TEXT_POS(i), 
                UI_CHAT_TEXT_SCALE, 
                itchy_ctx->irc->chats[i].color
            );
        }
    }
    pthread_mutex_unlock(&itchy_ctx->irc->chats_mutex);
}

void scene_twitch_render_bottom(itchy_ctx_t *itchy_ctx)
{
    C2D_TargetClear(SCREEN_BOTTOM, COLOR_BG);
    C2D_SceneBegin(SCREEN_BOTTOM);
    
    C2D_DrawRectangle(
        UI_NAV_RECT_POS, 
        UI_NAV_RECT_SIZE, 
        UI_NAV_RECT_COLOR
    );

    C2D_DrawText(
        &text_appname_static, 
        C2D_AlignLeft | C2D_WithColor, 
        UI_NAV_TEXT_POS_TOP_LEFT,
        UI_NAV_TEXT_SCALE,
        COLOR_BG
    );

    C2D_TextBufClear(text_buf_dynamic);
    C2D_Text text_username_dynamic;
    C2D_TextParse(&text_username_dynamic, text_buf_dynamic, itchy_ctx->irc->username);
    C2D_TextOptimize(&text_username_dynamic);
    C2D_DrawText(
        &text_username_dynamic, 
        C2D_AlignLeft | C2D_WithColor, 
        UI_NAV_TEXT_POS_TOP_RIGHT(itchy_ctx->irc->username),
        UI_NAV_TEXT_SCALE,
        COLOR_BG
    );
}

void scene_twitch_input(itchy_ctx_t *itchy_ctx)
{
    if (itchy_ctx_key_pressed(itchy_ctx) == KEY_B)
    {            
        scene_state_set(SCENE_TWITCH_CLOSE);
    }    
}

void scene_twitch_close_render_top(itchy_ctx_t *itchy_ctx)
{
    irc_ctx_thread_stop(itchy_ctx->irc);

    scene_state_set(SCENE_LOGIN);
}

void scene_toast_error_render_top(itchy_ctx_t *itchy_ctx)
{
    C2D_TargetClear(SCREEN_TOP, COLOR_BG);
    C2D_SceneBegin(SCREEN_TOP);

    C2D_DrawImageAt(scene_toast_error_img, UI_ERROR_IMG_POS);

    char errbuf[IRC_LOG_MAX+10] = {0};
    snprintf(errbuf, IRC_LOG_MAX+10, "ERROR\n%s", itchy_ctx->irc->log_error);

    C2D_TextBufClear(text_buf_dynamic);
    C2D_Text text_error;
    C2D_TextParse(&text_error, text_buf_dynamic, errbuf);
    C2D_TextOptimize(&text_error);
    C2D_DrawText(
        &text_error, 
        C2D_AtBaseline | C2D_AlignCenter | C2D_WithColor | C2D_WordWrap,
        UI_ERROR_TEXT_POS,
        UI_ERROR_TEXT_SCALE,
        COLOR_FG,
        UI_ERROR_TEXT_LINE_LENGTH
    );
}

void scene_toast_error_input(itchy_ctx_t *itchy_ctx)
{
    if (itchy_ctx_key_pressed(itchy_ctx) == KEY_B)
    {            
        irc_ctx_log_error_clear(itchy_ctx->irc);
    }    
}

void scene_toast_info_render_top(itchy_ctx_t *itchy_ctx)
{
    C2D_TargetClear(SCREEN_TOP, COLOR_BG);
    C2D_SceneBegin(SCREEN_TOP);

    C2D_DrawImageAt(scene_toast_error_img, UI_ERROR_IMG_POS);

    char errbuf[IRC_LOG_MAX+10] = {0};
    snprintf(errbuf, IRC_LOG_MAX+10, "ERROR\n%s", itchy_ctx->irc->log_error);

    C2D_TextBufClear(text_buf_dynamic);
    C2D_Text text_error;
    C2D_TextParse(&text_error, text_buf_dynamic, errbuf);
    C2D_TextOptimize(&text_error);
    C2D_DrawText(
        &text_error, 
        C2D_AtBaseline | C2D_AlignCenter | C2D_WithColor | C2D_WordWrap,
        UI_ERROR_TEXT_POS,
        UI_ERROR_TEXT_SCALE,
        COLOR_FG,
        UI_ERROR_TEXT_LINE_LENGTH
    );
}

void scene_toast_info_input(itchy_ctx_t *itchy_ctx)
{
    if (itchy_ctx_key_pressed(itchy_ctx) == KEY_A)
    {            
        irc_ctx_log_info_clear(itchy_ctx->irc);
    }    
}

static void scene_parse_text_line(C2D_Text* text, C2D_TextBuf buf, u32 lineNo, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    char line[IRC_LOG_MAX] = {0};
    vsnprintf(line, IRC_LOG_MAX, format, args);
    va_end(args);

    C2D_TextParseLine(text, buf, line, lineNo);
}

bool debug_which = false;

static void scene_debug_console_render_bottom_text(itchy_ctx_t* itchy_ctx, C2D_Text *text, int line_no, const char *format, ...)
{
    char buf[IRC_LOG_MAX] = {0};
    va_list args;
    va_start(args, format);
    vsnprintf(buf, IRC_LOG_MAX, format, args);
    va_end(args);

    C2D_TextParseLine(text, text_buf_dynamic, buf, line_no);
    // C2D_TextParseLine(text, text_buf_dynamic, "\0", line_no+1);
    C2D_TextOptimize(text);
}

void scene_debug_console_render_bottom(itchy_ctx_t* itchy_ctx)
{
    C2D_TargetClear(SCREEN_BOTTOM, COLOR_BG);
    C2D_SceneBegin(SCREEN_BOTTOM);

    C2D_Text text_debug;
    C2D_TextBufClear(text_buf_dynamic);

    // char buf[512] = {0};
    // snprintf(buf, 512, "state: %s\n irc: %d\n chats: %d\n time: %llus\n user: %s\n channel: %s\n error: %s\n info: %s\n", 
    //         scene_state_str(), itchy_ctx->irc->is_running, itchy_ctx->irc->chats_size, now.tv_sec, itchy_ctx->irc->username, itchy_ctx->irc->channel, itchy_ctx->irc->log_error, itchy_ctx->irc->log_info);
    if (debug_which)
    {
        for (int i = 0; i < itchy_ctx->irc->logs.num; i++)
        {
            log_ctx_t log = itchy_ctx->irc->logs.logs[i];
            scene_debug_console_render_bottom_text(itchy_ctx, &text_debug, i, "[%s] %s", log_level_str(log.level), log.line);
        }
    }
    else
    {
        // char line[IRC_LOG_MAX] = {0};
        // snprintf(line, IRC_LOG_MAX, 
        //     "itchy Debug Menu\n\nUptime: %.fs\nScene: %s\nKb: %d\nThread: %s\nIRC: %d\nError: %s\nInfo: %s\nUser: %s\nToken: %s\nChannel: %s\nLogs: %d\n", 
        //     itchy_ctx_uptime(itchy_ctx),
        //     scene_state_str(), 
        //     itchy_ctx->irc->kbr,
        //     itchy_ctx->thread_irc == NULL ? "No" : "Yes",
        //     itchy_ctx->irc->is_running,
        //     itchy_ctx->irc->log_error,
        //     itchy_ctx->irc->log_info,
        //     itchy_ctx->irc->username,
        //     itchy_ctx->irc->token,
        //     itchy_ctx->irc->channel,
        //     itchy_ctx->irc->logs.num
        // );
        
        // scene_debug_console_render_bottom_text(itchy_ctx, line);

        int i = 0;
        scene_debug_console_render_bottom_text(itchy_ctx, &text_debug, i, "itchy Debug Menu");
        // i++;
        // scene_debug_console_render_bottom_text(itchy_ctx, &text_debug, i, "");
        // i++;
        // scene_debug_console_render_bottom_text(itchy_ctx, &text_debug, i, "");
        // i++;

        // scene_debug_console_render_bottom_text(itchy_ctx, &text_debug, i++, "Scene: %s", scene_state_str());
        // scene_debug_console_render_bottom_text(itchy_ctx, &text_debug, i++, "Kb: %d", itchy_ctx->irc->kbr);
        // scene_debug_console_render_bottom_text(itchy_ctx, &text_debug, i++, "Thread: %s", itchy_ctx->thread_irc == NULL ? "Closed" : "Open");
        // scene_debug_console_render_bottom_text(itchy_ctx, &text_debug, i++, "IRC: %d", itchy_ctx->irc->is_running);
    }

    C2D_DrawText(
        &text_debug, 
        C2D_AlignLeft | C2D_WithColor | C2D_WordWrap,
        UI_DEBUG_TEXT_POS_TOP_LEFT,
        UI_DEBUG_TEXT_SCALE,
        COLOR_FG,
        SCREEN_WIDTH_BOTTOM-10
    );
}

void scene_debug_console_input(itchy_ctx_t* itchy_ctx)
{
    if (itchy_ctx_key_pressed(itchy_ctx) == KEY_X)
    {
        itchy_ctx->debug_mode = !itchy_ctx->debug_mode;
    }
    
    if (!itchy_ctx->debug_mode)
    {
        debug_which = false;
        return;
    }

    if (itchy_ctx_key_pressed(itchy_ctx) == KEY_SELECT)
    {
        debug_which = !debug_which;
    }
}
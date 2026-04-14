#include "common.hpp"
#include "itchy.hpp"
#include "scene.hpp"

#define SCREEN_HEIGHT 240

#define RGB_RESET "\x1b[0m"
#define BUF_SIZE 1024

/*
    ⊗
Ⓨ      Ⓑ
    Ⓐ 
*/

int main()
{
    srand((unsigned int)time(NULL));

    scene_init();

    itchy_ctx_t *itchy_ctx = itchy_ctx_new();
    
	while (aptMainLoop())
	{
        itchy_ctx_scan_input(itchy_ctx);          
        {
            // if (itchy_ctx_key_pressed(itchy_ctx) == KEY_X)
            // {
            //     itchy_ctx->debug_mode = !itchy_ctx->debug_mode;
            // }
            scene_debug_console_input(itchy_ctx);
            
            if (itchy_ctx->irc->has_error && scene_state != SCENE_LOGIN_CLOSE)
            {
                scene_toast_error_input(itchy_ctx);
            }
            switch (scene_state)
            {
                case SCENE_LOGIN:
                    scene_login_input(itchy_ctx);
                    break;
                case SCENE_TWITCH:
                    scene_twitch_input(itchy_ctx);
                    break;                
                case SCENE_TWITCH_CLOSE:
                    break;         
                case SCENE_LOGIN_CLOSE:
                    itchy_ctx_free(itchy_ctx);
                    scene_exit();
                    return 0;
                case SCENE_NONE:
                    return -1;
            }
        }

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        {
            if (itchy_ctx->irc->has_error && scene_state != SCENE_LOGIN_CLOSE)
            {
                scene_toast_error_render_top(itchy_ctx);
            }
            else
            {
                switch (scene_state)
                {
                    case SCENE_LOGIN:
                        scene_login_render_top(itchy_ctx);
                        break;
                    case SCENE_TWITCH:
                        scene_twitch_render_top(itchy_ctx);
                        break;                
                    case SCENE_TWITCH_CLOSE:
                        scene_twitch_close_render_top(itchy_ctx);
                        break;
                    case SCENE_LOGIN_CLOSE:
                        break;
                    case SCENE_NONE:
                        return -1;
                }
            }

            if (itchy_ctx->debug_mode && scene_state != SCENE_LOGIN_CLOSE)
            {
                scene_debug_console_render_bottom(itchy_ctx);
            }
            else
            {
                switch (scene_state)
                {
                    case SCENE_LOGIN:
                        scene_login_render_bottom(itchy_ctx);
                        break;
                    case SCENE_TWITCH:
                        scene_twitch_render_bottom(itchy_ctx);
                        break;                
                    case SCENE_TWITCH_CLOSE:
                        break;
                    case SCENE_LOGIN_CLOSE:
                        break;
                    case SCENE_NONE:
                        return -1;
                }
            }
        }
        C3D_FrameEnd(0);
    }
}
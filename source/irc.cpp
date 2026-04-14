#include "irc.hpp"

#include <sys/select.h> 

#define SOCKET_RCVTIMEO 0x1006 // Sockets receive timeout.

#define THREAD_TOTAL 1
#define THREAD_STACKSIZE (4 * 1024)

static void socket_shutdown()
{
    socExit();
}

typedef enum 
{
    C2D_COLOR_RED,
    C2D_COLOR_GREEN,
    C2D_COLOR_BLUE,
    C2D_COLOR_YELLOW,
    C2D_COLOR_PURPLE,
    C2D_COLOR_ORANGE,
    C2D_COLOR_WHITE,
    C2D_COLOR_CYAN,
    C2D_COLOR_MAGENTA,
    C2D_COLOR_COUNT 
} c2d_color_id;

static const u32 c2d_color_table[C2D_COLOR_COUNT] = {
    C2D_Color32(255, 0,   0,   255), // Red
    C2D_Color32(0,   255, 0,   255), // Green
    C2D_Color32(0,   0,   255, 255), // Blue
    C2D_Color32(255, 255, 0,   255), // Yellow
    C2D_Color32(128, 0,   128, 255), // Purple
    C2D_Color32(255, 165, 0,   255), // Orange
    C2D_Color32(255, 255, 255, 255), // White
    C2D_Color32(0,   255, 255, 255), // Cyan
    C2D_Color32(255, 0,   255, 255)  // Magenta
};

static u32 c2d_color_rand(void) {
    int index = rand() % C2D_COLOR_COUNT;
    return c2d_color_table[index];
}

irc_ctx_t *irc_ctx_new()
{
    irc_ctx_t *ctx = (irc_ctx_t *)calloc(1, sizeof(irc_ctx_t));
    ctx->username = (char *)calloc(IRC_KB_INPUT_MAX, sizeof(char));
    ctx->token = (char *)calloc(IRC_KB_INPUT_MAX, sizeof(char));
    ctx->channel = (char *)calloc(IRC_KB_INPUT_MAX, sizeof(char));
    ctx->log_error = (char *)calloc(IRC_LOG_MAX, sizeof(char));
    ctx->log_info = (char *)calloc(IRC_LOG_MAX, sizeof(char));

    ctx->static_text_buf = C2D_TextBufNew(4096);

    irc_init(ctx);

    return ctx;
}

void irc_ctx_free(irc_ctx_t *ctx)
{
	if (ctx == NULL)
	{
		return;
	}

    ctx->is_running = false;
    close(ctx->socket);

    for (int i=0; i<IRC_CHATS_MAX; i++)
    {
        C2D_TextBufDelete(ctx->static_text_buf);
        free(ctx->chats[i].msg);
    }

    free(ctx->username);
    free(ctx->token);
    free(ctx->channel);
    free(ctx->log_error);
    free(ctx->log_info);
    free(ctx->socket_buffer);
    
    free(ctx);
    ctx = NULL;
}

void irc_init(irc_ctx_t *ctx) 
{  	
    ctx->socket_buffer = (u32 *)memalign(SOCKET_ALIGN, SOCKET_BUFFERSIZE);
    if (ctx->socket_buffer == NULL)
    {
        irc_ctx_log_error(ctx, "socket: memalign: failed to allocate");
		return;
    }

	int ok = socInit(ctx->socket_buffer, SOCKET_BUFFERSIZE);
    if (R_FAILED(ok))
    {
        irc_ctx_log_error(ctx, "socket: socInit: 0x%08X", (unsigned int)ok);
		return;
    }

    // register socShutdown to run at exit
    // atexit functions execute in reverse order so this runs before gfxExit
    atexit(socket_shutdown);

    return;
}

void irc_ctx_clear_credentials(irc_ctx_t *ctx)
{
    memset(ctx->username, 0, IRC_KB_INPUT_MAX);
    memset(ctx->token, 0, IRC_KB_INPUT_MAX);
    memset(ctx->channel, 0, IRC_KB_INPUT_MAX);
}

void irc_ctx_log_error_clear(irc_ctx_t *ctx)
{
    ctx->has_error = false;
    memset(ctx->log_error, 0, IRC_LOG_MAX);
}

void irc_ctx_log_info_clear(irc_ctx_t *ctx)
{
    memset(ctx->log_info, 0, IRC_LOG_MAX);
}

void irc_ctx_log_error(irc_ctx_t *ctx, const char *format, ...)
{
    irc_ctx_log_error_clear(ctx);

    va_list args;
    va_start(args, format);
    vsnprintf(ctx->log_error, IRC_LOG_MAX, format, args);
    va_end(args);

    ctx->has_error = true;
}

void irc_ctx_log_info(irc_ctx_t *ctx, const char *format, ...)
{
    irc_ctx_log_info_clear(ctx);

    va_list args;
    va_start(args, format);
    vsnprintf(ctx->log_info, IRC_LOG_MAX, format, args);
    va_end(args);

    logs_ctx_add_log(&ctx->logs, LOG_INFO, ctx->log_info);
}

char *irc_strip_privmsg(const char *line)
{
    const char *user_start = NULL, *user_end = NULL, *msg_start = NULL;

    // Must start with ':'
    if (line[0] != ':')
    {
        return NULL;
    }

    // Extract username
    user_start = line + 1; // skip initial ':'
    user_end = strchr(user_start, '!');
    if (!user_end)
    {
        return NULL;
    }
    // Find start of the message (after second ':')
    msg_start = strstr(user_end, "PRIVMSG");
    if (!msg_start)
    {
        return NULL;
    }

    msg_start = strchr(msg_start, ':');
    if (!msg_start)
    {
        return NULL;
    }
    msg_start++; // skip ':'

    // Allocate result: "username: message"
    size_t len = (user_end - user_start) + 2 + strlen(msg_start) + 1;
    char *result = (char *)calloc(sizeof(char), len);
    if (!result)
    {
        return NULL;
    }

    snprintf(result, len, "%.*s: %s\n", (int)(user_end - user_start), user_start, msg_start);

    return result;
}

static int ikb_ctx_enter_text(irc_ctx_t *ctx, char *buffer, size_t buf_size, const char *hint_text)
{
    static SwkbdState swkbd;

    // Initialize software keyboard
    swkbdInit(&swkbd, SWKBD_TYPE_QWERTY, 1, IRC_KB_INPUT_MAX);
    swkbdSetValidation(&swkbd, SWKBD_NOTEMPTY_NOTBLANK, 0, IRC_KB_INPUT_MAX);
    swkbdSetHintText(&swkbd, hint_text);
    swkbdSetFeatures(&swkbd, SWKBD_DARKEN_TOP_SCREEN | SWKBD_DEFAULT_QWERTY | SWKBD_ALLOW_POWER | SWKBD_ALLOW_RESET | SWKBD_ALLOW_HOME);
    swkbdSetButton(&swkbd, SWKBD_BUTTON_CONFIRM, "Ok", false);
    swkbdInputText(&swkbd, buffer, buf_size);

    ctx->kbr = swkbdGetResult(&swkbd);
    switch (ctx->kbr)
    {
        case SWKBD_D0_CLICK:
        case SWKBD_D1_CLICK0:
        case SWKBD_D1_CLICK1:
        case SWKBD_D2_CLICK0:
        case SWKBD_D2_CLICK1:
        case SWKBD_D2_CLICK2:
        case SWKBD_PARENTAL_OK:
            return 0;
        case SWKBD_NONE:	
        case SWKBD_INVALID_INPUT: 
        case SWKBD_OUTOFMEM:	
        case SWKBD_HOMEPRESSED:
        case SWKBD_RESETPRESSED:
        case SWKBD_POWERPRESSED:
        case SWKBD_PARENTAL_FAIL:
        case SWKBD_BANNED_INPUT:
        default:
            return -1;
    }
}

int irc_ctx_enter_credentials(irc_ctx_t *ctx)
{
    irc_ctx_log_info(ctx, "irc_ctx_enter_credentials");

	if (ikb_ctx_enter_text(ctx, ctx->username, IRC_KB_INPUT_MAX, "Twitch Username") != 0)
	{
        irc_ctx_log_error(ctx, "Error: Invalid username");
        return -1;
	}

	if (ikb_ctx_enter_text(ctx, ctx->token, IRC_KB_INPUT_MAX, "Twitch OAuth Token") != 0)
	{
        irc_ctx_log_error(ctx, "Error: Invalid OAuth token");
		return -1;
	}

	if (ikb_ctx_enter_text(ctx, ctx->channel, IRC_KB_INPUT_MAX, "Twitch Chat Channel") != 0)
	{
	    irc_ctx_log_error(ctx, "Error: Invalid Twitch channel");
	    return -1;
	}
	
	return 0;
}

int irc_ctx_file_read_credentials(irc_ctx_t *ctx, const char* path)
{
    irc_ctx_clear_credentials(ctx);
   	
    FILE* f = fopen(path, "r");
	if (!f)
	{
        irc_ctx_log_info(ctx, "Failed to open %s", path);
        return -1;
    }
   
    char buf[IRC_LOG_MAX] = {0};
    int n = fscanf(f, "%[^\n]", buf);
    if (n <= 0)
    {
        return -1;
    }

    int i = 0;
    char * c = strtok(buf, ";");
    while(c != NULL) 
    {
        switch (i)
        {
            case 0:
                snprintf(ctx->username, IRC_KB_INPUT_MAX, "%s", c);
                break;
            case 1:
                snprintf(ctx->token, IRC_KB_INPUT_MAX, "%s", c);
                break;
            case 2:
                snprintf(ctx->channel, IRC_KB_INPUT_MAX, "%s", c);
                break;
        }
        c = strtok(NULL, ";");
        i++;
    }

    fclose(f);

    if (strlen(ctx->username) == 0 || strlen(ctx->token) == 0 || strlen(ctx->channel) == 0)
    {
        irc_ctx_log_error(ctx, "Invalid cache format", path);
        irc_ctx_clear_credentials(ctx);
        return -1;
    }

    return 0;
}

int irc_ctx_file_write_credentials(irc_ctx_t *ctx, const char* path)
{
    FILE* f = fopen(path, "w+");
	if (!f)
	{
        return -1;
    }

    char buf[1024] = {0};
    snprintf(buf, 1024, "%s;%s;%s;\n", ctx->username, ctx->token, ctx->channel);
    
    irc_ctx_log_info(ctx, "%s", buf);

    fprintf(f, buf);

    fclose(f);

    return 0;
}

int irc_ctx_file_delete_credentials(irc_ctx_t *ctx, const char* path)
{
    return remove(path);
}

static void irc_wait_for_socket(irc_ctx_t *ctx)
{
    fd_set fdset;
    FD_ZERO(&fdset);
    FD_SET(ctx->socket, &fdset);

    irc_ctx_log_info(ctx, "connecting...");

    struct timeval timeout = {5, 0}; 

    // Connection in progress -> have to wait until the connecting socket is marked as writable, i.e. connection completes
    int ok = select(ctx->socket+1, NULL, &fdset, NULL, &timeout);
    if (R_FAILED(ok)) 
    {
        irc_ctx_log_error(ctx, "connection failed on select");
        return;
    } 
    else if (ok == 0) 
    {
        irc_ctx_log_error(ctx, "connection timeout");
        return;
    } 
    else 
    {
        int sockerr;
        socklen_t len = (socklen_t)sizeof(int);

        if (R_FAILED(getsockopt(ctx->socket, SOL_SOCKET, SO_ERROR, (void*)(&sockerr), &len))) 
        {
            irc_ctx_log_error(ctx, "failed to check socket error");
            return;
        }
        if (sockerr) 
        {
            irc_ctx_log_error(ctx, "socket error: %d", sockerr);
            return;
        }
    }
}

static void irc_thread(void *arg)
{
	irc_ctx_t *ctx = (irc_ctx_t *)(arg);

    irc_ctx_log_info(ctx, "IRC thread started...");

    ctx->socket = socket(AF_INET, SOCK_STREAM, 0);
    if (R_FAILED(ctx->socket))
    {
        irc_ctx_log_error(ctx, "Socket: %s", strerror(errno));
        return;
    }

    // r = setsockopt(ctx->sock, SOL_SOCKET, SO_REUSEPORT, &yes, sizeof(int));
    // if (R_FAILED(ok))
    // {
    //     irc_ctx_error(ctx, "socket: setsockopt: 0x%08X", (unsigned int)r);
	// 	return r;
    // }

    struct timeval timeout;
    timeout.tv_sec = 5; // 5 second timeout
    timeout.tv_usec = 0;
    if (R_FAILED(setsockopt(ctx->socket, SOL_SOCKET, SOCKET_RCVTIMEO, &timeout, sizeof(timeout)))) 
    {
        irc_ctx_log_error(ctx, "Failed to set socket options: %s", strerror(errno));
        return;        
    }

    // Put the socket in non-blocking mode so the main thread can read input and close this thread.
    if (R_FAILED(fcntl(ctx->socket, F_SETFL, fcntl(ctx->socket, F_GETFL, 0) | O_NONBLOCK)))
    {
        irc_ctx_log_error(ctx, "Failed to modify socket: %s", strerror(errno));
        return;
    }

    ctx->server = gethostbyname(IRC_SERVER_HOSTNAME);
    if (!ctx->server)
    {
        irc_ctx_log_error(ctx, "DNS %s: %s", IRC_SERVER_HOSTNAME, hstrerror(h_errno));
        return;
    }
    
    memset(ctx->buffer, 0, sizeof(ctx->buffer));

    // Build server address struct
    memset(&ctx->server_addr, 0, sizeof(ctx->server_addr));
    ctx->server_addr.sin_family = AF_INET;
    memcpy(&ctx->server_addr.sin_addr.s_addr, ctx->server->h_addr, ctx->server->h_length);
    ctx->server_addr.sin_port = htons(IRC_SERVER_PORT);

    // Connect to server.
    if (R_FAILED(connect(ctx->socket, (struct sockaddr *)&ctx->server_addr, sizeof(ctx->server_addr))))
    {
        if (errno == EINPROGRESS)
        {
            irc_wait_for_socket(ctx);
        }
        else
        {
            irc_ctx_log_error(ctx, "Connection failed (%s:%d): %s", IRC_SERVER_HOSTNAME, IRC_SERVER_PORT, strerror(errno));
            return;
        }
    }
    
    // Send PASS (OAuth token).
    snprintf(ctx->buffer, sizeof(ctx->buffer), "PASS oauth:%s\r\n", ctx->token);
    if (R_FAILED(send(ctx->socket, ctx->buffer, strlen(ctx->buffer), 0)))
    {
        irc_ctx_log_error(ctx, "Failed to send OAuth Token: %s", strerror(errno));
        return;
    }

    // Send NICK (username).
    snprintf(ctx->buffer, sizeof(ctx->buffer), "NICK %s\r\n", ctx->username);
    if (R_FAILED(send(ctx->socket, ctx->buffer, strlen(ctx->buffer), 0)))
    {
        irc_ctx_log_error(ctx, "Failed to send username \"%s\": %s", ctx->username, strerror(errno));
        return;
    }

    // Join a channel.
    snprintf(ctx->buffer, sizeof(ctx->buffer), "JOIN #%s\r\n", ctx->channel);
    if (R_FAILED(send(ctx->socket, ctx->buffer, strlen(ctx->buffer), 0)))
    {
        irc_ctx_log_error(ctx, "Failed to join channel \"%s\": %s", ctx->channel, strerror(errno));
        return;
    }

    ctx->is_running = true;

    // Listen for messages.
    while (ctx->is_running)
    {
        memset(ctx->buffer, 0, sizeof(ctx->buffer));
        int bytes = recv(ctx->socket, ctx->buffer, sizeof(ctx->buffer) - 1, 0);
        if (bytes <= 0)
        {
		    continue;
        }

        // Respond to PINGs to keep connection alive.
        if (strncmp(ctx->buffer, "PING", 4) == 0)
        {
            snprintf(ctx->buffer, sizeof(ctx->buffer), "PONG :tmi.twitch.tv\r\n");
            send(ctx->socket, ctx->buffer, strlen(ctx->buffer), 0);
        }
        else if (strstr(ctx->buffer, "PRIVMSG") != NULL)
        {
			pthread_mutex_lock(&ctx->chats_mutex);
			{
                C2D_TextBufClear(ctx->static_text_buf);

                if (ctx->chats_size == IRC_CHATS_MAX-1) 
                {
                    free(ctx->chats[IRC_CHATS_MAX-1].msg);

                    for (int i=0; i<IRC_CHATS_MAX-2; i++) 
                    {
                        ctx->chats[i] = ctx->chats[i+1];
                    }
                
                    ctx->chats[ctx->chats_size].msg = irc_strip_privmsg(ctx->buffer);
                    ctx->chats[ctx->chats_size].color = c2d_color_rand();
                    C2D_TextParse(&ctx->chats[ctx->chats_size].txt, ctx->static_text_buf, ctx->chats[ctx->chats_size].msg);
                    C2D_TextOptimize(&ctx->chats[ctx->chats_size].txt);
                } 
                else 
                {
                    ctx->chats[ctx->chats_size].msg = irc_strip_privmsg(ctx->buffer);
                    ctx->chats[ctx->chats_size].color = c2d_color_rand();
                    C2D_TextParse(&ctx->chats[ctx->chats_size].txt, ctx->static_text_buf, ctx->chats[ctx->chats_size].msg);
                    C2D_TextOptimize(&ctx->chats[ctx->chats_size].txt);
                    ctx->chats_size++;
                }
            }
			pthread_mutex_unlock(&ctx->chats_mutex);
        }
        else
        {
            irc_ctx_log_info(ctx, "IRC command: %s", ctx->buffer);
        }
    }
}

int irc_ctx_thread_start(irc_ctx_t *ctx)
{
    s32 thread_priority = 0;
    svcGetThreadPriority(&thread_priority, CUR_THREAD_HANDLE);
    ctx->thread_irc = threadCreate(irc_thread, ctx, THREAD_STACKSIZE, thread_priority - 1, -2, true);
    if (ctx->thread_irc == NULL)
    {
        return -1;
    }
    return 0;
}

void irc_ctx_thread_stop(irc_ctx_t *ctx)
{
    ctx->is_running = false;
    threadJoin(ctx->thread_irc, U64_MAX);
    threadFree(ctx->thread_irc);
    ctx->thread_irc = NULL;
}
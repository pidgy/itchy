#pragma once

#include "log.hpp"

// irc
// ----------------------------------------------------------------------

#define IRC_KB_INPUT_MAX 256
#define IRC_SERVER_HOSTNAME "irc.chat.twitch.tv"
#define IRC_SERVER_PORT 6667
#define IRC_CHATS_MAX 256
#define IRC_LOG_MAX 512

#define SOCKET_ALIGN 0x1000
#define SOCKET_BUFFERSIZE 0x100000

#define DURATION_MS_1 1000000
#define DURATION_S_1 DURATION_MS_1*1000
#define DURATION_S_2 DURATION_S_1 * 2

typedef struct _irc_chat 
{
	char *msg;
	u32 color; 
	C2D_Text txt;

}	irc_chat;

typedef struct _irc_ctx
{
    char *username = NULL;
    char *token = NULL;
    char *channel = NULL;
    char *log_error = NULL;
    char *log_info = NULL;

    bool is_running;
    bool has_error;

    int socket;
    struct sockaddr_in server_addr;
    struct hostent *server;
    char buffer[512];

	C2D_TextBuf static_text_buf;

    irc_chat chats[IRC_CHATS_MAX] = {0};
    size_t chats_size = 0;
    pthread_mutex_t chats_mutex;

    u32 *socket_buffer;

    SwkbdResult kbr = SWKBD_NONE;

    logs_ctx_t logs;
} irc_ctx_t;

irc_ctx_t *irc_ctx_new();
void irc_ctx_free(irc_ctx_t *ctx);

void irc_init(irc_ctx_t *ctx);

void irc_ctx_clear_credentials(irc_ctx_t *ctx);
void irc_ctx_log_error(irc_ctx_t *ctx, const char *format, ...);
void irc_ctx_log_error_clear(irc_ctx_t *ctx);
void irc_ctx_log_info(irc_ctx_t *ctx, const char *format, ...);
void irc_ctx_log_info_clear(irc_ctx_t *ctx);
int irc_ctx_enter_credentials(irc_ctx_t *ctx);
int irc_ctx_file_read_credentials(irc_ctx_t *ctx, const char* path);
int irc_ctx_file_write_credentials(irc_ctx_t *ctx, const char* path);
int irc_ctx_file_delete_credentials(irc_ctx_t *ctx, const char *path);

char *irc_strip_privmsg(const char *line);

void irc_thread(void *arg);
void irc_wait_for_socket(irc_ctx_t *ctx);


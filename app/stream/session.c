#include "session.h"
#include "settings.h"

#include "connection.h"
#include "platform.h"
#include "util/user_event.h"
#include "input/absinput.h"

#include <Limelight.h>

#include "libgamestream/client.h"
#include "libgamestream/errors.h"

#include "backend/computer_manager.h"

STREAMING_STATUS streaming_status = STREAMING_NONE;
int streaming_errno = GS_OK;

SDL_bool session_running = SDL_FALSE;
SDL_Thread *streaming_thread;
SDL_mutex *lock;
SDL_cond *cond;

short streaming_display_width, streaming_display_height;
bool streaming_quitapp_requested;

typedef struct
{
    SERVER_DATA *server;
    CONFIGURATION *config;
    int appId;
} STREAMING_REQUEST;

static int _streaming_thread_action(STREAMING_REQUEST *req);

void streaming_init()
{
    streaming_thread = NULL;
    streaming_status = STREAMING_NONE;
    streaming_quitapp_requested = false;
    lock = SDL_CreateMutex();
    cond = SDL_CreateCond();

    absinput_init();
}

void streaming_destroy()
{
    streaming_interrupt(streaming_quitapp_requested);
    streaming_wait_for_stop();

    SDL_DestroyCond(cond);
    SDL_DestroyMutex(lock);
}

void streaming_begin(PSERVER_DATA server, int app_id)
{
    PCONFIGURATION config = settings_load();

    STREAMING_REQUEST *req = malloc(sizeof(STREAMING_REQUEST));
    req->server = server;
    req->config = config;
    req->appId = app_id;
    streaming_thread = SDL_CreateThread((SDL_ThreadFunction)_streaming_thread_action, "streaming", req);
}

void streaming_interrupt(bool quitapp)
{
    SDL_LockMutex(lock);
    streaming_quitapp_requested = quitapp;
    session_running = SDL_FALSE;
    SDL_CondSignal(cond);
    SDL_UnlockMutex(lock);
}

void streaming_wait_for_stop()
{
    if (streaming_thread == NULL)
    {
        return;
    }
    SDL_WaitThread(streaming_thread, NULL);
    streaming_thread = NULL;
}

bool streaming_running()
{
    return session_running == SDL_TRUE;
}

void streaming_display_size(short width, short height)
{
    streaming_display_width = width;
    streaming_display_height = height;
}

int _streaming_thread_action(STREAMING_REQUEST *req)
{
    streaming_status = STREAMING_CONNECTING;
    streaming_errno = GS_OK;
    PSERVER_DATA server = req->server;
    PCONFIGURATION config = req->config;
    absinput_no_control = config->viewonly;
    int appId = req->appId;

    int gamepads = absinput_gamepads();
    int gamepad_mask = 0;
    for (int i = 0; i < gamepads && i < 4; i++)
        gamepad_mask = (gamepad_mask << 1) + 1;

    if (config->debug_level > 0)
    {
        printf("Launch app %d...\n", appId);
    }
    int ret = gs_start_app(server, &config->stream, appId, config->sops, config->localaudio, gamepad_mask);
    if (ret < 0)
    {
        streaming_status = STREAMING_ERROR;
        streaming_errno = ret;
        free(req);
        return -1;
    }

    int drFlags = 0;

    if (config->debug_level > 0)
    {
        printf("Stream %d x %d, %d fps, %d kbps\n", config->stream.width, config->stream.height, config->stream.fps, config->stream.bitrate);
    }

    int startResult = LiStartConnection(&server->serverInfo, &config->stream, &connection_callbacks,
                                        platform_get_video(NONE), platform_get_audio(NONE, config->audio_device), NULL, drFlags, config->audio_device, 0);
    if (startResult != 0)
    {
        streaming_status = STREAMING_ERROR;
        streaming_errno = GS_WRONG_STATE;
        goto thread_cleanup;
    }
    session_running = true;
    streaming_status = STREAMING_STREAMING;
    rumble_handler = absinput_getrumble();

    SDL_LockMutex(lock);
    while (session_running)
    {
        // Wait until interrupted
        SDL_CondWait(cond, lock);
    }
    SDL_UnlockMutex(lock);

    rumble_handler = NULL;
    streaming_status = STREAMING_DISCONNECTING;
    LiStopConnection();

    if (config->quitappafter || streaming_quitapp_requested)
    {
        if (config->debug_level > 0)
            printf("Sending app quit request ...\n");
        int quitret = gs_quit_app(server);
        if (quitret == GS_OK)
        {
            server->currentGame = 0;
        }
    }
    // TODO https://github.com/mariotaku/moonlight-sdl/issues/3
    streaming_status = STREAMING_NONE;

thread_cleanup:
    streaming_thread = NULL;

    free(req);
    return 0;
}
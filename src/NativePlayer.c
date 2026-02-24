/*
 * Copyright (c) 2026 ByteDance Ltd. and/or its affiliates
 * SPDX-License-Identifier: LGPL-2.1-only
 */

#include "NativePlayer/NativePlayer.h"
#include <malloc.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdint.h>
#include "ijksdl/ijksdl_mutex.h"
#include "utils/nclog/log_util.h"
#include "utils/ncfence/ncfence.h"

#include "ijkplayer/ijkplayer.h"
#include "ijkplayer/ijkplayer_internal.h"
#include "ijksdl/android/ijksdl_android.h"
#include "ijksdl/android/resmgr.h"
#include "ijkplayer/ff_fferror.h"
#include "ijkplayer/ff_ffplay.h"
#include "ijksdl/android/ijksdl_android.h"
#include "ijkplayer/pipeline/ffpipeline_ffplay.h"
#include "ijkplayer/android/pipeline/ffpipeline_android.h"
#include "ijkplayer/android/ijkplayer_android.h"
#include "ijkplayer/backtrace.h"
#include <android/native_window_jni.h>
#include <android/looper.h>


struct CypressPlayer {
    IjkMediaPlayer *mp;
    int player_no;
    int has_reported_error;
    CypressPlayerCallback mMediaPlayerCallback;
    void *mCallbackUserData;
    SDL_mutex *callback_lock;
    int32_t mWidth;
    int32_t mHeight;
    ResMgr *res_mgr;

    struct {
        NCFence *prepare_fence;
        NCFence *start_fence;
        NCFence *pause_fence;
        NCFence *stop_fence;
        NCFence *user_callback_sync_fence;
        SDL_mutex *sync_interface_lock;
    } sync_block;

    MessageQueue user_callback_msg_queue;
    SDL_Thread user_callback_msg_thread;
    int user_callback_msg_tid;
};

#define LOCK_INTERFACE      SDL_LockMutex(player->sync_block.sync_interface_lock)
#define UNLOCK_INTERFACE    SDL_UnlockMutex(player->sync_block.sync_interface_lock)

static JavaVM* g_jvm;

static int message_loop(void *arg);

static int user_callback_msg_fn(void *arg) {
    CypressPlayer *player = (CypressPlayer *)arg;
    player->user_callback_msg_tid = gettid();

    AVMessage msg;
    while (msg_queue_get(&player->user_callback_msg_queue, &msg, 1) > 0) {
        int what = msg.what;
        switch (what) {
            case FFP_MSG_ERROR:

                CYLOGI("user callback thread msg(error: %d)", msg.arg1);
                SDL_LockMutex(player->callback_lock);
                player->mMediaPlayerCallback.onError ?
                    player->mMediaPlayerCallback.onError(player, player->mCallbackUserData, msg.arg1) : (void)0;
                SDL_UnlockMutex(player->callback_lock);

                break;
            case FFP_MSG_PREPARED:

                CYLOGI("user callback thread msg(prepared)");
                SDL_LockMutex(player->callback_lock);
                player->mMediaPlayerCallback.onPrepared ?
                    player->mMediaPlayerCallback.onPrepared(player, player->mCallbackUserData) : (void)0;
                SDL_UnlockMutex(player->callback_lock);

                break;
            case FFP_MSG_COMPLETED:

                CYLOGI("user callback thread msg(completed)");
                SDL_LockMutex(player->callback_lock);
                player->mMediaPlayerCallback.onComplete ?
                    player->mMediaPlayerCallback.onComplete(player, player->mCallbackUserData) : (void)0;
                SDL_UnlockMutex(player->callback_lock);

                break;
            case FFP_MSG_VIDEO_SIZE_CHANGED:

                CYLOGI("user callback thread msg(size changed: %d %d)", msg.arg1, msg.arg2);
                SDL_LockMutex(player->callback_lock);
                player->mMediaPlayerCallback.onVideoSizeChanged ?
                    player->mMediaPlayerCallback.onVideoSizeChanged(player, player->mCallbackUserData,
                                                                msg.arg1, msg.arg2) : (void)0;
                SDL_UnlockMutex(player->callback_lock);

                break;
            case FFP_MSG_VIDEO_RENDERING_START:

                CYLOGI("user callback thread msg(first frame)");
                SDL_LockMutex(player->callback_lock);
                player->mMediaPlayerCallback.onRenderedFirstFrame ?
                    player->mMediaPlayerCallback.onRenderedFirstFrame(player, player->mCallbackUserData) : (void)0;
                SDL_UnlockMutex(player->callback_lock);

                break;
            case FFP_MSG_SEEK_COMPLETE:

                CYLOGI("user callback thread msg(seek complete)");
                SDL_LockMutex(player->callback_lock);
                player->mMediaPlayerCallback.onSeekComplete ?
                    player->mMediaPlayerCallback.onSeekComplete(player, player->mCallbackUserData) : (void)0;
                SDL_UnlockMutex(player->callback_lock);

                break;
            case FFP_MSG_PLAYBACK_STATE_CHANGED:

                CYLOGI("user callback thread msg(state changed: %d)", msg.arg1);
                SDL_LockMutex(player->callback_lock);
                player->mMediaPlayerCallback.onPlaybackStateChanged ?
                    player->mMediaPlayerCallback.onPlaybackStateChanged(player, player->mCallbackUserData, msg.arg1) : (void)0;
                
                if (msg.arg1 == MP_STATE_STARTED) {
                    player->mMediaPlayerCallback.onStarted ?
                        player->mMediaPlayerCallback.onStarted(player, player->mCallbackUserData) : (void)0;
                }
                SDL_UnlockMutex(player->callback_lock);
                break;

            case FFP_REQ_WAIT_SYNC:
                CYLOGI("user callback thread msg(wait async)");
                ncfence_signal(player->sync_block.user_callback_sync_fence);
                break;
            default:
                CYLOGI("unknown FFP_MSG_xxx(%d) in user callback \n", msg.what);
                break;
        }
        msg_free_res(&msg);
        if (what == FFP_REQ_STOP) {
            break;
        }
    }
    CYLOGI("user callback thread exit");
    return 0;
}

static void message_loop_n(IjkMediaPlayer *mp){

    mp->msg_thread_tid = gettid();

    CypressPlayer* player;
    while (1) {
        AVMessage msg;

        int retval = ijkmp_get_msg(mp, &msg, 1);
        if (retval < 0)
            break;
        // block-get should never return 0
        assert(retval > 0);
        switch (msg.what) {
            case FFP_MSG_FLUSH:
                CYLOGI("FFP_MSG_FLUSH:\n");
                break;
            case FFP_MSG_ERROR:
                CYLOGI("FFP_MSG_ERROR: %d\n", msg.arg1);
                player = (CypressPlayer*)(mp->cypressplayer);

                if (!player->has_reported_error) {
                    msg_queue_put(&player->user_callback_msg_queue, &msg);
                    player->has_reported_error = msg.arg1;
                }

                ncfence_signal(player->sync_block.prepare_fence);
                ncfence_signal(player->sync_block.start_fence);
                ncfence_signal(player->sync_block.pause_fence);
                ncfence_signal(player->sync_block.stop_fence);
                break;
            case FFP_MSG_PREPARED:
                CYLOGI("FFP_MSG_PREPARED:\n");
                player = (CypressPlayer*)(mp->cypressplayer);

                msg_queue_put(&player->user_callback_msg_queue, &msg);

                break;
            case FFP_MSG_COMPLETED:
                CYLOGI("FFP_MSG_COMPLETED:\n");
                player = (CypressPlayer*)(mp->cypressplayer);

                msg_queue_put(&player->user_callback_msg_queue, &msg);

                break;
            case FFP_MSG_VIDEO_SIZE_CHANGED:
                CYLOGI("FFP_MSG_VIDEO_SIZE_CHANGED: %d, %d\n", msg.arg1, msg.arg2);
                player = (CypressPlayer*)(mp->cypressplayer);
                if (player->mWidth != msg.arg1 || player->mHeight != msg.arg2) {
                    player->mWidth = msg.arg1;
                    player->mHeight = msg.arg2;

                    msg_queue_put(&player->user_callback_msg_queue, &msg);
                }
                break;
            case FFP_MSG_SAR_CHANGED:
                CYLOGI("FFP_MSG_SAR_CHANGED: %d, %d\n", msg.arg1, msg.arg2);
                break;
            case FFP_MSG_VIDEO_RENDERING_START:
                CYLOGI("FFP_MSG_VIDEO_RENDERING_START:\n");
                player = (CypressPlayer*)(mp->cypressplayer);

                msg_queue_put(&player->user_callback_msg_queue, &msg);

                break;
            case FFP_MSG_AUDIO_RENDERING_START:
                CYLOGI("FFP_MSG_AUDIO_RENDERING_START:\n");
                break;
            case FFP_MSG_VIDEO_ROTATION_CHANGED:
                CYLOGI("FFP_MSG_VIDEO_ROTATION_CHANGED: %d\n", msg.arg1);
                break;
            case FFP_MSG_AUDIO_DECODED_START:
                CYLOGI("FFP_MSG_AUDIO_DECODED_START:\n");
                break;
            case FFP_MSG_VIDEO_DECODED_START:
                CYLOGI("FFP_MSG_VIDEO_DECODED_START:\n");
                player = (CypressPlayer*)(mp->cypressplayer);


                break;
            case FFP_MSG_OPEN_INPUT:
                CYLOGI("FFP_MSG_OPEN_INPUT:\n");
                break;
            case FFP_MSG_FIND_STREAM_INFO:
                CYLOGI("FFP_MSG_FIND_STREAM_INFO:\n");
                break;
            case FFP_MSG_COMPONENT_OPEN:
                CYLOGI("FFP_MSG_COMPONENT_OPEN:\n");
                break;
            case FFP_MSG_BUFFERING_START:
                CYLOGI("FFP_MSG_BUFFERING_START:\n");
                break;
            case FFP_MSG_BUFFERING_END:
                CYLOGI("FFP_MSG_BUFFERING_END:\n");
                break;
            case FFP_MSG_BUFFERING_UPDATE:
                CYLOGD("FFP_MSG_BUFFERING_UPDATE: %d, %d", msg.arg1, msg.arg2);
                break;
            case FFP_MSG_BUFFERING_BYTES_UPDATE:
                break;
            case FFP_MSG_BUFFERING_TIME_UPDATE:
                break;
            case FFP_MSG_SEEK_COMPLETE:
                CYLOGI("FFP_MSG_SEEK_COMPLETE:\n");
                player = (CypressPlayer*)(mp->cypressplayer);

                msg_queue_put(&player->user_callback_msg_queue, &msg);

                break;
            case FFP_MSG_ACCURATE_SEEK_COMPLETE:
                CYLOGI("FFP_MSG_ACCURATE_SEEK_COMPLETE:\n");
                break;
            case FFP_MSG_PLAYBACK_STATE_CHANGED:
                CYLOGI("FFP_MSG_PLAYBACK_STATE_CHANGED: %d\n", msg.arg1);
                player = (CypressPlayer*)(mp->cypressplayer);

                switch (msg.arg1) {
                case MP_STATE_PREPARED:
                    ncfence_reset(player->sync_block.stop_fence);
                    ncfence_signal(player->sync_block.prepare_fence);
                    CYLOGI("prepare signal done");
                    break;
                case MP_STATE_STARTED:
                    ncfence_reset(player->sync_block.pause_fence);
                    ncfence_signal(player->sync_block.start_fence);
                    CYLOGI("start signal done");
                    break;
                case MP_STATE_PAUSED:
                    ncfence_reset(player->sync_block.start_fence);
                    ncfence_signal(player->sync_block.pause_fence);
                    CYLOGI("pause signal done");
                    break;
                case MP_STATE_STOPPED:
                    ncfence_reset(player->sync_block.prepare_fence);
                    ncfence_reset(player->sync_block.start_fence);
                    ncfence_reset(player->sync_block.pause_fence);
                    ncfence_signal(player->sync_block.stop_fence);
                    CYLOGI("stop signal done");
                    break;
                case MP_STATE_COMPLETED:
                    ncfence_reset(player->sync_block.start_fence);
                    ncfence_reset(player->sync_block.pause_fence);
                    break;
                }

                msg_queue_put(&player->user_callback_msg_queue, &msg);

                break;
            case FFP_MSG_TIMED_TEXT:
//                if (msg.obj) {
//                    jstring text = (*env)->NewStringUTF(env, (char *)msg.obj);
//                    post_event2(env, weak_thiz, MEDIA_TIMED_TEXT, 0, 0, text);
//                    J4A_DeleteLocalRef__p(env, &text);
//                }
//                else {
//                    post_event2(env, weak_thiz, MEDIA_TIMED_TEXT, 0, 0, NULL);
//                }
                break;
            case FFP_MSG_GET_IMG_STATE:
//                if (msg.obj) {
//                    jstring file_name = (*env)->NewStringUTF(env, (char *)msg.obj);
//                    post_event2(env, weak_thiz, MEDIA_GET_IMG_STATE, msg.arg1, msg.arg2, file_name);
//                    J4A_DeleteLocalRef__p(env, &file_name);
//                }
//                else {
//                    post_event2(env, weak_thiz, MEDIA_GET_IMG_STATE, msg.arg1, msg.arg2, NULL);
//                }
                break;
            case FFP_MSG_VIDEO_SEEK_RENDERING_START:
                CYLOGI("FFP_MSG_VIDEO_SEEK_RENDERING_START:\n");
                //post_event(env, weak_thiz, MEDIA_INFO, MEDIA_INFO_VIDEO_SEEK_RENDERING_START, msg.arg1);
                break;
            case FFP_MSG_AUDIO_SEEK_RENDERING_START:
                CYLOGI("FFP_MSG_AUDIO_SEEK_RENDERING_START:\n");
                //post_event(env, weak_thiz, MEDIA_INFO, MEDIA_INFO_AUDIO_SEEK_RENDERING_START, msg.arg1);
                break;
            default:
                CYLOGI("unknown FFP_MSG_xxx(%d)\n", msg.what);
                break;
        }
        msg_free_res(&msg);
    }
}

static int message_loop(void *arg){


    IjkMediaPlayer *player = (IjkMediaPlayer*) arg;


    message_loop_n(player);

    CYLOGI("message_loop end");

    return 0;
}

static int32_t sync_block_init(CypressPlayer *player) {
    player->sync_block.sync_interface_lock = SDL_CreateMutex();
    if (!player->sync_block.sync_interface_lock) {
        CYLOGE("SDL_CreateMutex failed for sync block lock");
        return -1;
    }

    if (ncfence_init(&player->sync_block.prepare_fence, false) != 0) {
        CYLOGE("ncfence_init failed for sync block prepare fence");
        return -1;
    }

    if (ncfence_init(&player->sync_block.start_fence, false) != 0) {
        CYLOGE("ncfence_init failed for sync block start fence");
        return -1;
    }

    if (ncfence_init(&player->sync_block.pause_fence, false) != 0) {
        CYLOGE("ncfence_init failed for sync block pause fence");
        return -1;
    }

    if (ncfence_init(&player->sync_block.stop_fence, false) != 0) {
        CYLOGE("ncfence_init failed for sync block stop fence");
        return -1;
    }

    if (ncfence_init(&player->sync_block.user_callback_sync_fence, false) != 0) {
        CYLOGE("ncfence_init failed for sync block user callback sync fence");
        return -1;
    }

    return 0;
}

static void sync_block_destroy(CypressPlayer *player) {
    player->sync_block.sync_interface_lock ? SDL_DestroyMutex(player->sync_block.sync_interface_lock) : (void)0;
    player->sync_block.prepare_fence ? ncfence_destroy(&player->sync_block.prepare_fence) : (void)0;
    player->sync_block.start_fence ? ncfence_destroy(&player->sync_block.start_fence) : (void)0;
    player->sync_block.pause_fence ? ncfence_destroy(&player->sync_block.pause_fence) : (void)0;
    player->sync_block.stop_fence ? ncfence_destroy(&player->sync_block.stop_fence) : (void)0;
    player->sync_block.user_callback_sync_fence ? ncfence_destroy(&player->sync_block.user_callback_sync_fence) : (void)0;
    player->sync_block.sync_interface_lock = NULL;
}

static void update_callback_wich_cb_thread_check(CypressPlayer *cypress_player, CypressPlayerCallback callback, void *userdata) {
    int tid = gettid();
    int cb_tid = cypress_player->user_callback_msg_tid;
    CYLOGI("%s: tid %d cb_tid %d", __func__, tid, cb_tid);
    if (tid == cb_tid) {
        cypress_player->mMediaPlayerCallback = callback;
        cypress_player->mCallbackUserData = userdata;
    } else {
        SDL_LockMutex(cypress_player->callback_lock);
        cypress_player->mMediaPlayerCallback = callback;
        cypress_player->mCallbackUserData = userdata;
        SDL_UnlockMutex(cypress_player->callback_lock);
    }
}

CypressPlayer *CypressPlayer_create() {
    CYLOGI("create player enter");
    ijkmp_global_init();

    CypressPlayer *cypress_player = (CypressPlayer *)calloc(1, sizeof(*cypress_player));
    if (!cypress_player) {
        CYLOGE("%s: alloc for cypress_player failed", __func__);
        return NULL;
    }

    cypress_player->mp = ijkmp_create(message_loop);
    if (!cypress_player->mp) {
        CYLOGE("%s: create ijkmp failed", __func__);
        goto fail1;
    }
    cypress_player->mp->cypressplayer = cypress_player;

    cypress_player->mp->ffplayer->vout = SDL_VoutAndroid_CreateForAndroidSurface();
    if (!cypress_player->mp->ffplayer->vout) {
        CYLOGE("%s: create vout failed", __func__);
        goto fail2;
    }

    cypress_player->mp->ffplayer->pipeline = ffpipeline_create_from_android(cypress_player->mp->ffplayer);
    if (!cypress_player->mp->ffplayer->pipeline) {
        CYLOGE("%s: create pipeline failed", __func__);
        goto fail2;
    }
    ffpipeline_set_vout(cypress_player->mp->ffplayer->pipeline, cypress_player->mp->ffplayer->vout);

    if (res_mgr_init(&cypress_player->res_mgr) != 0) {
        CYLOGE("%s: res_mgr_init failed", __func__);
        goto fail2;
    }
    cypress_player->mp->ffplayer->res_mgr = cypress_player->res_mgr;

    if (sync_block_init(cypress_player) != 0) {
        CYLOGE("%s: sync block init failed", __func__);
        // NOTE: we need to clean sync block even though init failed
        goto fail4;
    }

    cypress_player->callback_lock = SDL_CreateMutex();
    if (!cypress_player->callback_lock) {
        CYLOGE("%s: create callback lock failed", __func__);
        goto fail4;
    }

    msg_queue_init(&cypress_player->user_callback_msg_queue);
    msg_queue_start(&cypress_player->user_callback_msg_queue);
    if (SDL_CreateThreadEx(&cypress_player->user_callback_msg_thread, 
        user_callback_msg_fn,
        cypress_player, "user_callback") == NULL) {

        CYLOGE("%s: create user callback msg thread failed", __func__);
        goto fail6;
    }
    CYLOGI("%s: cy %p mp %p ffp %p", __func__, cypress_player, cypress_player->mp, cypress_player->mp->ffplayer);
    msg_queue_put_simple1(&cypress_player->user_callback_msg_queue, FFP_REQ_WAIT_SYNC);
    ncfence_wait(cypress_player->sync_block.user_callback_sync_fence);
    ncfence_reset(cypress_player->sync_block.user_callback_sync_fence);
    CYLOGI("%s: cy %p user cb tid %d", __func__, cypress_player, cypress_player->user_callback_msg_tid);

    return cypress_player;
fail6:
    msg_queue_abort(&cypress_player->user_callback_msg_queue);
    msg_queue_destroy(&cypress_player->user_callback_msg_queue);
fail5:
    SDL_DestroyMutexP(&cypress_player->callback_lock);
fail4:
    sync_block_destroy(cypress_player);
fail3:
    res_mgr_destroy(&cypress_player->res_mgr);
fail2:
    ijkmp_release(&cypress_player->mp);
fail1:
    free(cypress_player);
    cypress_player = NULL;
    return NULL;
}

int CypressPlayer_setDataSourceAndHeaders(CypressPlayer* player, char* path)
{
    if (path == NULL) {
        CYLOGE("invalid null path");
        return -EINVAL;
    }
    int ret = ijkmp_set_data_source(player->mp, path);
    CYLOGI("set data source %s status:%d", path, ret);
    return ret;
}

int CypressPlayer_setDataSourceFd2(CypressPlayer* player, int fd, int64_t offset, int64_t length) {
    if (fd < 0) {
        CYLOGE("invalid fd %d < 0", fd);
        return -EINVAL;
    }

    char uri[128];
    snprintf(uri, sizeof(uri), "fd_offset_length:%d/%" PRIi64 "/%" PRIi64, fd, offset, length);
    int ret = ijkmp_set_data_source(player->mp, uri);
    CYLOGI("set data source fd %s ret %d", uri, ret);
    return ret;
}

int CypressPlayer_setDataSourceFd(CypressPlayer* player, int fd)
{
    if (fd < 0) {
        CYLOGE("invalid fd %d < 0", fd);
        return -EINVAL;
    }

    int64_t offset = 0, length = INT64_MAX;
    struct stat fd_info;
    if (fstat(fd, &fd_info) == 0) {
        length = (fd_info.st_size >= 0 ? fd_info.st_size : length);
        CYLOGI("get datasource fd %d length %" PRIi64, fd, length);
    } else {
        CYLOGE("couldn't get stat for fd %d", fd);
        // not return here, assume that length = INT_MAX
    }
    return CypressPlayer_setDataSourceFd2(player, fd, offset, length);
}

void CypressPlayer_setSurface(CypressPlayer* player, void *surface) {
    LOCK_INTERFACE;
    ijkmp_android_set_surface(player->mp , surface);
    UNLOCK_INTERFACE;
}

void
CypressPlayer_prepareAsync(CypressPlayer* player)
{
    //CYLOGI("prepare async state: %d", player->mp->mp_state);
//    if(player->mp->mp_state != MP_STATE_INITIALIZED)
//        return;
    CYLOGI("prepare async start %p", player);
    LOCK_INTERFACE;
    int ret = ijkmp_prepare_async(player->mp);
    if (ret != 0) {
        CYLOGE("prepare player async: %d", ret);
        UNLOCK_INTERFACE;
        return;
    }
    UNLOCK_INTERFACE;
    CYLOGI("prepare async : %d", ret);
}

int CypressPlayer_prepareSync(CypressPlayer *player) {
    CYLOGI("prepare sync start");

    LOCK_INTERFACE;
    int ret = ijkmp_prepare_async(player->mp);
    CYLOGI("ijkmp_prepare_async : %d", ret);

    if (ret != 0) {
        UNLOCK_INTERFACE;
        CYLOGE("prepare sync failed");
        return ret;
    }

    CYLOGI("wait prepare fence");
    ncfence_wait(player->sync_block.prepare_fence);
    CYLOGI("wait prepare fence done");
    ret = player->has_reported_error;
    UNLOCK_INTERFACE;
    return ret;
}

int CypressPlayer_start(CypressPlayer* player)
{
    LOCK_INTERFACE;
    //CYLOGI("start player state: %d", player->mp->mp_state);
    int ret = ijkmp_start(player->mp);
    CYLOGI("start player : %d", ret);
    if (ret != 0) {
        CYLOGE("start player failed: %d", ret);
        UNLOCK_INTERFACE;
        return ret;
    }
    CYLOGI("wait start fence");
    ncfence_wait(player->sync_block.start_fence);
    CYLOGI("wait start fence done");
    ret = player->has_reported_error;
    UNLOCK_INTERFACE;
    return ret;
}

int CypressPlayer_stop(CypressPlayer* player)
{
    LOCK_INTERFACE;
    CYLOGI("CypressPlayer_stop");
    int ret = ijkmp_stop(player->mp);

    if (ret != 0) {
        CYLOGE("stop player failed: %d", ret);
        UNLOCK_INTERFACE;
        return ret;
    }

    CYLOGI("wait stop fence");
    ncfence_wait(player->sync_block.stop_fence);
    CYLOGI("wait stop fence done");

    ret = player->has_reported_error;
    UNLOCK_INTERFACE;

    return ret;
}

int CypressPlayer_pause(CypressPlayer* player)
{
    LOCK_INTERFACE;
    int ret = ijkmp_pause(player->mp);

    if (ret != 0) {
        CYLOGE("pause player failed: %d", ret);
        UNLOCK_INTERFACE;
        return ret;
    }

    CYLOGI("wait pause fence");
    ncfence_wait(player->sync_block.pause_fence);
    CYLOGI("wait pause fence done");

    ret = player->has_reported_error;
    UNLOCK_INTERFACE;

    return ret;
}

void
CypressPlayer_seekTo(CypressPlayer* player, long msec)
{
    LOCK_INTERFACE;
    ijkmp_seek_to(player->mp, msec);
    UNLOCK_INTERFACE;
}

bool
CypressPlayer_isPlaying(CypressPlayer* player)
{
    int retval;
    retval = ijkmp_is_playing(player->mp);
    return retval;
}

long
CypressPlayer_getCurrentPosition(CypressPlayer* player)
{
    long retval = ijkmp_get_current_position(player->mp);
    // CYLOGI("current position: %ld", retval);
    return retval;
}

long
CypressPlayer_getDuration(CypressPlayer* player)
{
    long retval = ijkmp_get_duration(player->mp);
    CYLOGI("duration: %ld", retval);
    return retval;
}

void
CypressPlayer_release(CypressPlayer* player)
{
    LOCK_INTERFACE;
    CYLOGI("release player user_cb tid %d curr tid %d", player->user_callback_msg_tid, gettid());
    // ijkmp_android_set_surface(player->mp, NULL);
    ijkmp_release(&player->mp);
    res_mgr_destroy(&player->res_mgr);
    msg_queue_put_simple2(&player->user_callback_msg_queue, FFP_MSG_PLAYBACK_STATE_CHANGED, MP_STATE_END);
    msg_queue_put_simple1(&player->user_callback_msg_queue, FFP_REQ_STOP);
    if (player->user_callback_msg_tid != gettid()) {
        CYLOGI("%s: wait user_callback start", __func__);
        SDL_WaitThread(&player->user_callback_msg_thread, NULL);
        CYLOGI("%s: wait user_callback end", __func__);
        msg_queue_abort(&player->user_callback_msg_queue);
        msg_queue_destroy(&player->user_callback_msg_queue);
    } else {
        CYLOGE("dead lock check, couldn't call release in callback");
        void *backtrace[100] = {NULL};
        capture_backtrace(backtrace, 100);

        for (int i = 0; i < 100; i++) {
            if (!backtrace[i]) break;
            BacktraceInfo info = backtrace_info(backtrace[i]);
            CYLOGE("trace level %d sym %s f_sym %s addr %p", i, info.sym, info.f_sym, info.relative_addr);
        }

        assert(false);
        abort();
    }
    UNLOCK_INTERFACE;
    SDL_DestroyMutex(player->callback_lock);
    sync_block_destroy(player);
    free(player);
}


int
CypressPlayer_reset(CypressPlayer* player)
{
    LOCK_INTERFACE;
    CYLOGI("reset player %p mp %p user_cb tid %d curr tid %d", player, player->mp, player->user_callback_msg_tid, gettid());
    CYLOGI("release player in %s", __func__);
    // ijkmp_android_set_surface(player->mp, NULL);
    ijkmp_release(&player->mp);

    CYLOGI("release player finish in %s", __func__);

    CYLOGI("%s: wait user_callback start", __func__);
    if (player->user_callback_msg_tid != gettid()) {
        msg_queue_put_simple1(&player->user_callback_msg_queue, FFP_REQ_WAIT_SYNC);
        ncfence_wait(player->sync_block.user_callback_sync_fence);
        ncfence_reset(player->sync_block.user_callback_sync_fence);
    } else {
        msg_queue_flush(&player->user_callback_msg_queue);
    }
    CYLOGI("%s: wait user_callback end", __func__);

    CYLOGI("%s: clean callback", __func__);
    update_callback_wich_cb_thread_check(player, (CypressPlayerCallback) {}, NULL);

    IjkMediaPlayer *mp = ijkmp_create(message_loop);
    CYLOGI("create mp: %p", mp);
    if (!mp)
        goto fail;

    mp->ffplayer->vout = SDL_VoutAndroid_CreateForAndroidSurface();
    if (!mp->ffplayer->vout)
        goto fail;

    mp->ffplayer->pipeline = ffpipeline_create_from_android(mp->ffplayer);
    if (!mp->ffplayer->pipeline)
        goto fail;

    mp->ffplayer->res_mgr = player->res_mgr;
    ffpipeline_set_vout(mp->ffplayer->pipeline, mp->ffplayer->vout);

    player->mp = mp;
    mp->cypressplayer = player;
    player->mWidth = 0;
    player->mHeight = 0;
    player->has_reported_error = 0;
    ncfence_reset(player->sync_block.prepare_fence);
    ncfence_reset(player->sync_block.start_fence);
    ncfence_reset(player->sync_block.pause_fence);
    ncfence_reset(player->sync_block.stop_fence);
    CYLOGI("reset player done");
    UNLOCK_INTERFACE;
    return 0;
fail:
    CYLOGI("reset player failed");
    UNLOCK_INTERFACE;
    return -1;
}

void
CypressPlayer_setLoopCount(CypressPlayer* player, int loop_count)
{
    ijkmp_set_loop(player->mp, loop_count);
}

int
CypressPlayer_getLoopCount(CypressPlayer* player)
{
    int loop_count = ijkmp_get_loop(player->mp);
    return loop_count;
}

// int
// CypressPlayer_getStreamSelected(CypressPlayer* player, int stream_type) {
//     switch (stream_type) {
//         case MEDIA_TRACK_TYPE_AUDIO: return ijkmp_get_property_int64(player->mp, FFP_PROP_INT64_SELECTED_AUDIO_STREAM, -1);
//         case MEDIA_TRACK_TYPE_VIDEO: return ijkmp_get_property_int64(player->mp, FFP_PROP_INT64_SELECTED_VIDEO_STREAM, -1);
//         case MEDIA_TRACK_TYPE_SUBTITLE: return ijkmp_get_property_int64(player->mp, FFP_PROP_INT64_SELECTED_TIMEDTEXT_STREAM, -1);
//         default: return -1;
//     }
//     return -1;
// }

void
CypressPlayer_setStreamSelected(CypressPlayer* player, int stream, bool selected)
{
    UNLOCK_INTERFACE;
    ijkmp_set_stream_selected(player->mp, stream, selected);
    UNLOCK_INTERFACE;
}

int CypressPlayer_setVolume(CypressPlayer* player, float leftVolume, float rightVolume)
{
    if (leftVolume < 0 || leftVolume > 1 || rightVolume < 0 || rightVolume > 1) return -EINVAL;
    ijkmp_android_set_volume(player->mp, leftVolume, rightVolume);
    return 0;
}

void CypressPlayer_getVolume(CypressPlayer* player, float *volume, int size)
{
    ijkmp_android_get_volume(player->mp, volume, size);
}

int
CypressPlayer_getAudioSessionId(CypressPlayer* player)
{
    return 0;
}

void
CypressPlayer_setOption(CypressPlayer* player,int category, char* name, char* value)
{
    ijkmp_set_option(player->mp, category, name, value);
}

void
CypressPlayer_setOptionLong(CypressPlayer* player,int category, char* name, long value)
{
    ijkmp_set_option_int(player->mp, category, name, value);
}


char*
CypressPlayer_getVideoCodecInfo(CypressPlayer* player)
{
    return "video";
}

char*
CypressPlayer_getAudioCodecInfo(CypressPlayer* player)
{
    return "audio";
}

// CypressMediaMeta *
// CypressPlayer_getMediaMeta(CypressPlayer* player)
// {
//     IjkMediaMeta *meta = ijkmp_get_meta_l(player->mp);
//     if (meta == NULL) {
//         return NULL;
//     }
//     ijkmeta_lock(meta);
// 
// 
//     size_t count = ijkmeta_get_children_count_l(meta);
//     CypressMediaMeta *ret = calloc(1, sizeof(CypressMediaMeta) + sizeof(CypressTrackMeta) * count);
//     if (ret == NULL) {
//         ijkmeta_unlock(meta);
//         return NULL;
//     }
//     ret->track_count = count;
//     for (int i = 0; i < count; i++) {
//         IjkMediaMeta *track_meta = ijkmeta_get_child_l(meta, i);
//         if (!track_meta) {
//             ret->tracks_meta[i].track_type = MEDIA_TRACK_TYPE_UNKNOWN;
//             ret->tracks_meta[i].track_index = i;
//             continue;
//         }
//         ret->tracks_meta[i].track_index = i;
//         const char *track_type = ijkmeta_get_string_l(track_meta, IJKM_KEY_TYPE);
//         if (!strcmp(track_type, IJKM_VAL_TYPE__AUDIO)) {
//             ret->tracks_meta[i].track_type = MEDIA_TRACK_TYPE_AUDIO;
//         } else if (!strcmp(track_type, IJKM_VAL_TYPE__VIDEO)) {
//             ret->tracks_meta[i].track_type = MEDIA_TRACK_TYPE_VIDEO;
//         } else if (!strcmp(track_type, IJKM_VAL_TYPE__TIMEDTEXT)) {
//             ret->tracks_meta[i].track_type = MEDIA_TRACK_TYPE_SUBTITLE;
//         } else {
//             ret->tracks_meta[i].track_type = MEDIA_TRACK_TYPE_UNKNOWN;
//         }
//     }
//     ijkmeta_unlock(meta);
//     return ret;
// }

// void CypressPlayer_releaseMediaMeta(CypressPlayer *player, CypressMediaMeta *meta) {
//     free(meta);
// }

void CypressPlayer_setCallback(CypressPlayer* cypress_player, CypressPlayerCallback callback, void *userdata) {
    CYLOGI("%s: set callback ud %p", __func__, userdata);
    update_callback_wich_cb_thread_check(cypress_player, callback, userdata);
}

int CypressPlayer_getState(CypressPlayer *mp) {
    return mp->mp->mp_state;
}

void CypressPlayer_getVideoSize(CypressPlayer *mp, int *width, int *height) {
    if (width) {
        *width = mp->mWidth;
    }
    if (height) {
        *height = mp->mHeight;
    }
}


int CypressPlayer_getPlayerNo(CypressPlayer* mp){
   return mp->player_no;
}

int CypressPlayer_setPlaySpeed(CypressPlayer* player, float speed) {
    if (speed <= 0) return -EINVAL;
    ijkmp_set_property_float(player->mp, FFP_PROP_FLOAT_PLAYBACK_RATE, speed);
    return 0;
}

float
CypressPlayer_getPlaySpeed(CypressPlayer* player){
    float value = ijkmp_get_property_float(player->mp, FFP_PROP_FLOAT_PLAYBACK_RATE, 1.0);
    return value;
}

void CypressPlayer_setDummyWindow(CypressPlayer* player, void* dummy_window) {
    res_mgr_set_dummy_window(player->res_mgr, dummy_window);
}

void CypressPlayer_setLogCallback(CypressPlayerOnLog log_cb) {
    set_cylog_callback(log_cb);
}

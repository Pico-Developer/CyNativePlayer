/*****************************************************************************
 * ijksdl_aout_android_audiotrack.c
 *****************************************************************************
 *
 * Copyright (c) 2013 Bilibili
 * copyright (c) 2013 Zhang Rui <bbcallen@gmail.com>
 *
 * This file is part of ijkPlayer.
 *
 * ijkPlayer is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * ijkPlayer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with ijkPlayer; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "ijksdl_aout_android_audiotrack.h"

#include <stdbool.h>
#include <assert.h>
#include <jni.h>
#include "../ijksdl_inc_internal.h"
#include "../ijksdl_thread.h"
#include "../ijksdl_aout_internal.h"
#include "ijksdl_android_jni.h"
#include "android_audiotrack.h"
#include "ijksdl_aout_android_audiotrack_latency_estimator.h"

#ifdef SDLTRACE
#undef SDLTRACE
#define SDLTRACE(...)
//#define SDLTRACE CYLOGE
#endif

static SDL_Class g_audiotrack_class = {
    .name = "AudioTrack",
};

typedef struct SDL_Aout_Opaque {
    SDL_cond *wakeup_cond;
    SDL_mutex *wakeup_mutex;

    SDL_AudioSpec spec;
    SDL_Android_AudioTrack* atrack;
    uint8_t *buffer;
    int buffer_size;

    volatile bool need_flush;
    volatile bool pause_on;
    volatile bool abort_request;

    volatile bool need_set_volume;
    volatile float left_volume;
    volatile float right_volume;

    SDL_Thread *audio_tid;
    SDL_Thread _audio_tid;

    int audio_session_id;

    volatile float speed;
    volatile bool speed_changed;

    SDL_mutex *atrack_mutex;

    AoutLatencyEstimator *lat;
} SDL_Aout_Opaque;

static inline int sdl_format_bits(int fmt) {
    switch (fmt) {
    case AUDIO_U8: return 8;
    case AUDIO_S16SYS: return 16;
    case AUDIO_S32SYS: return 32;
    case AUDIO_F32SYS: return 32;
    default: return 16;
    }
}

static int aout_recreate_audiotrack_l(JNIEnv *env, SDL_Aout_Opaque *opaque, SDL_Android_AudioTrack **p_atrack, bool apply_pause_or_play)
{
    if (opaque->lat) aout_latency_estimator_on_flush(opaque->lat);

    SDL_Android_AudioTrack_free(env, opaque->atrack);
    opaque->atrack = SDL_Android_AudioTrack_new_from_sdl_spec(env, &opaque->spec);
    *p_atrack = opaque->atrack;
    if (!*p_atrack) {
        CYLOGE("audiotrack recreate failed");
        return -1;
    }

    SDL_Android_AudioTrack_set_volume(env, *p_atrack, opaque->left_volume, opaque->right_volume);
    SDL_Android_AudioTrack_setSpeed(env, *p_atrack, opaque->speed);
    if (opaque->lat) aout_latency_estimator_set_playback_speed(opaque->lat, opaque->speed);

    int bits_per_sample = sdl_format_bits((int)opaque->spec.format);
    if (opaque->lat)
        aout_latency_estimator_on_recreate(opaque->lat, *p_atrack, opaque->spec.channels, opaque->spec.freq, bits_per_sample);

    if (apply_pause_or_play) {
        if (opaque->pause_on) {
            SDL_Android_AudioTrack_pause(env, *p_atrack);
            if (opaque->lat) aout_latency_estimator_on_pause(opaque->lat, true);
        } else {
            SDL_Android_AudioTrack_play(env, *p_atrack);
            if (opaque->lat) aout_latency_estimator_on_pause(opaque->lat, false);
        }
    }

    return 0;
}

static int aout_thread_n(JNIEnv *env, SDL_Aout *aout)
{
    SDL_Aout_Opaque *opaque = aout->opaque;
    SDL_Android_AudioTrack *atrack = opaque->atrack;
    SDL_AudioCallback audio_cblk = opaque->spec.callback;
    void *userdata = opaque->spec.userdata;
    uint8_t *buffer = opaque->buffer;
    int copy_size = 256;

    assert(atrack);
    assert(buffer);

    SDL_SetThreadPriority(SDL_THREAD_PRIORITY_HIGH);

    if (!opaque->abort_request && !opaque->pause_on) {
        SDL_LockMutex(opaque->atrack_mutex);
        if (atrack) {
            SDL_Android_AudioTrack_play(env, atrack);
            if (opaque->lat) aout_latency_estimator_on_pause(opaque->lat, false);
        }
        SDL_UnlockMutex(opaque->atrack_mutex);
    }

    while (!opaque->abort_request) {
        SDL_LockMutex(opaque->wakeup_mutex);
        if (!opaque->abort_request && opaque->pause_on) {
            SDL_LockMutex(opaque->atrack_mutex);
            if (atrack) {
                SDL_Android_AudioTrack_pause(env, atrack);
                if (opaque->lat) aout_latency_estimator_on_pause(opaque->lat, true);
            }
            SDL_UnlockMutex(opaque->atrack_mutex);
            while (!opaque->abort_request && opaque->pause_on) {
                SDL_CondWaitTimeout(opaque->wakeup_cond, opaque->wakeup_mutex, 1000);
            }
            if (!opaque->abort_request && !opaque->pause_on) {
                if (opaque->need_flush) {
                    opaque->need_flush = 0;
                    SDL_LockMutex(opaque->atrack_mutex);
                    aout_recreate_audiotrack_l(env, opaque, &atrack, false);
                    SDL_UnlockMutex(opaque->atrack_mutex);
                }
                SDL_LockMutex(opaque->atrack_mutex);
                if (atrack) {
                    SDL_Android_AudioTrack_play(env, atrack);
                    if (opaque->lat) aout_latency_estimator_on_pause(opaque->lat, false);
                }
                SDL_UnlockMutex(opaque->atrack_mutex);
            }
        }
        if (opaque->need_flush) {
            opaque->need_flush = 0;
            SDL_LockMutex(opaque->atrack_mutex);
            aout_recreate_audiotrack_l(env, opaque, &atrack, true);
            SDL_UnlockMutex(opaque->atrack_mutex);
        }
        if (opaque->need_set_volume) {
            opaque->need_set_volume = 0;
            SDL_LockMutex(opaque->atrack_mutex);
            if (atrack) SDL_Android_AudioTrack_set_volume(env, atrack, opaque->left_volume, opaque->right_volume);
            SDL_UnlockMutex(opaque->atrack_mutex);
        }
        if (opaque->speed_changed) {
            opaque->speed_changed = 0;
            SDL_LockMutex(opaque->atrack_mutex);
            if (atrack) {
                SDL_Android_AudioTrack_setSpeed(env, atrack, opaque->speed);
                if (opaque->lat) aout_latency_estimator_set_playback_speed(opaque->lat, opaque->speed);
            }
            SDL_UnlockMutex(opaque->atrack_mutex);
        }
        SDL_UnlockMutex(opaque->wakeup_mutex);

        audio_cblk(userdata, buffer, copy_size);
        if (opaque->need_flush) {
            opaque->need_flush = false;
            SDL_LockMutex(opaque->atrack_mutex);
            aout_recreate_audiotrack_l(env, opaque, &atrack, true);
            SDL_UnlockMutex(opaque->atrack_mutex);
        }

        if (opaque->need_flush) {
            opaque->need_flush = 0;
            SDL_LockMutex(opaque->atrack_mutex);
            aout_recreate_audiotrack_l(env, opaque, &atrack, true);
            SDL_UnlockMutex(opaque->atrack_mutex);
        } else {
            int written = 0;
            SDL_LockMutex(opaque->atrack_mutex);
            if (atrack) written = SDL_Android_AudioTrack_write(env, atrack, buffer, copy_size);
            SDL_UnlockMutex(opaque->atrack_mutex);
            if (opaque->lat) aout_latency_estimator_on_write(opaque->lat, written);
            if (written != copy_size) {
                CYLOGW("AudioTrack: not all data copied %d/%d", (int)written, (int)copy_size);
            }
        }

        // TODO: 1 if callback return -1 or 0
    }

    SDL_LockMutex(opaque->atrack_mutex);
    SDL_Android_AudioTrack_free(env, atrack);
    SDL_UnlockMutex(opaque->atrack_mutex);
    return 0;
}

static int aout_thread(void *arg)
{
    CYLOGI("aout thread [%d]", gettid());
    SDL_Aout *aout = arg;
    // SDL_Aout_Opaque *opaque = aout->opaque;
    JNIEnv *env = NULL;

    if (JNI_OK != SDL_JNI_SetupThreadEnv(&env)) {
        CYLOGE("aout_thread: SDL_AndroidJni_SetupEnv: failed");
        return -1;
    }

    return aout_thread_n(env, aout);
}


static int aout_open_audio_n(JNIEnv *env, SDL_Aout *aout, const SDL_AudioSpec *desired, SDL_AudioSpec *obtained)
{
    assert(desired);
    SDL_Aout_Opaque *opaque = aout->opaque;

    opaque->spec = *desired;
    opaque->atrack = SDL_Android_AudioTrack_new_from_sdl_spec(env, desired);
    if (!opaque->atrack) {
        CYLOGE("aout_open_audio_n: failed to new AudioTrcak()");
        return -1;
    }

    opaque->buffer_size = SDL_Android_AudioTrack_get_min_buffer_size(opaque->atrack);
    if (opaque->buffer_size <= 0) {
        CYLOGE("aout_open_audio_n: failed to getMinBufferSize()");
        SDL_Android_AudioTrack_free(env, opaque->atrack);
        opaque->atrack = NULL;
        return -1;
    }

    opaque->buffer = malloc(opaque->buffer_size);
    if (!opaque->buffer) {
        CYLOGE("aout_open_audio_n: failed to allocate buffer");
        SDL_Android_AudioTrack_free(env, opaque->atrack);
        opaque->atrack = NULL;
        return -1;
    }

    SDL_AudioSpec target_spec = {0};
    SDL_Android_AudioTrack_get_target_spec(opaque->atrack, &target_spec);
    if (obtained) {
        *obtained = target_spec;
        SDLTRACE("audio target format fmt:0x%x, channel:0x%x", (int)obtained->format, (int)obtained->channels);
    }

    int bits_per_sample = sdl_format_bits((int)target_spec.format);
    int target_channels = target_spec.channels ? target_spec.channels : desired->channels;
    int target_sample_rate = target_spec.freq ? target_spec.freq : desired->freq;
    opaque->lat = aout_latency_estimator_create(opaque->atrack, target_channels, target_sample_rate, bits_per_sample);
    if (opaque->lat) {
        aout_latency_estimator_set_min_interval_ns(opaque->lat, 10000000000LL);
    }

    SDL_LockMutex(opaque->atrack_mutex);
    SDL_Android_AudioTrack* __atrack = opaque->atrack;
    if (__atrack) opaque->audio_session_id = SDL_Android_AudioTrack_getAudioSessionId(env, __atrack);
    SDL_UnlockMutex(opaque->atrack_mutex);
    CYLOGI("audio_session_id = %d\n", opaque->audio_session_id);

    opaque->pause_on = 1;
    opaque->abort_request = 0;
    opaque->audio_tid = SDL_CreateThreadEx(&opaque->_audio_tid, aout_thread, aout, "ff_aout_android");
    if (!opaque->audio_tid) {
        CYLOGE("aout_open_audio_n: failed to create audio thread");
        SDL_Android_AudioTrack_free(env, opaque->atrack);
        opaque->atrack = NULL;
        aout_latency_estimator_destroy(&opaque->lat);
        return -1;
    }

    return 0;
}

static int aout_open_audio(SDL_Aout *aout, const SDL_AudioSpec *desired, SDL_AudioSpec *obtained)
{
    // SDL_Aout_Opaque *opaque = aout->opaque;
    JNIEnv *env = NULL;
    if (JNI_OK != SDL_JNI_SetupThreadEnv(&env)) {
        CYLOGE("aout_open_audio: AttachCurrentThread: failed");
        return -1;
    }

    return aout_open_audio_n(env, aout, desired, obtained);
}

static void aout_pause_audio(SDL_Aout *aout, int pause_on)
{
    SDL_Aout_Opaque *opaque = aout->opaque;

    SDL_LockMutex(opaque->wakeup_mutex);
    SDLTRACE("aout_pause_audio(%d)", pause_on);
    opaque->pause_on = pause_on;
    if (!pause_on)
        SDL_CondSignal(opaque->wakeup_cond);
    SDL_UnlockMutex(opaque->wakeup_mutex);
}

static void aout_flush_audio(SDL_Aout *aout)
{
    SDL_Aout_Opaque *opaque = aout->opaque;
    SDL_LockMutex(opaque->wakeup_mutex);
    SDLTRACE("aout_flush_audio()");
    opaque->need_flush = 1;
    SDL_CondSignal(opaque->wakeup_cond);
    SDL_UnlockMutex(opaque->wakeup_mutex);
}

static void aout_set_volume(SDL_Aout *aout, float left_volume, float right_volume)
{
    SDL_Aout_Opaque *opaque = aout->opaque;
    SDL_LockMutex(opaque->wakeup_mutex);
    SDLTRACE("aout_flush_audio()");
    opaque->left_volume = left_volume;
    opaque->right_volume = right_volume;
    opaque->need_set_volume = 1;
    SDL_CondSignal(opaque->wakeup_cond);
    SDL_UnlockMutex(opaque->wakeup_mutex);
}

static void aout_close_audio(SDL_Aout *aout)
{
    SDL_Aout_Opaque *opaque = aout->opaque;

    SDL_LockMutex(opaque->wakeup_mutex);
    opaque->abort_request = true;
    SDL_CondSignal(opaque->wakeup_cond);
    SDL_UnlockMutex(opaque->wakeup_mutex);

    // when open audio failed before create thread, the audio_tid was null
    if (opaque->audio_tid) {
        SDL_WaitThread(opaque->audio_tid, NULL);
        opaque->audio_tid = NULL;
    }
    aout_latency_estimator_destroy(&opaque->lat);

    opaque->audio_tid = NULL;
}

static int aout_get_audio_session_id(SDL_Aout *aout)
{
    SDL_Aout_Opaque *opaque = aout->opaque;

    return opaque->audio_session_id;
}

static void aout_free_l(SDL_Aout *aout)
{
    if (!aout)
        return;

    aout_close_audio(aout);

    SDL_Aout_Opaque *opaque = aout->opaque;
    if (opaque) {
        free(opaque->buffer);
        opaque->buffer = NULL;
        opaque->buffer_size = 0;

        SDL_DestroyCond(opaque->wakeup_cond);
        SDL_DestroyMutex(opaque->wakeup_mutex);
        SDL_DestroyMutex(opaque->atrack_mutex);
        aout_latency_estimator_destroy(&opaque->lat);
    }

    SDL_Aout_FreeInternal(aout);
}

static void func_set_playback_rate(SDL_Aout *aout, float speed)
{
    if (!aout)
        return;

    SDL_Aout_Opaque *opaque = aout->opaque;
    SDL_LockMutex(opaque->wakeup_mutex);
    SDLTRACE("%s %f", __func__, (double)speed);
    opaque->speed = speed;
    opaque->speed_changed = 1;
    SDL_CondSignal(opaque->wakeup_cond);
    SDL_UnlockMutex(opaque->wakeup_mutex);
}

static double func_get_time_stamp(SDL_Aout *aout)
{
    if (!aout)
        return 0;

    JNIEnv *env = NULL;
    if (JNI_OK != SDL_JNI_SetupThreadEnv(&env)) {
        CYLOGE("aout_open_audio: AttachCurrentThread: failed");
        return -1;
    }

    SDL_Aout_Opaque *opaque = aout->opaque;

    int64_t frame_position;
    int64_t nano_time;
    SDL_LockMutex(opaque->atrack_mutex);
    SDL_Android_AudioTrack* atrack = opaque->atrack;
    if (atrack) SDL_Android_AudioTrack_getTimestamp(env, atrack, &frame_position, &nano_time);
    SDL_UnlockMutex(opaque->atrack_mutex);
    return frame_position;
}

static double func_get_latency(SDL_Aout *aout)
{
    if (!aout)
        return 0.0;

    JNIEnv *env = NULL;
    if (JNI_OK != SDL_JNI_SetupThreadEnv(&env)) {
        CYLOGE("aout_open_audio: AttachCurrentThread: failed");
        return -1;
    }

    SDL_Aout_Opaque *opaque = aout->opaque;
    if (!opaque || !opaque->lat)
        return 0.0;

    SDL_LockMutex(opaque->atrack_mutex);
    double latency = aout_latency_estimator_get_latency(opaque->lat, env);
    SDL_UnlockMutex(opaque->atrack_mutex);

    // CYLOGI("latency %lf", latency);
    return latency;
}

SDL_Aout *SDL_AoutAndroid_CreateForAudioTrack()
{
    SDL_Aout *aout = SDL_Aout_CreateInternal(sizeof(SDL_Aout_Opaque));
    if (!aout)
        return NULL;

    SDL_Aout_Opaque *opaque = aout->opaque;
    opaque->wakeup_cond  = SDL_CreateCond();
    opaque->wakeup_mutex = SDL_CreateMutex();
    opaque->atrack_mutex = SDL_CreateMutex();
    opaque->speed        = 1.0f;

    aout->opaque_class = &g_audiotrack_class;
    aout->free_l       = aout_free_l;
    aout->open_audio   = aout_open_audio;
    aout->pause_audio  = aout_pause_audio;
    aout->flush_audio  = aout_flush_audio;
    aout->set_volume   = aout_set_volume;
    aout->close_audio  = aout_close_audio;
    aout->func_get_audio_session_id = aout_get_audio_session_id;
    aout->func_set_playback_rate    = func_set_playback_rate;
    aout->func_get_time_stamp       = func_get_time_stamp;
    aout->func_get_latency_seconds = func_get_latency;

    return aout;
}

bool SDL_AoutAndroid_IsObjectOfAudioTrack(SDL_Aout *aout)
{
    if (aout)
        return false;

    return aout->opaque_class == &g_audiotrack_class;
}

void SDL_Init_AoutAndroid(JNIEnv *env)
{

}
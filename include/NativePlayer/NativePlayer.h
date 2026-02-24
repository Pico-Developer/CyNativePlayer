
/*
 * Copyright (c) 2026 ByteDance Ltd. and/or its affiliates
 * SPDX-License-Identifier: LGPL-2.1-only
 */

/**
 * @note any api function that has not CYPRESS_PLAYER_API prefix shouldn't be used
 */

#ifndef NATIVEPLAYER_NATIVEPLAYER_H
#define NATIVEPLAYER_NATIVEPLAYER_H

#include <stdbool.h>
#include <stdint.h>
//#include <jni.h>

#define CYPRESS_PLAYER_API __attribute__((visibility("default")))
#define CypressPlayer_setANativeWindow CypressPlayer_setSurface

typedef struct CypressPlayer CypressPlayer;

typedef void (*CypressPlayerOnPrepared)(CypressPlayer *player, void *userdata);
typedef void (*CypressPlayerOnStarted)(CypressPlayer *player, void *userdata);
typedef void (*CypressPlayerOnSeekComplete)(CypressPlayer *player, void *userdata);
typedef void (*CypressPlayerOnVideoSizeChanged)(CypressPlayer *player, void *userdata, int width, int height);
typedef void (*CypressPlayerOnError)(CypressPlayer *player, void *userdata, int error);
typedef void (*CypressPlayerOnComplete)(CypressPlayer *player, void *userdata);
typedef void (*CypressPlayerOnPlaybackStateChanged)(CypressPlayer *player, void *userdata, int state);
typedef void (*CypressPlayerOnRenderedFirstFrame)(CypressPlayer *player, void *userdata);
typedef void (*CypressPlayerOnLog)(int level, const char *tag, const char *msg);

// typedef void (*CypressPlayerOnMediaTransition)(CypressPlayer *player, void *userdata, int media_idx);
// typedef void (*CypressPlayerOnMeta)(CypressPlayer *player, void *userdata, const CypressMediaMeta *mediainfo);

typedef struct CypressPlayerCallback {
    CypressPlayerOnPrepared onPrepared;
    CypressPlayerOnStarted onStarted;
    CypressPlayerOnSeekComplete onSeekComplete;
    CypressPlayerOnVideoSizeChanged onVideoSizeChanged;
    CypressPlayerOnError onError;
    CypressPlayerOnComplete onComplete;
    CypressPlayerOnPlaybackStateChanged onPlaybackStateChanged;
    CypressPlayerOnRenderedFirstFrame onRenderedFirstFrame;
    // CypressPlayerOnMediaTransition onMediaTransition;
    // CypressPlayerOnMeta onMeta;
} CypressPlayerCallback;

// ----
#define CYPRESS_PLAYER_LOG_LEVEL_ERROR 0
#define CYPRESS_PLAYER_LOG_LEVEL_WARN 1
#define CYPRESS_PLAYER_LOG_LEVEL_INFO 2
#define CYPRESS_PLAYER_LOG_LEVEL_DEBUG 3
#define CYPRESS_PLAYER_LOG_LEVEL_VERBOSE 4


// ----
#define CYPRESS_PLAYER_STATE_IDLE               0
#define CYPRESS_PLAYER_STATE_INITIALIZED        1
#define CYPRESS_PLAYER_STATE_ASYNC_PREPARING    2
#define CYPRESS_PLAYER_STATE_PREPARED           3
#define CYPRESS_PLAYER_STATE_STARTED            4
#define CYPRESS_PLAYER_STATE_PAUSED             5
#define CYPRESS_PLAYER_STATE_COMPLETED          6
#define CYPRESS_PLAYER_STATE_STOPPED            7
#define CYPRESS_PLAYER_STATE_ERROR              8
#define CYPRESS_PLAYER_STATE_END                9


// -----
// error code base
#define CYPRESS_PLAYER_ERROR_BASE                       -90000
// unknown error
#define CYPRESS_PLAYER_ERROR_UNKNOWN                    (CYPRESS_PLAYER_ERROR_BASE - 1)

// no enough memory
#define CYPRESS_PLAYER_ERROR_NOMEM                      (CYPRESS_PLAYER_ERROR_BASE - 1001)
// invalid argument
#define CYPRESS_PLAYER_ERROR_INVALID_ARG                (CYPRESS_PLAYER_ERROR_BASE - 1002)
// invalid data
#define CYPRESS_PLAYER_ERROR_INVALID_DATA               (CYPRESS_PLAYER_ERROR_BASE - 1003)
// system error, for example create thread failed
#define CYPRESS_PLAYER_ERROR_SYSTEM_ERROR               (CYPRESS_PLAYER_ERROR_BASE - 1004)

// error code about IO
#define CYPRESS_PLAYER_ERROR_IO                         (CYPRESS_PLAYER_ERROR_BASE - 2000)
// file access failed, may be permission problem
#define CYPRESS_PLAYER_ERROR_IO_FILE_ACCESS_FAILED      (CYPRESS_PLAYER_ERROR_IO - 101)
// not found file
#define CYPRESS_PLAYER_ERROR_IO_FILE_NOT_FOUND          (CYPRESS_PLAYER_ERROR_IO - 102)

// error about HTTP
#define CYPRESS_PLAYER_ERROR_IO_HTTP                    (CYPRESS_PLAYER_ERROR_IO - 200)
// http code 3xx
#define CYPRESS_PLAYER_ERROR_IO_HTTP_REDIRECT           (CYPRESS_PLAYER_ERROR_IO_HTTP - 1)
// http code 400
#define CYPRESS_PLAYER_ERROR_IO_HTTP_BAD_REQUEST        (CYPRESS_PLAYER_ERROR_IO_HTTP - 2)
// http code 401
#define CYPRESS_PLAYER_ERROR_IO_HTTP_UNAUTHORIZED       (CYPRESS_PLAYER_ERROR_IO_HTTP - 3)
// http code 403
#define CYPRESS_PLAYER_ERROR_IO_HTTP_FORBIDDEN          (CYPRESS_PLAYER_ERROR_IO_HTTP - 4)
// http code 404
#define CYPRESS_PLAYER_ERROR_IO_HTTP_NOT_FOUND          (CYPRESS_PLAYER_ERROR_IO_HTTP - 5)
// http connect timeout
#define CYPRESS_PLAYER_ERROR_IO_HTTP_TIMEOUT            (CYPRESS_PLAYER_ERROR_IO_HTTP - 6)
// http code 4xx
#define CYPRESS_PLAYER_ERROR_IO_HTTP_OTHER_4XX          (CYPRESS_PLAYER_ERROR_IO_HTTP - 7)
// http code 5xx
#define CYPRESS_PLAYER_ERROR_IO_HTTP_SERVER_ERROR       (CYPRESS_PLAYER_ERROR_IO_HTTP - 8)
// http user interrupt
#define CYPRESS_PLAYER_ERROR_IO_HTTP_USERINTERRUPT      (CYPRESS_PLAYER_ERROR_IO_HTTP - 9)

// protocol not found
#define CYPRESS_PLAYER_ERROR_IO_PROTOCOL_NOT_FOUND      (CYPRESS_PLAYER_ERROR_IO - 301)

// error about track
#define CYPRESS_PLAYER_ERROR_STREAM                     (CYPRESS_PLAYER_ERROR_BASE - 3000)
// parse track failed
#define CYPRESS_PLAYER_ERROR_STREAM_INFO_NOT_FOUND      (CYPRESS_PLAYER_ERROR_STREAM - 1)
// coudn't find any track to play
#define CYPRESS_PLAYER_ERROR_STREAM_NO_PLAYABLE_AV      (CYPRESS_PLAYER_ERROR_STREAM - 2)

// error about demuxer
#define CYPRESS_PLAYER_ERROR_DEMUXER                    (CYPRESS_PLAYER_ERROR_BASE - 4000)
// demux failed
#define CYPRESS_PLAYER_ERROR_DEMUXER_READ_FAILED        (CYPRESS_PLAYER_ERROR_DEMUXER - 1)
// unsupported muxer format
#define CYPRESS_PLAYER_ERROR_DEMUXER_NOT_FOUND          (CYPRESS_PLAYER_ERROR_DEMUXER - 2)
// demuxer init failed
#define CYPRESS_PLAYER_ERROR_DEMUXER_INIT_FAILED        (CYPRESS_PLAYER_ERROR_DEMUXER - 3)

// error about decoder
#define CYPRESS_PLAYER_ERROR_DECODER                    (CYPRESS_PLAYER_ERROR_BASE - 5000)
// unsupported encode format
#define CYPRESS_PLAYER_ERROR_DECODER_NOT_FOUND          (CYPRESS_PLAYER_ERROR_DECODER - 1)
// init decoder failed
#define CYPRESS_PLAYER_ERROR_DECODER_INIT_FAILED        (CYPRESS_PLAYER_ERROR_DECODER - 2)
// decode failed
#define CYPRESS_PLAYER_ERROR_DECODE_FAILED              (CYPRESS_PLAYER_ERROR_DECODER - 3)

// error about audio
#define CYPRESS_PLAYER_ERROR_AUDIO                      (CYPRESS_PLAYER_ERROR_BASE - 6000)
// open audio output failed
#define CYPRESS_PLAYER_ERROR_AUDIO_OPEN_FAILED          (CYPRESS_PLAYER_ERROR_AUDIO - 1)

// error about video
#define CYPRESS_PLAYER_ERROR_VIDEO                      (CYPRESS_PLAYER_ERROR_BASE - 7000)
// open video output failed
#define CYPRESS_PLAYER_ERROR_VIDEO_OPEN_FAILED          (CYPRESS_PLAYER_ERROR_VIDEO - 1)

// #define CYPRESS_PLAYER_ERROR_SUBTITLE                   (CYPRESS_PLAYER_ERROR_BASE - 8000)
// #define CYPRESS_PLAYER_ERROR_SUBTITLE_OPEN_FAILED       (CYPRESS_PLAYER_ERROR_SUBTITLE - 1)

// ----
CYPRESS_PLAYER_API CypressPlayer *CypressPlayer_create();

CYPRESS_PLAYER_API int CypressPlayer_setDataSourceAndHeaders(CypressPlayer *player, char* path);

CYPRESS_PLAYER_API int CypressPlayer_setDataSourceFd(CypressPlayer *player, int fd);

CYPRESS_PLAYER_API int CypressPlayer_setDataSourceFd2(CypressPlayer *player, int fd, int64_t offset, int64_t length);

CYPRESS_PLAYER_API void CypressPlayer_setSurface(CypressPlayer *player, void* surface);

CYPRESS_PLAYER_API void CypressPlayer_setCallback(CypressPlayer *player, CypressPlayerCallback callback, void *userdata);

CYPRESS_PLAYER_API void CypressPlayer_prepareAsync(CypressPlayer *player);

CYPRESS_PLAYER_API int CypressPlayer_prepareSync(CypressPlayer *player);

CYPRESS_PLAYER_API int CypressPlayer_start(CypressPlayer *player);

CYPRESS_PLAYER_API int CypressPlayer_pause(CypressPlayer *player);

CYPRESS_PLAYER_API int CypressPlayer_stop(CypressPlayer *player);

CYPRESS_PLAYER_API void CypressPlayer_seekTo(CypressPlayer *player, long msec);

CYPRESS_PLAYER_API int CypressPlayer_getState(CypressPlayer *player);

CYPRESS_PLAYER_API long CypressPlayer_getCurrentPosition(CypressPlayer *player);

CYPRESS_PLAYER_API long CypressPlayer_getDuration(CypressPlayer *player);

CYPRESS_PLAYER_API int CypressPlayer_reset(CypressPlayer *player);

CYPRESS_PLAYER_API void CypressPlayer_release(CypressPlayer *player);

CYPRESS_PLAYER_API void CypressPlayer_setLoopCount(CypressPlayer *player, int loop_count);

CYPRESS_PLAYER_API int CypressPlayer_getLoopCount(CypressPlayer *player);

CYPRESS_PLAYER_API int CypressPlayer_setPlaySpeed(CypressPlayer *player, float speed);

CYPRESS_PLAYER_API float CypressPlayer_getPlaySpeed(CypressPlayer *player);

CYPRESS_PLAYER_API int CypressPlayer_setVolume(CypressPlayer *player, float leftVolume, float rightVolume);

CYPRESS_PLAYER_API void CypressPlayer_getVolume(CypressPlayer *player, float *volume, int size);

CYPRESS_PLAYER_API void CypressPlayer_getVideoSize(CypressPlayer *mp, int *width, int *height);

CYPRESS_PLAYER_API bool CypressPlayer_isPlaying(CypressPlayer *player);

CYPRESS_PLAYER_API void CypressPlayer_setLogCallback(CypressPlayerOnLog log_cb);


// -----
//test

int CypressPlayer_getPlayerNo(CypressPlayer *player);
int CypressPlayer_getStreamSelected(CypressPlayer *player, int stream_type);
void CypressPlayer_setStreamSelected(CypressPlayer *player, int stream, bool selected);
int CypressPlayer_getAudioSessionId(CypressPlayer *player);
void CypressPlayer_setOption(CypressPlayer *player,int category, char* name, char* value);
void CypressPlayer_setOptionLong(CypressPlayer *player,int category, char* name, long value);
char* CypressPlayer_getAudioCodecInfo(CypressPlayer *player);

// typedef struct CypressTrackMeta {
//     int track_type;
//     int track_index;
// } CypressTrackMeta;
// 
// typedef struct CyrpessMediaMeta {
//     long duration;
//     int track_count;
//     CypressTrackMeta tracks_meta[0];
// } CypressMediaMeta;

// typedef struct FdInfo {
//     int fd;
//     int64_t offset;
//     int64_t length;
// } FdInfo;
// 
// typedef enum DataSrcType {
//     DATA_SRC_TYPE_FD, 
//     DATA_SRC_TYPE_URL
// } DataSrcType;
// 
// typedef struct DataSrc {
//     DataSrcType type;
//     union {
//         FdInfo fd_info;
//         const char *url;
//     };
// } DataSrc;
// 
// typedef enum LoopMode {
//     SINGLE,
//     LIST_LOOP,
//     LIST_RANDOM
// } LoopMode;

// #define CYPRESS_PLAYER_MEDIA_TRACK_TYPE_UNKNOWN    0
// #define CYPRESS_PLAYER_MEDIA_TRACK_TYPE_VIDEO      1
// #define CYPRESS_PLAYER_MEDIA_TRACK_TYPE_AUDIO      2
// #define CYPRESS_PLAYER_MEDIA_TRACK_TYPE_SUBTITLE   3
// #define CYPRESS_PLAYER_MEDIA_TRACK_TYPE_METADATA   4

// CypressMediaMeta *CypressPlayer_getMediaMeta(CypressPlayer *player);
// void CypressPlayer_releaseMediaMeta(CypressPlayer *player, CypressMediaMeta *meta);

// /**
//  * @param player  the player to setup
//  * @param srcs    a array of DataSrc which describes all mediums the player will use
//  * @param count   the amount of elements in srcs
//  * Note: this function does not keep the srcs so the caller should free it if neccessary
//  */
// void CypressPlayer_setDataSourceList(CypressPlayer *player, const DataSrc srcs[], int count);
// 
// /**
//  * @param player    the player to operate
//  * @param src_idx   the index of media in list
//  * @param msec      the position where to start
//  */
// void CypressPlayer_seekTo2(CypressPlayer *player, int src_idx, long msec);
// 
// /**
//  * @param    the player to operate
//  * @return   the idx of current selected media, <0 means error
//  */
// int CypressPlayer_getCurrIdx(CypressPlayer *player);
// 
// /**
//  * @param player        the player to operate
//  * @param loop_mode     refer to LoopMode
//  * @param loop_count    the times the list or single media will be repeated,
//  *                      == 0 to infinite, >= 1 to specify repeat count
//  */
// int CypressPlayer_setLoop(CypressPlayer *player, LoopMode loop_mode, int loop_count);

#endif //NATIVEPLAYER_NATIVEPLAYER_H

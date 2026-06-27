//
// Created on 11/8/24.
//

#ifndef NATIVEPLAYER_NATIVEPLAYER_H
#define NATIVEPLAYER_NATIVEPLAYER_H


//#include <jni.h>

typedef struct CypressPlayer CypressPlayer;


typedef void (*CypressPlayrOnPrepared)(CypressPlayer *player, void *userdata);
typedef void (*CypressPlayrOnStarted)(CypressPlayer*player, void *userdata);
typedef void (*CypressPlayrOnSeekComplete)(CypressPlayer*player, void *userdata);
typedef void (*CypressPlayrOnFormatChanged)(CypressPlayer*player, void *userdata, int width, int height);
typedef void (*CypressPlayrOnError)(CypressPlayer*player, void *userdata, int error);
typedef void (*CypressPlayerOnComplete)(CypressPlayer*player, void *userdata);
typedef void (*CypressPlayerOnPlaybackStateChanged)(CypressPlayer *player, int state, void *userdata);

typedef struct CypressPlayerCallback{
    CypressPlayrOnPrepared onPrepared;
    CypressPlayrOnStarted onStarted;
    CypressPlayrOnSeekComplete onSeekComplete;
    CypressPlayrOnFormatChanged onFormatChanged;
    CypressPlayrOnError onError;
    CypressPlayerOnComplete onComplete;
    CypressPlayerOnPlaybackStateChanged onPlaybackStateChanged;
} CypressPlayerCallback;


CypressPlayer *CypressPlayer_create();

void
CypressPlayer_setDataSourceAndHeaders(CypressPlayer* player, char* path);

void
CypressPlayer_setDataSourceFd(CypressPlayer* player, int fd);

void
CypressPlayer_setPlaySpeed(CypressPlayer* player, float speed);

void
CypressPlayer_setPlaySpeed(CypressPlayer* player, float speed);

float
CypressPlayer_getPlaySpeed(CypressPlayer* player);

void
CypressPlayer_setVideoSurface(CypressPlayer* player, void* ANativeWidnow);


void
CypressPlayer_prepareAsync(CypressPlayer* player);

void
CypressPlayer_start(CypressPlayer* player);


void
CypressPlayer_stop(CypressPlayer* player);

void
CypressPlayer_pause(CypressPlayer* player);

void
CypressPlayer_seekTo(CypressPlayer* player, long msec);

bool
CypressPlayer_isPlaying(CypressPlayer* player);

long
CypressPlayer_getCurrentPosition(CypressPlayer* player);

long
CypressPlayer_getDuration(CypressPlayer* player);

void
CypressPlayer_release(CypressPlayer* player);


void
CypressPlayer_reset(CypressPlayer* player);

void
CypressPlayer_setLoopCount(CypressPlayer* player, int loop_count);

int
CypressPlayer_getLoopCount(CypressPlayer* player);


void
CypressPlayer_setStreamSelected(CypressPlayer* player, int stream, bool selected);

void
CypressPlayer_setVolume(CypressPlayer* player, float leftVolume, float rightVolume);

void
CypressPlayer_getVolume(CypressPlayer* player, float *volume, int size);

int
CypressPlayer_getAudioSessionId(CypressPlayer* player);

void
CypressPlayer_setOption(CypressPlayer* player,int category, char* name, char* value);

void
CypressPlayer_setOptionLong(CypressPlayer* player,int category, char* name, long value);


void
CypressPlayer_setANativeWindow(CypressPlayer* player, void *nativeWindow);


void
CypressPlayer_setSurface(CypressPlayer* player, void* surface);

char*
CypressPlayer_getAudioCodecInfo(CypressPlayer* player);



void
CypressPlayer_getMediaMeta(CypressPlayer* player);

void CypressPlayer_setCallback(CypressPlayer* player, CypressPlayerCallback callback, void *userdata);


int CypressPlayer_getState(CypressPlayer *player);


//test
int CypressPlayer_getPlayerNo(CypressPlayer* player);

#endif //NATIVEPLAYER_NATIVEPLAYER_H

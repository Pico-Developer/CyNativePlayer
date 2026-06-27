
#ifndef IJKSDL_ANDROID__ANDROID_CODEC_ANDROID_MEDIACODEC_NDK_H
#define IJKSDL_ANDROID__ANDROID_CODEC_ANDROID_MEDIACODEC_NDK_H

#include "ijksdl_codec_android_mediacodec.h"

//typedef struct ASDK_MediaCodec ASDK_MediaCodec;

typedef struct ResMgr ResMgr;

SDL_AMediaCodec  *SDL_AMediaCodec_native_create(ResMgr *res_mgr);
//jobject           SDL_AMediaCodecJava_getObject(JNIEnv *env, const SDL_AMediaCodec *thiz);

#endif
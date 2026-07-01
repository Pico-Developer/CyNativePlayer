#ifndef IJKSDL_ANDROID__ANDROID_CODEC_ANDROID_MEDIAFORMAT_NDK_H
#define IJKSDL_ANDROID__ANDROID_CODEC_ANDROID_MEDIAFORMAT_NDK_H

#include "ijksdl_codec_android_mediaformat.h"
#include "media/NdkMediaFormat.h"

SDL_AMediaFormat *SDL_AMediaFormatNDK_init(AMediaFormat* android_format);
SDL_AMediaFormat *SDL_AMediaFormatNDK_createVideoFormat(const char *mime, int width, int height);
AMediaFormat     *SDL_AMediaFormatNDK_getAMediaFormat(const SDL_AMediaFormat *thiz);

#endif

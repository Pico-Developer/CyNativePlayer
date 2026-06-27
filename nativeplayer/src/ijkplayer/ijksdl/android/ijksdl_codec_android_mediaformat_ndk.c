#include "ijksdl_android_jni.h"
#include "ijksdl_codec_android_mediaformat_internal.h"
#include "ijksdl_inc_internal_android.h"
#include "ijksdl_codec_android_mediaformat_ndk.h"

typedef struct SDL_AMediaFormat_Opaque {
    AMediaFormat *android_media_format;

    jobject android_byte_buffer;
} SDL_AMediaFormat_Opaque;

AMediaFormat *SDL_AMediaFormatNDK_getAMediaFormat(const SDL_AMediaFormat *thiz)
{
    if (!thiz || !thiz->opaque)
        return NULL;

    SDL_AMediaFormat_Opaque *opaque = (SDL_AMediaFormat_Opaque *)thiz->opaque;
    return opaque->android_media_format;
}

static AMediaFormat *getNdkMediaFormat(const SDL_AMediaFormat *aformat)
{
    if (!aformat ||!aformat->opaque)
        return NULL;

    SDL_AMediaFormat_Opaque *opaque = (SDL_AMediaFormat_Opaque *)aformat->opaque;
    return opaque->android_media_format;
}

static sdl_amedia_status_t SDL_AMediaFormatNDK_delete(SDL_AMediaFormat* aformat)
{
    if (!aformat)
        return SDL_AMEDIA_OK;
    
    SDL_AMediaFormat_Opaque *opaque = (SDL_AMediaFormat_Opaque *)aformat->opaque;

    SDL_AMediaFormat_FreeInternal(aformat);
    return SDL_AMEDIA_OK;
}

static bool SDL_AMediaFormatNDK_getInt32(SDL_AMediaFormat* aformat, const char* name, int32_t *out)
{
    AMediaFormat *android_media_format = getNdkMediaFormat(aformat);
    if (!android_media_format) {
        CYLOGE("%s: getAndroidMediaFormat: failed", __func__);
        return false;
    }
    bool ret = AMediaFormat_getInt32(android_media_format, name, out);
    return ret;
}

static void SDL_AMediaFormatNDK_setInt32(SDL_AMediaFormat* aformat, const char* name, int32_t value)
{   

    AMediaFormat *android_media_format = getNdkMediaFormat(aformat);
    if (!android_media_format) {
        CYLOGE("%s: getAndroidMediaFormat: failed", __func__);
        return;
    }

    AMediaFormat_setInt32(android_media_format, name, value);

}

static void SDL_AMediaFormatNDK_setBuffer(SDL_AMediaFormat* aformat, const char* name, void* data, size_t size)
{
    SDL_AMediaFormat_Opaque *opaque = (SDL_AMediaFormat_Opaque *)aformat->opaque;
    AMediaFormat *android_media_format = opaque->android_media_format;
    if (!android_media_format) {
        CYLOGE("%s: getAndroidMediaFormat: failed", __func__);
        return;
    }
    AMediaFormat_setBuffer(android_media_format, name, data, size);
}

static void setup_aformat(SDL_AMediaFormat *aformat, AMediaFormat* android_media_format) {
    SDL_AMediaFormat_Opaque *opaque = aformat->opaque;
    opaque->android_media_format = android_media_format;

    aformat->func_delete    = SDL_AMediaFormatNDK_delete;
    aformat->func_getInt32  = SDL_AMediaFormatNDK_getInt32;
    aformat->func_setInt32  = SDL_AMediaFormatNDK_setInt32;
    aformat->func_setBuffer = SDL_AMediaFormatNDK_setBuffer;
}

SDL_AMediaFormat *SDL_AMediaFormatNDK_init(AMediaFormat* android_format)
{
    // SDLTRACE("%s", __func__);

    SDL_AMediaFormat *aformat = SDL_AMediaFormat_CreateInternal(sizeof(SDL_AMediaFormat_Opaque));
    AMediaFormat* android_media_format = AMediaFormat_new();

    setup_aformat(aformat, android_media_format);
    return aformat;
}

SDL_AMediaFormat *SDL_AMediaFormatNDK_createVideoFormat(const char *mime, int width, int height)
{
    // SDLTRACE("%s", __func__);
    AMediaFormat* android_media_format = AMediaFormat_new();    
    if (!android_media_format) {
        return NULL;
    }

    SDL_AMediaFormat *aformat = SDL_AMediaFormat_CreateInternal(sizeof(SDL_AMediaFormat_Opaque));
    if (!aformat) { 
        AMediaFormat_delete(android_media_format);       
        return NULL;
    }
    AMediaFormat_setString(android_media_format, AMEDIAFORMAT_KEY_MIME, mime);
    AMediaFormat_setInt32(android_media_format, AMEDIAFORMAT_KEY_WIDTH, width);
    AMediaFormat_setInt32(android_media_format, AMEDIAFORMAT_KEY_HEIGHT, height);   

    setup_aformat(aformat, android_media_format);
    SDL_AMediaFormat_setInt32(aformat, AMEDIAFORMAT_KEY_MAX_INPUT_SIZE, 0);
    return aformat;
}

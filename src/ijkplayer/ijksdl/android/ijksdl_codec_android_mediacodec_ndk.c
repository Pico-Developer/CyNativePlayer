#include "ijksdl_codec_android_mediacodec_ndk.h"
#include "ijksdl_codec_android_mediaformat_ndk.h"
#include "ijksdl_codec_android_mediacodec_internal.h"
#include "media/NdkMediaCodec.h"
#include "media/NdkMediaFormat.h"
#include "ijksdl_inc_internal_android.h"
#include "ijksdl/android/resmgr.h"

static SDL_Class g_amediacodec_native_class = {
        .name = "AMediaCodec",
};

typedef struct SDL_AMediaCodec_Opaque {
    AMediaCodec *codec;
    AMediaCodecBufferInfo *output_buffer_info;
    bool            is_input_buffer_valid;
    ResMgr *res_mgr;
} SDL_AMediaCodec_Opaque;

static sdl_amedia_status_t SDL_AMediaCodecNative_start(SDL_AMediaCodec* acodec)
{
    CYLOGI("%s", __func__);

    SDL_AMediaCodec_Opaque *opaque = (SDL_AMediaCodec_Opaque *) acodec->opaque;

    sdl_amedia_status_t status = SDL_AMEDIA_OK;

    status = res_mgr_start_codec(opaque->res_mgr, opaque->codec);
    CYLOGI("%s %p ret %d", __func__, opaque->codec, status);
    return status;
}

static sdl_amedia_status_t SDL_AMediaCodecNative_delete(SDL_AMediaCodec* acodec){
    CYLOGI("%s", __func__);

    SDL_AMediaCodec_Opaque *opaque = (SDL_AMediaCodec_Opaque *) acodec->opaque;
    CYLOGI("%s %p", __func__, opaque->codec);
    if (opaque->codec) {
        // sdk may not start codec, in which case sdk will call delete directly without stop.
        CYLOGI("release codec %p in %p to %p", opaque->codec, acodec, opaque->res_mgr);
        int ret = res_mgr_release_codec(opaque->res_mgr, &opaque->codec);
        if (ret != 0) {
            CYLOGE("release codec %p in %p failed, ret %d", opaque->codec, opaque->res_mgr, ret);
            return ret;
        }
    }
    return SDL_AMEDIA_OK;
}
static sdl_amedia_status_t SDL_AMediaCodecNative_configure_surface(        
        SDL_AMediaCodec* acodec,
        const SDL_AMediaFormat* aformat,
        void *android_surface,
        SDL_AMediaCrypto *crypto,
        uint32_t flags){
    CYLOGI("%s", __func__);
    SDL_AMediaCodec_Opaque *opaque = (SDL_AMediaCodec_Opaque *) acodec->opaque;

    AMediaFormat *format = SDL_AMediaFormatNDK_getAMediaFormat(aformat);

    int ret = res_mgr_obtain_codec(opaque->res_mgr, format, android_surface, &opaque->codec);
    CYLOGI("%s obtain codec from res_mgr %p for %p format %s surface %p codec %p ret %d",
        __func__, opaque->res_mgr, acodec, AMediaFormat_toString(format), android_surface, opaque->codec, ret);


    if (ret != 0) {
        CYLOGE("obtain codec for %p failed ret %d", acodec, ret);
        return SDL_AMEDIA_ERROR_UNKNOWN;
    }

    opaque->is_input_buffer_valid = true;
    return SDL_AMEDIA_OK;

}

static sdl_amedia_status_t SDL_AMediaCodecNative_set_surface(SDL_AMediaCodec* acodec, void *android_surface){
    CYLOGI("%s set surface %p for acodec %p", __func__, android_surface, acodec);
    SDL_AMediaCodec_Opaque *opaque = (SDL_AMediaCodec_Opaque *) acodec->opaque;
    int ret = res_mgr_setup_codec_surface(opaque->res_mgr, opaque->codec, android_surface);
    if (ret != 0) {
        CYLOGE("%s setup codec %p surface %p ret %d", __func__, opaque->codec, android_surface, ret);
    }
    return ret;
}

bool SDL_AMediaCodecNative_isInputBuffersValid(SDL_AMediaCodec* acodec){
    SDL_AMediaCodec_Opaque *opaque = (SDL_AMediaCodec_Opaque *) acodec->opaque;
    return opaque->is_input_buffer_valid;
}
SDL_AMediaFormat *SDL_AMediaCodecNative_getOutputFormat(SDL_AMediaCodec *thiz){
    SDL_AMediaCodec_Opaque *opaque = (SDL_AMediaCodec_Opaque *) thiz->opaque;
    // CYLOGI("%s %p", __func__, opaque->codec);
    AMediaFormat *format = AMediaCodec_getOutputFormat(opaque->codec);
    if(format == NULL){
        return NULL;
    }
    SDL_AMediaFormat *aformat = SDL_AMediaFormatNDK_init(format);
    return aformat;
}
static sdl_amedia_status_t SDL_AMediaCodecNative_stop(SDL_AMediaCodec* acodec){
    CYLOGI("%s", __func__);
    SDL_AMediaCodec_Opaque *opaque = (SDL_AMediaCodec_Opaque *) acodec->opaque;
    // CYLOGI("%s %p", __func__, opaque->codec);
    // AMediaCodec_stop(opaque->codec);
    CYLOGI("release codec %p in %p to %p", opaque->codec, acodec, opaque->res_mgr);
    int ret = res_mgr_release_codec(opaque->res_mgr, &opaque->codec);
    if (ret != 0) {
        CYLOGE("release codec to res_mgr failed, ret %d", ret);
        return SDL_AMEDIA_ERROR_UNKNOWN;
    }
    return SDL_AMEDIA_OK;
}

static sdl_amedia_status_t SDL_AMediaCodecNative_flush(SDL_AMediaCodec* acodec){
    SDL_AMediaCodec_Opaque *opaque = (SDL_AMediaCodec_Opaque *) acodec->opaque;
    CYLOGI("%s %p", __func__, opaque->codec);
    AMediaCodec_flush(opaque->codec);
    return SDL_AMEDIA_OK;
}

static ssize_t SDL_AMediaCodecNative_writeInputData(SDL_AMediaCodec* acodec, size_t idx, const uint8_t *data, size_t size){
    SDL_AMediaCodec_Opaque *opaque = (SDL_AMediaCodec_Opaque *) acodec->opaque;
    size_t out_size = 0;
    // CYLOGI("%s %p", __func__, opaque->codec);
    uint8_t *buffer = AMediaCodec_getInputBuffer(opaque->codec, idx, &out_size);
    if(out_size < size){
        return -1;
    }
    if (buffer == NULL) {
        return -1;
    }
    memcpy(buffer, data, size);
    return size;
}
ssize_t SDL_AMediaCodecNative_dequeueInputBuffer(SDL_AMediaCodec* acodec, int64_t timeoutUs){
    SDL_AMediaCodec_Opaque *opaque = (SDL_AMediaCodec_Opaque *) acodec->opaque;
    // CYLOGI("%s %p", __func__, opaque->codec);
    ssize_t index = AMediaCodec_dequeueInputBuffer(opaque->codec, timeoutUs);
    return index;
}
sdl_amedia_status_t SDL_AMediaCodecNative_queueInputBuffer(SDL_AMediaCodec* acodec, size_t idx, off_t offset, size_t size, uint64_t time, uint32_t flags){
    SDL_AMediaCodec_Opaque *opaque = (SDL_AMediaCodec_Opaque *) acodec->opaque;

    // CYLOGI("%s %p", __func__, opaque->codec);
    media_status_t ret = AMediaCodec_queueInputBuffer(opaque->codec, idx, offset, size, time, flags);
    return ret;
}
ssize_t SDL_AMediaCodecNative_dequeueOutputBuffer(SDL_AMediaCodec* acodec, SDL_AMediaCodecBufferInfo *info, int64_t timeoutUs){
    SDL_AMediaCodec_Opaque *opaque = (SDL_AMediaCodec_Opaque *) acodec->opaque;
    if(opaque->output_buffer_info == NULL){
        opaque->output_buffer_info = (AMediaCodecBufferInfo *) malloc(sizeof(AMediaCodecBufferInfo));
    }

    // CYLOGI("%s %p", __func__, opaque->codec);
    int idx = AMEDIACODEC__UNKNOWN_ERROR;
    while (1) {
        idx = AMediaCodec_dequeueOutputBuffer(opaque->codec, opaque->output_buffer_info, timeoutUs);
        // CYLOGE("checkpoint idx %d pts %ld flag %d", idx, idx >= 0 ? opaque->output_buffer_info->presentationTimeUs : -1, idx >= 0 ? opaque->output_buffer_info->flags & AMEDIACODEC__BUFFER_FLAG_END_OF_STREAM : -1);

        if (idx ==  AMEDIACODEC_INFO_OUTPUT_BUFFERS_CHANGED) {
            CYLOGI("%s: INFO_OUTPUT_BUFFERS_CHANGED\n", __func__);
            continue;
        } else if (idx == AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED) {
            // CYLOGI("%s: INFO_OUTPUT_FORMAT_CHANGED\n", __func__);
        } else if (idx >= 0) {
            AMCTRACE("%s: buffer ready (%d) ====================\n", __func__, idx);
            if (info) {
                info->offset              = opaque->output_buffer_info->offset;
                info->size                =  opaque->output_buffer_info->size;
                info->presentationTimeUs  = opaque->output_buffer_info->presentationTimeUs;
                info->flags               = opaque->output_buffer_info->flags;
            }
        }
        break;
    }
    // CYLOGI("%s %p end, idx: %d", __func__, opaque->codec, idx);
    return idx;    
}
sdl_amedia_status_t  SDL_AMediaCodecNative_releaseOutputBuffer(SDL_AMediaCodec* acodec, size_t idx, bool render){
    SDL_AMediaCodec_Opaque *opaque = (SDL_AMediaCodec_Opaque *) acodec->opaque;
    // CYLOGI("%s %p", __func__, opaque->codec);
    media_status_t ret = AMediaCodec_releaseOutputBuffer(opaque->codec, idx, render);
    return ret;
}

static SDL_AMediaCodec* SDL_AMediaCodec_native_init(ResMgr *res_mgr){
    SDL_AMediaCodec *acodec = SDL_AMediaCodec_CreateInternal(sizeof(SDL_AMediaCodec_Opaque));
    SDL_AMediaCodec_Opaque *opaque = acodec->opaque;
    opaque->codec = NULL;
    opaque->res_mgr = res_mgr;
    acodec->opaque_class = &g_amediacodec_native_class;
    acodec->func_delete = SDL_AMediaCodecNative_delete;
    acodec->func_configure              = NULL;
    acodec->func_configure_surface = SDL_AMediaCodecNative_configure_surface;
    acodec->func_set_surface = SDL_AMediaCodecNative_set_surface;
    acodec->func_start = SDL_AMediaCodecNative_start;
    acodec->func_stop = SDL_AMediaCodecNative_stop;
    acodec->func_flush = SDL_AMediaCodecNative_flush;
    acodec->func_writeInputData = SDL_AMediaCodecNative_writeInputData;
    acodec->func_dequeueInputBuffer = SDL_AMediaCodecNative_dequeueInputBuffer;
    acodec->func_queueInputBuffer = SDL_AMediaCodecNative_queueInputBuffer;
    acodec->func_dequeueOutputBuffer = SDL_AMediaCodecNative_dequeueOutputBuffer;
    acodec->func_getOutputFormat = SDL_AMediaCodecNative_getOutputFormat;
    acodec->func_releaseOutputBuffer = SDL_AMediaCodecNative_releaseOutputBuffer;
    acodec->func_isInputBuffersValid    = SDL_AMediaCodecNative_isInputBuffersValid;
    SDL_AMediaCodec_increaseReference(acodec);
    return acodec;
}

SDL_AMediaCodec* SDL_AMediaCodec_native_create(ResMgr *res_mgr){
    //AMediaCodec *codec = AMediaCodec_createCodecByName(codec_name);
    return SDL_AMediaCodec_native_init(res_mgr);
}


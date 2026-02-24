/*
 * Copyright (c) 2026 ByteDance Ltd. and/or its affiliates
 * SPDX-License-Identifier: LGPL-2.1-only
 */

#include <media/NdkMediaError.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <media/NdkMediaCodec.h>
#include <string.h>
#include <android/native_window.h>
#include "ijksdl/android/resmgr.h"
#include "ijksdl/ijksdl_mutex.h"
#include "ijksdl_log.h"
#include "ijksdl/android/ijksdl_codec_android_mediadef.h"
#include "resmgr.h"

typedef enum ResMgrCodecType {
    RES_MGR_CODE_TYPE_AVC,
    RES_MGR_CODE_TYPE_HEVC,
    RES_MGR_CODE_TYPE_AV1,
    RES_MGR_CODE_TYPE_VP9,
    RES_MGR_CODE_TYPE_MVHEVC,
    RES_MGR_CODE_TYPE_UNKNOWN
} ResMgrCodecType;

static ResMgrCodecType mime_to_codec_type(const char *mime) {
    if (mime == NULL) return RES_MGR_CODE_TYPE_UNKNOWN;

    if (strcmp(mime, SDL_AMIME_VIDEO_AVC) == 0) {
        return RES_MGR_CODE_TYPE_AVC;
    } else if (strcmp(mime, SDL_AMIME_VIDEO_HEVC) == 0) {
        return RES_MGR_CODE_TYPE_HEVC;
    } else if (strcmp(mime, SDL_AMIME_VIDEO_AV1) == 0) {
        return RES_MGR_CODE_TYPE_AV1;
    } else if (strcmp(mime, SDL_AMIME_VIDEO_VP9) == 0) {
        return RES_MGR_CODE_TYPE_VP9;
    } else if (strcmp(mime, SDL_AMIME_VIDEO_MVHEVC) == 0) {
        return RES_MGR_CODE_TYPE_MVHEVC;
    }

    return RES_MGR_CODE_TYPE_UNKNOWN;
}

typedef struct CodecRes {
    ResMgrCodecType type;
    int32_t rotation;
    int32_t width, height;
    void *surface;

    AMediaCodec *codec;
    bool used;
    bool started;
} CodecRes;

struct ResMgr {
    CodecRes codec_res;
    void *dummy_window;
    SDL_mutex *lock;
};

int res_mgr_init(ResMgr **res_mgr) {
    if (!res_mgr) return EINVAL;

    ResMgr *r = calloc(1, sizeof(*r));
    if (!r) return ENOMEM;

    if ((r->lock = SDL_CreateMutex()) == NULL) {
        free(r);
        return EAGAIN;
    }

    *res_mgr = r;
    return 0;
}

int res_mgr_destroy(ResMgr **res_mgr) {
    if (!res_mgr || !*res_mgr) return EINVAL;

    ResMgr *r = *res_mgr;

    if (r->codec_res.codec) {
        CYLOGI("res_mgr: %s stop & delete codec %p", __func__, r->codec_res.codec);
        if (r->codec_res.started) {
            AMediaCodec_stop(r->codec_res.codec);
        }
        AMediaCodec_delete(r->codec_res.codec);
        r->codec_res.codec = NULL;
    }

    SDL_DestroyMutex(r->lock);

    free(r);
    *res_mgr = r = NULL;

    return 0;
}

int res_mgr_reset(ResMgr *res_mgr) {
    if (!res_mgr) return EINVAL;

    SDL_LockMutex(res_mgr->lock);
    if (res_mgr->codec_res.codec && res_mgr->codec_res.used) {
        CYLOGE("res_mgr: reset used codec %p", res_mgr->codec_res.codec);
        SDL_UnlockMutex(res_mgr->lock);
        return EINVAL;
    }

    if (res_mgr->codec_res.codec) {
        CYLOGI("res_mgr: stop and release codec %p", res_mgr->codec_res.codec);
        if (res_mgr->codec_res.started) {
            AMediaCodec_stop(res_mgr->codec_res.codec);
        }
        AMediaCodec_delete(res_mgr->codec_res.codec);
    }

    memset(&res_mgr->codec_res, 0, sizeof(res_mgr->codec_res));

    SDL_UnlockMutex(res_mgr->lock);
    return 0;
}

int res_mgr_set_dummy_window(ResMgr *res_mgr, ANativeWindow *window) {
    if (!res_mgr) return EINVAL;

    SDL_LockMutex(res_mgr->lock);
    res_mgr->dummy_window = window;
    SDL_UnlockMutex(res_mgr->lock);
    return 0;
}

static int create_codec_res(AMediaFormat *format, ANativeWindow *surface, CodecRes *codec_res) {
    const char *mime = NULL;
    AMediaFormat_getString(format, AMEDIAFORMAT_KEY_MIME, &mime);
    codec_res->type = mime_to_codec_type(mime);
    if (codec_res->type == RES_MGR_CODE_TYPE_UNKNOWN) {
        CYLOGE("res_mgr: unknown codec type %s", mime);
        return EINVAL;
    }

    AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_WIDTH, &codec_res->width);
    AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_HEIGHT, &codec_res->height);
    if (!AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_ROTATION, &codec_res->rotation)) {
        codec_res->rotation = 0;
    }

    codec_res->surface = surface;
    codec_res->used = false;
    codec_res->started = false;

    CYLOGI("res_mgr: create codec_res with mime %s", mime);
    codec_res->codec = AMediaCodec_createDecoderByType(mime);
    bool mvhevc_fallback_to_hevc = false;
    if (!codec_res->codec) {
        if (codec_res->type == RES_MGR_CODE_TYPE_MVHEVC) {
            CYLOGW("res_mgr: create mvhevc codec failed, try hevc codec");
            codec_res->codec = AMediaCodec_createDecoderByType(SDL_AMIME_VIDEO_HEVC);
            if (codec_res->codec) mvhevc_fallback_to_hevc = true;
        }
        if (!codec_res->codec) {
            CYLOGE("res_mgr: create codec %s failed", mime);
            return EAGAIN;
        }
    }
    CYLOGI("res_mgr: create codec %p succ, mvhevc_fallback_to_hevc: %d", codec_res->codec, mvhevc_fallback_to_hevc);

    media_status_t status = AMEDIA_OK;
    if (mvhevc_fallback_to_hevc) {
        CYLOGI("res_mgr: mvhevc fallback to hevc codec %p, update format", codec_res->codec);
        codec_res->type = RES_MGR_CODE_TYPE_HEVC;
        AMediaFormat *new_format = AMediaFormat_new();
        AMediaFormat_copy(new_format, format);
        AMediaFormat_setString(new_format, AMEDIAFORMAT_KEY_MIME, SDL_AMIME_VIDEO_HEVC);
        void *hvcc_lhvc_sps_pps_vps_buf = NULL;
        size_t hvcc_lhvc_sps_pps_vps_buf_size = 0;
        int32_t hvcc_sps_pps_vps_size = 0;
        if (!AMediaFormat_getBuffer(format, "csd-0", &hvcc_lhvc_sps_pps_vps_buf, &hvcc_lhvc_sps_pps_vps_buf_size) ||
            !AMediaFormat_getInt32(format, "cypressplayer-mvhevc-hvcC-sps-pps-vps-size", &hvcc_sps_pps_vps_size) ||
            hvcc_lhvc_sps_pps_vps_buf == NULL || (int32_t)hvcc_lhvc_sps_pps_vps_buf_size < hvcc_sps_pps_vps_size || hvcc_sps_pps_vps_size <= 0) {
            status = AMEDIA_ERROR_INVALID_PARAMETER;
            CYLOGE("res_mgr: get hvcc_lhvc_sps_pps_vps_buf failed, create hevc codec failed");
        } else {
            AMediaFormat_setBuffer(new_format, "csd-0", hvcc_lhvc_sps_pps_vps_buf, hvcc_sps_pps_vps_size);
            CYLOGI("res_mgr: configue codec with format %s surface %p hvcc_sps_pps_vps_size %d", AMediaFormat_toString(new_format), codec_res->surface, hvcc_sps_pps_vps_size);
            status = AMediaCodec_configure(codec_res->codec, new_format, codec_res->surface, NULL, 0);
        }
        AMediaFormat_delete(new_format);
    } else {
        CYLOGI("res_mgr: configue codec with format %s surface %p", AMediaFormat_toString(format), codec_res->surface);
        status = AMediaCodec_configure(codec_res->codec, format, codec_res->surface, NULL, 0);
    }
    if (status != AMEDIA_OK) {
        AMediaCodec_delete(codec_res->codec);
        codec_res->codec = NULL;
        CYLOGE("res_mgr: create codec with format %s and surface %p failed, status: %d", AMediaFormat_toString(format), surface, status);
    } else {
        CYLOGI("res_mgr: create codec_res successfully, codec: %p, surface: %p", codec_res->codec, surface);
    }
    return status;
}

bool check_can_reuse_codec(const CodecRes *codec_res, AMediaFormat *format) {
    const char *req_mime = NULL;
    AMediaFormat_getString(format, AMEDIAFORMAT_KEY_MIME, &req_mime);
    int32_t req_width = 0, req_height = 0;
    AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_WIDTH, &req_width);
    AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_HEIGHT, &req_height);
    int32_t req_roation = 0;
    if (!AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_ROTATION, &req_roation)) {
        req_roation = 0;
    }

    // when mvhevc fallback to hevc, we never reuse the codec for mvhevc
    // TODO: is it possible to create mvhevc codec successfully next time?
    ResMgrCodecType req_codec_type = mime_to_codec_type(req_mime);

    return codec_res->type == req_codec_type &&
           codec_res->width == req_width     &&
           codec_res->height == req_height   &&
           codec_res->rotation == req_roation;
}

int res_mgr_obtain_codec(ResMgr *res_mgr, AMediaFormat *format, ANativeWindow *surface, AMediaCodec **codec) {
    if (!res_mgr || !codec) return EINVAL;

    int ret = 0;
    bool new_codec = false;
    SDL_LockMutex(res_mgr->lock);
    if (!res_mgr->codec_res.codec) {
        ret = create_codec_res(format, surface, &res_mgr->codec_res);
        if (ret != 0) goto end;
        new_codec = true;
    }

    if (res_mgr->codec_res.used) {
        ret = EINVAL;
        goto end;
    }

    if (new_codec) {
        res_mgr->codec_res.used = true;
        *codec = res_mgr->codec_res.codec;
        goto end;
    }


    if (check_can_reuse_codec(&res_mgr->codec_res, format)) {
        media_status_t status = AMEDIA_OK;
        // if (surface != res_mgr->codec_res.surface) {
        CYLOGI("res_mgr: set output surface %p to codec %p old surface %p", surface, res_mgr->codec_res.codec, res_mgr->codec_res.surface);
        status = AMediaCodec_setOutputSurface(res_mgr->codec_res.codec, surface);
        if (status != AMEDIA_OK) {
            CYLOGE("res_mgr: set output surface %p to codec %p failed, status: %d", surface, res_mgr->codec_res.codec, status);
            ret = status;
            goto end;
        }
        res_mgr->codec_res.surface = surface;
        // }
        CYLOGI("resmgr: start queue csd-0 to %p", res_mgr->codec_res.codec);
        int idx = AMediaCodec_dequeueInputBuffer(res_mgr->codec_res.codec, 150000);
        if (idx < 0) {
            CYLOGE("res_mgr: dequeue input buffer of codec %p failed, ret: %d", res_mgr->codec_res.codec, idx);
            ret = idx;
            goto end;
        }
        size_t out_size = 0;
        uint8_t *in_buf = AMediaCodec_getInputBuffer(res_mgr->codec_res.codec, idx, &out_size);
        if (!in_buf) {
            AMediaCodec_queueInputBuffer(res_mgr->codec_res.codec, idx, 0, 0, 0, 0);
            CYLOGE("res_mgr: get input buffer of codec %p is NULL", res_mgr->codec_res.codec);
            ret = EAGAIN;
            goto end;
        }
        uint8_t *csd0_data = NULL, *csd1_data = NULL;
        size_t csd0_size = 0, csd1_size = 0;
        if (!AMediaFormat_getBuffer(format, AMEDIAFORMAT_KEY_CSD_0, (void **)&csd0_data, &csd0_size)) {
            csd0_data = NULL;
            csd0_size = 0;
        }
        if (!AMediaFormat_getBuffer(format, AMEDIAFORMAT_KEY_CSD_1, (void **)&csd1_data, &csd1_size)) {
            csd1_data = NULL;
            csd1_size = 0;
        }
        CYLOGI("res_mgr: csd-0 size: %zu, csd-1 size: %zu, out size: %zu", csd0_size, csd1_size, out_size);
        if ((csd0_size + csd1_size) > out_size) {
            AMediaCodec_queueInputBuffer(res_mgr->codec_res.codec, idx, 0, 0, 0, 0);
            CYLOGE("res_mgr: csd size %zu is greater than out size %zu", (csd0_size + csd1_size), out_size);
            ret = EINVAL;
            goto end;
        }
        memcpy(in_buf, csd0_data, csd0_size);
        memcpy(in_buf + csd0_size, csd1_data, csd1_size);
        status = AMediaCodec_queueInputBuffer(res_mgr->codec_res.codec, idx, 0, (csd0_size + csd1_size), 0, 0);
        if (status != AMEDIA_OK) {
            CYLOGE("res_mgr: queue input buffer of codec %p failed, status: %d", res_mgr->codec_res.codec, status);
            ret = status;
            goto end;
        }
        CYLOGI("res_mgr: queue csd-0 to codec %p end", res_mgr->codec_res.codec);

        res_mgr->codec_res.used = true;
        CYLOGI("res_mgr: reuse codec %p", res_mgr->codec_res.codec);
        *codec = res_mgr->codec_res.codec;
        goto end;
    }

    CYLOGI("res_mgr: %s stop & delete codec %p", __func__, res_mgr->codec_res.codec);
    if (res_mgr->codec_res.started) {
        AMediaCodec_stop(res_mgr->codec_res.codec);
    }
    AMediaCodec_delete(res_mgr->codec_res.codec);

    res_mgr->codec_res.codec = NULL;

    ret = create_codec_res(format, surface, &res_mgr->codec_res);
    if (ret != 0) goto end;
    new_codec = true;

    res_mgr->codec_res.used = true;
    *codec = res_mgr->codec_res.codec;

end:
    SDL_UnlockMutex(res_mgr->lock);
    return ret;
}

int res_mgr_setup_codec_surface(ResMgr *res_mgr, AMediaCodec *codec, ANativeWindow *surface) {
    if (!res_mgr || !codec) return EINVAL;
    int ret = 0;
    SDL_LockMutex(res_mgr->lock);
    if (res_mgr->codec_res.codec != codec || !res_mgr->codec_res.used) {
        ret = EINVAL;
        CYLOGE("res_mgr: codec %p not used", codec);
        goto end;
    }

    if (res_mgr->codec_res.surface == NULL && surface != NULL) {
        ret = EINVAL;
        CYLOGE("res_mgr: couldn't setup surface %p to codec %p because configure the codec without surface", surface, codec);
        goto end;
    }

    if (res_mgr->codec_res.surface != NULL && surface == NULL) {
        CYLOGI("res_mgr: use dummy surface %p instead of null for codec %p", res_mgr->dummy_window, codec);
        surface = res_mgr->dummy_window;
    }

    if (res_mgr->codec_res.surface == surface) {
        CYLOGW("res_mgr: codec %p already setup surface %p", codec, surface);
        goto end;
    }

    media_status_t status = AMediaCodec_setOutputSurface(codec, surface);
    CYLOGI("res_mgr: setup surface %p to codec %p ret %d", surface, codec, status);

    if (status != AMEDIA_OK) {
        CYLOGE("res_mgr: setup surface %p to codec %p failed ret %d", surface, codec, status);
        ret = status;
        goto end;
    }

    res_mgr->codec_res.surface = surface;

end:
    SDL_UnlockMutex(res_mgr->lock);
    return ret;
}


int res_mgr_start_codec(ResMgr *res_mgr, AMediaCodec *codec) {
    if (!res_mgr || !codec) return EINVAL;
    int ret = 0;
    SDL_LockMutex(res_mgr->lock);
    if (res_mgr->codec_res.codec != codec || !res_mgr->codec_res.used) {
        ret = EINVAL;
        CYLOGE("res_mgr: codec %p not used or is not owned by %p", codec, res_mgr);
    } else if (!res_mgr->codec_res.started) {
        media_status_t status = AMediaCodec_start(res_mgr->codec_res.codec);
        if (status != AMEDIA_OK) {
            CYLOGE("res_mgr: start codec %p failed, status: %d", res_mgr->codec_res.codec, status);
            ret = status;
        } else {
            res_mgr->codec_res.started = true;
        }
    }
    SDL_UnlockMutex(res_mgr->lock);
    return ret;
}


int res_mgr_release_codec(ResMgr *res_mgr, AMediaCodec **codec) {
    if (!res_mgr || !codec || !*codec) return EINVAL;

    SDL_LockMutex(res_mgr->lock);
    if (res_mgr->codec_res.codec != *codec || !res_mgr->codec_res.used) {
        CYLOGE("res_mgr: release codec %p not used", *codec);
        SDL_UnlockMutex(res_mgr->lock);
        return EINVAL;
    }

    media_status_t status = AMEDIA_OK;
    if (res_mgr->codec_res.started) {
        CYLOGI("res_mgr: flush codec %p", *codec);
        status = AMediaCodec_flush(*codec);
        if (status != AMEDIA_OK) {
            CYLOGE("res_mgr: flush codec %p failed, status: %d", *codec, status);
        }
    }
    CYLOGI("res_mgr: set dummy surface %p to codec %p", res_mgr->dummy_window, *codec);
    if (res_mgr->dummy_window != NULL) {
        status = AMediaCodec_setOutputSurface(*codec, res_mgr->dummy_window);
        if (status != AMEDIA_OK) {
            CYLOGE("res_mgr: set dummy output surface %p to codec %p failed origin surface %p, status: %d", res_mgr->dummy_window, *codec, res_mgr->codec_res.surface, status);
        }
    }

    res_mgr->codec_res.used = false;
    *codec = NULL;
    SDL_UnlockMutex(res_mgr->lock);
    return 0;
}

/*
 * Copyright (c) 2026 ByteDance Ltd. and/or its affiliates
 * SPDX-License-Identifier: LGPL-2.1-only
 */

#ifndef IJKSDL_ANDROID__RESMGR_H
#define IJKSDL_ANDROID__RESMGR_H

#include <stdbool.h>

typedef struct ResMgr ResMgr;

typedef struct AMediaCodec AMediaCodec;
typedef struct AMediaFormat AMediaFormat;
typedef struct ANativeWindow ANativeWindow;

int res_mgr_init(ResMgr **res_mgr);
int res_mgr_destroy(ResMgr **res_mgr);
int res_mgr_reset(ResMgr *res_mgr);
int res_mgr_set_dummy_window(ResMgr *res_mgr, ANativeWindow *window);
int res_mgr_obtain_codec(ResMgr *res_mgr, AMediaFormat *format, ANativeWindow *surface, AMediaCodec **codec);
int res_mgr_setup_codec_surface(ResMgr *res_mgr, AMediaCodec *codec, ANativeWindow *surface);
int res_mgr_start_codec(ResMgr *res_mgr, AMediaCodec *codec);
int res_mgr_release_codec(ResMgr *res_mgr, AMediaCodec **codec);

#endif

/*
 * Copyright (c) 2026 ByteDance Ltd. and/or its affiliates
 * SPDX-License-Identifier: LGPL-2.1-only
 */

//
// Created on 11/17/24.
//

#ifndef IJKPLAYERTEST_LOGUTIL_H
#define IJKPLAYERTEST_LOGUTIL_H

#include <stdint.h>
#ifndef TAG
#define TAG "CypressPlayer"
#endif
#include "utils/nclog/nclog.h"

#define CYLOGE(FMT, ...) nclog(get_callback_appender(), NC_TRACE_TYPE_NORMAL, NC_LOG_LEVEL_E, (TAG), (FMT), ##__VA_ARGS__)
#define CYLOGW(FMT, ...) nclog(get_callback_appender(), NC_TRACE_TYPE_NORMAL, NC_LOG_LEVEL_W, (TAG), (FMT), ##__VA_ARGS__)
#define CYLOGI(FMT, ...) nclog(get_callback_appender(), NC_TRACE_TYPE_NORMAL, NC_LOG_LEVEL_I, (TAG), (FMT), ##__VA_ARGS__)
#define CYLOGD(FMT, ...) nclog(get_callback_appender(), NC_TRACE_TYPE_NORMAL, NC_LOG_LEVEL_D, (TAG), (FMT), ##__VA_ARGS__)
#define CYLOGV(FMT, ...) nclog(get_callback_appender(), NC_TRACE_TYPE_NORMAL, NC_LOG_LEVEL_V, (TAG), (FMT), ##__VA_ARGS__)

#ifdef __cplusplus
extern "C" {
#endif

void set_cylog_callback(void (*log_cb)(int level, const char *tag, const char *msg));

Appender get_callback_appender();

#ifdef __cplusplus
} // extern "C"
#endif

#endif //IJKPLAYERTEST_LOGUTIL_H

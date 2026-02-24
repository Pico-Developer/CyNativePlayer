/*
 * Copyright (c) 2026 ByteDance Ltd. and/or its affiliates
 * SPDX-License-Identifier: LGPL-2.1-only
 */

#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <android/log.h>
#include <string.h>
#include <sys/stat.h>
#include <linux/limits.h>
#include <fcntl.h>
#include <time.h>
#include <dirent.h>
#include <sys/time.h>
#include <sys/system_properties.h>
#define TAG "CypressPlayerLog"
#include "utils/nclog/log_util.h"

static struct CyCbLogInfo {
    pthread_once_t init_once;
    pthread_rwlock_t rwlock;
    void (*log_cb)(int level, const char *tag, const char *msg);
    Appender cb_appender;
} g_cylog_info = {
    .init_once = PTHREAD_ONCE_INIT,
    .rwlock = PTHREAD_RWLOCK_INITIALIZER,
    .log_cb = NULL,
    .cb_appender = NULL
};

static void cy_log(Appender appender, TraceType type, LogLevel level, const char *tag, const char *fmt_str, va_list args) {

    static int android_log_level_map[] = {
        [NC_LOG_LEVEL_E] = ANDROID_LOG_ERROR,
        [NC_LOG_LEVEL_W] = ANDROID_LOG_WARN,
        [NC_LOG_LEVEL_I] = ANDROID_LOG_INFO,
        [NC_LOG_LEVEL_D] = ANDROID_LOG_DEBUG,
        [NC_LOG_LEVEL_V] = ANDROID_LOG_VERBOSE,
    };

    if (appender == NULL) {
        return;
    }

    struct CyCbLogInfo *log_info = user_data_from_appender(appender);
    if (log_info == NULL) {
        return;
    }

    char msg[4096] = {0};
    vsnprintf(msg, sizeof(msg), fmt_str, args);

    pthread_rwlock_rdlock(&log_info->rwlock);
    void (*log_cb)(int level, const char *tag, const char *msg) = log_info->log_cb;
    log_cb ? log_cb(level, tag, msg) : (void)0;
    pthread_rwlock_unlock(&log_info->rwlock);
}

void set_cylog_callback(void (*log_cb)(int level, const char *tag, const char *msg)) {
    pthread_rwlock_wrlock(&g_cylog_info.rwlock);
    g_cylog_info.log_cb = log_cb;
    pthread_rwlock_unlock(&g_cylog_info.rwlock);
}

static void init_cylog_once() {
    g_cylog_info.cb_appender = create_appender(cy_log, NULL, &g_cylog_info);
}

Appender get_callback_appender() {
    pthread_once(&g_cylog_info.init_once, init_cylog_once);
    return g_cylog_info.cb_appender;
}

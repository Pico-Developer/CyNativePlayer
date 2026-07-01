/*
 * Copyright (c) 2026 ByteDance Ltd. and/or its affiliates
 * SPDX-License-Identifier: LGPL-2.1-only
 */

#include <linux/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifdef __ANDROID__
#include <android/log.h>
#endif

#include "utils/nclog/nclog.h"

static const char *g_trace_type_str[] = {
    [NC_TRACE_TYPE_NORMAL] = "NORM",
    [NC_TRACE_TYPE_PERF] = "PERF"
};

struct AppenderImpl {
    LogFunc log_func;
    DestroyFunc destroy_func;
    void *data;
} AppenderImpl;

void *user_data_from_appender(Appender appender) {
    return appender->data;
}

Appender create_appender(LogFunc log_func, DestroyFunc destroy_func, void *data) {
    struct AppenderImpl *impl = calloc(1, sizeof(*impl));
    if (!impl) return NULL;

    impl->log_func = log_func;
    impl->destroy_func = destroy_func;
    impl->data = data;
    return impl;
}

void destroy_appender(Appender appender) {
    if (appender && appender->destroy_func)
        appender->destroy_func(appender);

    free(appender);
}

void nclog(Appender appender, TraceType type, LogLevel level, const char *tag, const char *fmt_str, ...) {
    if (!appender || !appender->log_func) return;
#ifndef NCLOG_DISABLED
    va_list args;
    va_start(args, fmt_str);
    appender->log_func(appender, type, level, tag, fmt_str, args);
    va_end(args);
#endif
}

#ifdef __ANDROID__
static void android_appender_log(struct AppenderImpl *impl, TraceType type, LogLevel level, const char *tag, const char *fmt_str, va_list args) {
    static int lm[] = {
        [NC_LOG_LEVEL_E] = ANDROID_LOG_ERROR,
        [NC_LOG_LEVEL_W] = ANDROID_LOG_WARN,
        [NC_LOG_LEVEL_D] = ANDROID_LOG_DEBUG,
        [NC_LOG_LEVEL_I] = ANDROID_LOG_INFO,
        [NC_LOG_LEVEL_V] = ANDROID_LOG_VERBOSE
    };

    char new_fmt_str[4096];
    snprintf(new_fmt_str, sizeof(new_fmt_str), "TraceType %s %s", g_trace_type_str[type], fmt_str);

    __android_log_vprint(lm[level], tag, new_fmt_str, args);
}
#endif

static void default_appender_log(struct AppenderImpl *impl, TraceType type, LogLevel level, const char *tag, const char *fmt_str, va_list args) {
    static char lm[] = {
        [NC_LOG_LEVEL_E] = 'E',
        [NC_LOG_LEVEL_W] = 'W',
        [NC_LOG_LEVEL_D] = 'D',
        [NC_LOG_LEVEL_I] = 'I',
        [NC_LOG_LEVEL_V] = 'V'
    };

    struct timeval tv;
    gettimeofday(&tv, NULL);
    time_t ts = tv.tv_sec;
    struct tm t;
    localtime_r(&ts, &t);


    char new_fmt_str[4096];
    snprintf(new_fmt_str, sizeof(new_fmt_str), "%04d-%02d-%02d %02d:%02d:%02d.%03d %c %-5s: TraceType %s %s\n",
    t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec, (int)tv.tv_usec / 1000, lm[level], tag, g_trace_type_str[type], fmt_str);
    vprintf(new_fmt_str, args);
}

Appender default_appender() {
#ifdef __ANDROID__
    static struct AppenderImpl appender_impl = {
        .log_func = android_appender_log,
        .destroy_func = NULL,
        .data = NULL
    };
    return &appender_impl;
#else
    static struct AppenderImpl appender_impl = {
       .log_func = default_appender_log,
       .destroy_func = NULL,
       .data = NULL
    };
    return &appender_impl;
#endif
}

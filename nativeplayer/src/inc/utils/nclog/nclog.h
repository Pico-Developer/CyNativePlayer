/*
 * Copyright (c) 2026 ByteDance Ltd. and/or its affiliates
 * SPDX-License-Identifier: LGPL-2.1-only
 */

#ifndef INC_NCLOG_NCLOG_H
#define INC_NCLOG_NCLOG_H

#include <stdarg.h>

#ifndef TAG
#define TAG "null"
#endif

#define NCLOGE(FMT, ...) nclog(default_appender(), NC_TRACE_TYPE_NORMAL, NC_LOG_LEVEL_E, (TAG), (FMT), ##__VA_ARGS__)
#define NCLOGW(FMT, ...) nclog(default_appender(), NC_TRACE_TYPE_NORMAL, NC_LOG_LEVEL_W, (TAG), (FMT), ##__VA_ARGS__)
#define NCLOGI(FMT, ...) nclog(default_appender(), NC_TRACE_TYPE_NORMAL, NC_LOG_LEVEL_I, (TAG), (FMT), ##__VA_ARGS__)
#define NCLOGD(FMT, ...) nclog(default_appender(), NC_TRACE_TYPE_NORMAL, NC_LOG_LEVEL_D, (TAG), (FMT), ##__VA_ARGS__)
#define NCLOGV(FMT, ...) nclog(default_appender(), NC_TRACE_TYPE_NORMAL, NC_LOG_LEVEL_V, (TAG), (FMT), ##__VA_ARGS__)

#ifdef __cplusplus
extern "C" {
#endif

typedef enum LogLevel {
    NC_LOG_LEVEL_E,
    NC_LOG_LEVEL_W,
    NC_LOG_LEVEL_I,
    NC_LOG_LEVEL_D,
    NC_LOG_LEVEL_V
} LogLevel;

typedef enum TraceType {
    NC_TRACE_TYPE_NORMAL,
    NC_TRACE_TYPE_PERF,
} TraceType;

typedef struct AppenderImpl *Appender;
typedef void (*LogFunc)(Appender appender, TraceType type, LogLevel level, const char *tag, const char *fmt_str, va_list args);
typedef void (*DestroyFunc)(Appender appender);

void *user_data_from_appender(Appender appender);
Appender create_appender(LogFunc log_func, DestroyFunc destroy_func, void *data);
void destroy_appender(Appender appender);

Appender default_appender();

void nclog(Appender appender, TraceType type, LogLevel level, const char *tag, const char *fmt_str, ...);


#ifdef __cplusplus
} // extern "C"
#endif

#endif

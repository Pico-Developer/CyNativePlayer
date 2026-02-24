/*
 * Copyright (c) 2026 ByteDance Ltd. and/or its affiliates
 * SPDX-License-Identifier: LGPL-2.1-only
 */

#ifndef PERF_MONITOR_H
#define PERF_MONITOR_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct PerfMonitor PerfMonitor;

int32_t perf_monitor_init(PerfMonitor **pm, const char *package_name, int32_t pid);
int32_t perf_monitor_destroy(PerfMonitor **pm);
int32_t perf_monitor_reset(PerfMonitor *pm);
int32_t perf_monitor_report(PerfMonitor *pm);

int32_t perf_monitor_on_prepare_start(PerfMonitor *pm);
int32_t perf_monitor_on_first_frame_rendered(PerfMonitor *pm);
int32_t perf_monitor_on_seek_start(PerfMonitor *pm);
int32_t perf_monitor_set_media_duration(PerfMonitor *pm, int64_t duration);
int32_t perf_monitor_set_audio_mime(PerfMonitor *pm, const char *mime);
int32_t perf_monitor_set_audio_sample_rate(PerfMonitor *pm, int32_t sample_rate);
int32_t perf_monitor_set_audio_channel_count(PerfMonitor *pm, int32_t channel_count);
int32_t perf_monitor_set_audio_bitrate(PerfMonitor *pm, int32_t bitrate);
int32_t perf_monitor_set_video_mime(PerfMonitor *pm, const char *mime);
int32_t perf_monitor_set_video_resolution(PerfMonitor *pm, int32_t width, int32_t height);
int32_t perf_monitor_set_video_fps(PerfMonitor *pm, int32_t fps);
int32_t perf_monitor_set_video_bitrate(PerfMonitor *pm, int32_t bitrate);
int32_t perf_monitor_on_stop_start(PerfMonitor *pm);
int32_t perf_monitor_on_stop_end(PerfMonitor *pm);
int32_t perf_monitor_on_release_start(PerfMonitor *pm);
int32_t perf_monitor_on_release_end(PerfMonitor *pm);
int32_t perf_monitor_on_error(PerfMonitor *pm, int error_code);
int32_t perf_monitor_on_audio_codec_init_done(PerfMonitor *pm);
int32_t perf_monitor_on_video_codec_init_done(PerfMonitor *pm);
int32_t perf_monitor_on_video_drop_frame(PerfMonitor *pm);
int32_t perf_monitor_on_video_render_frame(PerfMonitor *pm);

#ifdef __cplusplus
}
#endif

#endif

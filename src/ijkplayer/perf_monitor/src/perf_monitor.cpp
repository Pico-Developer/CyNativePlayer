/*
 * Copyright (c) 2026 ByteDance Ltd. and/or its affiliates
 * SPDX-License-Identifier: LGPL-2.1-only
 */

#include <chrono>
#include <dlfcn.h>
#include <cstdint>
#include <random>
#include <stdint.h>
#include <string>
#include <thread>
#include <mutex>
#include <optional>
#include <chrono>
#include "perf_monitor/perf_monitor.h"
#define TAG "PerfMonitor"
#include "utils/nclog/log_util.h"

struct PerfMonitor {
};

int32_t perf_monitor_init(PerfMonitor **pm, const char *package_name, int pid) {
    if (!pm) return -EINVAL;

    *pm = new(std::nothrow) PerfMonitor;
    if (!*pm) {
        return -ENOMEM;
    }

    return 0;
}

int32_t perf_monitor_destroy(PerfMonitor **pm) {
    if (!pm || !*pm) return -EINVAL;

    delete *pm;
    *pm = nullptr;

    CYLOGI("destroy");

    return 0;
}

int32_t perf_monitor_reset(PerfMonitor *pm) {
    if (!pm) return -EINVAL;


    CYLOGI("%s: reset", __func__);
    return 0;
}

int32_t perf_monitor_report(PerfMonitor *pm) {
    return 0;
}

int32_t perf_monitor_on_prepare_start(PerfMonitor *pm) {
    if (!pm) return -EINVAL;
    return 0;
}

int32_t perf_monitor_on_first_frame_rendered(PerfMonitor *pm) {
    if (!pm) return -EINVAL;
    return 0;
}

int32_t perf_monitor_on_seek_start(PerfMonitor *pm) {
    if (!pm) return -EINVAL;
    return 0;
}

int32_t perf_monitor_set_media_duration(PerfMonitor *pm, int64_t duration) {
    if (!pm) return -EINVAL;
    return 0;
}

int32_t perf_monitor_set_audio_mime(PerfMonitor *pm, const char *mime) {
    if (!pm) return -EINVAL;
    return 0;
}

int32_t perf_monitor_set_audio_sample_rate(PerfMonitor *pm, int32_t sample_rate) {
    if (!pm) return -EINVAL;
    return 0;
}

int32_t perf_monitor_set_audio_channel_count(PerfMonitor *pm, int32_t channel_count) {
    if (!pm) return -EINVAL;
    return 0;
}

int32_t perf_monitor_set_audio_bitrate(PerfMonitor *pm, int32_t bitrate) {
    if (!pm) return -EINVAL;
    return 0;
}

int32_t perf_monitor_set_video_mime(PerfMonitor *pm, const char *mime) {
    if (!pm) return -EINVAL;
    return 0;
}

int32_t perf_monitor_set_video_resolution(PerfMonitor *pm, int32_t width, int32_t height) {
    if (!pm) return -EINVAL;
    return 0;
}

int32_t perf_monitor_set_video_fps(PerfMonitor *pm, int32_t fps) {
    if (!pm) return -EINVAL;
    return 0;
}

int32_t perf_monitor_set_video_bitrate(PerfMonitor *pm, int32_t bitrate) {
    if (!pm) return -EINVAL;
    return 0;
}

int32_t perf_monitor_on_stop_start(PerfMonitor *pm) {
    if (!pm) return -EINVAL;
    return 0;
}

int32_t perf_monitor_on_stop_end(PerfMonitor *pm) {
    if (!pm) return -EINVAL;
    return 0;
}
int32_t perf_monitor_on_release_start(PerfMonitor *pm) {
    if (!pm) return -EINVAL;
    return 0;
}

int32_t perf_monitor_on_release_end(PerfMonitor *pm) {
    if (!pm) return -EINVAL;
    return 0;
}

int32_t perf_monitor_on_error(PerfMonitor *pm, int error_code) {
    if (!pm) return -EINVAL;
    return 0;
}

int32_t perf_monitor_on_audio_codec_init_done(PerfMonitor *pm) {
    if (!pm) return -EINVAL;
    return 0;
}

int32_t perf_monitor_on_video_codec_init_done(PerfMonitor *pm) {
    if (!pm) return -EINVAL;
    return 0;
}

int32_t perf_monitor_on_video_drop_frame(PerfMonitor *pm) {
    if (!pm) return -EINVAL;
    return 0;
}

int32_t perf_monitor_on_video_render_frame(PerfMonitor *pm) {
    if (!pm) return -EINVAL;
    return 0;
}

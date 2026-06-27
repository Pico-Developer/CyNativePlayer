#include "ijksdl_thread.h"
#include "ijksdl_aout_android_audiotrack_latency_estimator.h"
#include "android_audiotrack.h"
#include "ijksdl_log.h"
#include <time.h>
#include <stdlib.h>
#include <math.h>

static inline int64_t aout_clamp_i64(int64_t v, int64_t lo, int64_t hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

int64_t linear_interpolate(int64_t old_value, int64_t old_x, int64_t new_x, double rate) {
    double delta = (double)(new_x - old_x) * rate;
    return (int64_t)(old_value + delta);
}


static inline int64_t aout_now_monotonic_ns(void) {
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 0;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)ts.tv_sec * 1000000000LL + (int64_t)ts.tv_nsec;
}

typedef enum AoutTimestampState {
    AOUT_TS_WARMUP = 0,
    AOUT_TS_STABLE = 1,
    AOUT_TS_UNAVAILABLE = 2,
} AoutTimestampState;

struct AoutLatencyEstimator {
    SDL_mutex *mutex;
    SDL_Android_AudioTrack* atrack;

    int sample_rate;
    int bytes_per_frame;

    double playback_speed;
    bool paused;

    int64_t queued_duration_ns;

    int64_t anchor_frame_position_ns;
    int64_t anchor_nano_time;
    bool anchor_pending;

    int64_t last_ts_frame_position_ns;
    int64_t last_ts_nano_time;
    int64_t last_ts_query_mono_ns;
    bool ts_valid;

    AoutTimestampState ts_state;

    int64_t ts_min_interval_ns;

    double last_latency_s;
    
    int64_t latency_bias_ns;
};


AoutLatencyEstimator* aout_latency_estimator_create(SDL_Android_AudioTrack* atrack, int channels, int sample_rate, int bits_per_sample) {
    AoutLatencyEstimator *e = calloc(1, sizeof(AoutLatencyEstimator));
    if (!e) return NULL;
    e->mutex = SDL_CreateMutex();
    if (!e->mutex) {
        CYLOGE("SDL_CreateMutex failed");
        free(e);
        return NULL;
    }

    e->atrack = atrack;
    e->sample_rate = sample_rate;
    e->bytes_per_frame = channels * bits_per_sample / 8;
    e->playback_speed = 1.0;
    e->paused = true;
    e->queued_duration_ns = 0;
    e->anchor_frame_position_ns = 0;
    e->anchor_nano_time = 0;
    e->anchor_pending = true;
    e->last_ts_frame_position_ns = 0;
    e->last_ts_nano_time = 0;
    e->last_ts_query_mono_ns = 0;
    e->ts_valid = false;
    e->ts_state = AOUT_TS_WARMUP;
    e->ts_min_interval_ns = 0;
    e->last_latency_s = 0.0;
    e->latency_bias_ns = 50000000LL;        // 50ms

    return e;
}

void aout_latency_estimator_destroy(AoutLatencyEstimator** pp) {
    if (!pp || !*pp) return;
    AoutLatencyEstimator* e = *pp;
    if (e->mutex) SDL_DestroyMutex(e->mutex);
    free(e);
    *pp = NULL;
}

void aout_latency_estimator_set_min_interval_ns(AoutLatencyEstimator* e, int64_t ns) {
    if (!e) return;
    SDL_LockMutex(e->mutex);
    e->ts_min_interval_ns = ns;
    SDL_UnlockMutex(e->mutex);
}

void aout_latency_estimator_on_flush(AoutLatencyEstimator* e) {
    if (!e) return;
    SDL_LockMutex(e->mutex);
    e->queued_duration_ns = 0;
    e->anchor_frame_position_ns = 0;
    e->anchor_nano_time = 0;
    e->anchor_pending = true;
    e->last_ts_frame_position_ns = 0;
    e->last_ts_nano_time = 0;
    e->last_ts_query_mono_ns = 0;
    e->ts_valid = false;
    e->ts_state = AOUT_TS_WARMUP;
    SDL_UnlockMutex(e->mutex);
}

void aout_latency_estimator_on_write(AoutLatencyEstimator* e, int bytes) {
    if (!e) return;
    SDL_LockMutex(e->mutex);
    e->queued_duration_ns += (int64_t)bytes * 1000000000LL / (int64_t)e->bytes_per_frame / (int64_t)e->sample_rate;
    SDL_UnlockMutex(e->mutex);
}

void aout_latency_estimator_advance_anchor_l(AoutLatencyEstimator* e) {
    int64_t now_ns = aout_now_monotonic_ns();
    if (!e->anchor_pending) {
        int64_t new_anchor = linear_interpolate(e->anchor_frame_position_ns, e->anchor_nano_time, now_ns, e->playback_speed);
        e->anchor_frame_position_ns = new_anchor;
        e->anchor_nano_time = now_ns;
    }
}

void aout_latency_estimator_set_playback_speed(AoutLatencyEstimator* e, float speed) {
    if (!e) return;
    SDL_LockMutex(e->mutex);
    if (!e->paused) {
        aout_latency_estimator_advance_anchor_l(e);
    }
    e->playback_speed = speed;
    SDL_UnlockMutex(e->mutex);
}

void aout_latency_estimator_enter_warmup_l(AoutLatencyEstimator* e) {
    e->ts_state = AOUT_TS_WARMUP;
}

void aout_latency_estimator_on_pause(AoutLatencyEstimator* e, bool pause_on) {
    if (!e) return;
    SDL_LockMutex(e->mutex);
    if (pause_on) {
        // pause, record the anchor if possible
        if (!e->paused) aout_latency_estimator_advance_anchor_l(e);
    } else if (e->paused) {
        // start
        aout_latency_estimator_enter_warmup_l(e);
        if (!e->anchor_pending) {
            // update the anchor time so that we can interpolate correctly for latency
            e->anchor_nano_time = aout_now_monotonic_ns();
        }
    }
    e->paused = pause_on;
    SDL_UnlockMutex(e->mutex);
}

bool aout_latency_estimator_should_query_timestamp_l(AoutLatencyEstimator* e, int64_t now_ns) {
    if (!e) return false;
    bool should_query = false;
    if (!e->paused &&
        ((e->ts_state == AOUT_TS_WARMUP && (e->last_ts_query_mono_ns == 0 || now_ns - e->last_ts_query_mono_ns >= 100000000)) ||
        (e->ts_state == AOUT_TS_STABLE && (e->last_ts_query_mono_ns == 0 || now_ns - e->last_ts_query_mono_ns >= e->ts_min_interval_ns)))
    ) {
        should_query = true;
    }

    return should_query;
}

void aout_latency_estimator_update_timestamp_l(AoutLatencyEstimator* e, int64_t frame_position, int64_t nano_time, int64_t now_ns) {
    if (!e) return;
    if (e->ts_valid) {
        if (e->ts_state == AOUT_TS_WARMUP && frame_position > e->last_ts_frame_position_ns && nano_time > e->last_ts_nano_time) {
            e->ts_state = AOUT_TS_STABLE;
            CYLOGI("ale warmup to stable");
        }
        if (e->ts_state == AOUT_TS_STABLE) {
            if (e->anchor_pending || !e->anchor_pending && nano_time > e->anchor_nano_time) {
                e->anchor_frame_position_ns = frame_position;
                e->anchor_nano_time = nano_time;
            }
            e->anchor_pending = false;
        }
    }
    e->ts_valid = true;
    e->last_ts_frame_position_ns = frame_position;
    e->last_ts_nano_time = nano_time;
}

bool aout_latency_estimator_query_timestamp_helper(AoutLatencyEstimator* e, JNIEnv* env, int64_t* frame_position, int64_t* nano_time) {
    if (!e) return false;
    bool ret = SDL_Android_AudioTrack_getTimestamp(env, e->atrack, frame_position, nano_time);
    if (ret) {
        *frame_position = *frame_position * 1000000000.0 / e->sample_rate;
    }
    return ret;
}

void aout_latency_estimator_query_and_update_timestamp_l(AoutLatencyEstimator* e, JNIEnv* env, int64_t now_ns) {
    if (!e) return;
    int64_t frame_position = 0;
    int64_t nano_time = 0;
    bool ok = aout_latency_estimator_query_timestamp_helper(e, env, &frame_position, &nano_time);
    // CYLOGI("ale query ts %lf %lf", frame_position / 1000000000.0, nano_time / 1000000000.0);
    if (ok) {
        aout_latency_estimator_update_timestamp_l(e, frame_position, nano_time, now_ns);
    } else {
        aout_latency_estimator_enter_warmup_l(e);
    }
    e->last_ts_query_mono_ns = now_ns;
}


double aout_latency_estimator_compute_latency_l(AoutLatencyEstimator* e, int64_t now_ns) {
    if (!e) return 0.0;
    if (!e->anchor_pending) {
        int64_t frame_position_ns = linear_interpolate(e->anchor_frame_position_ns, e->anchor_nano_time, now_ns, e->playback_speed);
        // CYLOGI("ale anchor_fp2 %lf, anchor_s %lf now %lf", e->anchor_frame_position_ns / 1000000000.0, e->anchor_nano_time / 1000000000.0, now_ns / 1000000000.0);
        int64_t latency_ns = aout_clamp_i64(e->queued_duration_ns - frame_position_ns + e->latency_bias_ns, 0, e->queued_duration_ns);
        double latency_s = latency_ns / 1000000000.0;
        return latency_s;
    }
    return e->queued_duration_ns / 1000000000.0;
}

double aout_latency_estimator_get_latency(AoutLatencyEstimator* e, JNIEnv* env) {
    if (!e) return 0.0;
    SDL_LockMutex(e->mutex);

    int64_t now_ns = aout_now_monotonic_ns();
    if (aout_latency_estimator_should_query_timestamp_l(e, now_ns)) {
        aout_latency_estimator_query_and_update_timestamp_l(e, env, now_ns);
    }

    double latency = aout_latency_estimator_compute_latency_l(e, now_ns);
    SDL_UnlockMutex(e->mutex);
    // CYLOGI("ale latency %lf", latency);
    return latency;
}

void aout_latency_estimator_on_recreate(AoutLatencyEstimator* e, SDL_Android_AudioTrack* atrack, int channels, int sample_rate, int bits_per_sample) {
    if (!e) return;
    SDL_LockMutex(e->mutex);
    e->atrack = atrack;
    e->sample_rate = sample_rate;
    e->bytes_per_frame = channels * bits_per_sample / 8;
    e->queued_duration_ns = 0;
    e->anchor_frame_position_ns = 0;
    e->anchor_nano_time = 0;
    e->anchor_pending = true;
    e->last_ts_frame_position_ns = 0;
    e->last_ts_nano_time = 0;
    e->last_ts_query_mono_ns = 0;
    e->ts_valid = false;
    e->ts_state = AOUT_TS_WARMUP;
    SDL_UnlockMutex(e->mutex);
}

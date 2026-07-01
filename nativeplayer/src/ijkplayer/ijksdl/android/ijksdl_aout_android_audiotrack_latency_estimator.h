#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <jni.h>
#include "../ijksdl_thread.h"

typedef struct SDL_Android_AudioTrack SDL_Android_AudioTrack;
typedef struct AoutLatencyEstimator AoutLatencyEstimator;

AoutLatencyEstimator* aout_latency_estimator_create(SDL_Android_AudioTrack* atrack, int channels, int sample_rate, int bits_per_sample);
void aout_latency_estimator_destroy(AoutLatencyEstimator** pp);

void aout_latency_estimator_set_min_interval_ns(AoutLatencyEstimator* e, int64_t ns);

void aout_latency_estimator_on_flush(AoutLatencyEstimator* e);
void aout_latency_estimator_on_write(AoutLatencyEstimator* e, int bytes);

void aout_latency_estimator_set_playback_speed(AoutLatencyEstimator* e, float speed);
void aout_latency_estimator_on_pause(AoutLatencyEstimator* e, bool pause_on);

double aout_latency_estimator_get_latency(AoutLatencyEstimator* e, JNIEnv* env);

void aout_latency_estimator_on_recreate(AoutLatencyEstimator* e, SDL_Android_AudioTrack* atrack, int channels, int sample_rate, int bits_per_sample);
/*
 * Copyright [2025] - [2025] PICO. All rights reserved.
 *
 * NOTICE: All information contained herein is, and remains the property of
 * PICO. The intellectual and technical concepts contained herein are
 * proprietary to PICO. and may be covered by patents, patents in process, and
 * are protected by trade secret or copyright law. Dissemination of this
 * information or reproduction of this material is strictly forbidden unless
 * prior written permission is obtained from PICO.
 */

#ifndef __OPENSLES_SPATIAL_H__
#define __OPENSLES_SPATIAL_H__

#include <SLES/OpenSLES_Android.h>

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------*/
/* Android AudioPlayer configuration                                         */
/*---------------------------------------------------------------------------*/

/** Audio content spatialized
 * Specifies whether the audio data of this output stream has already been processed for
 * spatialization.
 *
 * If the stream has been processed for spatialization, setting this to true will avoid
 * platform to spatialize again
 */
/** Audio content spatialized key */
static const SLchar* const SL_ANDROID_KEY_SPATIAL_CONTENT_SPATIALIZED =
        (const SLchar*)"androidPlaybackContentSpatialized";
/** Audio spatial behaviour type values */
static const SLint32 SL_ANDROID_SPATIAL_CONTENT_NOT_SPATIALIZED = (SLint32)0x00000000;
static const SLint32 SL_ANDROID_SPATIAL_CONTENT_SPATIALIZED = (SLint32)0x00000001;

/** Audio spatialization mode
 * Specifies what kind of spatial type should be used for direction calculation
 */
/** Audio spatialization mode key */
static const SLchar* const SL_ANDROID_KEY_SPATIALIZATION_MODE =
        (const SLchar*)"androidPlaybackSpatializatonMode";
/** Audio spatial spatialization mode values */
/**
 * Constant indicating the audio content associated with this attribute should use default
 * strategy, maybe ambient or channel
 */
static const SLint32 SL_ANDROID_SPATIALIZATION_MODE_DEFAULT = (SLint32)0x00000000;
/**
 * Constant indicating the audio content associated with this attribute should not be spatial
 */
static const SLint32 SL_ANDROID_SPATIALIZATION_MODE_CHANNEL = (SLint32)0x00000001;
/**
 * Constant indicating the audio content associated with this attribute should be 3dof.
 */
static const SLint32 SL_ANDROID_SPATIALIZATION_MODE_AMBIENT = (SLint32)0x00000002;
/**
 * Constant indicating the audio content associated with this attribute should be 6dof.
 */
static const SLint32 SL_ANDROID_SPATIALIZATION_MODE_OBJECT = (SLint32)0x00000003;

/** Audio is ambisonic content
 * Specifies whether the playback should use ambisonic type effect to process
 */
/** Audio ambisonic content key */
static const SLchar* const SL_ANDROID_KEY_AMBISONIC_CONTENT =
        (const SLchar*)"androidPlaybackIsAmbisonicContent";
/** Audio ambisonic content values */
static const SLint32 SL_ANDROID_AMBISNOIC_CONTENT_DISABLE = (SLint32)0x00000000;
static const SLint32 SL_ANDROID_AMBISNOIC_CONTENT_ENABLE = (SLint32)0x00000001;

#ifndef SL_EXT_SYMBOL
#define SL_EXT_SYMBOL __attribute__((weak))
#endif
// do not find symbol in compile, but dont forget to check in runtime
extern SL_API const SLInterfaceID SL_IID_ANDROIDSPATIALAUDIOTRACK SL_EXT_SYMBOL;

struct SLAndroidSpatialAudioTrackItf_;

typedef const struct SLAndroidSpatialAudioTrackItf_* const* SLAndroidSpatialAudioTrackItf;

struct SLAndroidSpatialAudioTrackItf_ {
    SLresult (*AttachToEntity)(SLAndroidSpatialAudioTrackItf self, SLAint64 entityId);

    SLresult (*AttachToContainer)(SLAndroidSpatialAudioTrackItf self, SLAint64 audioBindId);

    SLresult (*EnableSpatializer)(SLAndroidSpatialAudioTrackItf self, SLboolean enable);
};

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // __OPENSLES_SPATIAL_H__
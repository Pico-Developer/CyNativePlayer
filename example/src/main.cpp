#include <android/native_activity.h>
#include <android/log.h>
extern "C" {
#include <NativePlayer/NativePlayer.h>
}
#include "main.h"

#define TAG "NativePlayerDemo"

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

static char videoPath[] = "/path/to/video.mp4";
static CypressPlayer *gPlayer = NULL;

void onNativeWindowCreated(ANativeActivity *activity, ANativeWindow *window) {
    LOGI("onNativeWindowCreated %p", window);

    CypressPlayer_setLogCallback([](int level, const char *tag, const char *msg) {
        LOGI("nativeplayerdemo %s %s", tag, msg);
    });

    CypressPlayer *player = CypressPlayer_create();
    if (player == NULL) {
        LOGE("NativePlayer_create failed");
        return;
    }

    CypressPlayer_setDataSourceAndHeaders(player, videoPath);
    CypressPlayer_setSurface(player, window);
    LOGI("CypressPlayer start prepare %p", player);
    CypressPlayer_prepareSync(player);
    LOGI("CypressPlayer start %p", player);
    CypressPlayer_start(player);
    LOGI("CypressPlayer start %p done", player);

    gPlayer = player;
}

void onNativeWindowDestroyed(ANativeActivity *activity, ANativeWindow *window) {
    LOGI("onNativeWindowDestroyed %p", window);
    if (gPlayer != NULL) {
        CypressPlayer_stop(gPlayer);
        CypressPlayer_release(gPlayer);
        gPlayer = NULL;
    }
}

void ANativeActivity_onCreate(ANativeActivity *activity, void *savedState, size_t savedStateSize) {
    activity->callbacks->onNativeWindowCreated = onNativeWindowCreated;
    activity->callbacks->onNativeWindowDestroyed = onNativeWindowDestroyed;
}


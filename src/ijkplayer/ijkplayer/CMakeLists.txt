#
# Copyright (c) 2013 Bilibili
# Copyright (c) 2013 Zhang Rui <bbcallen@gmail.com>
#
# This file is part of ijkPlayer.
#
# ijkPlayer is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# ijkPlayer is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with ijkPlayer; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

# -- LOCAL_PATH := $(call my-dir)
# -- 
# -- include $(CLEAR_VARS)
# -- # -mfloat-abi=soft is a workaround for FP register corruption on Exynos 4210
# -- # http://www.spinics.net/lists/arm-kernel/msg368417.html
# -- ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
# -- LOCAL_CFLAGS += -mfloat-abi=soft
# -- endif
# -- #LOCAL_CFLAGS += -std=c11
# -- LOCAL_LDLIBS += -llog -landroid
# -- LOCAL_LDFLAGS += -Wl,--exclude-libs,ALL
# -- LOCAL_C_INCLUDES += $(LOCAL_PATH)
# -- LOCAL_C_INCLUDES += $(realpath $(LOCAL_PATH)/..)
# -- LOCAL_C_INCLUDES += $(MY_APP_FFMPEG_INCLUDE_PATH)
# -- LOCAL_C_INCLUDES += $(realpath $(LOCAL_PATH)/../ijkj4a)
# -- 
# -- LOCAL_SRC_FILES += ff_cmdutils.c
# -- LOCAL_SRC_FILES += ff_ffplay.c
# -- LOCAL_SRC_FILES += ff_ffpipeline.c
# -- LOCAL_SRC_FILES += ff_ffpipenode.c
# -- LOCAL_SRC_FILES += ijkmeta.c
# -- LOCAL_SRC_FILES += ijkplayer.c
# -- 
# -- LOCAL_SRC_FILES += pipeline/ffpipeline_ffplay.c
# -- LOCAL_SRC_FILES += pipeline/ffpipenode_ffplay_vdec.c
# -- 
# -- LOCAL_SRC_FILES += android/ffmpeg_api_jni.c
# -- LOCAL_SRC_FILES += android/ijkplayer_android.c
# -- #LOCAL_SRC_FILES += android/ijkplayer_jni.c
# -- 
# -- LOCAL_SRC_FILES += android/pipeline/ffpipeline_android.c
# -- LOCAL_SRC_FILES += android/pipeline/ffpipenode_android_mediacodec_vdec.c
# -- 
# -- LOCAL_SRC_FILES += ijkavformat/allformats.c
# -- LOCAL_SRC_FILES += ijkavformat/cJSON.c
# -- LOCAL_SRC_FILES += ijkavformat/ijklas.c
# -- LOCAL_SRC_FILES += ijkavformat/ijklivehook.c
# -- //LOCAL_SRC_FILES += ijkavformat/ijkmediadatasource.c
# -- LOCAL_SRC_FILES += ijkavformat/ijkio.c
# -- LOCAL_SRC_FILES += ijkavformat/ijkiomanager.c
# -- LOCAL_SRC_FILES += ijkavformat/ijkiocache.c
# -- LOCAL_SRC_FILES += ijkavformat/ijkioffio.c
# -- //LOCAL_SRC_FILES += ijkavformat/ijkioandroidio.c
# -- LOCAL_SRC_FILES += ijkavformat/ijkioprotocol.c
# -- LOCAL_SRC_FILES += ijkavformat/ijkioapplication.c
# -- LOCAL_SRC_FILES += ijkavformat/ijkiourlhook.c
# -- 
# -- LOCAL_SRC_FILES  += ijkavformat/ijkasync.c
# -- LOCAL_SRC_FILES  += ijkavformat/ijkurlhook.c
# -- LOCAL_SRC_FILES  += ijkavformat/ijklongurl.c
# -- LOCAL_SRC_FILES  += ijkavformat/ijksegment.c
# -- 
# -- LOCAL_SRC_FILES += ijkavutil/ijkdict.c
# -- LOCAL_SRC_FILES += ijkavutil/ijkutils.c
# -- LOCAL_SRC_FILES += ijkavutil/ijkthreadpool.c
# -- LOCAL_SRC_FILES += ijkavutil/ijktree.c
# -- LOCAL_SRC_FILES += ijkavutil/ijkfifo.c
# -- LOCAL_SRC_FILES += ijkavutil/ijkstl.cpp
# -- 
# -- LOCAL_SHARED_LIBRARIES := ijkffmpeg  ijksdl
# -- LOCAL_STATIC_LIBRARIES := android-ndk-profiler ijksoundtouch
# -- 
# -- LOCAL_MODULE := ijkplayer
# -- 
# -- VERSION_SH  = $(LOCAL_PATH)/version.sh
# -- VERSION_H   = ijkversion.h
# -- $(info $(shell ($(VERSION_SH) $(LOCAL_PATH) $(VERSION_H))))
# -- #include $(BUILD_STATIC_LIBRARY)
# -- include $(BUILD_SHARED_LIBRARY)


cmake_minimum_required(VERSION 3.22.1)
project("ijkplayer")

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

set(IJKPLAYER_LIB_NAME "ijkplayer")

add_compile_options(
    -Wno-implicit-function-declaration
    -Wno-deprecated-declarations
    -Wno-int-conversion
    -Wno-implicit-const-int-float-conversion
    -Wno-incompatible-function-pointer-types
)

add_library(${IJKPLAYER_LIB_NAME} SHARED

 ff_cmdutils.c
 ff_fferror.c
 ff_ffplay.c
 ff_ffpipeline.c
 ff_ffpipenode.c
 ijkmeta.c
 ijkplayer.c
 backtrace.c

 pipeline/ffpipeline_ffplay.c
 pipeline/ffpipenode_ffplay_vdec.c

 android/ffmpeg_api_jni.c
 android/ijkplayer_android.c
 #android/ijkplayer_jni.c

 android/pipeline/ffpipeline_android.c
 android/pipeline/ffpipenode_android_mediacodec_vdec.c

 ijkavformat/allformats.c
 ijkavformat/cJSON.c
 ijkavformat/ijklas.c
 ijkavformat/ijklivehook.c
 #ijkavformat/ijkmediadatasource.c
 ijkavformat/ijkio.c
 ijkavformat/ijkiomanager.c
 ijkavformat/ijkiocache.c
 ijkavformat/ijkioffio.c
 #ijkavformat/ijkioandroidio.c
 ijkavformat/ijkioprotocol.c
 ijkavformat/ijkioapplication.c
 ijkavformat/ijkiourlhook.c

 ijkavformat/ijkasync.c
 ijkavformat/ijkurlhook.c
 ijkavformat/ijklongurl.c
 ijkavformat/ijksegment.c

 ijkavutil/ijkdict.c
 ijkavutil/ijkutils.c
 ijkavutil/ijkthreadpool.c
 ijkavutil/ijktree.c
 ijkavutil/ijkfifo.c
 ijkavutil/ijkstl.cpp

 ./libavformat/ijkutils.c
 ./libavutil/application.c
 ./libavutil/dict.c
)


target_include_directories(${IJKPLAYER_LIB_NAME}
    PRIVATE ./
    PRIVATE ../
    PRIVATE ../ijkj4a
    PRIVATE ../perf_monitor/include
    PRIVATE ../../inc/ffmpeg
    PRIVATE ../../inc/ffmpeg/config_header/${ANDROID_ABI}
    PRIVATE ../../inc
)

target_link_options(${IJKPLAYER_LIB_NAME} PRIVATE "-Wl,--exclude-libs=ALL")

target_link_directories(${IJKPLAYER_LIB_NAME}
    PRIVATE ../perf_monitor/src/lib/${ANDROID_ABI}
    PRIVATE ../../../lib/${ANDROID_ABI}
)

target_link_libraries(${IJKPLAYER_LIB_NAME}
    PRIVATE log nativewindow ijkffmpeg ijksdl ijkj4a ijksoundtouch perfmonitor
)



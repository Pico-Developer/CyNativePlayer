# Copyright (c) 2013 Bilibili
# copyright (c) 2013 Zhang Rui <bbcallen@gmail.com>
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
# -- #LOCAL_CFLAGS += -std=c11
# -- LOCAL_LDLIBS += -llog -landroid -lOpenSLES -lEGL -lGLESv2 -lmediandk
# -- #LOCAL_LDFLAGS += -Wl,--exclude-libs,ALL
# -- LOCAL_C_INCLUDES += $(LOCAL_PATH)
# -- LOCAL_C_INCLUDES += $(realpath $(LOCAL_PATH)/..)
# -- LOCAL_C_INCLUDES += $(MY_APP_FFMPEG_INCLUDE_PATH)
# -- LOCAL_C_INCLUDES += $(realpath $(LOCAL_PATH)/../ijkyuv/include)
# -- LOCAL_C_INCLUDES += $(realpath $(LOCAL_PATH)/../ijkj4a)
# -- 
# -- LOCAL_SRC_FILES += ijksdl_aout.c
# -- LOCAL_SRC_FILES += ijksdl_audio.c
# -- LOCAL_SRC_FILES += ijksdl_egl.c
# -- LOCAL_SRC_FILES += ijksdl_error.c
# -- LOCAL_SRC_FILES += ijksdl_mutex.c
# -- LOCAL_SRC_FILES += ijksdl_stdinc.c
# -- LOCAL_SRC_FILES += ijksdl_thread.c
# -- LOCAL_SRC_FILES += ijksdl_timer.c
# -- LOCAL_SRC_FILES += ijksdl_vout.c
# -- LOCAL_SRC_FILES += ijksdl_extra_log.c
# -- LOCAL_SRC_FILES += gles2/color.c
# -- LOCAL_SRC_FILES += gles2/common.c
# -- LOCAL_SRC_FILES += gles2/renderer.c
# -- LOCAL_SRC_FILES += gles2/renderer_rgb.c
# -- LOCAL_SRC_FILES += gles2/renderer_yuv420p.c
# -- LOCAL_SRC_FILES += gles2/renderer_yuv444p10le.c
# -- LOCAL_SRC_FILES += gles2/shader.c
# -- LOCAL_SRC_FILES += gles2/fsh/rgb.fsh.c
# -- LOCAL_SRC_FILES += gles2/fsh/yuv420p.fsh.c
# -- LOCAL_SRC_FILES += gles2/fsh/yuv444p10le.fsh.c
# -- LOCAL_SRC_FILES += gles2/vsh/mvp.vsh.c
# -- 
# -- LOCAL_SRC_FILES += dummy/ijksdl_vout_dummy.c
# -- 
# -- LOCAL_SRC_FILES += ffmpeg/ijksdl_vout_overlay_ffmpeg.c
# -- LOCAL_SRC_FILES += ffmpeg/abi_all/image_convert.c
# -- 
# -- LOCAL_SRC_FILES += android/android_audiotrack.c
# -- LOCAL_SRC_FILES += android/android_nativewindow.c
# -- LOCAL_SRC_FILES += android/ijksdl_android_jni.c
# -- LOCAL_SRC_FILES += android/ijksdl_aout_android_audiotrack.c
# -- LOCAL_SRC_FILES += android/ijksdl_aout_android_opensles.c
# -- LOCAL_SRC_FILES += android/ijksdl_codec_android_mediacodec_dummy.c
# -- LOCAL_SRC_FILES += android/ijksdl_codec_android_mediacodec_internal.c
# -- #LOCAL_SRC_FILES += android/ijksdl_codec_android_mediacodec_java.c
# -- LOCAL_SRC_FILES += android/ijksdl_codec_android_mediacodec.c
# -- LOCAL_SRC_FILES += android/ijksdl_codec_android_mediadef.c
# -- #LOCAL_SRC_FILES += android/ijksdl_codec_android_mediaformat_java.c
# -- LOCAL_SRC_FILES += android/ijksdl_codec_android_mediaformat_ndk.c
# -- LOCAL_SRC_FILES += android/ijksdl_codec_android_mediacodec_ndk.c
# -- LOCAL_SRC_FILES += android/ijksdl_codec_android_mediaformat.c
# -- LOCAL_SRC_FILES += android/ijksdl_vout_android_nativewindow.c
# -- LOCAL_SRC_FILES += android/ijksdl_vout_android_surface.c
# -- LOCAL_SRC_FILES += android/ijksdl_vout_overlay_android_mediacodec.c
# -- 
# -- LOCAL_SHARED_LIBRARIES := ijkffmpeg
# -- LOCAL_STATIC_LIBRARIES := cpufeatures yuv_static ijkj4a
# -- 
# -- LOCAL_MODULE := ijksdl
# -- include $(BUILD_SHARED_LIBRARY)
# -- #include $(BUILD_STATIC_LIBRARY)
# -- $(call import-module,android/cpufeatures)


cmake_minimum_required(VERSION 3.10.2)
project("ijksdl")

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

set(SDL_LIB_NAME "ijksdl")

add_library(${SDL_LIB_NAME} SHARED

 ijksdl_aout.c
 ijksdl_audio.c
 ijksdl_egl.c
 ijksdl_error.c
 ijksdl_mutex.c
 ijksdl_stdinc.c
 ijksdl_thread.c
 ijksdl_timer.c
 ijksdl_vout.c
 ijksdl_extra_log.c
 gles2/color.c
 gles2/common.c
 gles2/renderer.c
 gles2/renderer_rgb.c
 gles2/renderer_yuv420p.c
 gles2/renderer_yuv444p10le.c
 gles2/shader.c
 gles2/fsh/rgb.fsh.c
 gles2/fsh/yuv420p.fsh.c
 gles2/fsh/yuv444p10le.fsh.c
 gles2/vsh/mvp.vsh.c

 dummy/ijksdl_vout_dummy.c

 ffmpeg/ijksdl_vout_overlay_ffmpeg.c
 ffmpeg/abi_all/image_convert.c

 android/android_audiotrack.c
 android/android_nativewindow.c
 android/ijksdl_android_jni.c
 android/ijksdl_aout_android_audiotrack.c
 android/ijksdl_aout_android_audiotrack_latency_estimator.c
 android/ijksdl_aout_android_opensles.c
 android/ijksdl_codec_android_mediacodec_dummy.c
 android/ijksdl_codec_android_mediacodec_internal.c
 #android/ijksdl_codec_android_mediacodec_java.c
 android/ijksdl_codec_android_mediacodec.c
 android/ijksdl_codec_android_mediadef.c
 #android/ijksdl_codec_android_mediaformat_java.c
 android/ijksdl_codec_android_mediaformat_ndk.c
 android/ijksdl_codec_android_mediacodec_ndk.c
 android/ijksdl_codec_android_mediaformat.c
 android/ijksdl_vout_android_nativewindow.c
 android/ijksdl_vout_android_surface.c
 android/ijksdl_vout_overlay_android_mediacodec.c
 android/resmgr.c

 ../../utils/nclog/nclog.c
 ../../utils/nclog/log_util.c
)

# Gradle passes ANDROID_STL=c++_shared for all targets. If a shared library is linked by the C++ linker,
# it will automatically get DT_NEEDED: libc++_shared.so.
# ijksdl is mostly C, but it links ijkyuv_static (contains C++ sources). To avoid a runtime dependency
# on libc++_shared.so, statically link the C++ runtime into ijksdl.
set_target_properties(${SDL_LIB_NAME} PROPERTIES
    LINK_FLAGS "-static-libstdc++"
)

target_include_directories(${SDL_LIB_NAME}
    PRIVATE ./
    PRIVATE ../
    PRIVATE ../ijkj4a
    PRIVATE ../ijkyuv/include
    PRIVATE ../../../include
    PRIVATE ../../inc
    PRIVATE ${THIRDPARTY_HEADER_DIR}/cyffmpeg
)

#target_link_options(${SDL_LIB_NAME} PRIVATE "-Wl,--exclude-libs=ALL")

#target_link_directories(${SDL_LIB_NAME} PRIVATE ${CMAKE_SOURCE_DIR/jniLibs}/${ANDROID_ABI})
target_link_libraries(${SDL_LIB_NAME}
    PRIVATE log android OpenSLES EGL GLESv2 mediandk cyffmpeg ijkyuv_static ijkj4a
)

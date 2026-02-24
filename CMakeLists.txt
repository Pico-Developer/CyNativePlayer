# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html.
# For more examples on how to use CMake, see https://github.com/android/ndk-samples.

# Sets the minimum CMake version required for this project.
cmake_minimum_required(VERSION 3.22.1)

# Declares the project name. The project name can be accessed via ${ PROJECT_NAME},
# Since this is the top level CMakeLists.txt, the project name is also accessible
# with ${CMAKE_PROJECT_NAME} (both CMak e variables are in-sync within the top level
# build script scope).
project("nativeplayer")

set(NATIVE_PLAYER_NAME "nativeplayer")

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

# include_directories(PROJECT_SOURCE_DIR)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.
#
# In this top level CMakeLists.txt, ${CMAKE_PROJECT_NAME} is used to define
# the target library name; in the sub-module's CMakeLists.txt, ${PROJECT_NAME}
# is preferred for the same purpose.
#
# In order to load a library into your app from Java/Kotlin, you must call
# System.loadLibrary() and pass the name of the library defined here;
# for GameActivity/NativeActivity derived applications, the same library name must be
# used in the AndroidManifest.xml file.

add_library(${NATIVE_PLAYER_NAME} SHARED
    # List C/C++ source files with relative paths to this CMakeLists.txt.
    src/NativePlayer.c
    src/utils/ncfence/ncfence.c
)

target_include_directories(${NATIVE_PLAYER_NAME}
    PUBLIC ./include
    PRIVATE ./src/inc
    PRIVATE ./src/inc/ffmpeg
    PRIVATE ./src/inc/ffmpeg/config_header/${ANDROID_ABI}
    PRIVATE ./src/ijkplayer
)

add_subdirectory(./src/ijkplayer/ijksoundtouch)
add_subdirectory(./src/ijkplayer/ijkj4a)
add_subdirectory(./src/ijkplayer/ijkyuv)
add_subdirectory(./src/ijkplayer/ijkplayer)
add_subdirectory(./src/ijkplayer/ijksdl)
add_subdirectory(./src/ijkplayer/perf_monitor)

target_link_options(${NATIVE_PLAYER_NAME} PRIVATE "-Wl,--exclude-libs=ALL")
target_link_directories(${NATIVE_PLAYER_NAME}
    PRIVATE lib/${ANDROID_ABI}
)

target_link_libraries(${NATIVE_PLAYER_NAME}        # List libraries link to the target library
    android
    log
    ijkffmpeg
    ijksdl
    ijkplayer
)

add_subdirectory(./example)


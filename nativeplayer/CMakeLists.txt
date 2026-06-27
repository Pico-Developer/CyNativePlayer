# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html.
# For more examples on how to use CMake, see https://github.com/android/ndk-samples.

# Sets the minimum CMake version required for this project.
cmake_minimum_required(VERSION 3.10.2)

# Declares the project name. The project name can be accessed via ${ PROJECT_NAME},
# Since this is the top level CMakeLists.txt, the project name is also accessible
# with ${CMAKE_PROJECT_NAME} (both CMak e variables are in-sync within the top level
# build script scope).
project("nativeplayer")

set(NATIVE_PLAYER_NAME "nativeplayer")

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

get_filename_component(NATIVEPLAYER_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)

if(NOT DEFINED THIRDPARTY_HEADER_DIR)
    set(THIRDPARTY_HEADER_DIR "${NATIVEPLAYER_ROOT_DIR}/../NativePlayer-third-party/include")
endif()

if(NOT DEFINED THIRDPARTY_SO_DIR)
    set(THIRDPARTY_SO_DIR "${NATIVEPLAYER_ROOT_DIR}/../NativePlayer-third-party/libs")
endif()

add_library(cyffmpeg
        SHARED
        IMPORTED)

set_target_properties(cyffmpeg PROPERTIES IMPORTED_LOCATION ${THIRDPARTY_SO_DIR}/${ANDROID_ABI}/libcyffmpeg.so)



include_directories(./include)

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
        NativePlayer.c
        src/utils/ncfence/ncfence.c
        )

target_include_directories(${NATIVE_PLAYER_NAME}
        PUBLIC ./include
        PRIVATE ./src/inc
        PRIVATE ./src/ijkplayer)
include_directories(${THIRDPARTY_HEADER_DIR}/cyffmpeg)

# Specifies libraries CMake should link to your target library. You
# can link libraries from various origins, such as libraries defined in this
# build script, prebuilt third-party libraries, or Android system libraries.
set(NATIVE_PLAYER_OPTIONAL_LIBS "")
set(ISAC_LIBRARY_PATH "${THIRDPARTY_SO_DIR}/${ANDROID_ABI}/libisac.so")
if(EXISTS "${ISAC_LIBRARY_PATH}")
    add_library(isac
            SHARED
            IMPORTED)

    set_target_properties(isac
            PROPERTIES IMPORTED_LOCATION
            "${ISAC_LIBRARY_PATH}")
    list(APPEND NATIVE_PLAYER_OPTIONAL_LIBS isac)
else()
    message(STATUS "Optional isac library not found: ${ISAC_LIBRARY_PATH}; skip linking isac")
endif()


add_subdirectory(./src/ijkplayer/ijksoundtouch)
add_subdirectory(./src/ijkplayer/ijkj4a)
add_subdirectory(./src/ijkplayer/ijkyuv)
add_subdirectory(./src/ijkplayer/ijkplayer)
add_subdirectory(./src/ijkplayer/ijksdl)
add_subdirectory(./src/ijkplayer/perf_monitor)

    # add_library( ijkplayer
    #         SHARED
    #         IMPORTED)
    # 
    # set_target_properties( ijkplayer
    #         PROPERTIES IMPORTED_LOCATION
    #         ${distribution_DIR}/${ANDROID_ABI}/libijkplayer.so)
    # 
    # 
    # add_library( ijksdl
    #         SHARED
    #         IMPORTED)
    # 
    # set_target_properties( ijksdl
    #         PROPERTIES IMPORTED_LOCATION
    #         ${distribution_DIR}/${ANDROID_ABI}/libijksdl.so)
    # 

#add_library( bbqst_spaceos
#        SHARED
#        IMPORTED)
#
#set_target_properties( bbqst_spaceos
#        PROPERTIES IMPORTED_LOCATION
#        ${distribution_DIR}/${ANDROID_ABI}/libbbqst_spaceos.so)


set_target_properties(${NATIVE_PLAYER_NAME} PROPERTIES LINK_FLAGS "-Wl,--exclude-libs=ALL")

target_link_libraries(${NATIVE_PLAYER_NAME}        # List libraries link to the target library
        android
        log
        cyffmpeg
        ${NATIVE_PLAYER_OPTIONAL_LIBS}
        ijksdl
        ijkplayer
    )

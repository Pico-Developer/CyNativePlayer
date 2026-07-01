#!/bin/bash
set -e

top=`dirname $0`
top=`cd $top && pwd`
nativeplayer_top=$top/nativeplayer

echo "Using '$top' as NativePlayer top directory."

case "$1" in
    armv7a|x86|arm64|x86_64|all)
    TARGET_ARCH=$1
    shift
    ;;
    clean)
        cd $top
        shift
        rm -rf build release
        exit 0
    ;;
    *)
    echo "Usage:"
    echo "$0 <armv7a|x86|x86_64|arm64|all|clean> [cmake configure options]"
    echo "Environment:"
    echo "  ANDROID_NDK or ANDROID_NDK_HOME: Android NDK root"
    echo "  NATIVEPLAYER_NDK_VERSION: Android SDK NDK version, defaults to server-aligned 21.1.6352462 (ndk-r21b)"
    echo "  NATIVEPLAYER_THIRDPARTY_DIR: directory containing include/ and libs/"
    echo "  NATIVEPLAYER_THIRDPARTY_HEADER_DIR: third-party header directory"
    echo "  NATIVEPLAYER_THIRDPARTY_SO_DIR: third-party so directory"
    exit 0
    ;;
esac

cd $top

ARCHS=$TARGET_ARCH
if [ "$TARGET_ARCH" = "all" ]; then
    ARCHS="armv7a arm64 x86 x86_64"
fi

is_usable_ndk() {
    local ndk_dir=$1
    [ -f "$ndk_dir/build/cmake/android.toolchain.cmake" ] || return 1
    # Old NDKs keep explicit platforms/ and may not support android-29.
    if [ -d "$ndk_dir/platforms" ] && [ ! -d "$ndk_dir/platforms/android-29" ]; then
        return 1
    fi
    return 0
}

use_ndk_if_available() {
    local ndk_dir=$1
    if [ -n "$ndk_dir" ] && is_usable_ndk "$ndk_dir"; then
        ANDROID_NDK_DIR=$ndk_dir
        return 0
    fi
    return 1
}

PREFERRED_NDK_VERSION=${NATIVEPLAYER_NDK_VERSION:-21.1.6352462}
ANDROID_NDK_DIR=""

# Explicit environment variables have the highest priority. BITS cloud injects
# ANDROID_NDK_HOME=/opt/ndk/android-ndk-r21b, so this path keeps server builds aligned.
for ndk_dir in "$ANDROID_NDK" "$ANDROID_NDK_HOME"; do
    use_ndk_if_available "$ndk_dir" && break
done

if [ -z "$ANDROID_NDK_DIR" ]; then
    for sdk_dir in "$ANDROID_HOME" "$ANDROID_SDK_ROOT"; do
        if [ -n "$sdk_dir" ] && [ -d "$sdk_dir/ndk" ]; then
            use_ndk_if_available "$sdk_dir/ndk/$PREFERRED_NDK_VERSION" && break
        fi
    done
fi

if [ -z "$ANDROID_NDK_DIR" ]; then
    # Common BITS cloud paths for ndk-r21b.
    for ndk_dir in \
        "$NDK_HOME/android-ndk-r21b" \
        "$NDK_HOME/android:r21b" \
        "/opt/ndk/android-ndk-r21b" \
        "/opt/ndk/android:r21b"; do
        use_ndk_if_available "$ndk_dir" && break
    done
fi

if [ -z "$ANDROID_NDK_DIR" ]; then
    # Prefer the installed r21 series locally before falling back to newer NDKs,
    # so local builds reproduce server/compiler behavior as closely as possible.
    for sdk_dir in "$ANDROID_HOME" "$ANDROID_SDK_ROOT"; do
        if [ -n "$sdk_dir" ] && [ -d "$sdk_dir/ndk" ]; then
            for ndk_dir in `ls -d "$sdk_dir"/ndk/21.* 2>/dev/null | sort -V`; do
                use_ndk_if_available "$ndk_dir" && break
            done
        fi
        [ -n "$ANDROID_NDK_DIR" ] && break
    done
fi

if [ -z "$ANDROID_NDK_DIR" ]; then
    for sdk_dir in "$ANDROID_HOME" "$ANDROID_SDK_ROOT"; do
        if [ -n "$sdk_dir" ] && [ -d "$sdk_dir/ndk" ]; then
            for ndk_dir in `ls -d "$sdk_dir"/ndk/* 2>/dev/null | sort -Vr`; do
                use_ndk_if_available "$ndk_dir" && break
            done
        fi
        [ -n "$ANDROID_NDK_DIR" ] && break
    done
fi
if [ -z "$ANDROID_NDK_DIR" ] || [ ! -f "$ANDROID_NDK_DIR/build/cmake/android.toolchain.cmake" ]; then
    echo "Error: Android NDK with android-29 support not found. Please set ANDROID_NDK or ANDROID_NDK_HOME."
    exit 1
fi

echo "Using Android NDK: $ANDROID_NDK_DIR"

THIRDPARTY_DIR=${NATIVEPLAYER_THIRDPARTY_DIR:-$top/../NativePlayer-third-party}
THIRDPARTY_HEADER_DIR=${NATIVEPLAYER_THIRDPARTY_HEADER_DIR:-$THIRDPARTY_DIR/include}
THIRDPARTY_SO_DIR=${NATIVEPLAYER_THIRDPARTY_SO_DIR:-$THIRDPARTY_DIR/libs}

if [ ! -d "$THIRDPARTY_HEADER_DIR" ]; then
    echo "Error: third-party header directory not found: $THIRDPARTY_HEADER_DIR"
    exit 1
fi
if [ ! -d "$THIRDPARTY_SO_DIR" ]; then
    echo "Error: third-party so directory not found: $THIRDPARTY_SO_DIR"
    exit 1
fi

cmake_bin=${CMAKE_BIN:-cmake}

# Build directly with CMake. No Gradle is required inside NativePlayer-opensource.
for ARCH in $ARCHS; do
    ABI=$ARCH
    if [ "$ARCH" = "armv7a" ]; then
        ABI="armeabi-v7a"
    elif [ "$ARCH" = "arm64" ]; then
        ABI="arm64-v8a"
    fi

    build_dir=$top/build/cmake/$ABI
    output_dir=$build_dir/obj/$ABI
    mkdir -p "$output_dir"

    "$cmake_bin" \
        -S "$nativeplayer_top" \
        -B "$build_dir" \
        -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK_DIR/build/cmake/android.toolchain.cmake" \
        -DANDROID_ABI="$ABI" \
        -DANDROID_PLATFORM=android-29 \
        -DANDROID_STL=c++_static \
        -DCMAKE_CXX_STANDARD=17 \
        -DCMAKE_CXX_STANDARD_REQUIRED=ON \
        -DCMAKE_LIBRARY_OUTPUT_DIRECTORY="$output_dir" \
        -DTHIRDPARTY_HEADER_DIR="$THIRDPARTY_HEADER_DIR" \
        -DTHIRDPARTY_SO_DIR="$THIRDPARTY_SO_DIR" \
        "$@"

    "$cmake_bin" --build "$build_dir" --target nativeplayer --config Release
done

# Collect outputs
echo "copy nativeplayer output to release dir"
for ARCH in $ARCHS; do
    ABI=$ARCH
    if [ "$ARCH" = "armv7a" ]; then
        ABI="armeabi-v7a"
    elif [ "$ARCH" = "arm64" ]; then
        ABI="arm64-v8a"
    fi

    rm -rf $top/release/nativeplayer-$ARCH/include/
    rm -rf $top/release/nativeplayer-$ARCH/lib/
    mkdir -p $top/release/nativeplayer-$ARCH/include/
    mkdir -p $top/release/nativeplayer-$ARCH/lib/
    
    # Copy public headers only. FFmpeg headers are provided by the external third-party directory.
    cp $top/nativeplayer/NativePlayer.h $top/release/nativeplayer-$ARCH/include/
    cp -r $top/nativeplayer/include/CypressNCPlayer $top/release/nativeplayer-$ARCH/include/

    for SO_NAME in libnativeplayer.so libijkplayer.so libijksdl.so; do
        SO_PATH=$top/build/cmake/$ABI/obj/$ABI/$SO_NAME
        if [ -f "$SO_PATH" ]; then
            cp "$SO_PATH" $top/release/nativeplayer-$ARCH/lib/
        else
            echo "Error: $SO_NAME not found for $ARCH at $SO_PATH"
            exit 1
        fi
    done
done

exit 0

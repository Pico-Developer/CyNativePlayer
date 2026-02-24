
if [ -z "$ANDROID_NDK_HOME" ]; then
    echo "need ndk to compile, set ANDROID_NDK_HOME env variable please"
    exit
fi

ABI="${ABI:-"arm64-v8a"}"
BUILD_TYPE="${BUILD_TYPE:-"Debug"}"
BUILD_PATH="${BUILD_PATH:-"$(pwd)/build"}"
LIB_OUT_PATH="${LIB_OUT_PATH:-"${BUILD_PATH}/out/${BUILD_TYPE}/${ABI}"}"

echo "------------------"
echo "ndk home: $ANDROID_NDK_HOME"
echo "ABI: $ABI"
echo "build type: $BUILD_TYPE"
echo "build path: $BUILD_PATH"
echo "library output path: ${LIB_OUT_PATH}"
echo "---------\n\n\n"


cmake                                                               \
      -DCMAKE_SYSTEM_NAME=Android                                   \
      -DCMAKE_EXPORT_COMPILE_COMMANDS=ON                            \
      -DCMAKE_SYSTEM_VERSION=29                                     \
      -DANDROID_PLATFORM=android-29                                 \
      -DANDROID_ABI=${ABI}                                          \
      -DCMAKE_ANDROID_ARCH_ABI=${ABI}                               \
      -DANDROID_NDK=${ANDROID_NDK_HOME}                             \
      -DCMAKE_ANDROID_NDK=${ANDROID_NDK_HOME}                       \
      -DCMAKE_TOOLCHAIN_FILE=${ANDROID_NDK_HOME}/build/cmake/android.toolchain.cmake \
      -DCMAKE_CXX_FLAGS=-std=c++17                                  \
      -DCMAKE_LIBRARY_OUTPUT_DIRECTORY=${LIB_OUT_PATH}              \
      -DCMAKE_RUNTIME_OUTPUT_DIRECTORY=${LIB_OUT_PATH}              \
      -DCMAKE_BUILD_TYPE=${BUILD_TYPE}                              \
      -B${BUILD_PATH}                                               \
      -GNinja



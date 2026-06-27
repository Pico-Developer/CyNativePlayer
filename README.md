# NativePlayer Open-Source Build Guide

This directory can independently build NativePlayer with CMake. Before building, prepare the Android NDK, CMake, Ninja, and the FFmpeg headers and shared libraries. The FFmpeg headers and dynamic libraries are not included in this directory and must be imported by developers separately.

## Dependency Location

By default, third-party dependencies are read from a directory at the same level as `NativePlayer-opensource/`:

```text
NativePlayer-third-party/
├── include/
│   └── cyffmpeg/
│       ├── libavcodec/
│       ├── libavformat/
│       ├── libavutil/
│       ├── libswresample/
│       └── libswscale/
└── libs/
    ├── arm64-v8a/
    │   └── libcyffmpeg.so
    ├── armeabi-v7a/
    │   └── libcyffmpeg.so
    ├── x86/
    │   └── libcyffmpeg.so
    └── x86_64/
        └── libcyffmpeg.so
```

Place the FFmpeg headers under `include/cyffmpeg/`. `libs/<abi>/libcyffmpeg.so` is the FFmpeg dynamic library for the corresponding ABI. The FFmpeg files and shared libraries listed above are external dependencies and are not provided with the `NativePlayer-opensource/` directory. Developers must prepare and import them according to the FFmpeg version, build configuration, and license requirements they use.

You can also specify the paths through environment variables:

```bash
export NATIVEPLAYER_THIRDPARTY_HEADER_DIR=/path/to/NativePlayer-third-party/include
export NATIVEPLAYER_THIRDPARTY_SO_DIR=/path/to/NativePlayer-third-party/libs
```

## Build

Set the NDK first:

```bash
export ANDROID_NDK=/path/to/android-ndk
```

Build all ABIs:

```bash
./build-nativeplayer.sh all
```

Build a single ABI only:

```bash
./build-nativeplayer.sh arm64
```

Available options: `armv7a`, `arm64`, `x86`, `x86_64`, `all`.

## Output

Build artifacts are generated at:

```text
release/nativeplayer-<arch>/
├── include/
└── lib/
    ├── libnativeplayer.so
    ├── libijkplayer.so
    └── libijksdl.so
```

Clean build artifacts:

```bash
./build-nativeplayer.sh clean
```

## License

NativePlayer is modified from [ijkplayer](https://github.com/Bilibili/ijkplayer). The project as a whole is licensed under `LGPLv2.1`. See [COPYING.LGPLv2.1](COPYING.LGPLv2.1) for details.

ijkplayer required features are based on or derives from projects below:

- LGPL
  - [FFmpeg](http://git.videolan.org/?p=ffmpeg.git)
  - [libVLC](http://git.videolan.org/?p=vlc.git)
  - [kxmovie](https://github.com/kolyvan/kxmovie)
  - [soundtouch](http://www.surina.net/soundtouch/sourcecode.html)
- zlib license
  - [SDL](http://www.libsdl.org)
- BSD-style license
  - [libyuv](https://code.google.com/p/libyuv/)
- ISC license
  - [libyuv/source/x86inc.asm](https://code.google.com/p/libyuv/source/browse/trunk/source/x86inc.asm)

android/ijkplayer-exo is based on or derives from projects below:

- Apache License 2.0
  - [ExoPlayer](https://github.com/google/ExoPlayer)

android/example is based on or derives from projects below:

- GPL
  - [android-ndk-profiler](https://github.com/richq/android-ndk-profiler) (not included by default)

ios/IJKMediaDemo is based on or derives from projects below:

- Unknown license
  - [iOS7-BarcodeScanner](https://github.com/jpwiddy/iOS7-BarcodeScanner)

ijkplayer's build scripts are based on or derives from projects below:

- [gas-preprocessor](http://git.libav.org/?p=gas-preprocessor.git)
- [VideoLAN](http://git.videolan.org)
- [yixia/FFmpeg-Android](https://github.com/yixia/FFmpeg-Android)
- [kewlbear/FFmpeg-iOS-build-script](https://github.com/kewlbear/FFmpeg-iOS-build-script)

### Commercial Use

ijkplayer is licensed under LGPLv2.1 or later, so itself is free for commercial use under LGPLv2.1 or later.

But ijkplayer is also based on other different projects under various licenses, which I have no idea whether they are compatible to each other or to your product.

[IANAL](https://en.wikipedia.org/wiki/IANAL), you should always ask your lawyer for these stuffs before use it in your product.

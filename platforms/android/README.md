# Android Shell

This directory contains a minimal SDLActivity-based Android app wrapper for Dusk.

## Prerequisites

- Android SDK installed (`ANDROID_HOME`)
- Android NDK version used by CMake presets (`ANDROID_NDK_VERSION`)
- JDK 17+

Example:

```bash
export ANDROID_HOME="$HOME/Android/Sdk"
export ANDROID_NDK_VERSION="29.0.14206865"
export JAVA_HOME="/usr/lib/jvm/java-17-openjdk"
```

## Build Native Libraries

```bash
cmake --preset android-armv7
cmake --build --preset android-armv7

cmake --preset android-arm64
cmake --build --preset android-arm64

cmake --preset android-x86_64
cmake --build --preset android-x86_64
```

These builds produce:

- `build/android-armv7/libmain.so`
- `build/android-arm64/libmain.so`
- `build/android-x86_64/libmain.so`

## Stage Libraries Into APK Project

```bash
# Default stages armeabi-v7a, arm64-v8a, and x86_64 when all three libs exist.
# For an Android TV that only supports 32-bit ARM, build/package just armeabi-v7a:
ANDROID_STAGE_ABIS=armeabi-v7a ./android/scripts/stage-jni-libs.sh
```

This copies:

- `libmain.so` -> `android/app/src/main/jniLibs/armeabi-v7a/`
- `libmain.so` -> `android/app/src/main/jniLibs/arm64-v8a/`
- `libmain.so` -> `android/app/src/main/jniLibs/x86_64/`

## Refresh SDL Java Shim (Optional)

If you update SDL and want to refresh the embedded Java shim files:

```bash
./android/scripts/sync-sdl-java.sh
```

## Build APK

```bash
cd android
./gradlew :app:assembleDebug
```

Output APK:

- `android/app/build/outputs/apk/debug/app-debug.apk`

## Launch With Runtime Args (adb)

You can pass command-line args through the activity intent:

```bash
adb shell am start -n dev.twilitrealm.dusk/.DuskActivity \
  --es dusk_args "--backend vulkan"
```

Supported extras:

- `dusk_args`: single shell-like argument string
- `dusk_argv`: string-array argv

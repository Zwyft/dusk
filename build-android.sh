#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ENV_FILE="$SCRIPT_DIR/.build-env"

if [ -f "$ENV_FILE" ]; then
    source "$ENV_FILE"
else
    echo "Run ./setup-build-env.sh first, or create .build-env with ANDROID_HOME"
    exit 1
fi

echo "=== Building Dusk Android (arm64-v8a) ==="
cd "$SCRIPT_DIR"

echo "[1/4] Configuring CMake..."
cmake --preset android-arm64

echo "[2/4] Building native libs..."
cmake --build --preset android-arm64

echo "[3/4] Staging JNI libs..."
./platforms/android/scripts/stage-jni-libs.sh

echo "[4/4] Assembling APK..."
cd platforms/android
chmod +x gradlew
./gradlew :app:assembleDebug

echo ""
echo "=== Build complete ==="
echo "APK: platforms/android/app/build/outputs/apk/debug/app-arm64-v8a-debug.apk"

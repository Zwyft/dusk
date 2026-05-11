#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
TOOL_DIR="$SCRIPT_DIR/.build-tools"
ENV_FILE="$SCRIPT_DIR/.build-env"

echo "=== Dusk Local Build Setup ==="

# Install system dependencies (requires sudo)
echo "[1/4] Installing system packages..."
sudo apt-get update -qq
sudo apt-get install -y cmake ninja-build g++-13 libfuse2 \
    libcurl4-openssl-dev libssl-dev libglu1-mesa-dev libasound2-dev \
    libpulse-dev libudev-dev libx11-xcb-dev libxrandr-dev libxi-dev \
    libxcursor-dev libxinerama-dev libdbus-1-dev libpng-dev \
    libvulkan-dev zlib1g-dev libfreetype-dev libgtk-3-dev \
    libdecor-0-dev libpipewire-0.3-dev libunwind-dev \
    libusb-1.0-0-dev libxss-dev

# Install Rust if not present
echo "[2/4] Checking Rust..."
if ! command -v rustc &>/dev/null; then
    curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y
    source "$HOME/.cargo/env"
fi
rustup target add aarch64-linux-android x86_64-linux-android 2>/dev/null || true

# Install Android SDK/NDK via sdkmanager
echo "[3/4] Checking Android SDK..."
ANDROID_HOME="${ANDROID_HOME:-$HOME/Android/Sdk}"
if [ ! -d "$ANDROID_HOME/ndk" ]; then
    echo "Android SDK not found at $ANDROID_HOME"
    echo "Please install Android Studio or set ANDROID_HOME in .build-env"
    echo "  ANDROID_HOME=/path/to/your/Android/Sdk"
    exit 1
fi

NDK_VERSION="29.0.14206865"
if [ ! -d "$ANDROID_HOME/ndk/$NDK_VERSION" ]; then
    echo "Installing NDK $NDK_VERSION..."
    "$ANDROID_HOME/cmdline-tools/latest/bin/sdkmanager" \
        "ndk;$NDK_VERSION" "platforms;android-35" "build-tools;35.0.0"
fi

# Init submodules
echo "[4/4] Initializing submodules..."
cd "$SCRIPT_DIR"
git submodule update --init --recursive

# Write env file
cat > "$ENV_FILE" <<EOF
export ANDROID_HOME="$ANDROID_HOME"
export ANDROID_NDK_VERSION="$NDK_VERSION"
export CC=gcc-13
export CXX=g++-13
export PATH="\$ANDROID_HOME/cmdline-tools/latest/bin:\$ANDROID_HOME/platform-tools:\$PATH"
EOF

echo ""
echo "=== Setup complete ==="
echo "Run:  source .build-env"
echo "Then: ./build-android.sh"

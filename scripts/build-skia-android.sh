#!/bin/bash
# scripts/build-skia-android.sh
# Builds Skia for Android (arm64 and x86_64)

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
SKIA_DIR="$PROJECT_ROOT/third_party/skia"

# Add depot_tools to PATH (required for gn and ninja)
export PATH="$PROJECT_ROOT/third_party/depot_tools:$PATH"

echo "=== Building Skia for Android ==="

# Find NDK
if [ -n "$ANDROID_NDK_HOME" ]; then
    NDK_PATH="$ANDROID_NDK_HOME"
elif [ -n "$ANDROID_HOME" ]; then
    NDK_PATH=$(find "$ANDROID_HOME/ndk" -maxdepth 1 -type d -name '2*' 2>/dev/null | sort -V | tail -1)
elif [ -d "$HOME/Android/Sdk/ndk" ]; then
    NDK_PATH=$(find "$HOME/Android/Sdk/ndk" -maxdepth 1 -type d -name '2*' 2>/dev/null | sort -V | tail -1)
fi

if [ -z "$NDK_PATH" ] || [ ! -d "$NDK_PATH" ]; then
    echo "ERROR: Android NDK not found"
    echo "Please set ANDROID_NDK_HOME or install NDK via Android Studio"
    exit 1
fi

echo "Using NDK: $NDK_PATH"

cd "$SKIA_DIR"

# Common build args for minimal Android build
COMMON_ARGS='
is_official_build=true
is_debug=false
skia_use_system_expat=false
skia_use_system_libpng=false
skia_use_system_libwebp=false
skia_use_system_zlib=false
skia_use_system_libjpeg_turbo=false
skia_use_system_freetype2=false
'

# Build arm64 (physical devices)
echo ""
echo "[1/2] Building Skia for arm64-v8a (physical devices)..."
bin/gn gen out/android-arm64 --args="
target_os=\"android\"
target_cpu=\"arm64\"
ndk=\"$NDK_PATH\"
$COMMON_ARGS
"
ninja -C out/android-arm64 skia

# Build x86_64 (emulators)
echo ""
echo "[2/2] Building Skia for x86_64 (emulators)..."
bin/gn gen out/android-x64 --args="
target_os=\"android\"
target_cpu=\"x64\"
ndk=\"$NDK_PATH\"
$COMMON_ARGS
"
ninja -C out/android-x64 skia

echo ""
echo "=== Skia build complete! ==="
echo "  arm64: $SKIA_DIR/out/android-arm64/libskia.a"
echo "  x64:   $SKIA_DIR/out/android-x64/libskia.a"

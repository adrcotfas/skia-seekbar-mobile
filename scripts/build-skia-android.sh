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

# Find NDK - check multiple common locations
find_ndk() {
    local sdk_path="$1"
    if [ -d "$sdk_path/ndk" ]; then
        find "$sdk_path/ndk" -maxdepth 1 -type d -name '2*' 2>/dev/null | sort -V | tail -1
    fi
}

if [ -n "$ANDROID_NDK_HOME" ] && [ -d "$ANDROID_NDK_HOME" ]; then
    NDK_PATH="$ANDROID_NDK_HOME"
elif [ -n "$ANDROID_HOME" ]; then
    NDK_PATH=$(find_ndk "$ANDROID_HOME")
elif [ -d "$HOME/Android/Sdk" ]; then
    # Linux default
    NDK_PATH=$(find_ndk "$HOME/Android/Sdk")
elif [ -d "$HOME/Library/Android/sdk" ]; then
    # macOS default
    NDK_PATH=$(find_ndk "$HOME/Library/Android/sdk")
elif [ -d "/usr/local/lib/android/sdk" ]; then
    # CI environments
    NDK_PATH=$(find_ndk "/usr/local/lib/android/sdk")
fi

if [ -z "$NDK_PATH" ] || [ ! -d "$NDK_PATH" ]; then
    echo "ERROR: Android NDK not found"
    echo ""
    echo "Checked locations:"
    echo "  - \$ANDROID_NDK_HOME: ${ANDROID_NDK_HOME:-not set}"
    echo "  - \$ANDROID_HOME/ndk: ${ANDROID_HOME:-not set}"
    echo "  - \$HOME/Android/Sdk/ndk"
    echo "  - \$HOME/Library/Android/sdk/ndk (macOS)"
    echo ""
    echo "Please either:"
    echo "  1. Set ANDROID_NDK_HOME to your NDK path, e.g.:"
    echo "     export ANDROID_NDK_HOME=\$HOME/Library/Android/sdk/ndk/27.0.12077973"
    echo "  2. Or set ANDROID_HOME to your SDK path"
    echo "  3. Or install NDK via Android Studio (SDK Manager > SDK Tools > NDK)"
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
skia_enable_pdf=false
skia_use_harfbuzz=false
skia_use_icu=false
skia_enable_skottie=false
skia_enable_skshaper=false
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

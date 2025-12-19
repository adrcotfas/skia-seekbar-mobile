#!/bin/bash
# scripts/build-skia-ios.sh
# Builds Skia for iOS devices (arm64 only, no simulator support)

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
SKIA_DIR="$PROJECT_ROOT/third_party/skia"

# Add depot_tools to PATH (required for gn and ninja)
export PATH="$PROJECT_ROOT/third_party/depot_tools:$PATH"

echo "=== Building Skia for iOS ==="

cd "$SKIA_DIR"

# Build args for minimal iOS build
BUILD_ARGS='
target_os="ios"
target_cpu="arm64"
is_official_build=true
is_debug=false
skia_use_system_expat=false
skia_use_system_libpng=false
skia_use_system_libwebp=false
skia_use_system_zlib=false
skia_use_system_libjpeg_turbo=false
skia_use_system_freetype2=false
skia_use_fonthost_mac=true
skia_enable_pdf=false
skia_use_harfbuzz=false
skia_use_icu=false
skia_enable_skottie=false
skia_enable_skshaper=false
'

# Build arm64 for iOS devices
echo ""
echo "Building Skia for iOS arm64 (devices only)..."
bin/gn gen out/ios-arm64 --args="$BUILD_ARGS"
ninja -C out/ios-arm64 skia

echo ""
echo "=== Skia build complete! ==="
echo "  Device arm64: $SKIA_DIR/out/ios-arm64/libskia.a"

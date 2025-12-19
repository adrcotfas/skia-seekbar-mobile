#!/bin/bash
# scripts/build-ios.sh
# Generates Xcode project and builds for iOS

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/ios/build"

echo "=== Building Skia SeekBar for iOS ==="

# Check if Skia is built
if [ ! -f "$PROJECT_ROOT/third_party/skia/out/ios-arm64/libskia.a" ]; then
    echo "Skia for iOS not found. Building..."
    "$SCRIPT_DIR/build-skia-ios.sh"
fi

# Generate Xcode project
echo ""
echo "Generating Xcode project..."
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

cmake "$PROJECT_ROOT" \
    -G Xcode \
    -DCMAKE_SYSTEM_NAME=iOS \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0 \
    -DCMAKE_XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH=NO

echo ""
echo "=== Xcode project generated! ==="
echo "Open: $BUILD_DIR/skia_player_demo.xcodeproj"
echo ""
echo "To build from command line:"
echo "  cd $BUILD_DIR"
echo "  xcodebuild -scheme SkiaSeekBar -configuration Debug -destination 'platform=iOS Simulator,name=iPhone 16'"

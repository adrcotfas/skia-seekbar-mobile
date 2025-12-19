# CMakeLists.txt
cmake_minimum_required(VERSION 3.22.1)

# Only enable Objective-C languages for iOS (not Android)
if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    project(skia_player_demo VERSION 0.1.0 LANGUAGES CXX C OBJC OBJCXX)
else()
    project(skia_player_demo VERSION 0.1.0 LANGUAGES CXX C)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# =============================================================================
# SDL3 (built from submodule)
# =============================================================================
if(IOS)
    set(SDL_SHARED OFF CACHE BOOL "")
    set(SDL_STATIC ON CACHE BOOL "Build SDL3 static library")
else()
    set(SDL_SHARED ON CACHE BOOL "Build SDL3 shared library")
    set(SDL_STATIC OFF CACHE BOOL "")
endif()
set(SDL_TEST OFF CACHE BOOL "")
add_subdirectory(third_party/SDL)

# Prevent SDL3 from being included in iOS archive
if(IOS AND TARGET SDL3-static)
    set_target_properties(SDL3-static PROPERTIES
        XCODE_ATTRIBUTE_SKIP_INSTALL "YES"
    )
endif()

# =============================================================================
# Skia (prebuilt from our build scripts)
# =============================================================================
if(ANDROID)
    if(CMAKE_ANDROID_ARCH_ABI STREQUAL "arm64-v8a")
        set(SKIA_ARCH "arm64")
    elseif(CMAKE_ANDROID_ARCH_ABI STREQUAL "x86_64")
        set(SKIA_ARCH "x64")
    else()
        message(FATAL_ERROR "Unsupported ABI: ${CMAKE_ANDROID_ARCH_ABI}")
    endif()
    set(SKIA_LIB_DIR "${CMAKE_SOURCE_DIR}/third_party/skia/out/android-${SKIA_ARCH}")
elseif(IOS)
    # iOS device only (no simulator support)
    set(SKIA_LIB_DIR "${CMAKE_SOURCE_DIR}/third_party/skia/out/ios-arm64")
endif()

# Validate that Skia library exists (only on mobile platforms)
if(ANDROID OR IOS)
    if(NOT EXISTS "${SKIA_LIB_DIR}/libskia.a")
        if(ANDROID)
            message(FATAL_ERROR
                "Skia library not found at: ${SKIA_LIB_DIR}/libskia.a\n"
                "Please build Skia first using scripts/build-skia-android.sh")
        elseif(IOS)
            message(FATAL_ERROR
                "Skia library not found at: ${SKIA_LIB_DIR}/libskia.a\n"
                "Please build Skia first using scripts/build-skia-ios.sh")
        endif()
    endif()
endif()

# Skia imported library
if(ANDROID OR IOS)
    add_library(skia STATIC IMPORTED)
    set_target_properties(skia PROPERTIES
        IMPORTED_LOCATION "${SKIA_LIB_DIR}/libskia.a"
        INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/third_party/skia"
    )
endif()

# =============================================================================
# skplayer_ui Library
# =============================================================================
add_subdirectory(libs/skplayer_ui)

# =============================================================================
# Main Application
# =============================================================================
if(ANDROID)
    add_library(main SHARED
        src/main.cpp
        src/OverlayTypefaceProvider.cpp
    )
    
    target_link_libraries(main PRIVATE
        SDL3::SDL3
        skia
        skplayer_ui
        android
        log
        EGL
        GLESv3
    )
    
    target_include_directories(main PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/third_party/skia
    )
elseif(IOS)
    # iOS app bundle
    add_executable(SkiaSeekBar MACOSX_BUNDLE
        src/main.cpp
        src/OverlayTypefaceProvider.cpp
        ios/LaunchScreen.storyboard
        ios/SkiaSeekBar/Assets.xcassets
        ios/SkiaSeekBar/product_icon.icon
    )
    
    # Set bundle properties
    set_target_properties(SkiaSeekBar PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/ios/Info.plist"
        MACOSX_BUNDLE_BUNDLE_NAME "Skia SeekBar"
        MACOSX_BUNDLE_BUNDLE_VERSION "1"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.example.skiaseekbar"
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.example.skiaseekbar"
        XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
        XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "15.0"
        XCODE_ATTRIBUTE_DEVELOPMENT_TEAM ""
        XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer"
        XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME "product_icon"
        XCODE_ATTRIBUTE_SKIP_INSTALL "NO"
        XCODE_ATTRIBUTE_INSTALL_PATH "$(LOCAL_APPS_DIR)"
        RESOURCE "ios/LaunchScreen.storyboard;ios/SkiaSeekBar/Assets.xcassets;ios/SkiaSeekBar/product_icon.icon"
    )
    
    target_link_libraries(SkiaSeekBar PRIVATE
        SDL3::SDL3-static
        skia
        skplayer_ui
        "-framework Foundation"
        "-framework UIKit"
        "-framework OpenGLES"
        "-framework QuartzCore"
        "-framework CoreGraphics"
        "-framework CoreText"
        "-framework CoreFoundation"
        "-framework ImageIO"
        "-framework MobileCoreServices"
        "-framework AVFoundation"
        "-framework AudioToolbox"
        "-framework GameController"
        "-framework CoreHaptics"
        "-framework CoreMotion"
        "-framework CoreBluetooth"
    )
    
    target_include_directories(SkiaSeekBar PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/third_party/skia
    )
    
    target_compile_definitions(SkiaSeekBar PRIVATE
        GL_SILENCE_DEPRECATION
    )
endif()

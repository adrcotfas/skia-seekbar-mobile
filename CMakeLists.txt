# CMakeLists.txt
cmake_minimum_required(VERSION 3.22.1)
project(skia_player_demo VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# =============================================================================
# SDL3 (built from submodule)
# =============================================================================
set(SDL_SHARED ON CACHE BOOL "Build SDL3 shared library")
set(SDL_STATIC OFF CACHE BOOL "")
set(SDL_TEST OFF CACHE BOOL "")
add_subdirectory(third_party/SDL)

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

    # Validate that Skia library exists
    if(NOT EXISTS "${SKIA_LIB_DIR}/libskia.a")
        message(FATAL_ERROR
            "Skia library not found at: ${SKIA_LIB_DIR}/libskia.a\n"
            "Please build Skia first using scripts/build-skia-android.sh")
    endif()
endif()

add_library(skia STATIC IMPORTED)
set_target_properties(skia PROPERTIES
    IMPORTED_LOCATION "${SKIA_LIB_DIR}/libskia.a"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/third_party/skia"
)

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
endif()

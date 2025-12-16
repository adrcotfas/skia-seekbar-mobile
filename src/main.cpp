// src/main.cpp

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "skplayer_ui/VideoContainer.h"
#include "SpaceExplorationShader.h"
#include "skplayer_ui/ThemeConstants.h"
#include "OverlayTypefaceProvider.h"

#include "include/core/SkCanvas.h"
#include "include/core/SkColorSpace.h"
#include "include/core/SkRect.h"
#include "include/core/SkSurface.h"
#include "include/core/SkTypeface.h"
#include "include/gpu/ganesh/GrBackendSurface.h"
#include "include/gpu/ganesh/GrDirectContext.h"
#include "include/gpu/ganesh/GrTypes.h"
#include "include/gpu/ganesh/SkSurfaceGanesh.h"
#include "include/gpu/ganesh/gl/GrGLDirectContext.h"
#include "include/gpu/ganesh/gl/GrGLBackendSurface.h"
#include "include/gpu/ganesh/gl/GrGLInterface.h"
#include "include/gpu/ganesh/gl/GrGLTypes.h"

#include <algorithm>
#include <cmath>
#include <memory>

#ifdef __ANDROID__
#include <android/log.h>
#include <GLES3/gl3.h>
#define LOG(...) __android_log_print(ANDROID_LOG_INFO, "SkiaSeekBar", __VA_ARGS__)
#else
#include <cstdio>
#define LOG(...) printf(__VA_ARGS__); printf("\n")
#include <GL/gl.h>
#endif

// Video duration in seconds (matches the shader animation loop feel)
static constexpr float kVideoDurationSeconds = 194.0f;

struct AppState : public skplayer_ui::VideoContainer::Listener {
    std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> window{nullptr, SDL_DestroyWindow};
    std::unique_ptr<std::remove_pointer_t<SDL_GLContext>, decltype(&SDL_GL_DestroyContext)> gl{nullptr, SDL_GL_DestroyContext};

    sk_sp<const GrGLInterface> glInterface;
    sk_sp<GrDirectContext> grContext;
    sk_sp<SkSurface> surface;
    sk_sp<SkTypeface> overlayTypeface;

    int width = 0;
    int height = 0;
    float dpiScale = 1.0f;
    Uint64 lastTime = 0;

    // Video simulation
    SpaceExplorationShader spaceShader;
    bool isPlaying = false;  // Start paused until loading completes
    float videoTime = 0.0f;  // Current playback time

    // UI
    std::unique_ptr<skplayer_ui::VideoContainer> videoContainer;

    // Layout
    SkRect videoBounds = SkRect::MakeEmpty();
    SkRect containerBounds = SkRect::MakeEmpty();
    bool isPortrait = true;

    bool createSurface() {
        SDL_GetWindowSizeInPixels(window.get(), &width, &height);

        GrGLenum fbFormat = GL_RGBA8;
#ifdef __ANDROID__
        GLint redBits = 0, greenBits = 0, blueBits = 0, alphaBits = 0;
        glGetIntegerv(GL_RED_BITS, &redBits);
        glGetIntegerv(GL_GREEN_BITS, &greenBits);
        glGetIntegerv(GL_BLUE_BITS, &blueBits);
        glGetIntegerv(GL_ALPHA_BITS, &alphaBits);

        // Determine format based on bit depths
        if (redBits == 5 && greenBits == 6 && blueBits == 5) {
            fbFormat = GL_RGB565;
        } else if (alphaBits == 0) {
            fbFormat = GL_RGB8;
        }
#endif

        // Query actual stencil buffer size
        GLint stencilBits = 0;
        glGetIntegerv(GL_STENCIL_BITS, &stencilBits);

        GrGLFramebufferInfo fbInfo;
        fbInfo.fFBOID = 0;
        fbInfo.fFormat = fbFormat;

        auto backendRT = GrBackendRenderTargets::MakeGL(width, height, 0, stencilBits, fbInfo);
        if (!backendRT.isValid()) {
            LOG("Failed to create backend render target");
            return false;
        }

        surface = SkSurfaces::WrapBackendRenderTarget(
            grContext.get(),
            backendRT,
            kBottomLeft_GrSurfaceOrigin,
            kRGBA_8888_SkColorType,
            nullptr,
            nullptr
        );

        if (!surface) {
            LOG("Failed to create Skia surface");
            return false;
        }

        updateLayout();
        return true;
    }

    void updateLayout() {
        isPortrait = height > width;
        
        // SeekBar area height (needed for portrait layout)
        const float seekBarAreaHeight = skplayer_ui::theme::layout::kSeekBarHeightDp * dpiScale;
        const float portraitTopMargin = skplayer_ui::theme::layout::kPortraitTopMarginDp * dpiScale;
        
        if (isPortrait) {
            // Portrait: top margin for cutout, then video (16:9), then seekbar below
            float videoWidth = static_cast<float>(width);
            float videoHeight = videoWidth * 9.0f / 16.0f;  // 16:9 aspect ratio
            
            // Video starts below the top margin
            videoBounds = SkRect::MakeXYWH(0, portraitTopMargin, videoWidth, videoHeight);
            
            // Container is positioned below the video, just for the seekbar area
            containerBounds = SkRect::MakeXYWH(0, portraitTopMargin + videoHeight, videoWidth, seekBarAreaHeight);
        } else {
            // Landscape: fullscreen video with overlay controls
            float screenWidth = static_cast<float>(width);
            float screenHeight = static_cast<float>(height);
            
            // Fit 16:9 video to screen (letterbox if needed)
            float videoAspect = 16.0f / 9.0f;
            float screenAspect = screenWidth / screenHeight;
            
            float videoWidth, videoHeight;
            if (screenAspect > videoAspect) {
                // Screen is wider than 16:9 - fit to height
                videoHeight = screenHeight;
                videoWidth = videoHeight * videoAspect;
            } else {
                // Screen is taller than 16:9 - fit to width
                videoWidth = screenWidth;
                videoHeight = videoWidth / videoAspect;
            }
            
            float videoX = (screenWidth - videoWidth) / 2;
            float videoY = (screenHeight - videoHeight) / 2;
            
            videoBounds = SkRect::MakeXYWH(videoX, videoY, videoWidth, videoHeight);
            containerBounds = SkRect::MakeXYWH(0, 0, screenWidth, screenHeight);
        }

        if (videoContainer) {
            videoContainer->setViewport(
                static_cast<int>(containerBounds.width()),
                static_cast<int>(containerBounds.height())
            );
            // Pass video center Y relative to container origin
            float videoCenterY;
            if (isPortrait) {
                // In portrait, container is below video, so video center is above (negative Y)
                videoCenterY = -videoBounds.height() / 2.0f;
            } else {
                // In landscape, container is fullscreen, video center is at videoBounds center
                videoCenterY = videoBounds.centerY();
            }
            videoContainer->setLayout(isPortrait, videoCenterY);
        }
    }

    // VideoContainer::Listener callbacks
    void onPlay() override {
        isPlaying = true;
        LOG("Play");
    }

    void onPause() override {
        isPlaying = false;
        LOG("Pause");
    }

    void onSeekTo(float positionSeconds) override {
        videoTime = std::clamp(positionSeconds, 0.0f, kVideoDurationSeconds);
        spaceShader.setTime(videoTime);
        LOG("Seek to: %.1f", positionSeconds);
    }
};

SDL_AppResult SDL_AppInit(void** appstate, int /*argc*/, char* /*argv*/[]) {
    LOG("=== Starting Skia Space Exploration Demo ===");

    auto state = std::make_unique<AppState>();

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        LOG("SDL_Init failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // OpenGL ES 3.0
    if (!SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES)) {
        LOG("Failed to set GL ES profile: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    if (!SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3)) {
        LOG("Failed to set GL major version: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    if (!SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0)) {
        LOG("Failed to set GL minor version: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    if (!SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8)) {
        LOG("Failed to set stencil size: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    state->window.reset(SDL_CreateWindow(
        "Space Exploration",
        800, 600,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY
    ));
    if (!state->window) {
        LOG("SDL_CreateWindow failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    state->gl.reset(SDL_GL_CreateContext(state->window.get()));
    if (!state->gl) {
        LOG("SDL_GL_CreateContext failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!SDL_GL_SetSwapInterval(1)) {
        LOG("Warning: VSync not available: %s", SDL_GetError());
    }

    SDL_GetWindowSizeInPixels(state->window.get(), &state->width, &state->height);
    LOG("Window: %d x %d", state->width, state->height);

    SDL_DisplayID displayID = SDL_GetDisplayForWindow(state->window.get());
    float scale = SDL_GetDisplayContentScale(displayID);
    state->dpiScale = (scale > 0.0f) ? scale : 1.0f;
    LOG("Display scale: %.2f", state->dpiScale);

    state->glInterface = GrGLMakeNativeInterface();
    if (!state->glInterface) {
        LOG("Failed to create GL interface");
        return SDL_APP_FAILURE;
    }
    state->grContext = GrDirectContexts::MakeGL(state->glInterface);
    if (!state->grContext) {
        LOG("Failed to create Skia context");
        return SDL_APP_FAILURE;
    }

    // Set GPU resource cache budget for mobile (96MB)
    size_t maxResourceBytes = 96 * 1024 * 1024;
    state->grContext->setResourceCacheLimit(maxResourceBytes);
    LOG("Set GPU resource cache limit: %zu MB", maxResourceBytes / (1024 * 1024));

    // Initialize shader
    if (!state->spaceShader.initialize()) {
        LOG("Failed to initialize shader");
        return SDL_APP_FAILURE;
    }
    LOG("Shader initialized");

    if (!state->createSurface()) {
        return SDL_APP_FAILURE;
    }

    // Video UI container
    state->overlayTypeface = CreateDefaultOverlayTypeface();
    if (!state->overlayTypeface) {
        LOG("Warning: Failed to load overlay typeface, text rendering may not work correctly");
        // Continue anyway - VideoContainer will handle null typeface gracefully
    }

    skplayer_ui::VideoContainer::Config cfg;
    cfg.durationSeconds = kVideoDurationSeconds;
    cfg.initialLoadingSeconds = 2.0f;
    cfg.dpiScale = state->dpiScale;
    cfg.chapters.emplace_back(0.0f, "Brittle Hollow");
    cfg.chapters.emplace_back(42.0f, "Giant's Deep");
    cfg.chapters.emplace_back(120.0f, "Timber Hearth");
    cfg.overlayTypeface = state->overlayTypeface.get();

    state->videoContainer = std::make_unique<skplayer_ui::VideoContainer>(cfg, state.get());
    state->updateLayout();

    state->lastTime = SDL_GetTicks();
    *appstate = state.release();
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
    auto state = static_cast<AppState*>(appstate);
    if (!state || !state->videoContainer) return SDL_APP_CONTINUE;

    switch (event->type) {
        case SDL_EVENT_QUIT:
            return SDL_APP_SUCCESS;

        case SDL_EVENT_KEY_DOWN:
            if (event->key.key == SDLK_AC_BACK || event->key.key == SDLK_ESCAPE) {
                return SDL_APP_SUCCESS;
            }
            break;

        case SDL_EVENT_WINDOW_RESIZED:
        case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
            if (!state->createSurface()) {
                LOG("Failed to recreate surface on resize - shutting down");
                return SDL_APP_FAILURE;
            }
            break;

        case SDL_EVENT_WILL_ENTER_BACKGROUND:
            // Pause playback when app goes to background
            // Note: This stops video time advancement but doesn't update VideoContainer's
            // UI state. The UI will resync when user interacts after foregrounding.
            if (state->isPlaying) {
                state->isPlaying = false;
                LOG("App backgrounding - pausing video time advancement");
            }
            break;

        case SDL_EVENT_DID_ENTER_FOREGROUND:
            LOG("App foregrounding - validating GL context");
            // Check if GL context was lost and needs recreation
            if (state->grContext && state->grContext->abandoned()) {
                LOG("GL context was lost, recreating Skia resources");

                // Recreate GL interface
                state->glInterface = GrGLMakeNativeInterface();
                if (!state->glInterface) {
                    LOG("Failed to recreate GL interface");
                    return SDL_APP_FAILURE;
                }

                // Recreate Skia GPU context
                state->grContext = GrDirectContexts::MakeGL(state->glInterface);
                if (!state->grContext) {
                    LOG("Failed to recreate Skia context");
                    return SDL_APP_FAILURE;
                }

                // Set resource cache budget
                size_t maxResourceBytes = 96 * 1024 * 1024;  // 96MB
                state->grContext->setResourceCacheLimit(maxResourceBytes);
            }

            // Recreate surface if needed (window size may have changed)
            if (!state->surface || !state->createSurface()) {
                LOG("Failed to recreate surface on foreground");
                return SDL_APP_FAILURE;
            }
            break;

        case SDL_EVENT_LOW_MEMORY:
            // Free GPU resources on low memory
            if (state->grContext) {
                LOG("Low memory warning - freeing GPU resources");
                state->grContext->freeGpuResources();
            }
            break;

        case SDL_EVENT_FINGER_DOWN: {
            float x = event->tfinger.x * state->width - state->containerBounds.left();
            float y = event->tfinger.y * state->height - state->containerBounds.top();
            state->videoContainer->onPointerDown(x, y, static_cast<uint64_t>(SDL_GetTicks()), false);
            break;
        }

        case SDL_EVENT_FINGER_MOTION: {
            float x = event->tfinger.x * state->width - state->containerBounds.left();
            float y = event->tfinger.y * state->height - state->containerBounds.top();
            state->videoContainer->onPointerMove(x, y);
            break;
        }

        case SDL_EVENT_FINGER_UP: {
            float x = event->tfinger.x * state->width - state->containerBounds.left();
            float y = event->tfinger.y * state->height - state->containerBounds.top();
            state->videoContainer->onPointerUp(x, y);
            break;
        }

#ifndef __ANDROID__
        case SDL_EVENT_MOUSE_BUTTON_DOWN: {
            float x = static_cast<float>(event->button.x) - state->containerBounds.left();
            float y = static_cast<float>(event->button.y) - state->containerBounds.top();
            bool forceDoubleTap = (event->button.clicks >= 2);
            state->videoContainer->onPointerDown(x, y, static_cast<uint64_t>(SDL_GetTicks()), forceDoubleTap);
            break;
        }

        case SDL_EVENT_MOUSE_MOTION: {
            float x = static_cast<float>(event->motion.x) - state->containerBounds.left();
            float y = static_cast<float>(event->motion.y) - state->containerBounds.top();
            state->videoContainer->onPointerMove(x, y);
            break;
        }

        case SDL_EVENT_MOUSE_BUTTON_UP: {
            float x = static_cast<float>(event->button.x) - state->containerBounds.left();
            float y = static_cast<float>(event->button.y) - state->containerBounds.top();
            state->videoContainer->onPointerUp(x, y);
            break;
        }
#endif
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate) {
    auto state = static_cast<AppState*>(appstate);
    if (!state || !state->videoContainer) return SDL_APP_CONTINUE;

    Uint64 now = SDL_GetTicks();
    float dt = (now - state->lastTime) / 1000.0f;
    state->lastTime = now;

    // Update video time if playing (and not loading)
    if (state->isPlaying && !state->videoContainer->isLoading()) {
        state->videoTime += dt;
        if (state->videoTime >= kVideoDurationSeconds) {
            state->videoTime = kVideoDurationSeconds;
            // Video ended - VideoContainer handles the pause
        }
        state->spaceShader.setTime(state->videoTime);
    }

    state->videoContainer->update(dt, static_cast<uint64_t>(now));

    SkCanvas* canvas = state->surface->getCanvas();
    
    // Clear background (black for letterboxing)
    canvas->clear(SK_ColorBLACK);

    // Render shader as the "video"
    if (state->spaceShader.isReady()) {
        state->spaceShader.render(canvas, state->videoBounds);
    }

    // Render VideoContainer UI overlay
    // In portrait mode, this renders at the bottom; in landscape, it overlays the video
    canvas->save();
    canvas->translate(state->containerBounds.left(), state->containerBounds.top());
    state->videoContainer->render(canvas);
    canvas->restore();

    state->grContext->flush();
    SDL_GL_SwapWindow(state->window.get());
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult /*result*/) {
    std::unique_ptr<AppState> state(static_cast<AppState*>(appstate));
    if (state) {
        // 1. Destroy Skia resources (requires valid GL context)
        state->videoContainer.reset();
        state->surface.reset();

        // 2. Flush and abandon Skia GPU context
        if (state->grContext) {
            state->grContext->flushAndSubmit();
            state->grContext->abandonContext();
        }
        state->grContext.reset();
        state->glInterface.reset();

        // 3. Destroy GL context (done by unique_ptr destructor)
        state->gl.reset();

        // 4. Destroy window (done by unique_ptr destructor)
        state->window.reset();
    }

    // 5. SDL shutdown after all resources destroyed
    SDL_Quit();
}

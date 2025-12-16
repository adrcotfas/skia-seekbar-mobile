#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

class SkCanvas;
class SkTypeface;

namespace skplayer_ui {

// Chapter marker for segmented progress bar
struct Chapter {
    float startTime = 0.0f;
    std::string title;

    explicit Chapter(float start, std::string t = "")
        : startTime(start), title(std::move(t)) {}
};

// High-level UI/controller for the demo "video" surface.
// Owns and orchestrates:
// - SeekBar (including loading animation via SeekBarState::isLoading)
// - PlayPauseButton
// - seek overlay text + chevrons + burst seeking logic
//
// Main/app should only:
// - forward pointer events
// - call update(dt, nowMs)
// - call render(canvas)
class VideoContainer {
public:
    struct Listener {
        virtual ~Listener() = default;
        virtual void onPlay() {}
        virtual void onPause() {}
        virtual void onSeekTo(float /*positionSeconds*/) {}
    };

    struct Config {
        float durationSeconds = 0.0f;
        std::vector<Chapter> chapters;
        float initialLoadingSeconds = 0.0f;
        float dpiScale = 1.0f;

        // Optional typeface for overlay text (caller retains ownership)
        SkTypeface* overlayTypeface = nullptr;
    };

    explicit VideoContainer(const Config& config, Listener* listener = nullptr);
    ~VideoContainer();

    // Move operations (noexcept for optimal container performance)
    VideoContainer(VideoContainer&&) noexcept;
    VideoContainer& operator=(VideoContainer&&) noexcept;

    // Copy operations deleted (pImpl with unique_ptr)
    VideoContainer(const VideoContainer&) = delete;
    VideoContainer& operator=(const VideoContainer&) = delete;

    void setViewport(int widthPx, int heightPx);
    
    // Set layout mode and video bounds
    // portrait: true for portrait mode (skplayer_ui below video), false for landscape (overlay)
    // videoCenterY: Y coordinate of video center relative to container (for centering play button/chevrons)
    void setLayout(bool portrait, float videoCenterY);

    // Input (pixel coordinates)
    void onPointerDown(float x, float y, uint64_t nowMs, bool forceDoubleTap = false);
    void onPointerMove(float x, float y);
    void onPointerUp(float x, float y);

    // Tick/update
    void update(float deltaTimeSeconds, uint64_t nowMs);

    // Draw
    void render(SkCanvas* canvas);

    [[nodiscard]] bool isLoading() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl;
};

} // namespace skplayer_ui

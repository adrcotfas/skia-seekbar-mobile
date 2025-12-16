#include "skplayer_ui/VideoContainer.h"
#include "PlayPauseButton.h"
#include "SeekBar.h"
#include "SeekPreviewTooltip.h"

#include "UIState.h"
#include "TimeBadge.h"
#include "SeekFeedbackOverlay.h"

#include "include/core/SkCanvas.h"
#include "include/core/SkRect.h"
#include "include/core/SkTypeface.h"

#include <algorithm>
#include <cmath>

namespace skplayer_ui {

namespace {
}

// =============================================================================
// Impl
// =============================================================================

class VideoContainer::Impl {
public:
    Impl(float dpi, SkTypeface* typeface)
        : dpiScale(std::max(dpi, 0.01f))
        , seekBar(dpiScale)
        , playPauseButton(dpiScale)
        , seekPreviewTooltip(dpiScale, sk_ref_sp(typeface))
        , timeBadge(dpiScale, sk_ref_sp(typeface))
        , seekFeedback(dpiScale, sk_ref_sp(typeface)) {}

    Listener* listener = nullptr;

    // Layout
    float width = 0;
    float height = 0;
    float dpiScale;
    bool isPortrait = false;
    float videoCenterY = 0.0f;
    SkRect seekBarBounds = SkRect::MakeEmpty();
    SkRect playPauseButtonBounds = SkRect::MakeEmpty();

    // Components
    SeekBar seekBar;
    SeekBarState state;
    PlayPauseButton playPauseButton;
    SeekPreviewTooltip seekPreviewTooltip;
    TimeBadge timeBadge;
    SeekFeedbackOverlay seekFeedback;

    // State machine
    UIState uiState = UIState::Loading;
    bool returnToPlaying = true;  // Resume playing after drag/seek session ends
    float autoHideTimer = 0.0f;

    // Gesture state
    TapGesture tap;
    SeekBurst burst;

    // Loading countdown
    float loadingSecondsRemaining = 0.0f;

    // =========================================================================
    // State queries
    // =========================================================================

    bool isPlaying() const {
        return uiState == UIState::PlayingHidden || uiState == UIState::PlayingVisible;
    }

    // =========================================================================
    // State transitions
    // =========================================================================

    void transitionTo(UIState newState) {
        if (uiState == newState) return;
        uiState = newState;

        // Derive control visibility from state
        bool showControls = (newState == UIState::PlayingVisible ||
                             newState == UIState::PausedVisible ||
                             newState == UIState::SeekSession ||
                             newState == UIState::Dragging);

        // Derive playing status from state
        // SeekSession: use returnToPlaying since playback will resume based on that
        bool playing = (newState == UIState::PlayingHidden ||
                        newState == UIState::PlayingVisible ||
                        (newState == UIState::SeekSession && returnToPlaying));

        showControls ? seekBar.expand() : seekBar.collapse();
        playPauseButton.setPlaying(playing);
        autoHideTimer = (newState == UIState::PlayingVisible)
                        ? theme::layout::kPlayPauseAutoHideSeconds : 0.0f;

        if (newState == UIState::Dragging) seekPreviewTooltip.show();
    }

    void play() {
        // Restart from beginning if at end
        if (state.currentPosition >= state.duration) {
            state.currentPosition = 0.0f;
            seekBar.setState(state);
            if (listener) listener->onSeekTo(0.0f);
        }
        if (listener) listener->onPlay();
        transitionTo(UIState::PlayingVisible);
    }

    void pause() {
        if (listener) listener->onPause();
        transitionTo(UIState::PausedVisible);
    }

    void togglePlayPause() {
        if (isPlaying()) pause();
        else play();
    }

    // =========================================================================
    // Helpers
    // =========================================================================

    bool canSeekBy(int deltaSec) const {
        if (deltaSec > 0) return state.currentPosition < state.duration;
        if (deltaSec < 0) return state.currentPosition > 0.0f;
        return true;
    }

    void performSeek(int deltaSec) {
        state.currentPosition = std::clamp(state.currentPosition + static_cast<float>(deltaSec),
                                           0.0f, state.duration);
        seekBar.setState(state);
        seekBar.expand();
        if (listener) listener->onSeekTo(state.currentPosition);
    }

    std::string getChapterNameAtPosition(float pos) const {
        if (state.chapters.empty() || state.duration <= 0.0f) return "";
        for (size_t i = state.chapters.size(); i-- > 0;) {
            if (pos >= state.chapters[i].startTime) return state.chapters[i].title;
        }
        return state.chapters[0].title;
    }

    void updateTooltipContent(float pos) {
        seekPreviewTooltip.setTime(pos);
        seekPreviewTooltip.setChapterName(getChapterNameAtPosition(pos));
    }

    // =========================================================================
    // Pointer event helpers
    // =========================================================================

    bool isInBounds(float y) const {
        if (!isPortrait) return true;
        float videoTop = 2.0f * videoCenterY;
        return y <= height && y >= videoTop;
    }

    bool trySeekBarDrag(float x, float y) {
        auto seekEvent = seekBar.onTouchDown(x, y);
        if (seekEvent.type == SeekBar::SeekEvent::Type::Started) {
            handleSeekStarted();
            burst.reset();
            return true;
        }
        return false;
    }

    bool tryPlayPauseButton(float x, float y) {
        if (playPauseButtonBounds.contains(x, y)) {
            playPauseButton.startRipple();
            togglePlayPause();
            burst.reset();
            return true;
        }
        return false;
    }

    bool tryBurstContinuation(SeekDirection dir, uint64_t nowMs) {
        if (!burst.canContinue(dir, nowMs)) return false;

        int step = 10 * static_cast<int>(dir);
        if (!canSeekBy(step)) {
            burst.reset();
            return true;  // Consumed the tap, just can't seek further
        }
        burst.accumulate(step, nowMs);
        performSeek(step);
        seekFeedback.show(burst.totalSeconds);
        return true;
    }

    bool tryDoubleTapSeek(float x, float y, SeekDirection dir, uint64_t nowMs, bool force) {
        if (!force && !tap.isDoubleTap(x, y, nowMs, theme::gesture::kDoubleTapDistanceDp * dpiScale)) {
            return false;
        }

        int step = 10 * static_cast<int>(dir);
        if (!canSeekBy(step)) return false;

        performSeek(step);
        burst.start(dir, step, nowMs);

        if (uiState != UIState::SeekSession) {
            returnToPlaying = isPlaying();
            transitionTo(UIState::SeekSession);
        }

        seekFeedback.show(burst.totalSeconds);
        tap.reset();
        return true;
    }

    void handleSingleTap(float x, float y, uint64_t nowMs) {
        // In SeekSession, use returnToPlaying since isPlaying() returns false
        bool wasPlaying = (uiState == UIState::SeekSession) ? returnToPlaying : isPlaying();
        
        if (wasPlaying) {
            transitionTo(UIState::PlayingVisible);
        } else {
            transitionTo(uiState == UIState::PausedHidden ? UIState::PausedVisible : UIState::PausedHidden);
        }
        burst.reset();
        tap.record(x, y, nowMs);
    }

    // =========================================================================
    // Update helpers
    // =========================================================================

    void updateBurstState(uint64_t nowMs) {
        if (burst.isTimedOut(nowMs)) {
            burst.reset();
        }
        seekFeedback.setBurstActive(burst.active);
    }

    void updateSeekSession() {
        if (uiState == UIState::SeekSession && !burst.active &&
            !seekFeedback.isAnimating() && !seekFeedback.isActive()) {
            transitionTo(returnToPlaying ? UIState::PlayingHidden : UIState::PausedVisible);
        }
    }

    void updatePlayback(float dt) {
        if (!isPlaying() || burst.active) return;

        state.currentPosition += dt;
        if (state.currentPosition >= state.duration) {
            state.currentPosition = state.duration;
            transitionTo(UIState::PausedVisible);
            if (listener) listener->onPause();
        }
        seekBar.setState(state);
    }

    void updateAutoHide(float dt) {
        if (uiState == UIState::PlayingVisible && autoHideTimer > 0.0f) {
            autoHideTimer -= dt;
            if (autoHideTimer <= 0.0f) {
                transitionTo(UIState::PlayingHidden);
            }
        }
    }

    void updateLoading(float dt) {
        if (!state.isLoading) return;

        loadingSecondsRemaining -= dt;
        if (loadingSecondsRemaining <= 0.0f) {
            loadingSecondsRemaining = 0.0f;
            state.isLoading = false;
            seekBar.setState(state);
            if (listener) listener->onPlay();
            transitionTo(UIState::PlayingHidden);
        }
    }

    // =========================================================================
    // Event handlers
    // =========================================================================

    void handleSeekStarted() {
        returnToPlaying = isPlaying();
        transitionTo(UIState::Dragging);
        updateTooltipContent(seekBar.getPreviewPosition());
        if (listener) listener->onPause();
    }

    void handleSeekCompleted(float position) {
        state.currentPosition = std::clamp(position, 0.0f, state.duration);
        seekBar.setState(state);
        seekPreviewTooltip.hide();

        if (returnToPlaying) {
            if (listener) listener->onPlay();
            transitionTo(UIState::PlayingVisible);
        } else {
            transitionTo(UIState::PausedVisible);
        }
        if (listener) listener->onSeekTo(state.currentPosition);
    }

    void onPointerDown(float x, float y, uint64_t nowMs, bool forceDoubleTap) {
        if (state.isLoading || !isInBounds(y)) return;

        if (trySeekBarDrag(x, y)) return;
        if (tryPlayPauseButton(x, y)) return;

        SeekDirection dir = (x < width * 0.5f) ? SeekDirection::Backward : SeekDirection::Forward;

        if (tryBurstContinuation(dir, nowMs)) return;

        // Reset stale burst if direction changed
        if (burst.active && !burst.canContinue(dir, nowMs)) {
            burst.reset();
        }

        if (tryDoubleTapSeek(x, y, dir, nowMs, forceDoubleTap)) return;

        handleSingleTap(x, y, nowMs);
    }

    void onPointerMove(float x, float y) {
        if (state.isLoading) return;

        auto seekEvent = seekBar.onTouchMove(x, y);
        if (seekEvent.type == SeekBar::SeekEvent::Type::Started) {
            handleSeekStarted();
            burst.reset();
        }

        if (uiState == UIState::Dragging) {
            updateTooltipContent(seekBar.getPreviewPosition());
        }
    }

    void onPointerUp(float x, float y) {
        if (state.isLoading) return;

        auto seekEvent = seekBar.onTouchUp(x, y);
        if (seekEvent.type == SeekBar::SeekEvent::Type::Completed) {
            handleSeekCompleted(seekEvent.position);
        }
    }

    void update(float deltaTimeSeconds, uint64_t nowMs) {
        float dt = std::max(deltaTimeSeconds, 0.0f);

        updateBurstState(nowMs);
        seekFeedback.update(dt);
        updateSeekSession();
        updatePlayback(dt);
        updateAutoHide(dt);
        updateLoading(dt);

        seekBar.update(dt, isPlaying());
        playPauseButton.update(dt);
    }

    void render(SkCanvas* canvas) {
        if (!canvas || width <= 0 || height <= 0) return;

        // SeekBar bounds
        const float seekBarHeight = theme::layout::kSeekBarHeightDp * dpiScale;
        seekBarBounds = isPortrait
            ? SkRect::MakeXYWH(0, 0, width, seekBarHeight)
            : SkRect::MakeXYWH(0, height - seekBarHeight, width, seekBarHeight);
        seekBar.render(canvas, seekBarBounds);

        float controlsAlpha = seekBar.alpha();

        // Track top for badge positioning
        const float trackHeight = (isPortrait ? theme::seekbar::kTrackHeightPortraitDp
                                              : theme::seekbar::kTrackHeightLandscapeDp) * dpiScale;
        const float trackTopY = isPortrait
            ? seekBarBounds.top()
            : seekBarBounds.top() + (seekBarBounds.height() - trackHeight) / 2.0f;

        // Time badge
        if (!state.isLoading && uiState != UIState::Dragging &&
            !seekPreviewTooltip.isVisible() && controlsAlpha > 0.01f && state.duration > 0.0f) {
            timeBadge.update(state.currentPosition, state.duration);
            const float badgeLeft = seekBarBounds.left() +
                (isPortrait ? 0.0f : theme::seekbar::kLandscapeMarginDp * dpiScale) +
                theme::timebadge::kMarginLeftDp * dpiScale;
            const float badgeTop = trackTopY - theme::timebadge::kMarginAboveSeekBarDp * dpiScale -
                (theme::timebadge::kFontSizeDp + theme::timebadge::kPaddingVerticalDp * 2) * dpiScale;
            timeBadge.render(canvas, badgeLeft, badgeTop, controlsAlpha);
        }

        // Play/pause button
        const float buttonSize = theme::layout::kPlayPauseButtonSizeDp * dpiScale;
        bool showButton = !state.isLoading &&
                          (uiState == UIState::PausedVisible || uiState == UIState::PlayingVisible);

        if (showButton) {
            playPauseButtonBounds = SkRect::MakeXYWH(
                (width - buttonSize) / 2.0f,
                videoCenterY - buttonSize / 2.0f,
                buttonSize, buttonSize);
            playPauseButton.render(canvas, playPauseButtonBounds, controlsAlpha);
        } else {
            playPauseButtonBounds = SkRect::MakeEmpty();
        }

        // Seek preview tooltip
        if (seekPreviewTooltip.isVisible()) {
            float tooltipY = trackTopY - theme::tooltip::kMarginAboveSeekBarDp * dpiScale -
                (theme::tooltip::kFontSizeDp + theme::tooltip::kPaddingVerticalDp) * dpiScale;
            seekPreviewTooltip.render(canvas, width / 2.0f, tooltipY);
        }

        // Seek feedback overlay
        if (seekFeedback.isActive()) {
            seekFeedback.render(canvas, videoCenterY, width, isPortrait);
        }
    }

    void setViewport(int w, int h) {
        width = static_cast<float>(w);
        height = static_cast<float>(h);
        playPauseButton.setPlaying(isPlaying());
    }

    void setLayout(bool portrait, float vidCenterY) {
        isPortrait = portrait;
        videoCenterY = vidCenterY;
        seekBar.setPortraitMode(portrait);
    }
};

// =============================================================================
// VideoContainer public interface
// =============================================================================

VideoContainer::VideoContainer(const Config& config, Listener* listener)
    : impl(std::make_unique<Impl>(config.dpiScale, config.overlayTypeface)) {
    impl->listener = listener;
    impl->state.duration = std::max(0.0f, config.durationSeconds);
    impl->state.currentPosition = 0.0f;
    impl->state.chapters = config.chapters;

    impl->loadingSecondsRemaining = std::max(0.0f, config.initialLoadingSeconds);
    if (impl->loadingSecondsRemaining > 0.0f) {
        impl->state.isLoading = true;
        impl->uiState = UIState::Loading;
    } else {
        impl->uiState = UIState::PlayingHidden;
    }

    impl->seekBar.setState(impl->state);
    impl->seekBar.collapse();
    impl->playPauseButton.setPlaying(impl->isPlaying());
}

VideoContainer::~VideoContainer() = default;
VideoContainer::VideoContainer(VideoContainer&&) noexcept = default;
VideoContainer& VideoContainer::operator=(VideoContainer&&) noexcept = default;

void VideoContainer::setViewport(int w, int h) { impl->setViewport(w, h); }
void VideoContainer::setLayout(bool portrait, float videoCenterY) { impl->setLayout(portrait, videoCenterY); }
void VideoContainer::onPointerDown(float x, float y, uint64_t nowMs, bool forceDoubleTap) { impl->onPointerDown(x, y, nowMs, forceDoubleTap); }
void VideoContainer::onPointerMove(float x, float y) { impl->onPointerMove(x, y); }
void VideoContainer::onPointerUp(float x, float y) { impl->onPointerUp(x, y); }
void VideoContainer::update(float dt, uint64_t nowMs) { impl->update(dt, nowMs); }
void VideoContainer::render(SkCanvas* canvas) { impl->render(canvas); }
bool VideoContainer::isLoading() const { return impl->state.isLoading; }

} // namespace skplayer_ui

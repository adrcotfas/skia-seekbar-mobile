// libs/skplayer_ui/src/SeekBar.h
#pragma once

#include "SeekBarState.h"
#include "Theme.h"

#include "include/core/SkCanvas.h"
#include "include/core/SkColor.h"
#include "include/core/SkPaint.h"
#include "include/core/SkRect.h"

#include <algorithm>
#include <cmath>

namespace skplayer_ui {

class SeekBar {
public:
    struct SeekEvent {
        enum class Type { None, Started, Completed };
        Type type = Type::None;
        float position = 0.0f;
    };

    explicit SeekBar(float dpi = 1.0f) : dpiScale_(std::max(dpi, 0.01f)) {}

    void setState(const SeekBarState& state) { state_ = state; }
    void setPortraitMode(bool portrait) { isPortrait_ = portrait; }

    SeekEvent onTouchDown(float x, float y) {
        if (state_.isLoading) return {};

        if (isPointOnThumb(x, y)) {
            isTouching_ = true;
            touchStartedOnBar_ = false;
            updatePositionFromTouch(x);
            resetInteractionTimer();
            return {SeekEvent::Type::Started, 0.0f};
        }

        if (isPointInInteractiveArea(x, y)) {
            touchStartedOnBar_ = true;
            touchStartX_ = x;
            resetInteractionTimer();
        }
        return {};
    }

    SeekEvent onTouchMove(float x, [[maybe_unused]] float y) {
        if (state_.isLoading) return {};

        if (isTouching_) {
            updatePositionFromTouch(x);
            return {};
        }

        if (touchStartedOnBar_) {
            float dragDistance = std::abs(x - touchStartX_);
            float threshold = theme::seekbar::kDragThresholdDp * dpiScale_;
            if (dragDistance >= threshold) {
                isTouching_ = true;
                updatePositionFromTouch(x);
                return {SeekEvent::Type::Started, 0.0f};
            }
        }
        return {};
    }

    SeekEvent onTouchUp([[maybe_unused]] float x, [[maybe_unused]] float y) {
        if (state_.isLoading) {
            touchStartedOnBar_ = false;
            return {};
        }

        touchStartedOnBar_ = false;

        if (isTouching_) {
            isTouching_ = false;
            return {SeekEvent::Type::Completed, seekPreviewPosition_};
        }
        return {};
    }

    void update(float deltaTimeSeconds, bool isPlaying = true) {
        updateVisibilityState(deltaTimeSeconds, isPlaying);
        updateLoadingAnimation(deltaTimeSeconds);
    }

    float alpha() const { return controlsVisibilityAlpha_; }
    float getPreviewPosition() const { return seekPreviewPosition_; }

    void expand() { resetInteractionTimer(); }

    void collapse() {
        controlsManuallyHidden_ = true;
        isControlsVisible_ = false;
    }

    void render(SkCanvas* canvas, const SkRect& bounds) {
        if (!canvas) return;

        auto p = computeRenderParams(bounds);

        if (state_.isLoading) {
            renderLoading(canvas, p);
            return;
        }

        if (!isPortrait_ && controlsVisibilityAlpha_ <= 0.01f) return;

        float progress = getProgress();
        float previewProgress = getPreviewProgress();
        int activeChapter = isTouching_ ? getCurrentChapterIndex(previewProgress) : -1;
        float chapterScale = isTouching_ ? theme::seekbar::kActiveChapterScale : 1.0f;

        renderTrack(canvas, p, progress, activeChapter, chapterScale);
        renderThumb(canvas, p, previewProgress);
    }

private:
    // State
    SeekBarState state_;
    float dpiScale_ = 1.0f;
    bool isPortrait_ = true;
    bool isTouching_ = false;
    bool touchStartedOnBar_ = false;
    float touchStartX_ = 0.0f;
    float seekPreviewPosition_ = 0.0f;
    float timeSinceLastInteraction_ = 0.0f;
    bool isControlsVisible_ = true;
    bool controlsManuallyHidden_ = false;
    float controlsVisibilityAlpha_ = 1.0f;
    float thumbDragScale_ = 1.0f;
    SkRect currentBounds_ = SkRect::MakeEmpty();
    float loadingPhase_ = 0.0f;

    // Layout helpers
    float getTrackHeight() const {
        float height = isPortrait_ ? theme::seekbar::kTrackHeightPortraitDp : theme::seekbar::kTrackHeightLandscapeDp;
        return height * dpiScale_;
    }

    float getTrackY(const SkRect& bounds) const {
        float trackHeight = getTrackHeight();
        return isPortrait_ ? bounds.top() : bounds.centerY() - trackHeight / 2.0f;
    }

    float getThumbY(const SkRect& bounds) const {
        return getTrackY(bounds) + getTrackHeight() / 2.0f;
    }

    bool isYInInteractiveRange(float y) const {
        float verticalExtension = theme::seekbar::kTouchExtensionAboveDp * dpiScale_;
        float minY = currentBounds_.top() - verticalExtension;
        float maxY = currentBounds_.bottom();
        return y >= minY && y <= maxY;
    }

    bool isPointOnThumb(float x, float y) const {
        if (currentBounds_.isEmpty() || state_.duration <= 0.0f) return false;
        if (x < currentBounds_.left() || x > currentBounds_.right()) return false;
        if (!isYInInteractiveRange(y)) return false;

        float touchRadius = theme::seekbar::kThumbTouchTargetDp / 2.0f * dpiScale_;
        float progress = getPreviewProgress();
        float thumbX = getThumbXPosition(currentBounds_, progress);
        return std::abs(x - thumbX) <= touchRadius;
    }

    bool isPointInInteractiveArea(float x, float y) const {
        if (currentBounds_.isEmpty()) return false;
        if (x < currentBounds_.left() || x > currentBounds_.right()) return false;
        return isYInInteractiveRange(y);
    }

    void resetInteractionTimer() {
        timeSinceLastInteraction_ = 0.0f;
        isControlsVisible_ = true;
        controlsManuallyHidden_ = false;
    }

    void updateVisibilityState(float deltaTime, bool isPlaying) {
        if (isPlaying && !isTouching_ && !state_.isLoading) {
            timeSinceLastInteraction_ += deltaTime;
            if (timeSinceLastInteraction_ >= theme::seekbar::kAutoHideDelaySeconds) {
                isControlsVisible_ = false;
            }
        } else if (!isPlaying && !controlsManuallyHidden_) {
            resetInteractionTimer();
        }

        float targetAlpha = isControlsVisible_ ? 1.0f : 0.0f;
        float fadeSpeed = (targetAlpha > controlsVisibilityAlpha_) 
            ? theme::seekbar::kFadeInSpeed : theme::seekbar::kFadeOutSpeed;
        float alphaDiff = targetAlpha - controlsVisibilityAlpha_;
        controlsVisibilityAlpha_ += alphaDiff * fadeSpeed * deltaTime;
        controlsVisibilityAlpha_ = std::clamp(controlsVisibilityAlpha_, 0.0f, 1.0f);

        float targetScale = isTouching_ ? 1.5f : 1.0f;
        float scaleDiff = targetScale - thumbDragScale_;
        thumbDragScale_ += scaleDiff * theme::seekbar::kThumbScaleAnimationSpeed * deltaTime;
        thumbDragScale_ = std::clamp(thumbDragScale_, 1.0f, 1.5f);
    }

    void updateLoadingAnimation(float deltaTime) {
        if (!state_.isLoading) return;
        float dt = std::max(deltaTime, 0.0f);
        loadingPhase_ += dt * theme::seekbar::kLoadingAnimationSpeed;
        loadingPhase_ -= std::floor(loadingPhase_);
    }

    float getAnimatedThumbRadius() const {
        float minRadius = getTrackHeight() / 1.5f;
        float maxRadius = theme::seekbar::kThumbRadiusDp * dpiScale_;
        float t = controlsVisibilityAlpha_;
        return (minRadius + (maxRadius - minRadius) * t) * thumbDragScale_;
    }

    SkColor getAnimatedProgressColor() const {
        return theme::lerpColor(theme::colors::kProgressHidden,
                                theme::colors::kProgressVisible,
                                controlsVisibilityAlpha_);
    }

    float getProgress() const {
        return (state_.duration > 0.0f) ? state_.currentPosition / state_.duration : 0.0f;
    }

    float getPreviewProgress() const {
        if (state_.duration > 0.0f) {
            return isTouching_ ? (seekPreviewPosition_ / state_.duration) 
                               : (state_.currentPosition / state_.duration);
        }
        return 0.0f;
    }

    void updatePositionFromTouch(float x) {
        if (currentBounds_.width() > 0 && state_.duration > 0.0f) {
            float localX = x - currentBounds_.left();
            float progress = std::clamp(localX / currentBounds_.width(), 0.0f, 1.0f);
            seekPreviewPosition_ = progress * state_.duration;
        }
    }

    float getThumbXPosition(const SkRect& bounds, float progress) const {
        return bounds.left() + bounds.width() * progress - (2.0f * dpiScale_);
    }

    int getCurrentChapterIndex(float progress) const {
        if (state_.chapters.empty() || state_.duration <= 0.0f) return -1;
        for (size_t i = 0; i < state_.chapters.size(); ++i) {
            float chapterStart = state_.chapters[i].startTime / state_.duration;
            float chapterEnd = (i + 1 < state_.chapters.size())
                ? state_.chapters[i + 1].startTime / state_.duration : 1.0f;
            if (progress >= chapterStart && progress <= chapterEnd) {
                return static_cast<int>(i);
            }
        }
        return -1;
    }

    // Render helpers
    struct RenderParams {
        SkRect bounds;
        float trackY;
        float trackHeight;
        uint8_t alpha;
    };

    RenderParams computeRenderParams(const SkRect& inputBounds) {
        float margin = isPortrait_ ? 0.0f : theme::seekbar::kLandscapeMarginDp * dpiScale_;
        SkRect bounds = SkRect::MakeXYWH(
            inputBounds.left() + margin, inputBounds.top(),
            inputBounds.width() - margin * 2, inputBounds.height()
        );
        currentBounds_ = bounds;

        float trackHeight = getTrackHeight();
        float trackY = getTrackY(bounds);
        float visAlpha01 = std::clamp(controlsVisibilityAlpha_, 0.0f, 1.0f);
        uint8_t alpha = isPortrait_ ? 255 : static_cast<uint8_t>(255.0f * visAlpha01);

        return {bounds, trackY, trackHeight, alpha};
    }

    void renderLoading(SkCanvas* canvas, const RenderParams& p) {
        SkPaint bgPaint;
        bgPaint.setColor(theme::withAlpha(theme::colors::kTrackBackground, p.alpha));
        bgPaint.setAntiAlias(true);
        canvas->drawRect(SkRect::MakeXYWH(p.bounds.left(), p.trackY, p.bounds.width(), p.trackHeight), bgPaint);

        SkPaint fgPaint;
        fgPaint.setColor(theme::withAlpha(theme::colors::kWhite, p.alpha));
        fgPaint.setAntiAlias(true);

        float segW = std::max(p.bounds.width() * 0.44f, 56.0f * dpiScale_);
        float travel = p.bounds.width() + segW * 2.0f;
        float x = p.bounds.left() - segW + loadingPhase_ * travel;
        canvas->drawRect(SkRect::MakeXYWH(x, p.trackY, segW, p.trackHeight), fgPaint);
    }

    void renderTrack(SkCanvas* canvas, const RenderParams& p, float progress, int activeChapter, float chapterScale) {
        SkPaint bgPaint;
        bgPaint.setColor(theme::withAlpha(theme::colors::kTrackBackground, p.alpha));
        bgPaint.setAntiAlias(true);
        SkColor activeChapterBgColor = (isTouching_ && activeChapter >= 0)
            ? theme::withAlpha(theme::colors::kWhite, p.alpha) : 0;
        drawSegmentedBar(canvas, p.bounds, p.trackY, p.trackHeight, 1.0f, bgPaint,
                         activeChapter, chapterScale, activeChapterBgColor);

        SkPaint progressPaint;
        progressPaint.setColor(theme::withAlpha(getAnimatedProgressColor(), p.alpha));
        progressPaint.setAntiAlias(true);
        drawSegmentedBar(canvas, p.bounds, p.trackY, p.trackHeight, progress, progressPaint,
                         activeChapter, chapterScale, 0);
    }

    void renderThumb(SkCanvas* canvas, const RenderParams& p, float previewProgress) {
        float thumbX = getThumbXPosition(p.bounds, previewProgress);
        float thumbY = getThumbY(p.bounds);
        float thumbRadius = getAnimatedThumbRadius();

        SkPaint thumbPaint;
        thumbPaint.setColor(theme::withAlpha(getAnimatedProgressColor(), p.alpha));
        thumbPaint.setAntiAlias(true);
        canvas->drawCircle(thumbX, thumbY, thumbRadius, thumbPaint);
    }

    void drawSegmentedBar(SkCanvas* canvas, const SkRect& bounds, float trackY, float trackHeight,
                          float endProgress, SkPaint& paint, int activeChapterIndex = -1,
                          float activeChapterScale = 1.0f, SkColor activeChapterColor = 0) {
        if (state_.chapters.empty() || state_.duration <= 0.0f) {
            float width = bounds.width() * endProgress;
            canvas->drawRect(SkRect::MakeXYWH(bounds.left(), trackY, width, trackHeight), paint);
            return;
        }

        float gapWidth = theme::seekbar::kChapterGapDp * dpiScale_;
        float halfGap = gapWidth / 2;
        bool hasActiveColor = (SkColorGetA(activeChapterColor) > 0);
        size_t numChapters = state_.chapters.size();

        for (size_t i = 0; i < numChapters; ++i) {
            float chapterStart = state_.chapters[i].startTime / state_.duration;
            float chapterEnd = (i + 1 < numChapters)
                ? state_.chapters[i + 1].startTime / state_.duration : 1.0f;

            bool isActiveChapter = (static_cast<int>(i) == activeChapterIndex);
            if (chapterStart >= endProgress) break;

            float segmentEnd = std::min(chapterEnd, endProgress);
            float startX = bounds.left() + bounds.width() * chapterStart;
            float endX = bounds.left() + bounds.width() * segmentEnd;

            if (i > 0) startX += halfGap;
            if (i + 1 < numChapters) endX -= halfGap;

            float segmentWidth = endX - startX;
            if (segmentWidth > 0) {
                float heightToUse = trackHeight;
                float yToUse = trackY;
                if (isActiveChapter && activeChapterScale > 1.0f) {
                    heightToUse = trackHeight * activeChapterScale;
                    yToUse = trackY - (heightToUse - trackHeight) / 2;
                }

                SkPaint paintToUse = paint;
                if (isActiveChapter && hasActiveColor) {
                    paintToUse.setColor(activeChapterColor);
                }

                canvas->drawRect(SkRect::MakeXYWH(startX, yToUse, segmentWidth, heightToUse), paintToUse);
            }
        }
    }
};

} // namespace skplayer_ui

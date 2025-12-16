// libs/skplayer_ui/src/PlayPauseButton.h
#pragma once

#include "Theme.h"

#include "include/core/SkCanvas.h"
#include "include/core/SkPaint.h"
#include "include/core/SkPath.h"
#include "include/core/SkPathBuilder.h"
#include "include/core/SkRect.h"

#include <algorithm>

namespace skplayer_ui {

class PlayPauseButton {
public:
    explicit PlayPauseButton(float dpiScale)
        : dpiScale_(std::max(dpiScale, 0.01f)) {}

    void setPlaying(bool playing) { isPlaying_ = playing; }

    void startRipple() {
        rippleActive_ = true;
        rippleT_ = 0.0f;
    }

    void update(float dt) {
        // Morph animation
        float targetMorph = isPlaying_ ? 1.0f : 0.0f;
        float diff = targetMorph - morphProgress_;
        morphProgress_ += diff * theme::playpause::kMorphSpeed * dt;
        morphProgress_ = std::clamp(morphProgress_, 0.0f, 1.0f);

        // Ripple animation
        if (rippleActive_) {
            rippleT_ += std::max(dt, 0.0f) / theme::playpause::kRippleDurationSeconds;
            if (rippleT_ >= 1.0f) {
                rippleT_ = 1.0f;
                rippleActive_ = false;
            }
        }
    }

    void render(SkCanvas* canvas, const SkRect& bounds, float alpha) {
        if (alpha < 0.01f) return;

        float radius = bounds.width() / 2;
        float cx = bounds.centerX();
        float cy = bounds.centerY();

        // Background
        SkPaint bgPaint;
        bgPaint.setAntiAlias(true);
        bgPaint.setColor(theme::withAlpha(theme::colors::kBlack, static_cast<uint8_t>(100 * alpha)));
        canvas->drawCircle(cx, cy, radius, bgPaint);

        // Ripple overlay
        if (rippleActive_) {
            float rippleAlpha = (1.0f - rippleT_) * alpha;
            SkPaint ripplePaint;
            ripplePaint.setAntiAlias(true);
            ripplePaint.setColor(theme::withAlpha(theme::colors::kWhite,
                static_cast<uint8_t>(theme::playpause::kPressBoostAlpha * rippleAlpha)));
            canvas->drawCircle(cx, cy, radius, ripplePaint);
        }

        // Initialize paths on first render
        float iconSize = (radius * 0.8f) / dpiScale_;
        if (!pathsInitialized_) {
            initializePaths(iconSize);
        }

        // Morph between play/pause
        SkPath morphedPath;
        if (!pausePath_.interpolate(playPath_, morphProgress_, &morphedPath)) {
            morphedPath = isPlaying_ ? pausePath_ : playPath_;
        }

        // Draw icon
        SkPaint iconPaint;
        iconPaint.setColor(theme::withAlpha(theme::colors::kWhite, static_cast<uint8_t>(255 * alpha)));
        iconPaint.setAntiAlias(true);
        iconPaint.setStyle(SkPaint::kFill_Style);

        canvas->save();
        canvas->translate(cx, cy);
        canvas->scale(dpiScale_, dpiScale_);
        canvas->drawPath(morphedPath, iconPaint);
        canvas->restore();
    }

private:
    void initializePaths(float iconSize) {
        float barWidth = iconSize * 0.28f;
        float barHeight = iconSize * 0.9f;
        float barGap = iconSize * 0.15f;

        float leftBarLeft = -barGap / 2 - barWidth;
        float leftBarRight = -barGap / 2;
        float rightBarLeft = barGap / 2;
        float rightBarRight = barGap / 2 + barWidth;
        float barTop = -barHeight / 2;
        float barBottom = barHeight / 2;

        // Pause icon (two bars)
        pausePath_ = SkPathBuilder()
            .moveTo(leftBarLeft, barTop)
            .lineTo(leftBarLeft, barBottom)
            .lineTo(leftBarRight, barBottom)
            .lineTo(leftBarRight, barTop)
            .lineTo(rightBarLeft, barTop)
            .lineTo(rightBarLeft, barBottom)
            .lineTo(rightBarRight, barBottom)
            .lineTo(rightBarRight, barTop)
            .close()
            .detach();

        // Play icon (triangle with matching vertex count for interpolation)
        float leftX = leftBarLeft;
        float triangleHeight = iconSize * 0.9f;
        float rightX = leftX + triangleHeight;
        float topY = -triangleHeight / 2;
        float bottomY = triangleHeight / 2;
        float midY = 0;
        float seamT = 0.5f;

        playPath_ = SkPathBuilder()
            .moveTo(leftX, topY)
            .lineTo(leftX, bottomY)
            .lineTo(leftX + (rightX - leftX) * seamT, bottomY + (midY - bottomY) * seamT)
            .lineTo(leftX + (rightX - leftX) * seamT, topY + (midY - topY) * seamT)
            .lineTo(leftX + (rightX - leftX) * seamT, topY + (midY - topY) * seamT)
            .lineTo(leftX + (rightX - leftX) * seamT, bottomY + (midY - bottomY) * seamT)
            .lineTo(rightX, midY)
            .lineTo(rightX, midY)
            .close()
            .detach();

        pathsInitialized_ = true;
    }

    float dpiScale_;
    bool isPlaying_ = true;
    float morphProgress_ = 1.0f;  // 0=play, 1=pause

    bool rippleActive_ = false;
    float rippleT_ = 1.0f;

    SkPath playPath_;
    SkPath pausePath_;
    bool pathsInitialized_ = false;
};

} // namespace skplayer_ui

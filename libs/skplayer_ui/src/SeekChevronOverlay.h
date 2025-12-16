// libs/skplayer_ui/src/SeekChevronOverlay.h
#pragma once

#include "SeekBarState.h"  // SeekDirection
#include "Theme.h"

#include "include/core/SkCanvas.h"
#include "include/core/SkPaint.h"
#include "include/core/SkPathBuilder.h"

#include <algorithm>
#include <array>

namespace skplayer_ui {

class SeekChevronOverlay {
public:
    explicit SeekChevronOverlay(float dpiScale)
        : dpiScale_(std::max(dpiScale, 0.01f)) {}

    void onSeekFeedbackUpdated(int totalSeconds, bool burstActive) {
        dir_ = (totalSeconds < 0) ? SeekDirection::Backward : SeekDirection::Forward;
        burstActive_ = burstActive;
        spawnParticle();
    }

    void setBurstActive(bool active) { burstActive_ = active; }

    void update(float dt) {
        dt = std::max(dt, 0.0f);
        for (auto& p : particles_) {
            if (!p.active) continue;
            p.age += dt;
            if (p.age >= theme::chevron::kParticleDurationSeconds) {
                p.active = false;
            }
        }
    }

    [[nodiscard]] bool isAnimating() const {
        return std::any_of(particles_.begin(), particles_.end(),
                           [](const auto& p) { return p.active; });
    }

    void draw(SkCanvas* canvas, float centerY, uint8_t baseAlpha,
              float screenWidth, bool isPortrait) const {
        if (!canvas || baseAlpha == 0) return;

        const float edgeMarginDp = isPortrait
            ? theme::layout::kOverlayEdgeMarginDp
            : theme::layout::kOverlayChevronEdgeMarginLandscapeDp;
        const float edgeMargin = edgeMarginDp * dpiScale_;
        const float chevronH = theme::chevron::kHeightDp * dpiScale_;
        const float chevronW = chevronH * 0.5f;

        auto clampX = [&](float x) {
            float minX = edgeMargin;
            float maxX = std::max(minX, screenWidth - edgeMargin - chevronW);
            return std::clamp(x, minX, maxX);
        };

        const int dirValue = static_cast<int>(dir_);
        float endX = clampX((dirValue > 0)
            ? (screenWidth - edgeMargin - chevronW)
            : edgeMargin);

        const float travel = theme::chevron::kTravelDistanceDp * dpiScale_;
        float startX = clampX(endX - dirValue * travel);

        SkPaint paint;
        paint.setAntiAlias(true);
        paint.setStyle(SkPaint::kStroke_Style);
        paint.setStrokeWidth(theme::chevron::kStrokeWidthDp * dpiScale_);
        paint.setStrokeCap(SkPaint::kRound_Cap);
        paint.setStrokeJoin(SkPaint::kRound_Join);

        // Stationary chevron at end while bursting
        if (burstActive_) {
            paint.setColor(theme::withAlpha(theme::colors::kWhite, baseAlpha));
            drawChevron(canvas, endX, chevronW, chevronH, centerY, paint, dir_);
        }

        // Transient chevrons moving towards end
        for (const auto& p : particles_) {
            if (!p.active) continue;
            float t = std::clamp(p.age / theme::chevron::kParticleDurationSeconds, 0.0f, 1.0f);
            float x = startX + (endX - startX) * t;
            auto a = static_cast<uint8_t>(std::clamp(baseAlpha * t, 0.0f, 255.0f));
            paint.setColor(theme::withAlpha(theme::colors::kWhite, a));
            drawChevron(canvas, x, chevronW, chevronH, centerY, paint, dir_);
        }
    }

private:
    struct Particle {
        bool active = false;
        float age = 0.0f;
    };

    void spawnParticle() {
        // Reuse first inactive slot, otherwise overwrite oldest
        int best = 0;
        float oldestAge = -1.0f;
        for (int i = 0; i < theme::chevron::kMaxParticles; ++i) {
            if (!particles_[i].active) {
                best = i;
                break;
            }
            if (particles_[i].age > oldestAge) {
                oldestAge = particles_[i].age;
                best = i;
            }
        }
        particles_[best] = {true, 0.0f};
    }

    static void drawChevron(SkCanvas* canvas, float x, float w, float h,
                            float centerY, const SkPaint& paint, SeekDirection dir) {
        float topY = centerY - h * 0.5f;
        float botY = centerY + h * 0.5f;

        SkPathBuilder b;
        if (dir == SeekDirection::Forward) {
            b.moveTo(x, topY);
            b.lineTo(x + w, centerY);
            b.lineTo(x, botY);
        } else {
            b.moveTo(x + w, topY);
            b.lineTo(x, centerY);
            b.lineTo(x + w, botY);
        }
        canvas->drawPath(b.detach(), paint);
    }

    float dpiScale_;
    SeekDirection dir_ = SeekDirection::Forward;
    bool burstActive_ = false;
    std::array<Particle, theme::chevron::kMaxParticles> particles_{};
};

} // namespace skplayer_ui

// libs/skplayer_ui/src/SeekFeedbackOverlay.h
#pragma once

#include "UIState.h"
#include "SeekChevronOverlay.h"
#include "Theme.h"

#include "include/core/SkCanvas.h"
#include "include/core/SkFont.h"
#include "include/core/SkPaint.h"
#include "include/core/SkTypeface.h"

namespace skplayer_ui {

// Renders the seek feedback overlay: "- 10" / "+ 20" text with chevrons
class SeekFeedbackOverlay {
public:
    explicit SeekFeedbackOverlay(float dpiScale = 1.0f, sk_sp<SkTypeface> typeface = nullptr)
        : dpiScale_(std::max(dpiScale, 0.01f))
        , chevrons_(dpiScale_)
        , typeface_(std::move(typeface)) {}

    void show(int totalSeconds) {
        feedback_.show(totalSeconds, theme::layout::kFeedbackDurationSeconds);
        chevrons_.onSeekFeedbackUpdated(totalSeconds, true);
    }

    void setBurstActive(bool active) {
        chevrons_.setBurstActive(active);
    }

    void update(float dt) {
        feedback_.update(dt);
        chevrons_.update(dt);
    }

    bool isActive() const { return feedback_.isActive(); }
    bool isAnimating() const { return chevrons_.isAnimating(); }
    SeekDirection direction() const { return feedback_.direction; }

    void render(SkCanvas* canvas, float centerY, float screenWidth, bool isPortrait) {
        if (!feedback_.isActive()) return;

        float alpha = std::min(feedback_.timer / 0.2f, 1.0f) * 255.0f;
        SkPaint textPaint;
        textPaint.setColor(theme::withAlpha(theme::colors::kWhite, static_cast<uint8_t>(alpha)));
        textPaint.setAntiAlias(true);

        SkFont font;
        font.setSize(theme::layout::kOverlayFontSizeDp * dpiScale_);
        font.setEmbolden(true);
        if (typeface_) font.setTypeface(typeface_);

        const float baselineY = centerY + font.getSize() * 0.35f;
        SkScalar textWidth = font.measureText(feedback_.text.c_str(), feedback_.text.size(), SkTextEncoding::kUTF8);

        const float edgeMargin = (isPortrait ? theme::layout::kOverlayEdgeMarginDp
                                             : theme::layout::kOverlayEdgeMarginLandscapeDp) * dpiScale_;
        const float gap = theme::layout::kOverlayChevronGapDp * dpiScale_;
        const float chevronH = theme::chevron::kHeightDp * dpiScale_;
        const float chevronW = chevronH * 0.5f;
        int dir = static_cast<int>(feedback_.direction);

        float chevronEndX = (dir > 0) ? (screenWidth - edgeMargin - chevronW) : edgeMargin;
        float textX = (dir > 0) ? (chevronEndX - gap - textWidth) : (chevronEndX + chevronW + gap);
        textX = std::clamp(textX, edgeMargin, std::max(edgeMargin, screenWidth - edgeMargin - textWidth));

        float textScale = 1.0f + 0.1f * feedback_.pulse;
        float pivotX = textX + textWidth * 0.5f;

        canvas->save();
        canvas->translate(pivotX, centerY);
        canvas->scale(textScale, textScale);
        canvas->translate(-pivotX, -centerY);
        canvas->drawSimpleText(feedback_.text.c_str(), feedback_.text.size(),
                               SkTextEncoding::kUTF8, textX, baselineY, font, textPaint);
        canvas->restore();

        chevrons_.draw(canvas, centerY, static_cast<uint8_t>(alpha), screenWidth, isPortrait);
    }

private:
    float dpiScale_;
    SeekFeedback feedback_;
    SeekChevronOverlay chevrons_;
    sk_sp<SkTypeface> typeface_;
};

} // namespace skplayer_ui

// libs/skplayer_ui/src/SeekPreviewTooltip.h
#pragma once

#include "TimeFormat.h"
#include "Theme.h"

#include "include/core/SkCanvas.h"
#include "include/core/SkFont.h"
#include "include/core/SkPaint.h"
#include "include/core/SkRRect.h"
#include "include/core/SkTypeface.h"

#include <array>
#include <string>

namespace skplayer_ui {

class SeekPreviewTooltip {
public:
    SeekPreviewTooltip(float dpiScale, sk_sp<SkTypeface> typeface)
        : dpiScale_(std::max(dpiScale, 0.01f)), typeface_(std::move(typeface)) {}

    void setTime(float seconds) {
        positionSeconds_ = seconds;
        rebuildDisplayText();
    }

    void setChapterName(const std::string& name) {
        chapterName_ = name;
        rebuildDisplayText();
    }

    void show() { visible_ = true; }
    void hide() { visible_ = false; }
    [[nodiscard]] bool isVisible() const { return visible_; }

    void render(SkCanvas* canvas, float centerX, float topY) {
        if (!canvas || !visible_) return;

        const float paddingH = theme::tooltip::kPaddingHorizontalDp * dpiScale_;
        const float paddingV = theme::tooltip::kPaddingVerticalDp * dpiScale_;
        const float fontSize = theme::tooltip::kFontSizeDp * dpiScale_;

        SkFont font;
        font.setSize(fontSize);
        if (typeface_) font.setTypeface(typeface_);

        SkScalar textWidth = font.measureText(displayText_.c_str(), displayText_.size(), SkTextEncoding::kUTF8);

        float tooltipWidth = textWidth + paddingH * 2;
        float tooltipHeight = fontSize + paddingV * 2;
        float cornerRadius = tooltipHeight / 2;

        float tooltipLeft = centerX - tooltipWidth / 2;
        SkRect tooltipRect = SkRect::MakeXYWH(tooltipLeft, topY, tooltipWidth, tooltipHeight);
        SkRRect roundedRect = SkRRect::MakeRectXY(tooltipRect, cornerRadius, cornerRadius);

        // Background
        SkPaint bgPaint;
        bgPaint.setAntiAlias(true);
        bgPaint.setColor(theme::withAlpha(theme::colors::kBlack, theme::tooltip::kBackgroundAlpha));
        canvas->drawRRect(roundedRect, bgPaint);

        // Text
        SkPaint textPaint;
        textPaint.setAntiAlias(true);
        textPaint.setColor(theme::colors::kWhite);

        float textX = tooltipLeft + paddingH;
        float textY = topY + paddingV + fontSize * 0.8f;
        canvas->drawSimpleText(displayText_.c_str(), displayText_.size(), SkTextEncoding::kUTF8,
                               textX, textY, font, textPaint);
    }

private:
    void rebuildDisplayText() {
        auto timeView = time_format::formatTime(positionSeconds_, timeTextBuf_);
        displayText_.assign(timeView.data(), timeView.size());
        if (!chapterName_.empty()) {
            displayText_.append("  ");
            displayText_.append(chapterName_);
        }
    }

    float dpiScale_;
    sk_sp<SkTypeface> typeface_;
    float positionSeconds_ = 0.0f;
    std::string chapterName_;
    bool visible_ = false;
    std::array<char, 16> timeTextBuf_{};
    std::string displayText_;
};

} // namespace skplayer_ui

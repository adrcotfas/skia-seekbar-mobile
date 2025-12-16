// libs/skplayer_ui/src/TimeBadge.h
#pragma once

#include "TimeFormat.h"
#include "Theme.h"

#include "include/core/SkCanvas.h"
#include "include/core/SkFont.h"
#include "include/core/SkPaint.h"
#include "include/core/SkRRect.h"
#include "include/core/SkTypeface.h"

#include <array>
#include <string_view>

namespace skplayer_ui {

// Renders the "0:00 / 3:14" time badge above the seek bar
class TimeBadge {
public:
    explicit TimeBadge(float dpiScale = 1.0f, sk_sp<SkTypeface> typeface = nullptr)
        : dpiScale_(std::max(dpiScale, 0.01f)), typeface_(std::move(typeface)) {}

    void update(float currentPosition, float duration) {
        const int cur = static_cast<int>(std::max(0.0f, currentPosition));
        const int dur = static_cast<int>(std::max(0.0f, duration));

        if (dur != cachedDur_) {
            cachedDur_ = dur;
            std::array<char, 16> durText{};
            auto durView = time_format::formatTime(static_cast<float>(dur), durText);
            int n = std::snprintf(rightText_.data(), rightText_.size(), " / %.*s",
                                  static_cast<int>(durView.size()), durView.data());
            rightView_ = std::string_view(rightText_.data(),
                static_cast<size_t>(std::clamp(n, 0, static_cast<int>(rightText_.size()) - 1)));
        }
        if (cur != cachedCur_) {
            cachedCur_ = cur;
            currentView_ = time_format::formatTime(static_cast<float>(cur), currentText_);
        }
    }

    void render(SkCanvas* canvas, float leftX, float topY, float alpha) const {
        if (alpha < 0.01f) return;

        const float paddingH = theme::timebadge::kPaddingHorizontalDp * dpiScale_;
        const float paddingV = theme::timebadge::kPaddingVerticalDp * dpiScale_;
        const float fontSize = theme::timebadge::kFontSizeDp * dpiScale_;

        SkFont font;
        font.setSize(fontSize);
        if (typeface_) font.setTypeface(typeface_);

        const SkScalar leftW = font.measureText(currentView_.data(), currentView_.size(), SkTextEncoding::kUTF8);
        const SkScalar rightW = font.measureText(rightView_.data(), rightView_.size(), SkTextEncoding::kUTF8);

        const float badgeW = leftW + rightW + paddingH * 2.0f;
        const float badgeH = fontSize + paddingV * 2.0f;
        const float cornerRadius = badgeH / 2.0f;

        SkRRect roundedRect = SkRRect::MakeRectXY(
            SkRect::MakeXYWH(leftX, topY, badgeW, badgeH), cornerRadius, cornerRadius);

        const auto bgAlpha = static_cast<uint8_t>(theme::timebadge::kBackgroundAlpha * alpha);
        SkPaint bgPaint;
        bgPaint.setAntiAlias(true);
        bgPaint.setColor(theme::withAlpha(theme::colors::kBlack, bgAlpha));
        canvas->drawRRect(roundedRect, bgPaint);

        const auto textAlpha = static_cast<uint8_t>(255.0f * alpha);
        SkPaint leftPaint, rightPaint;
        leftPaint.setAntiAlias(true);
        leftPaint.setColor(theme::withAlpha(theme::colors::kWhite, textAlpha));
        rightPaint.setAntiAlias(true);
        rightPaint.setColor(theme::withAlpha(theme::colors::kGray, textAlpha));

        const float textX = leftX + paddingH;
        const float textY = topY + paddingV + fontSize * 0.8f;
        canvas->drawSimpleText(currentView_.data(), currentView_.size(),
                               SkTextEncoding::kUTF8, textX, textY, font, leftPaint);
        canvas->drawSimpleText(rightView_.data(), rightView_.size(),
                               SkTextEncoding::kUTF8, textX + leftW, textY, font, rightPaint);
    }

private:
    float dpiScale_;
    sk_sp<SkTypeface> typeface_;
    int cachedCur_ = -1;
    int cachedDur_ = -1;
    std::array<char, 16> currentText_{};
    std::array<char, 20> rightText_{};
    std::string_view currentView_{};
    std::string_view rightView_{};
};

} // namespace skplayer_ui

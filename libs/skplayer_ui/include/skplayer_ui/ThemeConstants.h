// libs/skplayer_ui/include/skplayer_ui/ThemeConstants.h
#pragma once

#include <cstdint>

namespace skplayer_ui::theme {

// Public dp constants that both the app (`src/main.cpp`) and the library use.
// Keep this small and stable.

namespace layout {
inline constexpr float kSeekBarHeightDp = 64.0f;
inline constexpr float kPortraitTopMarginDp = 48.0f;  // Safe area for camera cutout
} // namespace layout

namespace tooltip {
inline constexpr float kFontSizeDp = 14.0f;
inline constexpr float kPaddingVerticalDp = 8.0f;
inline constexpr float kMarginAboveSeekBarDp = 48.0f;
inline constexpr uint8_t kBackgroundAlpha = 180;
} // namespace tooltip

namespace timebadge {
inline constexpr float kMarginAboveSeekBarDp = 14.0f;
inline constexpr float kMarginLeftDp = 12.0f;
inline constexpr float kFontSizeDp = 12.0f;
inline constexpr float kPaddingHorizontalDp = 12.0f;
inline constexpr float kPaddingVerticalDp = 6.0f;
inline constexpr uint8_t kBackgroundAlpha = 120;
} // namespace timebadge

} // namespace skplayer_ui::theme

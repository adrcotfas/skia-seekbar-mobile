// libs/skplayer_ui/src/Theme.h
#pragma once

#include "skplayer_ui/ThemeConstants.h"

#include "include/core/SkColor.h"

#include <cstdint>

namespace skplayer_ui::theme {

// =============================================================================
// Colors
// =============================================================================

namespace colors {

inline constexpr SkColor kWhite = SkColorSetRGB(255, 255, 255);
inline constexpr SkColor kBlack = SkColorSetRGB(0, 0, 0);
inline constexpr SkColor kGray = SkColorSetRGB(185, 185, 185);  // Secondary text
inline constexpr SkColor kTrackBackground = SkColorSetRGB(60, 60, 60);

// Progress bar interpolates between these based on controls visibility
inline constexpr SkColor kProgressVisible = SkColorSetRGB(255, 0, 0);    // Red
inline constexpr SkColor kProgressHidden = SkColorSetRGB(200, 200, 200); // Gray

} // namespace colors

// Helper: apply alpha to an existing SkColor
inline constexpr SkColor withAlpha(SkColor c, uint8_t a) {
    return SkColorSetARGB(a, SkColorGetR(c), SkColorGetG(c), SkColorGetB(c));
}

// Helper: linear interpolation between two colors
inline SkColor lerpColor(SkColor a, SkColor b, float t) {
    auto lerp = [](uint8_t a, uint8_t b, float t) {
        return static_cast<uint8_t>(a + (b - a) * t);
    };
    return SkColorSetARGB(
        lerp(SkColorGetA(a), SkColorGetA(b), t),
        lerp(SkColorGetR(a), SkColorGetR(b), t),
        lerp(SkColorGetG(a), SkColorGetG(b), t),
        lerp(SkColorGetB(a), SkColorGetB(b), t)
    );
}

// =============================================================================
// SeekBar
// =============================================================================

namespace seekbar {

// Timing (seconds)
inline constexpr float kAutoHideDelaySeconds = 2.5f;
inline constexpr float kFadeInSpeed = 14.0f;
inline constexpr float kFadeOutSpeed = 14.0f;
inline constexpr float kThumbScaleAnimationSpeed = 12.0f;
inline constexpr float kLoadingAnimationSpeed = 1.2f;  // cycles per second

// Dimensions (dp)
inline constexpr float kTrackHeightPortraitDp = 2.5f;
inline constexpr float kTrackHeightLandscapeDp = 4.5f;
inline constexpr float kThumbRadiusDp = 6.0f;
inline constexpr float kThumbTouchTargetDp = 48.0f;  // Minimum touch target for thumb (accessibility)
// Hit-testing extension above the seekbar bounds (into the video area).
// Kept separate from `layout::kSeekBarHeightDp` so interaction doesn't accidentally change with layout.
inline constexpr float kTouchExtensionAboveDp = ::skplayer_ui::theme::layout::kSeekBarHeightDp;
inline constexpr float kDragThresholdDp = 8.0f;      // Distance to move before drag-to-seek activates
inline constexpr float kChapterGapDp = 3.0f;
inline constexpr float kActiveChapterScale = 1.75f;

// Landscape mode
inline constexpr float kLandscapeMarginDp = 48.0f;      // Left/right margin in landscape

} // namespace skplayer_ui

// =============================================================================
// PlayPauseButton
// =============================================================================

namespace playpause {

// Animation
inline constexpr float kMorphSpeed = 20.0f;
inline constexpr float kRippleDurationSeconds = 0.15f;
inline constexpr uint8_t kPressBoostAlpha = 70;

} // namespace playpause

// =============================================================================
// SeekChevronOverlay
// =============================================================================

namespace chevron {

// Particles
inline constexpr float kParticleDurationSeconds = 0.16f;
inline constexpr int kMaxParticles = 6;

// Dimensions (dp)
inline constexpr float kHeightDp = 11.9f;         // ~15% smaller
inline constexpr float kStrokeWidthDp = 2.55f;    // ~15% smaller
inline constexpr float kTravelDistanceDp = 25.5f; // ~15% smaller

} // namespace chevron

// =============================================================================
// SeekPreviewTooltip
// =============================================================================

namespace tooltip {

// Dimensions (dp)
inline constexpr float kPaddingHorizontalDp = 16.0f;

} // namespace tooltip

// =============================================================================
// TimeBadge (current position / duration)
// =============================================================================

namespace timebadge {
// Constants are defined in `skplayer_ui/ThemeConstants.h`.
} // namespace timebadge

// =============================================================================
// Layout (VideoContainer)
// =============================================================================

namespace layout {

// Dimensions (dp)
inline constexpr float kPlayPauseButtonSizeDp = 56.0f;
inline constexpr float kPlayPauseButtonOffsetDp = 20.0f;
inline constexpr float kOverlayEdgeMarginDp = 40.0f;
inline constexpr float kOverlayEdgeMarginLandscapeDp = 100.0f;  // Larger margin in landscape for safe areas
inline constexpr float kOverlayChevronEdgeMarginLandscapeDp = 112.0f;  // Slightly larger margin just for chevrons in landscape
inline constexpr float kOverlayChevronGapDp = 42.0f;
inline constexpr float kOverlayFontSizeDp = 20.4f;  // 15% smaller

// Timing
inline constexpr float kPlayPauseAutoHideSeconds = 1.0f;
inline constexpr float kFeedbackDurationSeconds = 0.8f;

} // namespace layout

// =============================================================================
// Gestures
// =============================================================================

namespace gesture {

inline constexpr uint64_t kDoubleTapThresholdMs = 300;
inline constexpr float kDoubleTapDistanceDp = 100.0f;
inline constexpr uint64_t kSeekBurstContinueThresholdMs = 700;

} // namespace gesture

} // namespace skplayer_ui::theme

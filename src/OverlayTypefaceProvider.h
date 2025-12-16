#pragma once

#include "include/core/SkRefCnt.h"

class SkTypeface;

// Platform-specific helper to obtain a default UI typeface for overlay text.
// Returns nullptr if unavailable; callers should handle that case.
sk_sp<SkTypeface> CreateDefaultOverlayTypeface();

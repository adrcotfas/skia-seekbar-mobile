// libs/skplayer_ui/src/TimeFormat.h
#pragma once

#include <algorithm>
#include <array>
#include <cstdio>
#include <string_view>

namespace skplayer_ui::time_format {

// Formats a duration in seconds as "m:ss" or "h:mm:ss" (clamped to >= 0).
// Allocation-free: writes into caller-provided buffer.
// Returns a null-terminated view into `out` (length excludes '\0').
inline std::string_view formatTime(float seconds, char* out, size_t outSize) {
    if (!out || outSize == 0) return {};

    int totalSeconds = static_cast<int>(std::max(0.0f, seconds));
    int hours = totalSeconds / 3600;
    int minutes = (totalSeconds % 3600) / 60;
    int secs = totalSeconds % 60;

    int n = 0;
    if (hours > 0) {
        n = std::snprintf(out, outSize, "%d:%02d:%02d", hours, minutes, secs);
    } else {
        n = std::snprintf(out, outSize, "%d:%02d", minutes, secs);
    }

    if (n < 0) {
        out[0] = '\0';
        return {};
    }

    const size_t len = static_cast<size_t>(std::min(n, static_cast<int>(outSize - 1)));
    out[len] = '\0';
    return {out, len};
}

// Convenience overload using a fixed buffer.
inline std::string_view formatTime(float seconds, std::array<char, 16>& out) {
    return formatTime(seconds, out.data(), out.size());
}

} // namespace skplayer_ui::time_format

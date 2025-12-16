// libs/skplayer_ui/src/UIState.h
#pragma once

#include "SeekBarState.h"  // for SeekDirection
#include "Theme.h"

#include <cstdint>
#include <string>
#include <algorithm>

namespace skplayer_ui {

// =============================================================================
// UI State Machine
// =============================================================================

enum class UIState {
    Loading,           // Initial buffering
    PlayingHidden,     // 1a: Playing, controls hidden
    PlayingVisible,    // 1b: Playing, controls visible (auto-hide timer)
    PausedVisible,     // 2a: Paused, controls visible
    PausedHidden,      // 2b: Paused, controls hidden
    SeekSession,       // 3: Double-tap seek active
    Dragging,          // SeekBar thumb drag
};

// =============================================================================
// Gesture / Overlay State Helpers
// =============================================================================

struct TapGesture {
    uint64_t time = 0;
    float x = 0, y = 0;

    void record(float px, float py, uint64_t t) { x = px; y = py; time = t; }
    void reset() { time = 0; }

    bool isDoubleTap(float px, float py, uint64_t now, float distThresh) const {
        if (time == 0) return false;
        float dx = px - x, dy = py - y;
        return (now - time) < theme::gesture::kDoubleTapThresholdMs &&
               (dx * dx + dy * dy) < distThresh * distThresh;
    }
};

struct SeekBurst {
    bool active = false;
    int totalSeconds = 0;
    SeekDirection direction = SeekDirection::Forward;
    uint64_t lastTapTime = 0;

    void start(SeekDirection dir, int step, uint64_t now) {
        active = true;
        direction = dir;
        totalSeconds = step;
        lastTapTime = now;
    }

    void accumulate(int step, uint64_t now) {
        totalSeconds += step;
        lastTapTime = now;
    }

    bool isWithinTimeout(uint64_t now) const {
        return (now - lastTapTime) <= theme::gesture::kSeekBurstContinueThresholdMs;
    }

    bool canContinue(SeekDirection dir, uint64_t now) const {
        return active && dir == direction && isWithinTimeout(now);
    }

    bool isTimedOut(uint64_t now) const {
        return active && !isWithinTimeout(now);
    }

    void reset() {
        active = false;
        totalSeconds = 0;
        lastTapTime = 0;
    }
};

struct SeekFeedback {
    std::string text;
    float timer = 0;
    SeekDirection direction = SeekDirection::Forward;
    float pulse = 0;

    bool isActive() const { return timer > 0.0f && !text.empty(); }

    void show(int totalSec, float duration) {
        text = (totalSec > 0 ? "+ " : "- ") + std::to_string(std::abs(totalSec));
        timer = duration;
        direction = (totalSec < 0) ? SeekDirection::Backward : SeekDirection::Forward;
        pulse = 1.0f;
    }

    void update(float dt) {
        if (timer > 0.0f) {
            timer -= dt;
            if (timer <= 0.0f) { timer = 0.0f; text.clear(); }
        }
        if (pulse > 0.0f) {
            pulse = std::max(0.0f, pulse - dt * 10.0f);
        }
    }
};

} // namespace skplayer_ui

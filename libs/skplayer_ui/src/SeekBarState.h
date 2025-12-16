// libs/skplayer_ui/src/SeekBarState.h
#pragma once

#include "skplayer_ui/VideoContainer.h"  // for Chapter

#include <vector>

namespace skplayer_ui {

enum class SeekDirection : int {
    Backward = -1,
    Forward = 1
};

struct SeekBarState {
    float duration = 0.0f;
    float currentPosition = 0.0f;
    std::vector<Chapter> chapters;
    bool isLoading = false;
};

} // namespace skplayer_ui

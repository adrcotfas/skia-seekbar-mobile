// libs/skplayer_ui/src/Chapter.h
#pragma once

#include <string>

namespace seekbar {

struct Chapter {
    float startTime = 0.0f;
    std::string title;

    explicit Chapter(float start, std::string t = "")
        : startTime(start), title(std::move(t)) {}
};

} // namespace skplayer_ui

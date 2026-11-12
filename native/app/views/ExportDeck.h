#pragma once

#include "Song.h"
#include <string>

namespace trakr {

class ExportDeck {
public:
    ExportDeck() = default;

    void draw(bool* p_open, const Song& song, float footerHeight = 28.0f);

private:
    std::string statusMessage_;
    float statusTimer_{0.0f};
};

} // namespace trakr


#pragma once

#include "FillGenerator.h"
#include "History.h"
#include "Song.h"
#include "PatternSelectionHelper.h"

namespace trakr {

class FillModal {
public:
    FillModal() = default;

    void draw(bool* p_open, Pattern& pat, int patIdx, int track, int selectedInst,
              History& history, Song& song, float footerHeight = 28.0f,
              bool hasSelection = false, const SelectionRange& sel = {});

private:
    FillConfig config_;
    int lastMinRow_{-1};
    int lastMaxRow_{-1};
    int lastMinTrack_{-1};
    int lastMaxTrack_{-1};
};

} // namespace trakr

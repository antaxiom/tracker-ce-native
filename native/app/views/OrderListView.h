#pragma once

#include "View.h"
#include "Song.h"
#include "History.h"
#include "EngineHost.h"

namespace trakr {

// Song screen. Structural port of core/interface/songEditor/gSongEdit.cpp and
// core/libs/NewDisplay/songPlayer.cpp: cFrame + cBar + cSongPlayer.
class OrderListView : public View {
public:
    const char* name() const override { return "Order List"; }
    void draw(bool* p_open = nullptr) override;

    void update(Song& song, History& history, EngineHost& engineHost, int& currentOrderIdx);

private:
    // Firmware keeps the selected row visible by scrolling patternsList in
    // whole rows (cSongPlayer::setData / showList).
    int listStart_{0};
};

} // namespace trakr

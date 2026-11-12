#pragma once

// Master EQ (Master tab page 3) — semantic UI port.
//
// Firmware reference: core/interface/masterParams/gMasterParams.cpp
//   showEqualizerScreen(): five band cBars (columns 0..4), the selected-band
//   frame (framesPlaces 0..4) and the blank space cover over columns 5..7.

#include "ui/controls/primitives.h"
#include "ui/display.h"
#include "ui/layout.h"
#include "imgui.h"

namespace trakr {

class Song;
class History;
class EngineHost;
class MixerView;

class MasterEqScreen {
public:
    MasterEqScreen();

    void update(Song& song,
                History& history,
                EngineHost& engineHost,
                MixerView& view,
                const ImRect& workspace,
                ui::LayoutProfile profile = ui::LayoutProfile::Classic,
                bool shiftHeld = false);

    // Firmware band names (gMasterParams.cpp:827-831).
    static const char* bandLabel(int band);

    void render(ImDrawList* drawList,
                const ImRect& workspace,
                ui::LayoutProfile profile = ui::LayoutProfile::Classic) const;

private:
    void build();
    void syncFromModel(const Song& song, const MixerView& view);

    ui::Display display_;
    ui::BarControl* band_[8]{};
    ui::FrameControl* frame_{nullptr};
    ui::BlankCoverControl* cover_{nullptr};
    MixerView* view_{nullptr};
    bool built_{false};
};

} // namespace trakr

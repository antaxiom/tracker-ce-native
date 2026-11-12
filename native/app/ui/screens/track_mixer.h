#pragma once

// Track Mixer (Master 2/3) — semantic UI port.
//
// Firmware reference: core/interface/masterParams/gMasterParams.cpp
//   showMixerScreen(): cLabel trackNameLabel[i] (topBarLabelColors, Bold17),
//   cBar trackVolumeBar[i] (mixer gain fader) and cBar barControl[i]
//   (level bar). Frame and blank covers are hidden on this page; the bottom
//   mute labels are app chrome.

#include "ui/controls/primitives.h"
#include "ui/display.h"
#include "ui/layout.h"
#include "imgui.h"

namespace trakr {

class Song;
class History;
class EngineHost;
class MixerView;

class TrackMixerScreen {
public:
    TrackMixerScreen();

    void update(Song& song,
                History& history,
                EngineHost& engineHost,
                MixerView& view,
                const ImRect& workspace,
                ui::LayoutProfile profile = ui::LayoutProfile::Classic,
                bool shiftHeld = false);

    void render(ImDrawList* drawList,
                const ImRect& workspace,
                ui::LayoutProfile profile = ui::LayoutProfile::Classic) const;

private:
    void build();
    void syncFromModel(const Song& song, const MixerView& view);

    ui::Display display_;
    ui::BarControl* level_[8]{};
    ui::BarControl* volume_[8]{};
    ui::LabelControl* name_[8]{};
    MixerView* view_{nullptr};
    bool built_{false};
};

} // namespace trakr

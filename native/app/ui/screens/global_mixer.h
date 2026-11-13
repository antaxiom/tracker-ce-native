#pragma once

// Global Mixer (Master 3/3) — semantic UI port.
//
// Firmware reference: core/interface/masterParams/gMasterParams.cpp
//   showGlobalMixer(): four aux columns (Delay, Reverb, Dry Mix, Line In) with a
//   name label, a volume cBar, two level cBars per column and the selection
//   frame (framesPlaces 0..3). The bottom labels are app chrome.

#include "ui/controls/primitives.h"
#include "ui/display.h"
#include "ui/layout.h"
#include "imgui.h"

namespace trakr {

class Song;
class History;
class EngineHost;
class MixerView;

class GlobalMixerScreen {
public:
    GlobalMixerScreen();

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

    // True while a control on this screen samples the animation clock
    // (ui/animation.h); AppLayout then keeps the 60 Hz loop running.
    bool animatingAt(double nowMs) const { return display_.animatingAt(nowMs); }

private:
    void build();
    void syncFromModel(const Song& song, const MixerView& view, const EngineHost& engineHost);

    ui::Display display_;
    ui::LabelControl* name_[4]{};
    ui::BarControl* volume_[4]{};
    ui::BarControl* levelL_[4]{};
    ui::BarControl* levelR_[4]{};
    ui::FrameControl* frame_{nullptr};
    MixerView* view_{nullptr};
    bool built_{false};

};

} // namespace trakr

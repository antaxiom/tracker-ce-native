#pragma once

// Instrument Effects and Automation pages — semantic UI port.
//
// Firmware reference: core/interface/instrumentEditor/gInstrEdit.cpp
//   showEffectScreen() and showInstrumentEnv(): per-column cBars, cLists for
//   the automation destination/type/LFO shape/LFO speed, blank space cover on
//   the Effects page, and the frame places (8 general / 7 automation).

#include "ui/controls/primitives.h"
#include "ui/display.h"
#include "ui/layout.h"
#include "imgui.h"

namespace trakr {

class Song;
class History;
class EngineHost;
class InstrumentView;

class InstrumentPagesScreen {
public:
    InstrumentPagesScreen();

    void update(Song& song,
                History& history,
                EngineHost& engineHost,
                int& selectedInstIdx,
                InstrumentView& view,
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
    void syncFromModel(const Song& song, int selectedInstIdx, const InstrumentView& view);

    ui::Display display_;
    ui::FrameControl* frameGeneral_{nullptr};
    ui::FrameControl* frameAutomation_{nullptr};
    ui::BarControl* bars_[8]{};
    ui::ListControl* destList_{nullptr};
    ui::ListControl* typeList_{nullptr};
    ui::ListControl* shapeList_{nullptr};
    ui::ListControl* speedList_{nullptr};
    ui::BlankCoverControl* effectsCover_{nullptr};
    Song* song_{nullptr};
    EngineHost* engineHost_{nullptr};
    InstrumentView* view_{nullptr};
    int* selectedInstIdx_{nullptr};
    bool built_{false};

};

} // namespace trakr

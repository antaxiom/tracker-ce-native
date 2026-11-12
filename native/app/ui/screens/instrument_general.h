#pragma once

// Instrument Parameters 1/2 (General) — semantic UI port.
//
// Firmware reference: core/interface/instrumentEditor/gInstrEdit.cpp
//   initDisplayControls()      control creation order
//   showInstrumentParams()     visible controls and values
//   refreshFramePosition()     frame place = selected parameter
//
// The screen is the vertical slice for the UI restart: rendering goes through
// the control layer and Canvas; editing stays delegated to InstrumentView so
// the existing command/history behavior is preserved.

#include "ui/display.h"
#include "ui/layout.h"
#include "ui/controls/primitives.h"
#include "imgui.h"

namespace trakr {

class Song;
class History;
class EngineHost;
class InstrumentView;

class InstrumentGeneralScreen {
public:
    InstrumentGeneralScreen();

    void update(Song& song,
                History& history,
                EngineHost& engineHost,
                int& selectedInstIdx,
                InstrumentView& view,
                const ImRect& workspace,
                ui::LayoutProfile profile,
                bool shiftHeld);

    void render(ImDrawList* drawList,
                const ImRect& workspace,
                ui::LayoutProfile profile = ui::LayoutProfile::Classic) const;

private:
    void build();
    void syncFromModel(const Song& song, int selectedInstIdx, const InstrumentView& view);

    ui::Display display_;
    ui::FrameControl* frame_{nullptr};
    ui::ListControl* filterList_{nullptr};
    ui::BarControl* bars_[8]{};
    Song* song_{nullptr};
    EngineHost* engineHost_{nullptr};
    InstrumentView* view_{nullptr};
    int* selectedInstIdx_{nullptr};
    bool built_{false};
};

} // namespace trakr

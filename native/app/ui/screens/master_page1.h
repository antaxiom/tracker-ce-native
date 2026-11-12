#pragma once

// Master 1/3 (audio control) — semantic UI port.
//
// Firmware reference: core/interface/masterParams/gMasterParams.cpp
//   showMasterScreen(): volume bar (col 0), saturation list (col 5),
//   bass/space bars (cols 6/7), blank cover cols 1..4, frame place = selected.
//
// Editing is delegated to MixerView (Master sub-page) so commands and value
// clamping stay in one place.

#include "ui/display.h"
#include "ui/layout.h"
#include "ui/controls/primitives.h"
#include "imgui.h"

namespace trakr {

class Song;
class EngineHost;
class MixerView;

class MasterPage1Screen {
public:
    MasterPage1Screen();

    void update(Song& song,
                EngineHost& engineHost,
                MixerView& view,
                const ImRect& workspace,
                ui::LayoutProfile profile,
                bool shiftHeld);

    void render(ImDrawList* drawList,
                const ImRect& workspace,
                ui::LayoutProfile profile = ui::LayoutProfile::Classic) const;

private:
    void build();
    void syncFromModel(const Song& song, const MixerView& view);

    ui::Display display_;
    ui::FrameControl* frame_{nullptr};
    ui::ListControl* saturationList_{nullptr};
    ui::BarControl* bars_[8]{};
    Song* song_{nullptr};
    EngineHost* engineHost_{nullptr};
    MixerView* view_{nullptr};
    bool built_{false};
};

} // namespace trakr

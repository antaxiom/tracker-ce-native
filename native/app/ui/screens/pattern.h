#pragma once

// Pattern editor — semantic UI port (Classic).
//
// Rendering is the cTracker control. Navigation, editing and history stay in
// PatternView so the firmware-faithful key behavior is preserved.

#include "ui/display.h"
#include "ui/layout.h"
#include "ui/controls/tracker.h"
#include "imgui.h"

namespace trakr {

class Song;
class History;
class EngineHost;
class PatternView;

class PatternScreen {
public:
    PatternScreen();

    void update(Song& song,
                History& history,
                EngineHost& engineHost,
                int currentOrderIdx,
                int octave,
                int editStep,
                bool recordMode,
                int& selectedInstIdx,
                PatternView& view,
                const ImRect& workspace,
                ui::LayoutProfile profile,
                bool shiftHeld);

    void render(ImDrawList* drawList,
                const ImRect& workspace,
                ui::LayoutProfile profile = ui::LayoutProfile::Classic) const;

private:
    void build();
    ui::Display display_;
    TrackerControl* tracker_{nullptr};
    bool built_{false};
};

} // namespace trakr

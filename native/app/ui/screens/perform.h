#pragma once

// Performance Mode — semantic UI port.
//
// Firmware reference: core/interface/performanceMode/gPerformance.cpp
//   initDisplayControls(): cFrame (12 places), cLabel[8] + cBgLabel (app
//   chrome), then per column i: textLabel[i] (vertical cLabel) and
//   value1Label[i] (cMultiLabel).
//   multiLabel: core/libs/NewDisplay/multiLabel.cpp.

#include "ui/controls/multi_label.h"
#include "ui/controls/primitives.h"
#include "ui/display.h"
#include "ui/layout.h"
#include "imgui.h"

namespace trakr {

class Song;
class History;
class EngineHost;
class PerformView;

class PerformScreen {
public:
    PerformScreen();

    void update(Song& song,
                History& history,
                EngineHost& engineHost,
                PerformView& view,
                const ImRect& workspace,
                ui::LayoutProfile profile = ui::LayoutProfile::Classic);

    void render(ImDrawList* drawList,
                const ImRect& workspace,
                ui::LayoutProfile profile = ui::LayoutProfile::Classic) const;

private:
    void build();
    void syncFromModel(const PerformView& view);

    ui::Display display_;
    ui::FrameControl* frame_{nullptr};
    ui::MultiLabelControl* values_[12]{};
    ui::VerticalLabelControl* names_[12]{};
    PerformView* view_{nullptr};
    EngineHost* engineHost_{nullptr};
    bool built_{false};
};

} // namespace trakr

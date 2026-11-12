#pragma once

// Sample Loader — semantic UI port.
//
// Firmware reference: core/interface/sampleImporter/gSampleImport.cpp
//   initDisplayControls(): cFrame (3 places: SD card, instruments, preview
//   volume), cLabel[8] + cBgLabel (app chrome), explorer/instrument top labels,
//   explorer/instrument cLists, memory cBar (CompareTwoValues), preview-volume
//   cBar, and hidden crop controls.
//   showDefaultScreen(): "SD Card" / "Instruments" headers and list focus.

#include "ui/controls/primitives.h"
#include "ui/display.h"
#include "ui/layout.h"
#include "imgui.h"

namespace trakr {

class Song;
class History;
class EngineHost;
class SampleLoaderView;

class SampleLoaderScreen {
public:
    SampleLoaderScreen();

    void update(Song& song,
                History& history,
                EngineHost& engineHost,
                int& selectedInstIdx,
                SampleLoaderView& view,
                const ImRect& workspace,
                ui::LayoutProfile profile = ui::LayoutProfile::Classic);

    void render(ImDrawList* drawList,
                const ImRect& workspace,
                ui::LayoutProfile profile = ui::LayoutProfile::Classic) const;

private:
    void build();
    void syncFromModel(const Song& song, const SampleLoaderView& view);

    ui::Display display_;
    ui::FrameControl* frame_{nullptr};
    ui::LabelControl* explorerTitle_{nullptr};
    ui::LabelControl* instrumentTitle_{nullptr};
    ui::ListControl* explorerList_{nullptr};
    ui::ListControl* instrumentList_{nullptr};
    ui::BarControl* memoryBar_{nullptr};
    ui::BarControl* previewBar_{nullptr};
    Song* song_{nullptr};
    EngineHost* engineHost_{nullptr};
    SampleLoaderView* view_{nullptr};
    int selectedInst_{0};
    bool built_{false};
};

} // namespace trakr

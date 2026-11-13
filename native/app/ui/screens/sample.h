#pragma once

// Sample Playback — semantic UI port.
//
// Firmware reference: core/interface/samplePlayback/gSamplePlay.cpp
//   initDisplayControls(): cFrame (7 places; 0..5 are bottom labels, 6 is the
//   play-mode list), cLabel[8] + cBgLabel (app chrome), cList playModeList,
//   cProgressCursor, cLineIndicator, cPoints, cSlicePoints, cSpectrum[2],
//   cWtProgress.
//   showDefaultScreen(): control visibility per play mode.
//
// Editing, audition and history are delegated to SampleView.

#include "ui/controls/primitives.h"
#include "ui/controls/waveform.h"
#include "ui/display.h"
#include "ui/layout.h"
#include "imgui.h"

namespace trakr {

class Song;
class History;
class EngineHost;
class SampleView;

class SampleScreen {
public:
    SampleScreen();

    void update(Song& song,
                History& history,
                EngineHost& engineHost,
                int selectedInstIdx,
                SampleView& view,
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
    void syncFromModel(Song& song, EngineHost& engineHost, int selectedInstIdx, SampleView& view);

    ui::Display display_;
    ui::FrameControl* frame_{nullptr};
    ui::ListControl* playModeList_{nullptr};
    ui::ListControl* editList_{nullptr};
    ui::ProgressCursorControl* progressCursor_{nullptr};
    ui::LineIndicatorControl* granularCursor_{nullptr};
    ui::PointsControl* points_{nullptr};
    ui::SlicePointsControl* slicePoints_{nullptr};
    ui::SpectrumControl* spectrum_[2]{};
    ui::WtProgressControl* wtPosition_{nullptr};
    Song* song_{nullptr};
    EngineHost* engineHost_{nullptr};
    SampleView* view_{nullptr};
    int selectedInst_{0};
    int dragTarget_{-1};
    bool built_{false};

};

} // namespace trakr

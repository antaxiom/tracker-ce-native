#pragma once

// Polyend Tracker Instrument Parameters Screen Parity
// Firmware reference: core/interface/instrumentEditor/gInstrEdit.cpp & fIinstrEdit.cpp
// Manual reference: native/docs/reference/tracker-manual-1.7.0.txt Section 6.3 - 6.5 (lines 5732-6250)

#include "View.h"
#include "Song.h"
#include "History.h"
#include "EngineHost.h"
#include "InstrumentBars.h"
#include "../hw/ScreenButtons.h"
#include <string>
#include <vector>

namespace trakr {

class InstrumentView : public View {
public:
    enum class SubPage {
        General,    // Page 1/2: Volume, Panning, Tune, Finetune, Filter Type, Cutoff, Resonance, More
        Effects,    // Page 1/2 FX: Overdrive, Bit Depth, Reverb Send, Delay Send, Back
        Automation  // Page 2/2: Destination, Type (Off/Envelope/LFO), Shape/Speed/ADSR, Amount
    };

    InstrumentView();

    const char* name() const override { return "Instrument Parameters"; }
    void draw(bool* p_open = nullptr) override;

    void update(Song& song, History& history, EngineHost& engineHost, int& selectedInstIdx);

    SubPage subPage() const { return subPage_; }
    void setSubPage(SubPage sp) { subPage_ = sp; }
    void setShiftHeld(bool s) { shiftHeld_ = s; }
    void togglePage(); // Toggle the secondary Effects page

    int selectedColumn() const { return selectedCol_; }
    void setSelectedColumn(int col);

    void adjustSelectedParam(Song& song, EngineHost& engineHost, int selectedInstIdx, int delta, bool fine = false);
    void resetSelectedParam(Song& song, EngineHost& engineHost, int selectedInstIdx);

    // Screen buttons for contextual footer (y=424..480)
    std::vector<hw::ScreenButton> getScreenButtons(Song& song, EngineHost& engineHost, int selectedInstIdx);

private:
    void renderGeneralPage(ImDrawList* dl, const ImVec2& origin, float totalW, float totalH,
                           Instrument& inst, Song& song, EngineHost& engineHost, int selectedInstIdx);
    void renderEffectsPage(ImDrawList* dl, const ImVec2& origin, float totalW, float totalH,
                           Instrument& inst, Song& song, EngineHost& engineHost, int selectedInstIdx);
    void renderAutomationPage(ImDrawList* dl, const ImVec2& origin, float totalW, float totalH,
                             Instrument& inst, Song& song, EngineHost& engineHost, int selectedInstIdx);

    SubPage subPage_{SubPage::General};
    bool shiftHeld_{false};
    int selectedCol_{0};       // 0..7
    int selectedAutoDest_{2};  // 0: Volume, 1: Panning, 2: Cutoff, 3: WTPos, 4: GranPos, 5: Finetune
    bool mouseDraggingCol_{false};
    float dragStartVal_{0.0f};
};

} // namespace trakr

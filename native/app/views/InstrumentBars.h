#pragma once

#include "imgui.h"
#include "imgui_internal.h"
#include "Song.h"

// Instrument screen firmware drawing layer.
// Structural port of core/interface/instrumentEditor/gInstrEdit.cpp and the
// NewDisplay controls it creates (bar, list, label, frame, bgLabel).

namespace trakr {

extern const char* const kFilterTypes[4];
extern const char* const kAutoDests[6];
extern const char* const kAutoTypes[3];
extern const char* const kLfoShapes[5];
extern const char* const kLfoSpeeds[18];
constexpr int kNumSpeeds = 18;

// Complete state of one Instrument screen pass. Coordinates in the renderer
// stay in firmware device pixels (800x480); sx/sy map them to the workspace.
struct InstrumentScreenState {
    ImDrawList* dl{};
    ImVec2 origin{};
    float sx{1.0f};
    float sy{1.0f};
    const Instrument* inst{};
    int page{0};            // 0: General 1/2, 1: Effects, 2: Automation 2/2
    int selectedCol{0};     // firmware selected place / parameter column
    int selectedDest{2};    // selected automation destination
    ImFont* light{};
    ImFont* bold{};
    float lightSize{14.0f}; // controlStyleFont1 (RobotoMono Light 14)
    float boldSize{17.0f};  // controlStyleFont2/4 (RobotoMono Bold 17)
    int clickedCol{-1};
    int clickedDest{-1};
    int clickedType{-1};
    int clickedShape{-1};
    int clickedSpeed{-1};
    int clickedFilter{-1};
};

void drawInstrumentScreen(InstrumentScreenState& state);

} // namespace trakr

#pragma once

#include "Song.h"
#include "History.h"
#include "EngineHost.h"
#include "imgui.h"
#include <string>
#include <vector>
#include <cstdint>

namespace trakr {

struct FxDefinition {
    uint8_t type;
    char ch;
    const char* name;
    const char* desc;
};

// Returns display character for an FX type
char fxTypeToChar(uint8_t type);

// Maps a character or keypress to an FX type
uint8_t fxCharToType(char ch);

// Returns human-readable label
const char* fxTypeName(uint8_t type);

// Returns list of all available FX definitions
const std::vector<FxDefinition>& getAllFxDefinitions();

// Formats an FX type and value into standard Tracker display string (e.g. "RV 1", "C 50", "----")
void formatFxCell(char* outBuf, size_t bufSize, uint8_t type, uint8_t val);

// Renders the right-hand Step FX Inspector sidebar
void renderFxInspector(Cell& cell, int cursorSubCol, int patIdx, int cursorTrack,
                        int cursorRow, History& history, Song& song,
                        EngineHost& engineHost, float width);

// Renders the mini waveform strip of active instrument sample in pattern mode
void renderPatternWaveformStrip(ImDrawList* dl, const ImVec2& pos, const ImVec2& size,
                                const Instrument* inst, const EngineHost& engineHost);

// State consumed by the firmware-faithful Pattern screen pass. Mirrors
// strTrackerPattern (core/libs/NewDisplay/trackerControl.h) in app terms.
struct PatternRenderParams {
    const Song* song{};
    const Pattern* pat{};
    int cursorTrack{0};
    int cursorRow{0};
    int cursorSubCol{0};
    bool recording{false};
    bool hasSelection{false};
    int selStartTrack{0}, selEndTrack{0}, selStartRow{0}, selEndRow{0};
    bool playing{false};
    int playheadRow{0};
};

// Structural port of cPatternEditor::initDisplayControls()/showDefaultScreen()
// in core/interface/patternEditor/gPatternEdit.cpp: composites cTracker
// (verticalPatternControl) in reverse creation order, then the app-owned
// bottom chrome, then cFrame.
void renderPatternScreen(ImDrawList* dl, const ImVec2& origin, float sx, float sy,
                         const PatternRenderParams& p);

} // namespace trakr


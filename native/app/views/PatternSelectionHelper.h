#pragma once

#include "Song.h"
#include "History.h"
#include "EngineHost.h"
#include "imgui.h"
#include <algorithm>
#include <vector>

namespace trakr {

struct SelectionRange {
    int startTrack{0};
    int endTrack{0};
    int startRow{0};
    int endRow{0};

    int minTrack() const { return std::min(startTrack, endTrack); }
    int maxTrack() const { return std::max(startTrack, endTrack); }
    int minRow() const { return std::min(startRow, endRow); }
    int maxRow() const { return std::max(startRow, endRow); }
    bool contains(int track, int row) const {
        return track >= minTrack() && track <= maxTrack() && row >= minRow() && row <= maxRow();
    }
    int rowCount() const { return maxRow() - minRow() + 1; }
    int trackCount() const { return maxTrack() - minTrack() + 1; }
};

// 1. Relative Transpose: alters all selected notes preserving relative intervals
void transposeSelection(Pattern& pat, int patIdx, const SelectionRange& sel, int semitones,
                        History& history, Song& song, EngineHost& engineHost);

// 2. Relative Volume: nudges volumes of all selected cells preserving relative dynamics
void adjustVolumeSelection(Pattern& pat, int patIdx, const SelectionRange& sel, int deltaVol,
                           History& history, Song& song, EngineHost& engineHost);

// 3. Uniform Note Assignment: sets all selected cells to the exact same note
void setSelectionToNote(Pattern& pat, int patIdx, const SelectionRange& sel, int note, int inst,
                        History& history, Song& song, EngineHost& engineHost);

// 4. Uniform Instrument Assignment: sets all selected cells to the same instrument
void setSelectionInstrument(Pattern& pat, int patIdx, const SelectionRange& sel, int inst,
                            History& history, Song& song, EngineHost& engineHost);

// 5. Uniform Volume Assignment: sets all selected cells to the same volume
void setSelectionVolume(Pattern& pat, int patIdx, const SelectionRange& sel, int vol,
                        History& history, Song& song, EngineHost& engineHost);

// 6. Clear selected cells
void clearSelection(Pattern& pat, int patIdx, const SelectionRange& sel, int subCol,
                    History& history, Song& song, EngineHost& engineHost);

// 7. Multi-track / multi-row Copy & Paste
void copySelection(const Pattern& pat, const SelectionRange& sel,
                   std::vector<std::vector<Cell>>& clipboard);
void pasteSelection(Pattern& pat, int patIdx, int destTrack, int destRow,
                    const std::vector<std::vector<Cell>>& clipboard,
                    History& history, Song& song, EngineHost& engineHost);

// 8. Visual selection highlight overlay
void renderSelectionOverlay(ImDrawList* drawList, const ImVec2& canvasPos, float gutterWidth,
                            float headerHeight, float trackWidth, float rowHeight,
                            int startRow, int visibleRows, const SelectionRange& sel,
                            ImU32 colHighlight, ImU32 colBorder, int startTrack = 0);

// 9. Fixed-height Unified Pattern Header & Selection Action Bar (Zero layout shifts)
void renderPatternHeader(Pattern& pat, int patIdx, bool recordMode, bool hasSelection,
                         const SelectionRange& sel, int selectedInstIdx,
                         History& history, Song& song, EngineHost& engineHost,
                         bool* pHasSelection);

// 10. Unified 2D cursor step & selection navigation (harmonized across visual D-pad, jog wheel, and keyboard)
void navigateCursor(int dTrack, int dRow, int dSubCol, bool isShift,
                    int numTracks, int numRows,
                    int& cursorTrack, int& cursorRow, int& cursorSubCol,
                    bool& hasSelection, SelectionRange& sel);

// 11. Selection Keyboard & Navigation Router
bool handleSelectionShortcuts(Pattern& pat, int patIdx, Song& song, History& history,
                             EngineHost& engineHost, int octave, int editStep,
                             bool recordMode, int selectedInstIdx,
                             int& cursorTrack, int& cursorRow, int& cursorSubCol,
                             bool& hasSelection, SelectionRange& sel,
                             std::vector<std::vector<Cell>>& clipboard,
                             bool shiftHeld = false);

} // namespace trakr

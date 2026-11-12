#pragma once

#include "View.h"
#include "Song.h"
#include "History.h"
#include "EngineHost.h"
#include "PatternSelectionHelper.h"
#include <vector>

namespace trakr {

struct Theme;

class PatternView : public View {
public:
    PatternView();
    const char* name() const override { return "Pattern Editor"; }
    void draw(bool* p_open = nullptr) override;

    void update(Song& song, History& history, EngineHost& engineHost,
                int currentOrderIdx, int octave, int editStep, bool recordMode,
                int& selectedInstIdx, const Theme* theme = nullptr);
    void update(Song& song, History& history, EngineHost& engineHost,
                int currentOrderIdx, int octave, int editStep, bool recordMode,
                int&& selectedInstIdx, const Theme* theme = nullptr) {
        int idx = selectedInstIdx;
        update(song, history, engineHost, currentOrderIdx, octave, editStep, recordMode, idx, theme);
    }

    void syncInstrumentFromCursor(const Pattern& pat, const Song& song, int& selectedInstIdx);

    int cursorRow() const { return cursorRow_; }
    int cursorTrack() const { return cursorTrack_; }
    int cursorSubCol() const { return cursorSubCol_; }
    void setCursor(int track, int row, int subCol = 0) {
        cursorTrack_ = track;
        cursorRow_ = row;
        cursorSubCol_ = subCol;
    }

    bool hasSelection() const { return hasSelection_; }
    void clearSelection() { hasSelection_ = false; }
    void setSelection(int startTrk, int startRow, int endTrk, int endRow) {
        hasSelection_ = true;
        selRange_.startTrack = startTrk; selRange_.startRow = startRow;
        selRange_.endTrack = endTrk; selRange_.endRow = endRow;
    }
    SelectionRange getSelection() const { return selRange_; }
    bool shiftHeld() const { return shiftHeld_; }
    void setShiftHeld(bool s) { shiftHeld_ = s; }

    void navigate(int dTrack, int dRow, int dSubCol, bool isShift,
                  int numTracks, int numRows, const Pattern& pat,
                  const Song& song, int& selectedInstIdx);

    void handleKeyboardInput(Pattern& pat, int patIdx, Song& song, History& history,
                             EngineHost& engineHost, int octave, int editStep,
                             bool recordMode, int& selectedInstIdx);

    void insertRowAtCursor(Pattern& pat, int patIdx, Song& song, History& history, EngineHost& engineHost);
    void deleteRowOrCellAtCursor(Pattern& pat, int patIdx, Song& song, History& history, EngineHost& engineHost, bool shiftHeld);
    void copySelectionOrCell(Pattern& pat);
    void pasteAtCursor(Pattern& pat, int patIdx, Song& song, History& history, EngineHost& engineHost);
    void adjustValueAtCursor(Pattern& pat, int patIdx, Song& song, History& history, EngineHost& engineHost, int delta, bool shiftHeld);
    const std::vector<std::vector<Cell>>& getClipboard() const { return clipboard_; }

    bool compact8Track() const { return compact8Track_; }
    void setCompact8Track(bool c) { compact8Track_ = c; }

private:
    static std::string formatNoteName(int16_t note);

    int cursorRow_{0};
    int cursorTrack_{0};
    int cursorSubCol_{0}; // 0: Note, 1: Inst, 2: FX1, 3: FX2 (Polyend 4-column tracker)
    bool compact8Track_{false};

    // Selection
    bool hasSelection_{false};
    SelectionRange selRange_;
    bool shiftHeld_{false};

    // Clipboard
    std::vector<std::vector<Cell>> clipboard_;

    float scrollY_{0.0f};
    bool followPlayhead_{true};
    bool showFxInspector_{false};
};

} // namespace trakr

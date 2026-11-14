#pragma once

// cTracker (vertical pattern editor) semantic port.
//
// Firmware reference: core/libs/NewDisplay/tracker.cpp.
// Visual layout mirrors the existing app port in views/PatternFxHelper.cpp:
// 27px row-number gutters, four 186px track columns, 15 visible rows at 26px,
// the cursor on visible row 7, beat dividers every `stepDev` rows.

#include "ui/control.h"

namespace trakr {

class Song;
class Pattern;

struct TrackerState {
    const Song* song{nullptr};
    const Pattern* pattern{nullptr};
    int cursorTrack{0};
    int cursorRow{0};
    int cursorSubCol{0};
    bool recording{false};
    bool hasSelection{false};
    int selStartTrack{0};
    int selEndTrack{0};
    int selStartRow{0};
    int selEndRow{0};
    bool playing{false};
    int playheadRow{0};
    int stepDev{4};
};

class TrackerControl : public ui::Control {
public:
    explicit TrackerControl(std::string id);
    void setState(const TrackerState& state) { state_ = state; }
    void draw(ui::Canvas& canvas) const override;
    // The playhead row advances while the transport runs.
    bool animatingAt(double nowMs) const override {
        (void)nowMs;
        return state_.playing;
    }

private:
    int rowNumber(int visibleRow) const;
    int rowIndex(int visibleRow) const;
    int numTracks() const;
    int firstTrack() const;
    ImU32 color(int index) const;
    bool muted(int track) const;
    void drawBackground(ui::Canvas& canvas) const;
    void drawLines(ui::Canvas& canvas) const;
    void drawRowNumbers(ui::Canvas& canvas) const;
    // Number2Bitmaps: digits advance by the 7px cell with a compressed gap.
    void drawNumbers(ui::Canvas& canvas, int x, int centerY, int number, ImU32 col,
                     ImU32 bg) const;
    // String2Bitmaps: per-char advance is font->width (uniform for Light17),
    // one cell per glyph. Draws a cell string that may outlive param_length.
    void drawCellText(ui::Canvas& canvas, int x, int centerY, const char* text, ImU32 col,
                      ImU32 bg) const;
    void drawSelection(ui::Canvas& canvas) const;
    void drawNotes(ui::Canvas& canvas) const;
    void drawInstruments(ui::Canvas& canvas) const;
    void drawFx(ui::Canvas& canvas, int param, int offset, int slot,
                ImU32 base, ImU32 dim) const;
    void drawTrackNames(ui::Canvas& canvas) const;
    // Flat fill behind a cell: selection/cursor colour or the row's background.
    ImU32 cellBackground(int param, int track, int visibleRow) const;
    ImU32 cellColor(int param, int track, int visibleRow, bool dash, ImU32 base,
                    ImU32 dim) const;

    TrackerState state_{};
    mutable int sel1x_{0}, sel2x_{0}, sel1y_{0}, sel2y_{0};
    mutable bool selActive_{false};
};

} // namespace trakr

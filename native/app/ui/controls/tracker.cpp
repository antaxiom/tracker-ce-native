#include "tracker.h"

#include "Song.h"
#include "ui/canvas.h"
#include "ui/design.h"
#include "views/PatternFxHelper.h"

#include <algorithm>
#include <cstdio>
#include <string>

namespace trakr {
namespace {

constexpr int kRowsVisible = 15;
constexpr int kRowHeight = 26;
constexpr int kTrackWidth = 186;
constexpr int kGutter = 27;
constexpr int kRightGutter = 772;
constexpr int kCursorVisibleRow = 7;

constexpr int kOffset[4] = {8, 52, 86, 140};
constexpr int kSelOffset[4] = {1, 44, 78, 132};
constexpr int kSelWidth[4] = {40, 30, 50, 50};

ImU32 rgb(uint32_t value) {
    return IM_COL32((value >> 16) & 0xFF, (value >> 8) & 0xFF, value & 0xFF, 255);
}

void noteText(int16_t note, char* out) {
    static const char* const kNotes[] = {"C-", "C#", "D-", "D#", "E-", "F-",
                                         "F#", "G-", "G#", "A-", "A#", "B-"};
    if (note < 0) {
        std::snprintf(out, 4, "---");
    } else if (note == kNoteOff) {
        std::snprintf(out, 4, "OFF");
    } else if (note > 119) {
        std::snprintf(out, 4, "???");
    } else {
        const char* n = kNotes[note % 12];
        out[0] = n[0];
        out[1] = n[1];
        out[2] = static_cast<char>('0' + note / 12);
        out[3] = '\0';
    }
}

} // namespace

TrackerControl::TrackerControl(std::string id) : Control(std::move(id)) {}

int TrackerControl::numTracks() const {
    return state_.pattern ? static_cast<int>(state_.pattern->tracks.size()) : 0;
}

int TrackerControl::firstTrack() const {
    const int count = numTracks();
    return std::clamp(state_.cursorTrack - 3, 0, std::max(0, count - 4));
}

int TrackerControl::rowNumber(int visibleRow) const {
    const int length = state_.pattern ? std::max(1, state_.pattern->rows) : 1;
    int row = state_.cursorRow - 6 + visibleRow;
    if (row < 1) row = length + (row % length);
    else if (row > length) row = (row - 1) % length + 1;
    return row;
}

int TrackerControl::rowIndex(int visibleRow) const { return rowNumber(visibleRow) - 1; }

ImU32 TrackerControl::color(int index) const {
    return rgb(ui::kTrackerColors[std::clamp(index, 0, 20)]);
}

bool TrackerControl::muted(int track) const {
    return state_.song && track >= 0 && track < static_cast<int>(state_.song->tracks.size()) &&
           state_.song->tracks[track].mute;
}

void TrackerControl::drawBackground(ui::Canvas& canvas) const {
    canvas.fillRect(ui::RectI{1, 28, 797, 393}, color(1));
    for (int i = 0; i < kRowsVisible; ++i) {
        if ((rowNumber(i) - 1) % state_.stepDev == 0) {
            canvas.fillRect(ui::RectI{1, 28 + i * kRowHeight + 1, 797, 25}, color(11));
        }
    }
    canvas.vGradient(ui::RectI{1, 413, 798, 10}, color(1), IM_COL32(0, 0, 0, 255));
}

void TrackerControl::drawLines(ui::Canvas& canvas) const {
    const ImU32 line = color(15);
    canvas.fillRect(ui::RectI{kGutter, 28, 1, 395}, line);
    canvas.fillRect(ui::RectI{kRightGutter, 28, 1, 395}, line);
    for (int i = 0; i < 4; ++i) {
        canvas.fillRect(ui::RectI{kGutter + kTrackWidth * i, 28, 1, 395}, line);
    }
}

void TrackerControl::drawNumbers(ui::Canvas& canvas, int x, int centerY, int number, ImU32 col,
                                 ImU32 bg) const {
    char buffer[8];
    const int len = std::snprintf(buffer, sizeof(buffer), "%d", number);
    int dx = x;
    for (int k = 0; k < len; ++k) {
        const char one[2] = {buffer[k], '\0'};
        canvas.textVCentered(trakr::hw::FwFont::Light14, dx, centerY, col, one, &bg);
        dx += (k + 1 < len) ? 6 : 7;
    }
}

void TrackerControl::drawCellText(ui::Canvas& canvas, int x, int centerY, const char* text,
                                  ImU32 col, ImU32 bg) const {
    for (const char* p = text; p && *p; ++p) {
        const char ch[2] = {*p, '\0'};
        canvas.textVCentered(trakr::hw::FwFont::Light17, x, centerY, col, ch, &bg);
        x += trakr::hw::fwTextWidth(trakr::hw::FwFont::Light17, ch) + 1;
    }
}

void TrackerControl::drawRowNumbers(ui::Canvas& canvas) const {
    for (int i = 0; i < kRowsVisible; ++i) {
        const int centerY = 28 + i * kRowHeight + 12;
        const ImU32 bg = ((rowNumber(i) - 1) % state_.stepDev == 0) ? color(11) : color(1);
        drawNumbers(canvas, 3, centerY, rowNumber(i), color(16), bg);
        drawNumbers(canvas, 776, centerY, rowNumber(i), color(16), bg);
    }
    for (int i = 1; i < kRowsVisible; ++i) {
        if (rowNumber(i) == 1) {
            canvas.fillRect(ui::RectI{0, 28 + i * kRowHeight, 800, 1}, color(20));
        }
    }
}

ImU32 TrackerControl::cellBackground(int param, int track, int visibleRow) const {
    const int row = rowIndex(visibleRow);
    const bool inSelection =
        state_.hasSelection && param == state_.cursorSubCol &&
        track >= std::min(state_.selStartTrack, state_.selEndTrack) &&
        track <= std::max(state_.selStartTrack, state_.selEndTrack) &&
        row >= std::min(state_.selStartRow, state_.selEndRow) &&
        row <= std::max(state_.selStartRow, state_.selEndRow);
    const bool cursorCell = param == state_.cursorSubCol && track == state_.cursorTrack &&
                            visibleRow == kCursorVisibleRow;
    if (inSelection || cursorCell) return color(state_.recording ? 10 : 17);
    return ((rowNumber(visibleRow) - 1) % state_.stepDev == 0) ? color(11) : color(1);
}

ImU32 TrackerControl::cellColor(int param, int track, int visibleRow, bool dash, ImU32 base,
                                ImU32 dim) const {
    const int row = rowIndex(visibleRow);
    const bool inSelection =
        selActive_ && param == state_.cursorSubCol && track >= std::min(state_.selStartTrack, state_.selEndTrack) &&
        track <= std::max(state_.selStartTrack, state_.selEndTrack) &&
        row >= std::min(state_.selStartRow, state_.selEndRow) &&
        row <= std::max(state_.selStartRow, state_.selEndRow);
    const bool cursorCell = param == state_.cursorSubCol && track == state_.cursorTrack &&
                            visibleRow == kCursorVisibleRow;
    if (inSelection || cursorCell) return color(14);
    if (muted(track)) return color(12);
    return dash ? dim : base;
}

void TrackerControl::drawNotes(ui::Canvas& canvas) const {
    if (!state_.pattern) return;
    const int first = firstTrack();
    for (int i = 0; i < 4; ++i) {
        const int track = first + i;
        if (track >= numTracks()) continue;
        for (int j = 0; j < kRowsVisible; ++j) {
            const Cell& cell = state_.pattern->tracks[track][rowIndex(j)];
            char buffer[4];
            noteText(cell.note, buffer);
            const int x = kGutter + kOffset[0] + kTrackWidth * i;
            const int centerY = 28 + 12 + kRowHeight * j;
            const ImU32 c = cellColor(0, track, j, cell.note < 0, color(2), color(6));
            const ImU32 bg = cellBackground(0, track, j);
            drawCellText(canvas, x, centerY, buffer, c, bg);
        }
    }
}

void TrackerControl::drawInstruments(ui::Canvas& canvas) const {
    if (!state_.pattern) return;
    const int first = firstTrack();
    for (int i = 0; i < 4; ++i) {
        const int track = first + i;
        if (track >= numTracks()) continue;
        for (int j = 0; j < kRowsVisible; ++j) {
            const Cell& cell = state_.pattern->tracks[track][rowIndex(j)];
            char buffer[8];
            if (cell.instrument >= 0) std::snprintf(buffer, sizeof(buffer), "%02d", cell.instrument + 1);
            else std::snprintf(buffer, sizeof(buffer), "--");
            const int x = kGutter + kOffset[1] + kTrackWidth * i;
            const int centerY = 28 + 12 + kRowHeight * j;
            const ImU32 c = cellColor(1, track, j, cell.instrument < 0, color(3), color(7));
            const ImU32 bg = cellBackground(1, track, j);
            drawCellText(canvas, x, centerY, buffer, c, bg);
        }
    }
}

void TrackerControl::drawFx(ui::Canvas& canvas, int param, int offset, int slot, ImU32 base,
                            ImU32 dim) const {
    if (!state_.pattern) return;
    const int first = firstTrack();
    for (int i = 0; i < 4; ++i) {
        const int track = first + i;
        if (track >= numTracks()) continue;
        for (int j = 0; j < kRowsVisible; ++j) {
            const FxCmd& fx = state_.pattern->tracks[track][rowIndex(j)].fx[slot];
            char buffer[8];
            formatFxCell(buffer, sizeof(buffer), fx.type, fx.value);
            const int x = kGutter + offset + kTrackWidth * i;
            const int centerY = 28 + 12 + kRowHeight * j;
            const ImU32 c = cellColor(param, track, j, fx.type == 0, base, dim);
            const ImU32 bg = cellBackground(param, track, j);
            drawCellText(canvas, x, centerY, buffer, c, bg);
        }
    }
}

void TrackerControl::drawTrackNames(ui::Canvas& canvas) const {
    if (!state_.song) return;
    const int first = firstTrack();
    for (int i = 0; i < 4; ++i) {
        const int track = first + i;
        if (track >= numTracks()) continue;
        const std::string name =
            (track < static_cast<int>(state_.song->tracks.size())) ? state_.song->tracks[track].name
                                                                  : std::string();
        int width = canvas.textWidth(trakr::hw::FwFont::Bold17, name.c_str());
        width = std::min(width, 184);
        const ImU32 headerBg = muted(track) ? color(12) : color(19);
        canvas.fillRect(ui::RectI{kGutter + kTrackWidth * i + 1, 28, width + 2, 27}, headerBg);
        canvas.textVCentered(trakr::hw::FwFont::Bold17, kGutter + kTrackWidth * i + 4, 40,
                             color(0), name.c_str(), &headerBg);
    }
}

void TrackerControl::drawSelection(ui::Canvas& canvas) const {
    selActive_ = state_.hasSelection;
    if (selActive_) {
        const int t0 = std::min(state_.selStartTrack, state_.selEndTrack);
        const int t1 = std::max(state_.selStartTrack, state_.selEndTrack);
        const int r0 = std::min(state_.selStartRow, state_.selEndRow);
        const int r1 = std::max(state_.selStartRow, state_.selEndRow);
        const int first = firstTrack();
        sel1x_ = std::max(28, kGutter + (t0 - first) * kTrackWidth + 1);
        sel2x_ = std::min(800 - 31, kGutter + (t1 - first) * kTrackWidth + kTrackWidth - 2);
        sel1y_ = 28 + (r0 - state_.cursorRow + 7) * kRowHeight;
        sel2y_ = 28 + (r1 - state_.cursorRow + 7) * kRowHeight + kRowHeight;
    }

    const int sub = std::clamp(state_.cursorSubCol, 0, 3);
    const ImU32 selectionColor = color(state_.recording ? 10 : 17);

    if (selActive_) {
        const int bottom = std::min(sel2y_, 28 + kRowHeight * kRowsVisible);
        for (int i = 0; i < 8; ++i) {
            const int x = sel1x_ + kSelOffset[sub] + kTrackWidth * i;
            if (x < sel2x_ && x > sel1x_) {
                canvas.fillRect(ui::RectI{x, sel1y_ + 1, kSelWidth[sub],
                                          bottom - (sel1y_ + 1)},
                                selectionColor);
            }
        }
        const int top = std::max(sel1y_, 28);
        const int bottom2 = std::min(sel2y_, 28 + kRowHeight * kRowsVisible);
        canvas.fillRect(ui::RectI{sel1x_, top, sel2x_ - sel1x_, 2}, selectionColor);
        canvas.fillRect(ui::RectI{sel1x_, bottom2 - 2, sel2x_ - sel1x_, 2}, selectionColor);
        canvas.fillRect(ui::RectI{sel1x_, top, 2, bottom2 - top}, selectionColor);
        canvas.fillRect(ui::RectI{sel2x_ - 2, top, 2, bottom2 - top}, selectionColor);
    }

    const int first = firstTrack();
    const int selX = kGutter + (state_.cursorTrack - first) * kTrackWidth + 2;
    const int y1 = 28 + kRowHeight * kCursorVisibleRow;
    const int y2 = y1 + kRowHeight - 1;
    const int ox = selX + kSelOffset[sub];
    const int ow = kSelWidth[sub];
    canvas.fillRect(ui::RectI{ox, y1 + 1, ow - 1, y2 - 1 - (y1 + 1)}, selectionColor);
    if (!state_.hasSelection) {
        canvas.strokeRectCentered(ui::RectI{selX - 1, y1 - 1, 183, kRowHeight + 2},
                                  selectionColor, 1.5f);
    }
}

void TrackerControl::draw(ui::Canvas& canvas) const {
    if (!state_.pattern) return;

    drawBackground(canvas);

    if (state_.playing && state_.playheadRow > state_.cursorRow - 8 &&
        state_.playheadRow < state_.cursorRow + 8) {
        const int row = state_.playheadRow - (state_.cursorRow - 8);
        const int y1 = 28 + kRowHeight * (row - 1);
        const int y2 = 28 + kRowHeight * row;
        canvas.vGradient(ui::RectI{1, y1 - 11, 798, 11}, color(1), color(13));
        canvas.vGradient(ui::RectI{1, y2, 798, 11}, color(13), color(1));
    }

    drawLines(canvas);
    drawRowNumbers(canvas);
    drawSelection(canvas);
    drawNotes(canvas);
    drawInstruments(canvas);
    drawFx(canvas, 2, kOffset[2], 0, color(4), color(8));
    drawFx(canvas, 3, kOffset[3], 1, color(5), color(9));
    drawTrackNames(canvas);
}

} // namespace trakr

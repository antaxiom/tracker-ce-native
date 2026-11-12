#include "PatternFxHelper.h"
#include "StepFx.h"
#include "Command.h"
#include "FontLoader.h"
#include "../kit/Widgets.h"
#include "imgui.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace trakr {

namespace {

// core/source/Themes.cpp themeDefault.patternTrackerColors (21 entries), mirrored
// verbatim. Kept file-local so parallel screen ports do not contend on Theme.cpp.
static const uint32_t kTrackerColors[21] = {
    0x000000, 0x080808, 0x00E8BE, 0xFEF749, 0xBB58F1, 0x57F1FF, 0x003933,
    0x453B1B, 0x321F3A, 0x183744, 0xCD1818, 0x0E0E0E, 0x171717, 0x000000,
    0xFFFFFF, 0x000000, 0x575757, 0x30CB3F, 0x444344, 0x575757, 0xFEF749,
};
// themeDefault.frameColors[2].
static const uint32_t kFrameColors[2] = {0xCD1818, 0x3F3F3F};
// gPatternEdit.cpp framesPlaces[8][4] (bottom label places).
static const int kFramesPlaces[8][4] = {
    {1, 423, 800 / 8 - 1, 55},                 {(800 / 8) + 1, 423, 800 / 8 - 1, 55},
    {(800 / 8) * 2 + 1, 423, 800 / 8 - 1, 55}, {(800 / 8) * 3 + 1, 423, 800 / 8 - 1, 55},
    {(800 / 8) * 4 + 1, 423, 800 / 8 - 1, 55}, {(800 / 8) * 5 + 1, 423, 800 / 8 - 1, 55},
    {(800 / 8) * 6 + 1, 423, 800 / 8 - 1, 55}, {(800 / 8) * 7 + 1, 423, 800 / 8 - 1, 55},
};
// tracker.cpp fourParamsOffset / fourParamsSelOffset / fourParamsSelWidth.
constexpr int kOff[4] = {8, 52, 86, 140};
constexpr int kSelOff[4] = {1, 44, 78, 132};
constexpr int kSelW[4] = {40, 30, 50, 50};

constexpr ImU32 c24(uint32_t c) {
    return IM_COL32((c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF, 0xFF);
}

void noteText(int16_t note, char* out) {
    static const char* kNotes[] = {"C-", "C#", "D-", "D#", "E-", "F-",
                                   "F#", "G-", "G#", "A-", "A#", "B-"};
    if (note < 0) { out[0] = '-'; out[1] = '-'; out[2] = '-'; out[3] = 0; }
    else if (note == kNoteOff) { out[0] = 'O'; out[1] = 'F'; out[2] = 'F'; out[3] = 0; }
    else if (note > 119) { out[0] = '?'; out[1] = '?'; out[2] = '?'; out[3] = 0; }
    else {
        const char* n = kNotes[note % 12];
        out[0] = n[0]; out[1] = n[1]; out[2] = static_cast<char>('0' + note / 12); out[3] = 0;
    }
}

// One composited pass of the firmware cTracker (verticalPatternControl).
struct TrackerPass {
    ImDrawList* dl{};
    ImVec2 o{};
    float sx{1.0f}, sy{1.0f};
    const Song* song{};
    const Pattern* pat{};
    int cursorTrack{0}, cursorRow{0}, subCol{0};
    bool recording{false}, hasSel{false}, playing{false};
    int s0t{0}, s1t{0}, s0r{0}, s1r{0}, playRow{0};
    ImFont* num{}; ImFont* cell{}; ImFont* bold{};
    float numSize{14.0f}, cellSize{17.0f}, boldSize{17.0f};
    int patternLength{1}, actualStep{0}, middle{0}, firstTrack{0};
    int stepDev{4}, columns{4}, rowsN{15}, displayMode{0}, selectedPlace{-1};
    int s1x{0}, s2x{0}, s1y{0}, s2y{0};
    bool selActive{false};

    float px(float x) const { return o.x + x * sx; }
    float py(float y) const { return o.y + (y - 28) * sy; }
    ImU32 col(int i) const { return c24(kTrackerColors[i]); }
    bool muted(int track) const {
        return track < static_cast<int>(song->tracks.size()) && song->tracks[track].mute;
    }
    void fill(float x, float y, float w, float h, ImU32 c) const {
        dl->AddRectFilled(ImVec2(px(x), py(y)), ImVec2(px(x + w), py(y + h)), c);
    }
    void text(float x, float yc, ImFont* f, float size, ImU32 c, const char* s) const {
        if (!s || !*s) return;
        const ImVec2 ts = f->CalcTextSizeA(size, FLT_MAX, 0.0f, s);
        dl->AddText(f, size, ImVec2(px(x), py(yc) - ts.y * 0.5f), c, s);
    }
    void vgrad(int x, int w, int y0, int y1, ImU32 c0, ImU32 c1) const {
        dl->AddRectFilledMultiColor(ImVec2(px(x), py(y0)), ImVec2(px(x + w), py(y1)),
                                    c0, c0, c1, c1);
    }
    // cTracker::update(): rowNumbers[i] = actualStep - 6 + i, wrapped into
    // 1..patternLength. The tracker is 1-based on screen; the cursor sits on
    // visible row 7, which is row actualStep + 1.
    int rowNumber(int i) const {
        int r = actualStep - 6 + i;
        if (r < 1) r = patternLength + (r % patternLength);
        else if (r > patternLength) r = (r - 1) % patternLength + 1;
        return r;
    }
    int rowIndex(int i) const { return rowNumber(i) - 1; }
    int numTracks() const { return static_cast<int>(pat->tracks.size()); }

    // cTracker::backgroundDivider(): body background + beat divider + shadow.
    void backgroundDivider() {
        fill(1, 28, 797, 393, col(1));
        if (patternLength > 1) {
            for (int i = 0; i < rowsN; ++i) {
                if ((rowNumber(i) - 1) % stepDev == 0) fill(1, 28 + i * 26 + 1, 797, 25, col(11));
            }
        }
        vgrad(1, 798, 413, 423, col(1), IM_COL32(0, 0, 0, 0));
    }

    // cTracker::calculateSelection().
    void calculateSelection() {
        selActive = false;
        if (!hasSel) return;
        selActive = true;
        const int t0 = std::min(s0t, s1t), t1 = std::max(s0t, s1t);
        const int r0 = std::min(s0r, s1r), r1 = std::max(s0r, s1r);
        s1x = 27 + (t0 - firstTrack) * 186 + 1;
        if (s1x < 28) s1x = 28;
        s2x = 27 + (t1 - firstTrack) * 186 + 186 - 2;
        if (s2x > 800 - 27 - 4) s2x = 800 - 27 - 4;
        s1y = 28 + (r0 - actualStep + 7) * 26;
        s2y = 28 + (r1 - actualStep + 7) * 26 + 26;
    }

    // cTracker::playHead() with controlStyleGradient.
    void playHead() const {
        if (!playing || !(playRow > actualStep - 8 && playRow < actualStep + 8)) return;
        const int row = playRow - (actualStep - 8);
        const int y1 = 28 + 26 * (row - 1), y2 = 28 + 26 * row;
        vgrad(1, 798, y1 - 11, y1, col(1), col(13));
        vgrad(1, 798, y2, y2 + 11, col(13), col(1));
    }

    // cTracker::lines(): column separators.
    void lines() const {
        const ImU32 c = col(15);
        dl->AddLine(ImVec2(px(27), py(28)), ImVec2(px(27), py(28 + 390)), c, 1.0f);
        dl->AddLine(ImVec2(px(772), py(28)), ImVec2(px(772), py(28 + 390)), c, 1.0f);
        for (int i = 0; i < columns; ++i) {
            const float x = 27 + 186 * i;
            dl->AddLine(ImVec2(px(x), py(28)), ImVec2(px(x), py(28 + 395)), c, 1.0f);
        }
    }

    // cTracker::drawRowNumbers().
    void drawRowNumbers() const {
        char b[8];
        for (int i = 0; i < rowsN; ++i) {
            std::snprintf(b, sizeof(b), "%d", rowNumber(i));
            text(3, 28 + i * 26 + 12, num, numSize, col(16), b);
            text(776, 28 + i * 26 + 12, num, numSize, col(16), b);
        }
        // Pattern end/start line (cTracker::drawRowNumbers, colors[20]) is a
        // hard 1px line, so fill a row instead of using antialiased AddLine.
        for (int i = 1; i < rowsN; ++i) {
            if (rowNumber(i) != 1) continue;
            const float y = 28 + i * 26;
            dl->AddRectFilled(ImVec2(px(0), py(y)), ImVec2(px(800), py(y + 1)), col(20));
        }
    }

    // cTracker::selection(): selection blocks, border and current-step cursor.
    void selection() const {
        // cTracker::selection(): selection_color = colors[10] (edit mode /
        // recording), colors[17] (green, focused), colors[18] (unfocused).
        const ImU32 sc = col(recording ? 10 : 17);
        const float th = std::max(1.0f, std::round(1.5f * sy));
        if (hasSel) {
            int y2 = std::min(s2y, 28 + 26 * 15);
            for (int i = 0; i < 8; ++i) {
                const int x = s1x + kSelOff[subCol] + 186 * i;
                if (x < s2x && x > s1x)
                    fill(x, s1y + 1, kSelW[subCol], y2 - (s1y + 1), sc);
            }
            int y1 = s1y, ys2 = s2y;
            if (y1 >= 28) dl->AddLine(ImVec2(px(s1x), py(y1)), ImVec2(px(s2x), py(y1)), sc, th);
            else y1 = 28;
            if (ys2 <= 28 + 26 * 15) dl->AddLine(ImVec2(px(s2x), py(ys2)), ImVec2(px(s1x), py(ys2)), sc, th);
            else ys2 = 28 + 26 * 15;
            if (s2x <= 27 + 186 * 4) dl->AddLine(ImVec2(px(s2x), py(y1)), ImVec2(px(s2x), py(ys2)), sc, th);
            if (s1x >= 27) dl->AddLine(ImVec2(px(s1x), py(ys2)), ImVec2(px(s1x), py(y1)), sc, th);
        }
        const int selX = 27 + (cursorTrack - firstTrack) * 186 + 2;
        const int y1 = 28 + 26 * 7, y2 = y1 + 26 - 1;
        const int ox = selX + kSelOff[subCol], ow = kSelW[subCol];
        fill(ox, y1 + 1, ow - 1, y2 - 1 - (y1 + 1), sc);
        // vertices (select_x, select1_y)..(select_x+181, select2_y) with 1.5px
        // line width; the outline straddles the 1-based cell edge.
        if (!hasSel)
            dl->AddRect(ImVec2(px(selX - 1), py(y1 - 1)), ImVec2(px(selX + 182), py(y2 + 1)),
                        sc, 0.0f, 0, th);
    }

    ImU32 cellColor(int param, int i, int j, float x, float y, int track, bool dash,
                    ImU32 base, ImU32 dim) const {
        const int mark = (subCol == param) ? cursorTrack - firstTrack : -1;
        if (selActive && mark >= 0 && x > s1x && x < s2x && y > s1y && y < s2y) return col(14);
        if (j == 7 && i == mark) return col(14);
        if (muted(track)) return col(12);
        return dash ? dim : base;
    }

    // cTracker::update(): background, selection geometry, playhead, lines,
    // row numbers, selection.
    void update() {
        backgroundDivider();
        calculateSelection();
        playHead();
        lines();
        drawRowNumbers();
        selection();
    }
    // cTracker::notes().
    void notes() const {
        for (int i = 0; i < columns; ++i) {
            const int track = firstTrack + i;
            if (track >= numTracks()) continue;
            for (int j = 0; j < rowsN; ++j) {
                const int r = rowIndex(j);
                const Cell& c = pat->tracks[track][r];
                char b[4];
                noteText(c.note, b);
                const float x = 27 + kOff[0] + 186 * i, y = 28 + 12 + 26 * j;
                text(x, y, cell, cellSize, cellColor(0, i, j, x, y, track, c.note < 0, col(2), col(6)), b);
            }
        }
    }
    // cTracker::instruments().
    void instruments() const {
        for (int i = 0; i < columns; ++i) {
            const int track = firstTrack + i;
            if (track >= numTracks()) continue;
            for (int j = 0; j < rowsN; ++j) {
                const int r = rowIndex(j);
                const Cell& c = pat->tracks[track][r];
                char b[8] = "--";
                if (c.instrument >= 0) std::snprintf(b, sizeof(b), "%02d", c.instrument + 1);
                const float x = 27 + kOff[1] + 186 * i, y = 28 + 12 + 26 * j;
                text(x, y, cell, cellSize, cellColor(1, i, j, x, y, track, c.instrument < 0, col(3), col(7)), b);
            }
        }
    }
    // cTracker::fxes1() / fxes2().
    void fxColumn(int param, int off, int slot, ImU32 base, ImU32 dim) const {
        for (int i = 0; i < columns; ++i) {
            const int track = firstTrack + i;
            if (track >= numTracks()) continue;
            for (int j = 0; j < rowsN; ++j) {
                const int r = rowIndex(j);
                const FxCmd& f = pat->tracks[track][r].fx[slot];
                char b[8];
                formatFxCell(b, sizeof(b), f.type, f.value);
                const float x = 27 + off + 186 * i, y = 28 + 12 + 26 * j;
                text(x, y, cell, cellSize, cellColor(param, i, j, x, y, track, f.type == 0, base, dim), b);
            }
        }
    }
    // cTracker::tracksNumbers().
    void tracksNumbers() const {
        for (int i = 0; i < columns; ++i) {
            const int track = firstTrack + i;
            if (track >= numTracks()) continue;
            const char* name = (track < static_cast<int>(song->tracks.size()))
                                 ? song->tracks[track].name.c_str() : "";
            const ImVec2 ts = bold->CalcTextSizeA(boldSize, FLT_MAX, 0.0f, name);
            float dw = ts.x / std::max(1.0f, sx);
            dw = std::min(dw, 184.0f);
            fill(27 + 186 * i + 1, 28, dw + 2, 27, muted(track) ? col(12) : col(19));
            text(27 + 186 * i + 4, 40, bold, boldSize, col(0), name);
        }
    }
    void update2() const { if (!displayMode || (displayMode & 1)) notes(); }
    void update3() const { if (!displayMode || (displayMode & 2)) instruments(); }
    void update4() const { if (!displayMode || (displayMode & 4)) fxColumn(2, kOff[2], 0, col(4), col(8)); }
    void update5() const {
        if (!displayMode || (displayMode & 8)) fxColumn(3, kOff[3], 1, col(5), col(9));
        tracksNumbers();
    }

    void verticalPatternControl() { update(); update2(); update3(); update4(); update5(); }
    void horizontalPatternControl() const {} // cHorizontalTracker: unused arrangement
    // Popup controls (fill / note / instrument) are not exposed by the app screen.
    void patternPopupLabel() const {}
    void patternPopupTitleLabel() const {}
    void val1PopupBar() const {}
    void val2PopupBar() const {}
    void val3PopupBar() const {}
    void valCancelEmptyPopupBar() const {}
    void valFillEmptyPopupBar() const {}
    void intstrumentsFromPopupListControl() const {}
    void intstrumentsToPopupListControl() const {}
    void param1PopupListControl() const {}
    void param2PopupListControl() const {}
    void param3PopupListControl() const {}
    // Bottom label chrome is owned by the app ButtonDeck layer.
    void labels() const {}
    void bgLabel() const {}
    // cFrame::update(): selected bottom label place (hidden when none held).
    void frameControl() const {
        if (selectedPlace < 0 || selectedPlace > 7) return;
        const int* p = kFramesPlaces[selectedPlace];
        dl->AddRect(ImVec2(px(p[0]), py(p[1])), ImVec2(px(p[0] + p[2]), py(p[1] + p[3])),
                    c24(kFrameColors[0]), 0.0f, 0, std::max(1.0f, std::round(1.5f * sy)));
    }

    // cPatternEditor::showDefaultScreen(): composite in reverse creation order.
    void showDefaultScreen() {
        horizontalPatternControl();
        verticalPatternControl();
        patternPopupLabel();
        patternPopupTitleLabel();
        valFillEmptyPopupBar();
        valCancelEmptyPopupBar();
        val3PopupBar();
        val2PopupBar();
        val1PopupBar();
        intstrumentsToPopupListControl();
        intstrumentsFromPopupListControl();
        param3PopupListControl();
        param2PopupListControl();
        param1PopupListControl();
        bgLabel();
        labels();
        frameControl();
    }
};

} // namespace

void renderPatternScreen(ImDrawList* dl, const ImVec2& origin, float sx, float sy,
                         const PatternRenderParams& p) {
    TrackerPass t;
    t.dl = dl;
    t.o = origin;
    t.sx = sx;
    t.sy = sy;
    t.song = p.song;
    t.pat = p.pat;
    t.cursorTrack = p.cursorTrack;
    t.cursorRow = p.cursorRow;
    t.subCol = std::clamp(p.cursorSubCol, 0, 3);
    t.recording = p.recording;
    t.hasSel = p.hasSelection;
    t.s0t = p.selStartTrack; t.s1t = p.selEndTrack;
    t.s0r = p.selStartRow; t.s1r = p.selEndRow;
    t.playing = p.playing;
    t.playRow = p.playheadRow;
    t.num = getMonoLightFont();
    t.cell = getMonoLightFont();
    t.bold = getMonoBoldFont();
    // Calibrated so the TTF cell advances match the firmware L4 cells:
    // row numbers 8px, tracker cells 11px, track names 10px per character.
    t.numSize = 17.0f * sy;
    t.cellSize = 23.4f * sy;
    t.boldSize = 21.25f * sy;
    t.patternLength = std::max(1, p.pat->rows);
    t.actualStep = p.cursorRow;
    t.middle = p.cursorRow;
    t.firstTrack = std::clamp(p.cursorTrack - 3, 0,
                              std::max(0, static_cast<int>(p.pat->tracks.size()) - 4));
    t.showDefaultScreen();
}

} // namespace trakr


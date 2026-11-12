#include "PerformView.h"
#include "FontLoader.h"
#include "BitmapText.h"
#include "imgui.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

// Structural port of the Perform screen editor:
//   core/interface/performanceMode/gPerformance.cpp  (initDisplayControls)
//   core/libs/NewDisplay/multiLabel.cpp              (cMultiLabel)
//   core/libs/NewDisplay/label.cpp                   (cLabel)
//   core/libs/NewDisplay/frame.cpp                   (cFrame)
//   core/libs/NewDisplay/bgLabel.cpp                 (cBgLabel)
// Creation order in gPerformance.cpp: frameControl, label[0..7], bgLabel, then
// for each column i: textLabel[i], value1Label[i]. NewDisplay composites in
// REVERSE creation order, so this body draws value1Label/textLabel from column
// 11 down to 0, then bgLabel/labels (app ButtonDeck chrome), then frameControl.
// The title bar and the eight bottom track labels are app-layer chrome.

namespace trakr {
namespace {

constexpr int kChromeHeight = 28;       // app header; firmware body starts below
constexpr int kRedFrameWidth = 24;      // config.h; 24/16 device px
constexpr int kColumnCount = 12;
constexpr int kColumnX = 42;            // gPerformance.cpp: prop.x = 42 + 65*i
constexpr int kColumnY = 34;            // prop.y
constexpr int kColumnW = 62;            // prop.w
constexpr int kColumnH = 174 + 210 + 5; // prop.h = 389
constexpr int kColumnStride = 65;
constexpr int kFxTextHole = 210;        // multiLabelData[i].yForFxTextHole
constexpr int kTextLines = 5;           // multiLabelData[i].textLinesCount
constexpr int kTextYOffset = 16;        // multiLabelData[i].yOffset
constexpr int kTextGlyphHeight = 22;    // fonts[1] RobotoMono-Light_17 cell height
constexpr int kTextAdvance = 10;        // fonts[1] glyph advance
// multiLabel.cpp: lines_space = (text_h - font->height * lines) / (lines + 1).
constexpr int kTextLinesSpace = (kColumnH - kTextYOffset - kFxTextHole -
                                 kTextGlyphHeight * kTextLines) / (kTextLines + 1);
constexpr int kTextLineStep = kTextLinesSpace + kTextGlyphHeight; // 8 + 22
constexpr int kFxNameY = 210 + 34;      // textLabel[i] posY

// gPerformance.cpp framesPlaces[12][4] verbatim.
constexpr int kFramePlaces[kColumnCount][4] = {
    {11, 31, 62, 391},          {11 + 65 * 1, 31, 62, 391},  {11 + 65 * 2, 31, 62, 391},
    {11 + 65 * 3, 31, 62, 391}, {11 + 65 * 4, 31, 62, 391},  {11 + 65 * 5, 31, 62, 391},
    {11 + 65 * 6, 31, 62, 391}, {11 + 65 * 7, 31, 62, 391},  {11 + 65 * 8, 31, 62, 391},
    {11 + 65 * 9, 31, 62, 391}, {11 + 65 * 10, 31, 62, 391}, {11 + 65 * 11, 31, 62, 391},
};

// Perform-screen colors mirrored verbatim from core/source/Themes.cpp
// (themeDefault). Kept file-local so parallel screen ports do not contend on
// native/app/Theme.cpp; consolidation into Theme is deferred.
static const ImU32 kPerfFxNameTextLabelColors[4] = {0xFFFFFF, 0x121212, 0xFF0000, 0xFFFFFF};
static const ImU32 kPerfValueNormalMLabelColors[3] = {0xFFFFFF, 0x0A0A0A, 0x30CB3F};
static const ImU32 kPerfValueEditMLabelColors[3] = {0xFFFFFF, 0x0A0A0A, 0xCD1818};
static const ImU32 kFrameColors[2] = {0xCD1818, 0x3F3F3F};
[[maybe_unused]] static const ImU32 kGreenFrameColors[2] = {0x30CB3F, 0x3F3F3F};
static const ImU32 kBackgroundColor = 0x000000;

constexpr ImU32 rgba(ImU32 c) {
    return IM_COL32((c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF, 0xFF);
}

// One pass of the firmware performance controls. Coordinates stay in firmware
// device pixels; px()/py() only map them into the app workspace.
struct PerformControls {
    ImDrawList* dl{};
    ImVec2 origin{};
    float sx{1.0f}, sy{1.0f};
    std::array<PerformSlot12, kColumnCount>* slots{};
    int performanceEditPlace{-1};
    bool editing{false};
    int clickedTop{-1}, clickedRowCol{-1}, clickedRow{-1};
    ImFont* font{};
    float fontSize{22.0f};

    float px(float x) const { return origin.x + x * sx; }
    float py(float y) const { return origin.y + (y - kChromeHeight) * sy; }
    float stroke() const { return std::max(1.0f, std::round(kRedFrameWidth / 16.0f * sy)); }

    void fill(float x, float y, float w, float h, ImU32 c) const {
        dl->AddRectFilled(ImVec2(px(x), py(y)), ImVec2(px(x + w), py(y + h)), c);
    }
    bool hit(const char* id, float x, float y, float w, float h) const {
        ImGui::SetCursorScreenPos(ImVec2(px(x), py(y)));
        return ImGui::InvisibleButton(id, ImVec2(w * sx, h * sy));
    }
    // multiLabel.cpp controlStyleFont2 text ("17 Regular" = Light17 L4):
    // fixed-width cells, OPT_CENTERX.
    void textCentered(float centerX, float yTop, ImU32 c, const char* s) const {
        if (!s || !*s) return;
        const float w = hw::fwTextWidth(hw::FwFont::Light17, s) * sx;
        hw::fwDrawText(dl, hw::FwFont::Light17, px(centerX) - w * 0.5f, py(yTop), s, c, sx, sy);
    }
    // label.cpp controlStyleVerticalText: 270-degree text reading upwards.
    void verticalText270(float centerX, float bottomY, ImU32 c, const char* s) const {
        if (!font || !s) return;
        const float step = kTextAdvance * sy;
        const float xOffset = fontSize * 0.5f + 0.5f * sy;
        float curY = bottomY;
        for (const char* p = s; *p; ++p) {
            if (*p != ' ') {
                char ch[2] = {*p, '\0'};
                const int v0 = dl->VtxBuffer.Size;
                dl->PushClipRectFullScreen();
                dl->AddText(font, fontSize, ImVec2(0.0f, 0.0f), c, ch);
                dl->PopClipRect();
                for (int i = v0; i < dl->VtxBuffer.Size; ++i) {
                    const float rx = dl->VtxBuffer[i].pos.x;
                    const float ry = dl->VtxBuffer[i].pos.y;
                    dl->VtxBuffer[i].pos.x = (centerX - xOffset) + ry;
                    dl->VtxBuffer[i].pos.y = curY - rx;
                }
            }
            curY -= step;
        }
    }

    // cMultiLabel::update() for value1Label[place].
    void value1Label(int place) {
        const PerformSlot12& slot = (*slots)[place];
        const ImU32* colors = editing ? kPerfValueEditMLabelColors : kPerfValueNormalMLabelColors;
        const float cx = kColumnX + kColumnStride * place;
        const float bx = cx - kColumnW * 0.5f; // controlStyleCenterX
        const int ccH = (kColumnH - kFxTextHole) / 5;       // 35
        const int perfH = ((kColumnH - kFxTextHole) * 4) / 5; // 143
        const int perfY = kColumnY + ccH + kFxTextHole;     // 279
        const ImU32 bg = rgba(colors[1]);
        fill(bx, kColumnY, kColumnW, ccH, bg);
        fill(bx, perfY, kColumnW, perfH, bg);
        // controlStyleBottomShadow: colors[1] -> transparent over 12px at the bottom.
        const int gradY = kColumnY + kColumnH - 10;
        const ImU32 clear = bg & 0x00FFFFFFu;
        dl->AddRectFilledMultiColor(ImVec2(px(bx), py(gradY)), ImVec2(px(bx + kColumnW), py(gradY + 12)),
                                    bg, bg, clear, clear);
        // textLinesCount-1 is the top cell; textYOffset/2 is its half offset.
        // cMultiLabel top cell (line 0) is the current CC value; the reference
        // state has no live CC, so it renders "---" (fxValues[slot][4]). The
        // four option labels live in the preset rows, and selectedRows = 1.
        textCentered(cx, kColumnY + kTextYOffset / 2, rgba(colors[0]), "---");
        // Preset rows: text_y = border_y + ccRectHeight + yForFxTextHole + yOffset/2.
        const int lineY = kColumnY + ccH + kFxTextHole + kTextYOffset / 2; // 287
        for (int r = 0; r < 4; ++r) {
            const ImU32 c = (r == slot.activeRow) ? rgba(colors[2]) : rgba(colors[0]);
            textCentered(cx, lineY + r * kTextLineStep, c, slot.labels[r].c_str());
        }
        char id[40];
        std::snprintf(id, sizeof(id), "##perform_value_top_%d", place);
        if (hit(id, bx, kColumnY, kColumnW, ccH)) clickedTop = place;
        for (int r = 0; r < 4; ++r) {
            std::snprintf(id, sizeof(id), "##perform_value_row_%d_%d", place, r);
            if (hit(id, bx, perfY + r * 30, kColumnW, 30)) { clickedRowCol = place; clickedRow = r; }
        }
    }

    // cLabel::update() with controlStyleVerticalText for textLabel[place].
    void textLabel(int place) const {
        const float cx = kColumnX + kColumnStride * place;
        verticalText270(px(cx), py(kFxNameY), rgba(kPerfFxNameTextLabelColors[0]),
                        (*slots)[place].name.c_str());
    }

    // cLabel update(): bottom label[0..7] are app ButtonDeck chrome.
    void labels() const {}
    // cBgLabel::update(): bottom background is app ButtonDeck chrome.
    void bgLabel() const {}
    // cFrame::update(): selected edit place, frameColors[0].
    // (greenFrameColors is TRACKER_MINI-only; desktop uses frameColors.)
    void frameControl(int place) const {
        const int* p = kFramePlaces[std::clamp(place, 0, kColumnCount - 1)];
        dl->AddRect(ImVec2(px(p[0]), py(p[1])),
                    ImVec2(px(p[0] + p[2]), py(p[1] + p[3])),
                    rgba(kFrameColors[0]), 0.0f, 0, stroke());
    }

    // gPerformance.cpp: showPerformaceValue(place).
    void showPerformaceValue(int place) { value1Label(place); }
    // gPerformance.cpp: showFxNames(place) (single place path).
    void showFxNames(int place) const { textLabel(place); }
    // gPerformance.cpp: showTracksPatterns()/showTracksState() text2 line.
    void showTracksPatterns() const {}
    void showTracksState() const {}
    void showMuteButtons() const {}
    void showEditFrame(int place) const { frameControl(place); }
    void hideEditFrame(int place) const { (void)place; } // frame is not drawn

    // gPerformance.cpp: showPerformanceFxes(), composited in reverse creation order.
    void showPerformanceFxes() {
        fill(0, kChromeHeight, 800, 397, rgba(kBackgroundColor)); // Themes.cpp backgroundColor
        for (int i = kColumnCount - 1; i >= 0; --i) {
            showPerformaceValue(i); // value1Label[i] (cMultiLabel)
            showFxNames(i);         // textLabel[i] (cLabel, vertical)
        }
        showTracksPatterns();
        showTracksState();
        bgLabel();
        labels();
        if (performanceEditPlace >= 0) showEditFrame(performanceEditPlace);
        else hideEditFrame(performanceEditPlace);
    }
};

} // namespace

PerformView::PerformView() {
    slots_[0]  = {"Tune",              {"---", "+2", "-2", "-3"},       {0.0f, 2.0f, -2.0f, -3.0f},   0};
    slots_[1]  = {"Delay send",        {"---", "+94", "+60", "-100"},   {0.0f, 0.94f, 0.60f, -1.0f},  0};
    slots_[2]  = {"Reverb send",       {"---", "+98", "+50", "-100"},   {0.0f, 0.98f, 0.50f, -1.0f},  0};
    slots_[3]  = {"Low-pass cutoff",   {"---", "-22", "-51", "-62"},    {1.0f, 0.78f, 0.49f, 0.38f},  0};
    slots_[4]  = {"High-Pass cutoff",  {"---", "-25", "-44", "-61"},    {0.0f, 0.25f, 0.44f, 0.61f},  0};
    slots_[5]  = {"Sample end",        {"---", "-60", "-82", "-95"},    {1.0f, 0.40f, 0.18f, 0.05f},  0};
    slots_[6]  = {"Panning LFO Speed", {"---", "+20", "-12", "+8"},     {0.0f, 2.0f, 0.5f, 1.5f},     0};
    slots_[7]  = {"Pattern play mode", {"Fwd", "Back", "Rnd", "C19"},   {0.0f, 1.0f, 2.0f, 3.0f},     0};
    slots_[8]  = {"Volume LFO Speed",  {"---", "+23", "+20", "+18"},    {0.0f, 2.3f, 2.0f, 1.8f},     0};
    slots_[9]  = {"Step repeater",     {"Off", "1/2", "1/4", "1/6"},    {0.0f, 2.0f, 4.0f, 6.0f},     0};
    slots_[10] = {"Sample playback",   {">>>", "<<<", "<<<", "<<<"},    {0.0f, 1.0f, 2.0f, 3.0f},     0};
    slots_[11] = {"Bit depth",         {"Off", "1", "2", "3"},          {0.0f, 1.0f, 2.0f, 3.0f},     0};
}

void PerformView::draw(bool* p_open) { (void)p_open; }

void PerformView::triggerSlotPad(int col, int row, EngineHost& engineHost) {
    if (col < 0 || col >= 12 || row < 0 || row >= 4) return;
    slots_[col].activeRow = (slots_[col].activeRow == row && row > 0) ? 0 : row;
    applyToEngine(engineHost);
}

void PerformView::selectTop(int place, EngineHost& engineHost) {
    if (place < 0 || place >= 12) return;
    slots_[place].activeRow = 0;
    performanceEditPlace_ = (performanceEditPlace_ == place) ? -1 : place;
    applyToEngine(engineHost);
}

void PerformView::selectRow(int place, int row, EngineHost& engineHost) {
    if (place < 0 || place >= 12) return;
    performanceEditPlace_ = place;
    triggerSlotPad(place, row, engineHost);
}

void PerformView::clearAllFx(EngineHost& engineHost) {
    for (auto& s : slots_) s.activeRow = 0;
    applyToEngine(engineHost);
}

void PerformView::applyToEngine(EngineHost& engineHost) {
    PerformanceState ps;
    ps.pitchShiftSemitones = slots_[0].values[slots_[0].activeRow];
    ps.delaySendBoost = slots_[1].values[slots_[1].activeRow];
    ps.reverbSendBoost = slots_[2].values[slots_[2].activeRow];
    ps.lowpassCutoffMultiplier = slots_[3].values[slots_[3].activeRow];
    ps.highpassCutoffHz = slots_[4].values[slots_[4].activeRow] * 2000.0f;
    ps.playMode = static_cast<int>(slots_[7].values[slots_[7].activeRow]);
    ps.repeatRoll = static_cast<int>(slots_[9].values[slots_[9].activeRow]);
    engineHost.setPerformanceState(ps);
}

void PerformView::update(Song& song, History& history, EngineHost& engineHost) {
    (void)song; (void)history;
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                                   ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    if (!ImGui::Begin("Performance Mode", nullptr, flags)) { ImGui::End(); ImGui::PopStyleVar(); return; }

    const ImVec2 avail = ImGui::GetContentRegionAvail();
    ImFont* font = getMonoBoldFont();
    if (avail.x < 100.0f || avail.y < 100.0f || !font) {
        ImGui::End(); ImGui::PopStyleVar(); return;
    }
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    PerformControls c;
    c.dl = ImGui::GetWindowDrawList();
    c.origin = origin;
    c.sx = avail.x / 800.0f;
    c.sy = avail.y / 397.0f;
    c.slots = &slots_;
    c.performanceEditPlace = performanceEditPlace_;
    c.editing = performanceEditPlace_ >= 0;
    c.font = font;
    c.fontSize = font->LegacySize * c.sy; // controlStyleFont2

    c.showPerformanceFxes();

    if (c.clickedTop >= 0) {
        slots_[c.clickedTop].activeRow = 0;
        performanceEditPlace_ = (performanceEditPlace_ == c.clickedTop) ? -1 : c.clickedTop;
        applyToEngine(engineHost);
    }
    if (c.clickedRowCol >= 0) {
        performanceEditPlace_ = c.clickedRowCol;
        triggerSlotPad(c.clickedRowCol, c.clickedRow, engineHost);
    }
    ImGui::SetCursorScreenPos(origin);
    ImGui::End();
    ImGui::PopStyleVar();
}

} // namespace trakr

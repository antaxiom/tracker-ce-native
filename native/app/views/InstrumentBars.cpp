#include "InstrumentBars.h"
#include "../FontLoader.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

// Polyend Tracker Instrument screen, structural port of:
//   core/interface/instrumentEditor/gInstrEdit.cpp   (initDisplayControls, show*)
//   core/libs/NewDisplay/bar.cpp                     (cBar)
//   core/libs/NewDisplay/list.cpp                    (cList)
//   core/libs/NewDisplay/label.cpp                   (cLabel)
//   core/libs/NewDisplay/frame.cpp                   (cFrame)
//   core/libs/NewDisplay/bgLabel.cpp                 (cBgLabel)
// Creation order in gInstrEdit.cpp: frameControl, doubleLabelAutomation,
// {label[i], barControl[i]} for i=0..7, bgLabel, filterModeListControl,
// list1..list4, instrumentSynthCCListControl, instrumentListControl,
// paramsListControl, modelListControl, imgGridControl, ccNameListControl[6],
// noteLabel, blankSpaceCover[0..1]. NewDisplay composites controls in REVERSE
// creation order; the page bodies below follow that order. label[]/bgLabel are
// the app ButtonDeck bottom chrome, so labels()/bgLabel() are intentionally no-ops.

namespace trakr {
namespace {

// --- Firmware palette, mirrored verbatim from core/source/Themes.cpp --------
// themeDefault; kept file-local so this port does not touch native/app/Theme.h.
constexpr uint32_t kWhite = 0xFFFFFF;
constexpr uint32_t kBlack = 0x000000;
constexpr uint32_t kRed = 0xFF0000;
constexpr uint32_t kGreen = 0x00FF00;
constexpr uint32_t kOneTrueRed = 0xCD1818;
constexpr uint32_t kGrey08 = 0x080808;
constexpr uint32_t kGrey0A = 0x0A0A0A;
constexpr uint32_t kGrey22 = 0x222222;
constexpr uint32_t kGrey32 = 0x323132;
constexpr uint32_t kGrey33 = 0x333333;
constexpr uint32_t kGrey3F = 0x3F3F3F;
constexpr uint32_t kGrey57 = 0x575757;
constexpr uint32_t kGrey77 = 0x777777;

static const uint32_t kActiveBarColors[5] = {kWhite, kRed, kGreen, kGrey08, kGrey0A};
static const uint32_t kInactiveBarColors[5] = {kGrey22, kRed, kGreen, kGrey08, kGrey0A};
static const uint32_t kActiveListColors[7] = {kWhite, kWhite, kGrey57, kGrey0A,
                                              kWhite, kWhite, kBlack};
static const uint32_t kFrameColors[2] = {kOneTrueRed, kGrey3F};
static const uint32_t kBlankSpaceCoverLabelColors[4] = {kWhite, kGrey0A, kRed, kBlack};
[[maybe_unused]] static const uint32_t kActiveLabelsColors[4] = {kGrey77, kGrey32,
                                                                 kOneTrueRed, kWhite};
[[maybe_unused]] static const uint32_t kInactiveLabelsColors[4] = {kGrey33, kGrey32,
                                                                   kOneTrueRed, kGrey57};
[[maybe_unused]] static const uint32_t kActiveButtonLabelsColors[4] = {kGrey77, kGrey32,
                                                                       kOneTrueRed, kGrey77};
[[maybe_unused]] static const uint32_t kMoreButtonColors[4] = {kGrey77, kGrey22, kGrey77,
                                                               kGrey22};

// --- Firmware geometry (device pixels, 800x480) -----------------------------
constexpr int kChromeHeight = 28; // app header; firmware body begins at y=28
constexpr int kBarY = 29;
constexpr int kBarW = 800 / 8 - 3; // 97
constexpr int kBarH = 394;
constexpr int kRowH = 27;
constexpr int kRedFrameWidth = 24; // config.h; 1.5 device px

// gInstrEdit.cpp framesPlaces[8][4].
constexpr int kFramesPlaces[8][4] = {
    {1, 31, kBarW, 391},       {101, 31, kBarW, 391},    {201, 31, kBarW, 391},
    {301, 31, kBarW, 391},     {401, 31, kBarW, 391},    {501, 31, kBarW, 391},
    {601, 31, kBarW, 391},     {701, 31, kBarW, 391},
};
// framesPlacesAutomation[7][4]: destination spans two columns.
constexpr int kFramesPlacesAutomation[7][4] = {
    {1, 31, 2 * (800 / 8) - 3, 391}, {201, 31, kBarW, 391}, {301, 31, kBarW, 391},
    {401, 31, kBarW, 391},           {501, 31, kBarW, 391}, {601, 31, kBarW, 391},
    {701, 31, kBarW, 391},
};

ImU32 rgb(uint32_t c) { return IM_COL32((c >> 16) & 255, (c >> 8) & 255, c & 255, 255); }

struct Controls {
    InstrumentScreenState& s;

    float px(float x) const { return s.origin.x + x * s.sx; }
    float py(float y) const { return s.origin.y + (y - kChromeHeight) * s.sy; }
    float stroke() const { return std::max(1.0f, std::round(kRedFrameWidth / 16.0f * s.sy)); }

    void fill(float x, float y, float w, float h, ImU32 c) const {
        s.dl->AddRectFilled(ImVec2(px(x), py(y)), ImVec2(px(x + w), py(y + h)), c);
    }
    void outline(float x, float y, float w, float h, ImU32 c) const {
        s.dl->AddRect(ImVec2(px(x), py(y)), ImVec2(px(x + w), py(y + h)), c, 0.0f, 0, stroke());
    }
    void shadow(float x, float y, float w, float h, ImU32 bg) const {
        const ImU32 clear = bg & 0x00FFFFFFu;
        s.dl->AddRectFilledMultiColor(ImVec2(px(x), py(y + h - 10)),
                                      ImVec2(px(x + w), py(y + h + 2)), bg, bg, clear, clear);
    }
    void text(float x, float yCenter, ImU32 c, const char* str) const {
        if (!str || !s.light) return;
        s.dl->AddText(s.light, s.lightSize, ImVec2(px(x), py(yCenter) - s.lightSize * 0.5f), c,
                      str);
    }
    bool hit(const char* id, float x, float y, float w, float h) const {
        ImGui::SetCursorScreenPos(ImVec2(px(x), py(y)));
        return ImGui::InvisibleButton(id, ImVec2(w * s.sx, h * s.sy));
    }

    // cBar::update(): Background | Gradient | BottomShadow, Value_0_100 or
    // ValueLeftRight_100_100. Inactive controls swap in inactiveBarColors.
    void barAt(int x, int y, int w, int h, int value, bool leftRight, bool inactive) const {
        const uint32_t* c = inactive ? kInactiveBarColors : kActiveBarColors;
        const int barWidth = w - 20;
        const int barX = x + (w - barWidth) / 2;
        const int barY = y + 10;
        const int barHeight = h - 25;
        const ImU32 bg = rgb(c[4]);
        fill(x, y, w, h, bg);
        shadow(x, y, w, h, bg);
        fill(x + 10, y + 11, w - 20, h - 25, rgb(c[3]));
        const ImU32 clear = bg & 0x00FFFFFFu;
        s.dl->AddRectFilledMultiColor(ImVec2(px(x + 10), py(y + 11)),
                                      ImVec2(px(x + 18), py(y + h - 14)), clear, rgb(c[3]),
                                      rgb(c[3]), clear);
        if (leftRight) {
            if (value < -100 || value > 100) return;
            const int half = barHeight / 2;
            const int top = (barY + 1) + (value > 0 ? ((half * (100 - value)) / 100) : half);
            const int bot = barY + barHeight - (value < 0 ? ((half * (100 + value)) / 100) : half);
            if (bot > top) fill(barX + 1, top, barWidth - 2, bot - top, rgb(c[0]));
        } else {
            if (value < 0 || value > 100) return;
            const int fillY = barHeight - (barHeight * value) / 100;
            fill(barX + 1, barY + fillY, barWidth - 2, barHeight - fillY, rgb(c[0]));
        }
    }
    void barControl(int i, int value, bool leftRight = false, bool inactive = false) const {
        barAt(100 * i + 1, kBarY, kBarW, kBarH, value, leftRight, inactive);
    }

    // cList::update(): Background | BottomShadow, 27px rows, white selection.
    void listBackground(int x, int y, int w, int h) const {
        const ImU32 bg = rgb(kActiveListColors[3]);
        fill(x, y, w, h, bg);
        shadow(x, y, w, h, bg);
    }
    void listControl(int x, int y, int w, int h, const char* const* items, int count, int lines,
                     int sel, const char* idPrefix, int& clicked) {
        listBackground(x, y, w, h);
        const int top = y + 4;
        const int visible = std::min(lines, count);
        const int first = (count > lines) ? std::clamp(sel - (lines - 1), 0, count - lines) : 0;
        const int barPos = sel - first;
        const int wBar = (count > lines) ? w - 13 : w - 6;
        if (barPos >= 0 && barPos < visible) {
            outline(x + 3, top + barPos * kRowH, wBar, kRowH, rgb(kActiveListColors[0]));
        }
        for (int i = 0; i < visible; ++i) {
            const int idx = first + i;
            const ImU32 color = rgb(kActiveListColors[idx == sel ? 5 : 4]);
            text(x + 13, top + kRowH / 2 + i * kRowH, color, items[idx]);
            char id[48];
            std::snprintf(id, sizeof(id), "##%s_%d", idPrefix, idx);
            if (hit(id, x + 3, top + i * kRowH, wBar, kRowH)) clicked = idx;
        }
        if (count > lines) {
            const int innerH = h - 14;
            const int length = std::max(1, (innerH * lines) / count);
            const int thumbY = y + 6 + (first * (innerH - length)) / count;
            fill(x + w - 6, y + 4, 3, innerH, rgb(kActiveListColors[6]));
            fill(x + w - 5, thumbY, 2, length, rgb(kActiveListColors[2]));
        }
    }

    // cFrame::update(): firmware places array, frameColors[0].
    void frameControl(int place, bool automation) const {
        const int count = automation ? 7 : 8;
        const int p = std::clamp(place, 0, count - 1);
        const int* rect = automation ? kFramesPlacesAutomation[p] : kFramesPlaces[p];
        outline(rect[0], rect[1], rect[2], rect[3], rgb(kFrameColors[0]));
    }
    // cLabel blankSpaceCover: Background | BottomShadow, colors[1].
    void blankSpaceCover(int x, int y, int w, int h) const {
        const ImU32 bg = rgb(kBlankSpaceCoverLabelColors[1]);
        fill(x, y, w, h, bg);
        shadow(x, y, w, h, bg);
    }
    // label[] / cBgLabel are bottom ButtonDeck chrome in the app.
    void labels() const {}
    void bgLabel() const {}

    // cList: filterModeListControl (General 1/2, column 4).
    void filterModeListControl(int filterType) {
        listControl(401, kBarY, kBarW, kBarH, kFilterTypes, 4, 5, filterType,
                    "inst_filter", s.clickedFilter);
    }
    // cList: list1Control (automation destinations, two columns).
    void list1Control(int selected) {
        listControl(1, kBarY, 2 * (800 / 8) - 3, kBarH, kAutoDests, 6, 6, selected, "inst_dest",
                    s.clickedDest);
    }
    // cList: list2Control (automation type).
    void list2Control(int selected) {
        listControl(201, kBarY, kBarW, kBarH, kAutoTypes, 3, 5, selected, "inst_type",
                    s.clickedType);
    }
    // cList: list3Control (LFO shape).
    void list3Control(int selected) {
        listControl(301, kBarY, kBarW, kBarH, kLfoShapes, 5, 5, selected, "inst_shape",
                    s.clickedShape);
    }
    // cList: list4Control (LFO speed, 14 visible rows).
    void list4Control(int selected) {
        listControl(401, kBarY, kBarW, kBarH, kLfoSpeeds, kNumSpeeds, 14, selected, "inst_speed",
                    s.clickedSpeed);
    }

    void hitColumns(int first, int last) const {
        for (int i = first; i <= last; ++i) {
            char id[32];
            std::snprintf(id, sizeof(id), "##inst_col_%d", i);
            if (hit(id, 100.0f * i, kBarY, 100.0f, kBarH)) s.clickedCol = i;
        }
    }

    // gInstrEdit.cpp showInstrumentParams() (Instrument Parameters 1/2).
    void showInstrumentParams() {
        const Instrument& in = *s.inst;
        const bool filterOn = in.filterType != Instrument::FilterType::None;
        blankSpaceCover(701, kBarY, kBarW, kBarH);      // blankSpaceCover[0], column 7
        filterModeListControl(static_cast<int>(in.filterType));
        barControl(6, in.resonance, false, !filterOn);  // Resonance
        barControl(5, in.cutoff, false, !filterOn);     // Cutoff
        barControl(3, in.finetune, true, false);        // Finetune
        barControl(2, (in.tune * 100) / 24, true, false); // Tune
        barControl(1, in.panning * 2, true, false);     // Panning (panning*2-100)
        barControl(0, in.volume, false, false);         // Volume
        labels();
        frameControl(s.selectedCol, false);
        hitColumns(0, 6);
        if (hit("##inst_col_7", 700.0f, kBarY, 100.0f, kBarH)) s.clickedCol = 7;
    }

    // gInstrEdit.cpp showEffectScreen() (Instrument Effects, page 1/2 FX).
    void showEffectScreen() {
        const Instrument& in = *s.inst;
        blankSpaceCover(401, kBarY, 397, kBarH);         // blankSpaceCover[0]
        barControl(3, in.delaySend);                    // Delay send
        barControl(2, in.reverbSend);                   // Reverb send
        barControl(1, ((in.bitDepth - 4) * 100) / 12);  // Bit depth (4..16 -> 0..100)
        barControl(0, in.overdrive);                    // Overdrive
        labels();
        if (s.selectedCol <= 3) frameControl(s.selectedCol, false); // showSendScreenFrame
        hitColumns(0, 7);
    }

    // gInstrEdit.cpp showInstrumentEnv() (Instrument Automation 2/2).
    void showInstrumentEnv() {
        const Instrument& in = *s.inst;
        const int dest = std::clamp(s.selectedDest, 0, 5);
        const auto& slot = in.automation[dest];
        const bool enabled = slot.type != Instrument::AutoType::Off;
        const bool lfo = slot.type == Instrument::AutoType::Lfo;
        const int attack = std::clamp(static_cast<int>(std::lround(slot.attackSec * 10.0f)), 0, 100);
        const int decay = std::clamp(static_cast<int>(std::lround(slot.decaySec * 10.0f)), 0, 100);
        const int sustain = std::clamp(static_cast<int>(std::lround(slot.sustain * 100.0f)), 0, 100);
        const int release = std::clamp(static_cast<int>(std::lround(slot.releaseSec * 10.0f)), 0, 100);
        const int amount = std::clamp(static_cast<int>(std::lround(slot.amount * 100.0f)), 0, 100);

        if (lfo) {
            list4Control(std::clamp(slot.speedStepIdx, 0, kNumSpeeds - 1));
            list3Control(static_cast<int>(slot.shape));
        }
        list2Control(static_cast<int>(slot.type));
        list1Control(dest);

        if (lfo) {
            barControl(5, amount); // Amount (showLfoAmount writes column 5)
        } else {
            barControl(7, amount, false, !enabled);   // Amount
            barControl(6, release, false, !enabled);  // Release
            barControl(5, sustain, false, !enabled);  // Sustain
            barControl(4, decay, false, !enabled);    // Decay
            barControl(3, attack, false, !enabled);   // Attack
        }
        labels();
        frameControl(s.selectedCol <= 1 ? 0 : s.selectedCol - 1, true);
        if (lfo) hitColumns(5, 7);
        else hitColumns(3, 7);
    }
};

} // namespace

const char* const kFilterTypes[4] = {"Disabled", "Low-pass", "High-pass", "Band-pass"};
const char* const kAutoDests[6] = {"Volume",          "Panning",         "Cutoff",
                                   "Wavetable Position", "Granular Position", "Finetune"};
const char* const kAutoTypes[3] = {"Off", "Envelope", "LFO"};
const char* const kLfoShapes[5] = {"Rev Saw", "Saw", "Triangle", "Square", "Random"};
const char* const kLfoSpeeds[18] = {
    "128 steps", "96 steps", "64 steps", "48 steps", "32 steps", "24 steps",
    "16 steps",  "12 steps", "8 steps",  "6 steps",  "4 steps",  "3 steps",
    "2 steps",   "3/2 step", "1 step",   "1/2 step", "1/4 step", "1/8 step",
};

void drawInstrumentScreen(InstrumentScreenState& state) {
    if (!state.inst || !state.dl) return;
    Controls c{state};
    if (state.page == 1) c.showEffectScreen();
    else if (state.page == 2) c.showInstrumentEnv();
    else c.showInstrumentParams();
}

} // namespace trakr

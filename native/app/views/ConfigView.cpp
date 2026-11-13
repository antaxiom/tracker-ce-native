#include "ConfigView.h"
#include "ScreenManager.h"
#include "MidiManager.h"
#include "../FontLoader.h"
#include "imgui.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

// Config screen structural port:
//   core/interface/configEditor/gConfigEdit.cpp  (initDisplayControls)
//   core/libs/NewDisplay/{list,paramValueList,textBox,label,bgLabel,frame}.cpp
// Creation order in gConfigEdit.cpp: popoutWindowLabel, frameControl, label[8],
// bgLabel, configBasemenuListControl, configSubmenuListControl,
// configListControl, configLabel, creditsTextBox, descriptionTextBox,
// blankSpaceCover[0..1]. NewDisplay composites controls in REVERSE creation
// order, so update() draws blank covers -> text boxes -> labels -> lists ->
// bgLabel/labels (app ButtonDeck chrome) -> frameControl -> popout.

namespace trakr {
namespace {

const char* const kGroups[] = {
    "Project Settings", "General", "MIDI", "Metronome", "About"
};
constexpr int kGroupCount = static_cast<int>(sizeof(kGroups) / sizeof(kGroups[0]));

// Firmware geometry (device pixels, 800x480); gConfigEdit.cpp.
constexpr int kChromeHeight = 28;
constexpr int kListLines = 14;
constexpr int kListRowHeight = 27;
constexpr int kRedFrameWidth = 24;
constexpr int kConfigX = 1, kConfigY = 29, kConfigW = 800 / 4 - 3, kConfigH = 394;
constexpr int kSubmenuX = (800 / 8) * 2 + 1, kSubmenuW = 800 / 2 - 3;      // 201, 397
constexpr int kConfigListX = (800 / 8) * 5 + 1;                            // 501
constexpr int kConfigLabelX = (800 / 8) * 2 + 15, kConfigLabelY = 27;      // 215, 27
constexpr int kBlankX[2] = {(800 / 8) * 6 + 1, (800 / 8) * 2 + 1};         // 601, 201
constexpr int kBlankW[2] = {800 / 4 - 2, 800 / 4 - 2};                     // 198

// framesPlacesConfig[4][4].
constexpr int kFramePlaces[4][4] = {
    {(800 / 8) * 0 + 1, 31, 800 / 4 - 3, 391},
    {(800 / 8) * 2 + 1, 31, 800 / 2 - 3, 391},
    {(800 / 8) * 5 + 1, 31, 800 / 4 - 3, 391},
    {(800 / 8) * 5 + 1, 31, 600 / 2 - 3, 391},
};

ImU32 rgb(const ImVec4& c) { return ImGui::ColorConvertFloat4ToU32(c); }

// Option tables copied verbatim from core/interface/configEditor/configMenuDefs.h
// (menu item setups in configMenu.cpp) and core/source/scales.h.
constexpr const char* kOnOff[] = {"Off", "On"};
constexpr const char* kLimiterMode[] = {"Subtle", "Extreme"};
constexpr const char* kPerformanceSource[] = {"Global", "Per Project"};
constexpr const char* kPadsLayout[] = {"1", "2", "3", "4", "5", "6", "7", "8", "9",
                                       "10", "11", "12"};
constexpr const char* kNumberingMode[] = {"Decimal, Steps from 0", "Decimal, Steps from 1",
                                          "Hexadecimal"};
constexpr const char* kPatternInfo[] = {"Additional info", "Track names", "Off"};
constexpr const char* kRecOptions[] = {"Only notes", "Microtiming", "Velocity",
                                       "Microtiming + Velocity"};
constexpr const char* kClockIn[] = {"Internal", "USB", "MIDI In jack"};
constexpr const char* kMidiOut[] = {"Off", "USB", "MIDI Out jack", "USB + MIDI Out jack"};
constexpr const char* kMiddleC[] = {"C-3", "C-4", "C-5", "C-6"};
constexpr const char* kNotesChannel[] = {"ALL",  "CH1",  "CH2",  "CH3",  "CH4",  "CH5",
                                         "CH6",  "CH7",  "CH8",  "CH9",  "CH10", "CH11",
                                         "CH12", "CH13", "CH14", "CH15", "CH16"};
constexpr const char* kClockComp[] = {"-6", "-5", "-4", "-3", "-2", "-1", "0",
                                      "1",  "2",  "3",  "4",  "5",  "6"};
constexpr const char* kMetroTimeSig[] = {"1", "2", "3", "4", "5", "6",
                                         "7", "8", "9", "10", "11", "12"};
// core/source/scales.h scaleNames[39]; the trailing space in "Phrygian " is
// firmware data and is kept verbatim.
constexpr const char* kScaleNames[] = {
    "Chromatic", "Minor",     "Major",     "Dorian",    "Lyd Maj",   "Lyd Min",   "Locrian",
    "Phrygian ", "PhrygDom",  "Mixlydian", "Melo Min",  "Harm Min",  "BeBopMaj",  "BeBopDor",
    "BeBopMix",  "Blues Min", "Blues Maj", "Penta Min", "Penta Maj", "Hung Min",  "Ukrainian",
    "Marva",     "Todi",      "Wholetone", "Dim",       "SLocrian",  "Hirajoshi", "In Sen",
    "Yo",        "Iwato",     "WholeHalf", "Kumoi",     "Overtone",  "DoubHarm",  "Indian",
    "GypsyMin",  "NeapoMaj",  "NeapoMin",  "Enigmatic",
};
// allNotes[12..48] (C1..B3): setupPadsRootNote uses &allNotes[12], count 37.
constexpr const char* kRootNotes[] = {
    "C1",  "C#1", "D1",  "D#1", "E1",  "F1",  "F#1", "G1",  "G#1", "A1",  "A#1", "B1",
    "C2",  "C#2", "D2",  "D#2", "E2",  "F2",  "F#2", "G2",  "G#2", "A2",  "A#2", "B2",
    "C3",  "C#3", "D3",  "D#3", "E3",  "F3",  "F#3", "G3",  "G#3", "A3",  "A#3", "B3",
    "C4",
};

// Item descriptions from configMenuDefs.h (shown in the cTextBox while the
// item editor is open; nullptr items show no description, like the firmware).
constexpr const char* kPerfPresDesc =
    "You can store your Perform presets globally (the same settings for all projects) or "
    "locally (each project has its own presets).";
constexpr const char* kPattDivDesc =
    "Highlights each chosen step, to give you better visual feedback in the pattern. ";
constexpr const char* kScaleDesc =
    "All scales use equal temperament tuning. Use the chromatic scale if you want the pad board "
    "to use all twelve semitones. This gives you four full octaves with the base note in the "
    "first column of each line. ";
constexpr const char* kRootNoteDesc =
    "Defines the note at the bottom-left of the Note popup.";
constexpr const char* kPadLayDesc =
    "For the first (leftmost) column in the Note popup, this value sets how many semitones "
    "higher should each note in the row above be compared to the row below. For example, a "
    "value of 3 will make the note above the Root the 4th note of the scale (1 + 3), the note "
    "two rows above the Root the 7th note (1 + 3 + 3), and so on. Use this setting to customise "
    "your Note popup for more convenient note selection. ";
constexpr const char* kAntialDesc =
    "Disabling anti-aliasing gives the vintage feeling. The samples are harsher when detuned "
    "heavily. ";
constexpr const char* kLimiterDesc =
    "Determines how attack and release of the limiter are treated. Subtle limiting gives a more "
    "transparent sound but will not be able to catch very sudden changes in amplitude. Extreme "
    "limiting reacts much faster which colors the sound. ";
constexpr const char* kHeadroomDesc =
    "Enables more headroom in the audio signal path by reducing the input gain of sample "
    "playback at the source to avoid clipping and improve overall dynamic range.";
constexpr const char* kSnapToZeroDesc =
    "When setting Start/End markers in Sample Playback editor, sudden sample amplitude changes "
    "can cause pops and clicks. To avoid this, activate this mode to automatically snap the "
    "markers when they are set close to the zero amplitude points which are indicated with the "
    "green marker color.";
constexpr const char* kHexModeDesc =
    "Enable the throwback hex goodness: pattern step numbers start with 00 and use sixteen "
    "digits (A means 10, B means 11, and so on all the way to F which means 15). This allows "
    "for easy alignment with music: your music bars will always start with pattern steps 00, "
    "10, 20, 30, ... Each bar's quarternote beat steps end with the digits ..0, ..4, ..8, and "
    "..C. ";
constexpr const char* kPatternInfoDesc = "Pattern info";
constexpr const char* kRecNotesDesc =
    "Sets what information is stored when recording: quantized notes only, notes with "
    "microtiming, quantized notes with velocity, or notes with microtiming and velocity. You "
    "need to record with an external MIDI device to record velocity. ";
constexpr const char* kClockCompDesc =
    "Allows compensating latency of incoming clock signal by number of clock pulses, where 1 "
    "pulse = 1/6 of step. Use this with \"Clock in\" set to MIDI IN jack or USB. ";

template <size_t N>
std::vector<std::string> names(const char* const (&table)[N]) {
    return std::vector<std::string>(std::begin(table), std::end(table));
}


// One pass of the firmware config controls. Coordinates stay in firmware device
// pixels; px()/py() only map them into the app workspace.
struct ConfigControls {
    ImDrawList* dl{};
    ImVec2 origin{};
    float sx{1.0f}, sy{1.0f};
    const Theme* theme{};
    int group{0}, selected{0}, place{0};
    bool configListShown{false};
    std::vector<ConfigView::ConfigRow>* rows{};
    int clickedGroup{-1}, clickedItem{-1};
    ImFont* font{};
    ImFont* boldFont{};
    float fontSize{14.0f}, boldSize{14.0f};

    float px(float x) const { return origin.x + x * sx; }
    float py(float y) const { return origin.y + (y - kChromeHeight) * sy; }
    float stroke() const { return std::max(1.0f, std::round(kRedFrameWidth / 16.0f * sy)); }
    int barPos() const { return std::clamp(selected, 0, kListLines - 1); }
    void fill(float x, float y, float w, float h, ImU32 c) const {
        dl->AddRectFilled(ImVec2(px(x), py(y)), ImVec2(px(x + w), py(y + h)), c);
    }
    void frame(float x, float y, float w, float h, ImU32 c) const {
        dl->AddRect(ImVec2(px(x), py(y)), ImVec2(px(x + w), py(y + h)), c, 0.0f, 0, stroke());
    }
    void text(float x, float yc, ImU32 c, const char* s) const {
        dl->AddText(font, fontSize, ImVec2(px(x), py(yc) - fontSize * 0.5f), c, s);
    }
    void textRight(float xr, float yc, ImU32 c, const char* s) const {
        const ImVec2 ts = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, s);
        dl->AddText(font, fontSize, ImVec2(px(xr) - ts.x, py(yc) - fontSize * 0.5f), c, s);
    }
    void textCenter(float x, float yc, ImU32 c, const char* s) const {
        const ImVec2 ts = boldFont->CalcTextSizeA(boldSize, FLT_MAX, 0.0f, s);
        dl->AddText(boldFont, boldSize, ImVec2(px(x) - ts.x * 0.5f, py(yc) - boldSize * 0.5f), c, s);
    }
    bool hit(const char* id, float x, float y, float w, float h) const {
        ImGui::SetCursorScreenPos(ImVec2(px(x), py(y)));
        return ImGui::InvisibleButton(id, ImVec2(w * sx, h * sy));
    }
    // list.cpp controlStyleBackground + controlStyleBottomShadow.
    void listBackground(float x, float y, float w, float h, ImU32 bg) const {
        fill(x, y, w, h, bg);
        if (theme->bottomScreenShadow > 0) {
            const ImU32 clear = bg & 0x00FFFFFFu;
            dl->AddRectFilledMultiColor(ImVec2(px(x), py(y + h - 10)),
                                        ImVec2(px(x + w), py(y + h + 2)), bg, bg, clear, clear);
        }
    }
    // cConfigEditor::popoutWindowLabel (cLabel: background, center, font2).
    void popoutWindowLabel(const char* text) const {
        if (!text) return;
        const ImVec4* c = theme->configConfirmPopUpLabelColors;
        fill(5, 270, 790, 100, rgb(c[1])); // x=400 w=790, y=320 h=100, centered
        textCenter(400, 370, rgb(c[0]), text);
    }
    // cFrame::update(): framesPlacesConfig[selectedPlace], frameColors[0].
    void frameControl() const {
        const int* p = kFramePlaces[std::clamp(place, 0, 3)];
        frame(p[0], p[1], p[2], p[3], rgb(theme->frameColors[0]));
    }
    // label[0..7]/labelArrow[2] and bgLabel are bottom ButtonDeck chrome.
    void labels() const {}
    void bgLabel() const {}

    // cList: configBasemenuListControl.
    void configBasemenuListControl() {
        const ImVec4* c = theme->activeListColors;
        listBackground(kConfigX, kConfigY, kConfigW, kConfigH, rgb(c[3]));
        const int y0 = kConfigY + 4, bar = std::min(group, kGroupCount - 1);
        frame(kConfigX + 3, y0 + bar * kListRowHeight, kConfigW - 6, kListRowHeight, rgb(c[0]));
        dl->PushClipRect(ImVec2(px(kConfigX), py(y0)),
                         ImVec2(px(kConfigX + kConfigW - 6), py(y0 + kListRowHeight * kListLines)), true);
        for (int i = 0; i < kGroupCount; ++i)
            text(kConfigX + 13, y0 + kListRowHeight / 2 + i * kListRowHeight,
                 rgb(c[i == bar ? 5 : 4]), kGroups[i]);
        dl->PopClipRect();
        for (int i = 0; i < kGroupCount; ++i) {
            char id[32];
            std::snprintf(id, sizeof(id), "##config_base_%d", i);
            if (hit(id, kConfigX + 3, y0 + i * kListRowHeight, kConfigW - 6, kListRowHeight))
                clickedGroup = i;
        }
    }
    // cParamValueList: configSubmenuListControl.
    void configSubmenuListControl() {
        const ImVec4* c = theme->activeListColors;
        listBackground(kSubmenuX, kConfigY, kSubmenuW, kConfigH, rgb(c[3]));
        const int y0 = kConfigY + 4, length = static_cast<int>(rows->size()), bar = barPos();
        frame(kSubmenuX + 3, y0 + bar * kListRowHeight, kSubmenuW - 6, kListRowHeight, rgb(c[0]));
        dl->PushClipRect(ImVec2(px(kSubmenuX), py(y0)),
                         ImVec2(px(kSubmenuX + kSubmenuW - 3), py(y0 + kListRowHeight * kListLines)), true);
        for (int i = 0; i < std::min(length, kListLines); ++i) {
            const ImU32 color = rgb(c[i == bar ? 5 : 4]);
            const float yc = y0 + kListRowHeight / 2 + i * kListRowHeight;
            text(kSubmenuX + 13, yc, color, (*rows)[i].name.c_str());
            textRight(kSubmenuX + kSubmenuW - 10, yc, color, (*rows)[i].value.c_str());
        }
        dl->PopClipRect();
        for (int i = 0; i < std::min(length, kListLines); ++i) {
            char id[32];
            std::snprintf(id, sizeof(id), "##config_item_%d", i);
            if (hit(id, kSubmenuX + 3, y0 + i * kListRowHeight, kSubmenuW - 6, kListRowHeight))
                clickedItem = i;
        }
    }
    // cList: configListControl, only shown by the item editor.
    void configListControl() const {
        if (!configListShown) return;
        const ImVec4* c = theme->activeListColors;
        listBackground(kConfigListX, kConfigY, kConfigW, kConfigH, rgb(c[3]));
        frame(kConfigListX + 3, kConfigY + 4, kConfigW - 6, kListRowHeight, rgb(c[0]));
    }
    // cLabel: configLabel (item editor title), font3.
    void configLabel() const {
        if (!configListShown || rows->empty()) return;
        dl->AddText(boldFont, boldSize, ImVec2(px(kConfigLabelX), py(kConfigLabelY + 18 - 7)),
                    rgb(theme->configItemEditorTextLabelColors[0]), (*rows)[barPos()].name.c_str());
    }
    // cTextBox: descriptionTextBox and creditsTextBox (font5).
    void descriptionTextBox() const {}
    void creditsTextBox() const {}
    // cLabel blankSpaceCover[2]: background colours[1].
    void blankSpaceCover() const {
        const ImU32 bg = rgb(theme->blankSpaceCoverLabelColors[1]);
        fill(kBlankX[1], kConfigY, kBlankW[1], kConfigH, bg);
        fill(kBlankX[0], kConfigY, kBlankW[0], kConfigH, bg);
    }
};

} // namespace

ConfigView::ConfigView() = default;
void ConfigView::draw(bool* p_open) { (void)p_open; }

const char* ConfigView::groupName(int index) {
    static const char* const names[] = {"Project Settings", "General", "MIDI", "Metronome", "About"};
    if (index < 0 || index >= kGroupCount) return "";
    return names[index];
}

std::vector<ConfigView::ConfigRow> ConfigView::currentRows(const Song& song) const {
    return rowsForGroup(static_cast<int>(activeGroup_), song);
}

std::vector<ConfigView::ConfigRow> ConfigView::rowsForGroup(int group, const Song& song) const {
    switch (static_cast<Group>(group)) {
        case Group::ProjectSettings:
            return {
                {"Performance presets", kPerformanceSource[perfSource_]},
                {"Pattern divider", std::to_string(song.lpb)},
                {"Notes scale", kScaleNames[notesScale_]},
                {"Root note", kRootNotes[rootNote_]},
                {"Notes layout", kPadsLayout[notesLayout_]},
                // configMenuDefs.h onOffStateTexts / limiterModeTexts.
                {"Anti-aliasing", kOnOff[antialiasing_]},
                {"Limiter mode", kLimiterMode[limiterMode_]},
                {"Extra headroom", song.headroom ? "On" : "Off"},
                {"Snap to zero", kOnOff[snapToZero_]}};
        case Group::General:
            return {
                {"Pattern top info", kPatternInfo[patternTopInfo_]},
                {"Recording options", kRecOptions[recOptions_]},
                {"Numbering mode", kNumberingMode[numberingMode_]},
                {"Middle C", kMiddleC[std::clamp(middleC_, 3, 6) - 3]},
                {"Clock sync correction", kClockComp[clockComp_]}};
        case Group::Midi: {
            auto ports = MidiManager::instance().getPortNames();
            int cur = MidiManager::instance().getCurrentPort();
            std::string portName =
                (cur >= 0 && cur < static_cast<int>(ports.size())) ? ports[cur]
                                                                   : "None / Disconnected";
            return {
                {"MIDI in device", portName},
                {"Clock in", kClockIn[clockIn_]},
                {"Clock out", kMidiOut[clockOut_]},
                {"Transport in", kClockIn[transportIn_]},
                {"Transport out", kMidiOut[transportOut_]},
                {"Notes in channel", kNotesChannel[notesChannel_]},
                {"Clock sync correction", kClockComp[clockComp_]}};
        }
        case Group::Metronome:
            return {{"State", kOnOff[metroState_]},
                    {"Volume", std::to_string(metroVolume_)},
                    {"Pre roll", kOnOff[metroPreRoll_]},
                    {"Time signature num", kMetroTimeSig[metroNum_]},
                    {"Time signature denum", kMetroTimeSig[metroDenum_]}};
        case Group::About:
            return {{"Product name", "Trakr Native"},
                    {"Architecture", "Desktop Tracker"},
                    {"Audio engine", "Tracker DSP Engine"},
                    {"Display engine", "SDL GPU Accelerated"}};
    }
    return {};
}

void ConfigView::moveSelection(int delta) {
    int count = 0;
    switch (activeGroup_) {
        case Group::ProjectSettings: count = 9; break;
        case Group::General: count = 5; break;
        case Group::Midi: count = 7; break;
        case Group::Metronome: count = 5; break;
        case Group::About: count = 4; break;
    }
    selectedItem_ = std::clamp(selectedItem_ + delta, 0, std::max(0, count - 1));
}

void ConfigView::switchGroup(int delta) {
    activeGroup_ =
        static_cast<Group>(((static_cast<int>(activeGroup_) + delta) % kGroupCount +
                            kGroupCount) % kGroupCount);
    selectedItem_ = 0;
    selectedPlace_ = 0;
}

void ConfigView::selectGroup(int index) {
    activeGroup_ = static_cast<Group>(std::clamp(index, 0, kGroupCount - 1));
    selectedItem_ = 0;
    selectedPlace_ = 0;
}

void ConfigView::selectItem(int index) {
    selectedItem_ = std::max(0, index);
    selectedPlace_ = 1;
}

void ConfigView::adjustSelectedValue(Song& song, EngineHost& engineHost, Theme& theme, int delta) {
    (void)theme;
    const int group = static_cast<int>(activeGroup_);
    const ItemOptions opts = optionsFor(group, selectedItem_, song);
    if (!opts.editable) return;
    const int next = std::clamp(opts.value + delta, 0, static_cast<int>(opts.options.size()) - 1);
    if (next == opts.value) return;
    setItemValue(group, selectedItem_, next, song, engineHost);
}

ConfigView::ItemOptions ConfigView::optionsFor(int group, int item, const Song& song) const {
    ItemOptions out;
    auto use = [&out](std::vector<std::string> options, int value, const char* description) {
        out.options = std::move(options);
        out.value = std::clamp(value, 0, std::max(0, static_cast<int>(out.options.size()) - 1));
        out.description = description ? description : "";
        out.editable = true;
        return out;
    };
    switch (static_cast<Group>(group)) {
        case Group::ProjectSettings:
            switch (item) {
                case 0: return use(names(kPerformanceSource), perfSource_, kPerfPresDesc);
                case 1: {
                    std::vector<std::string> v;
                    for (int i = 1; i <= 16; ++i) v.push_back(std::to_string(i));
                    return use(std::move(v), song.lpb - 1, kPattDivDesc);
                }
                case 2: return use(names(kScaleNames), notesScale_, kScaleDesc);
                case 3: return use(names(kRootNotes), rootNote_, kRootNoteDesc);
                case 4: return use(names(kPadsLayout), notesLayout_, kPadLayDesc);
                case 5: return use(names(kOnOff), antialiasing_, kAntialDesc);
                case 6: return use(names(kLimiterMode), limiterMode_, kLimiterDesc);
                case 7: return use(names(kOnOff), song.headroom ? 1 : 0, kHeadroomDesc);
                case 8: return use(names(kOnOff), snapToZero_, kSnapToZeroDesc);
                default: break;
            }
            break;
        case Group::General:
            switch (item) {
                case 0: return use(names(kPatternInfo), patternTopInfo_, kPatternInfoDesc);
                case 1: return use(names(kRecOptions), recOptions_, kRecNotesDesc);
                case 2: return use(names(kNumberingMode), numberingMode_, kHexModeDesc);
                case 3: return use(names(kMiddleC), middleC_ - 3, nullptr);
                case 4: return use(names(kClockComp), clockComp_, kClockCompDesc);
                default: break;
            }
            break;
        case Group::Midi:
            switch (item) {
                case 0: {
                    std::vector<std::string> ports = MidiManager::instance().getPortNames();
                    ports.push_back("None / Disconnected");
                    const int cur = MidiManager::instance().getCurrentPort();
                    const int value =
                        (cur >= 0 && cur < static_cast<int>(ports.size()) - 1) ? cur + 1 : 0;
                    return use(std::move(ports), value, nullptr);
                }
                case 1: return use(names(kClockIn), clockIn_, nullptr);
                case 2: return use(names(kMidiOut), clockOut_, nullptr);
                case 3: return use(names(kClockIn), transportIn_, nullptr);
                case 4: return use(names(kMidiOut), transportOut_, nullptr);
                case 5: return use(names(kNotesChannel), notesChannel_, nullptr);
                case 6: return use(names(kClockComp), clockComp_, kClockCompDesc);
                default: break;
            }
            break;
        case Group::Metronome:
            switch (item) {
                case 0: return use(names(kOnOff), metroState_, nullptr);
                case 1: {
                    std::vector<std::string> v;
                    for (int i = 0; i <= 100; ++i) v.push_back(std::to_string(i));
                    return use(std::move(v), metroVolume_, nullptr);
                }
                case 2: return use(names(kOnOff), metroPreRoll_, nullptr);
                case 3: return use(names(kMetroTimeSig), metroNum_, nullptr);
                case 4: return use(names(kMetroTimeSig), metroDenum_, nullptr);
                default: break;
            }
            break;
        case Group::About:
            break;
    }
    return out; // label-only row (firmware menuTypeItemLabel)
}

void ConfigView::setItemValue(int group, int item, int value, Song& song,
                              EngineHost& engineHost) {
    switch (static_cast<Group>(group)) {
        case Group::ProjectSettings:
            switch (item) {
                case 0: perfSource_ = value; break;
                case 1: song.lpb = std::clamp(value + 1, 1, 16); engineHost.publish(song); break;
                case 2: notesScale_ = value; break;
                case 3: rootNote_ = value; break;
                case 4: notesLayout_ = value; break;
                case 5: antialiasing_ = value; break;
                case 6: limiterMode_ = value; break;
                case 7: song.headroom = (value == 1); engineHost.publish(song); break;
                case 8: snapToZero_ = value; break;
                default: break;
            }
            break;
        case Group::General:
            switch (item) {
                case 0: patternTopInfo_ = value; break;
                case 1: recOptions_ = value; break;
                case 2: numberingMode_ = value; break;
                case 3: middleC_ = std::clamp(value + 3, 3, 6); break;
                case 4: clockComp_ = value; break;
                default: break;
            }
            break;
        case Group::Midi:
            switch (item) {
                case 0: {
                    auto ports = MidiManager::instance().getPortNames();
                    const int port = value - 1;
                    if (port < 0 || port >= static_cast<int>(ports.size()))
                        MidiManager::instance().closePort();
                    else
                        MidiManager::instance().openPort(port);
                    break;
                }
                case 1: clockIn_ = value; break;
                case 2: clockOut_ = value; break;
                case 3: transportIn_ = value; break;
                case 4: transportOut_ = value; break;
                case 5: notesChannel_ = value; break;
                case 6: clockComp_ = value; break;
                default: break;
            }
            break;
        case Group::Metronome:
            switch (item) {
                case 0: metroState_ = value; break;
                case 1: metroVolume_ = value; break;
                case 2: metroPreRoll_ = value; break;
                case 3: metroNum_ = value; break;
                case 4: metroDenum_ = value; break;
                default: break;
            }
            break;
        case Group::About:
            break;
    }
}

bool ConfigView::openEditor(Song& song, EngineHost& engineHost) {
    (void)engineHost;
    const int group = static_cast<int>(activeGroup_);
    const std::vector<ConfigRow> rows = currentRows(song);
    const int item = std::clamp(selectedItem_, 0, std::max(0, static_cast<int>(rows.size()) - 1));
    selectedItem_ = item;
    ItemOptions opts = optionsFor(group, item, song);
    if (!opts.editable) return false;
    editor_.open = true;
    editor_.item = item;
    editor_.title = rows[static_cast<size_t>(item)].name;
    editor_.description = std::move(opts.description);
    editor_.options = std::move(opts.options);
    editor_.selected = opts.value;
    selectedPlace_ = 1; // firmware keeps the middle-column frame while editing
    configListShown_ = true;
    return true;
}

bool ConfigView::openEditorForName(const std::string& name, Song& song,
                                   EngineHost& engineHost) {
    for (int g = 0; g < kGroupCount; ++g) {
        const std::vector<ConfigRow> rows = rowsForGroup(g, song);
        for (int i = 0; i < static_cast<int>(rows.size()); ++i) {
            if (rows[i].name != name) continue;
            if (!optionsFor(g, i, song).editable) return false;
            activeGroup_ = static_cast<Group>(g);
            selectedItem_ = i;
            return openEditor(song, engineHost);
        }
    }
    return false;
}

void ConfigView::moveEditorSelection(int delta) {
    if (!editor_.open) return;
    setEditorSelection(editor_.selected + delta);
}

void ConfigView::setEditorSelection(int index) {
    if (!editor_.open) return;
    editor_.selected =
        std::clamp(index, 0, std::max(0, static_cast<int>(editor_.options.size()) - 1));
}

void ConfigView::applyEditor(Song& song, EngineHost& engineHost) {
    if (!editor_.open) return;
    setItemValue(static_cast<int>(activeGroup_), editor_.item, editor_.selected, song, engineHost);
    cancelEditor();
}

void ConfigView::cancelEditor() {
    editor_ = ConfigEditor{};
    configListShown_ = false;
    selectedPlace_ = 1;
}

std::vector<hw::ScreenButton> ConfigView::getScreenButtons(Song& song, EngineHost& engineHost,
                                                           Theme& theme, Screen& activeScreen,
                                                           bool firmwareParity) {
    if (firmwareParity) {
        // gConfigEdit.cpp bottom cLabels: the idle state shows every cell blank
        // except slot 7 "Enter" (showDefaultConfigScreen). While the item
        // editor is open the firmware shows "Cancel" (6) and "Apply" (7)
        // (loadConfigTextList/loadConfigValuesList via changeLabelText).
        std::vector<hw::ScreenButton> btns(8);
        for (auto& b : btns) b = {"", "", "", "", hw::UnderlineColor::None, false, false, nullptr};
        if (editor_.open) {
            btns[6] = {"Cancel", "", "", "", hw::UnderlineColor::Green, true, false,
                       [this]() { cancelEditor(); }};
            btns[7] = {"Apply", "", "", "", hw::UnderlineColor::Green, true, false,
                       [this, &song, &engineHost]() { applyEditor(song, engineHost); }};
        } else {
            btns[7] = {"Enter", "", "", "", hw::UnderlineColor::Green, true, false,
                       [this, &song, &engineHost]() { openEditor(song, engineHost); }};
        }
        return btns;
    }
    std::vector<hw::ScreenButton> btns(8);
    btns[0] = {"Value -", "", "", "", hw::UnderlineColor::Green, true, false, [&]() { selectedPlace_ = 1; adjustSelectedValue(song, engineHost, theme, -1); }};
    btns[1] = {"Value +", "", "", "", hw::UnderlineColor::Green, true, false, [&]() { selectedPlace_ = 1; adjustSelectedValue(song, engineHost, theme, 1); }};
    btns[2] = {"Reset", "Default", "", "", hw::UnderlineColor::Dimmed, true, false, [&]() { song.masterGainDb = -3.5f; song.lpb = 4; engineHost.publish(song); }};
    btns[3] = {"Theme", "Classic", "", "", hw::UnderlineColor::Green, true, false, [&]() { theme = makeTrackerClassicTheme(); }};
    btns[4] = {"Format", numberingMode_ == 2 ? "Hex" : "Dec", "", "", hw::UnderlineColor::Green, true, numberingMode_ == 2, [&]() { numberingMode_ = (numberingMode_ == 2) ? 0 : 2; }};
    btns[5] = {"Prev", "Group", "", "", hw::UnderlineColor::Dimmed, true, false, [this]() {
        activeGroup_ = static_cast<Group>((static_cast<int>(activeGroup_) + 4) % 5);
        selectedItem_ = 0; selectedPlace_ = 0;
    }};
    btns[6] = {"Next", "Group", "", "", hw::UnderlineColor::Dimmed, true, false, [this]() {
        activeGroup_ = static_cast<Group>((static_cast<int>(activeGroup_) + 1) % 5);
        selectedItem_ = 0; selectedPlace_ = 0;
    }};
    btns[7] = {"Pattern", "Page", "", "", hw::UnderlineColor::Orange, true, false, [&activeScreen]() { activeScreen = Screen::Pattern; }};
    return btns;
}

void ConfigView::update(Song& song, EngineHost& engineHost, Theme& theme) {
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                                   ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar;
    if (!ImGui::Begin("Config", nullptr, flags)) { ImGui::End(); return; }

    if (ImGui::IsWindowHovered()) {
        const int g = static_cast<int>(activeGroup_);
        // Manual: Up/Down navigates the selected list; Left/Right switches the
        // focused window. Up/Down must not move the focus frame.
        if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) { selectedItem_ = std::max(0, selectedItem_ - 1); }
        if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) { selectedItem_++; }
        // The firmware footer is blank except "Enter"; navigation keeps the
        // original Up/Down (item) and Left/Right (group) mapping, and value
        // editing moves to '-'/'=' so both remain available.
        if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) {
            activeGroup_ = static_cast<Group>((g + 4) % kGroupCount); selectedItem_ = 0; selectedPlace_ = 0;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) {
            activeGroup_ = static_cast<Group>((g + 1) % kGroupCount); selectedItem_ = 0; selectedPlace_ = 0;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Minus)) { selectedPlace_ = 1; adjustSelectedValue(song, engineHost, theme, -1); }
        if (ImGui::IsKeyPressed(ImGuiKey_Equal)) { selectedPlace_ = 1; adjustSelectedValue(song, engineHost, theme, 1); }
    }

    std::vector<ConfigView::ConfigRow> rows = currentRows(song);

    selectedItem_ = std::clamp(selectedItem_, 0, std::max(0, static_cast<int>(rows.size()) - 1));
    const ImVec2 avail = ImGui::GetContentRegionAvail();
    ImFont* font = getMonoFont();
    ImFont* bold = getMonoBoldFont();
    if (avail.x < 100.0f || avail.y < 100.0f || !font) { ImGui::End(); return; }

    const ImVec2 origin = ImGui::GetCursorScreenPos();
    ConfigControls c;
    c.dl = ImGui::GetWindowDrawList();
    c.origin = origin;
    c.sx = avail.x / 800.0f;
    c.sy = avail.y / 397.0f;
    c.theme = &theme;
    c.group = static_cast<int>(activeGroup_);
    c.selected = selectedItem_;
    c.place = selectedPlace_;
    c.configListShown = configListShown_;
    c.rows = &rows;
    c.font = font;
    c.boldFont = bold;
    c.fontSize = 14.0f * c.sy; // controlStyleFont1: 14 Regular
    c.boldSize = 14.0f * c.sy; // controlStyleFont3: 14 Bold

    // Reverse creation order (see header comment).
    c.blankSpaceCover();
    c.descriptionTextBox();
    c.creditsTextBox();
    c.configLabel();
    c.configListControl();
    c.configSubmenuListControl();
    c.configBasemenuListControl();
    c.bgLabel();
    c.labels();
    c.frameControl();
    c.popoutWindowLabel(nullptr);

    if (c.clickedGroup >= 0) {
        activeGroup_ = static_cast<Group>(c.clickedGroup);
        selectedItem_ = 0;
        selectedPlace_ = 0;
    }
    if (c.clickedItem >= 0) {
        selectedItem_ = c.clickedItem;
        selectedPlace_ = 1;
    }
    ImGui::SetCursorScreenPos(origin);
    ImGui::End();
}

} // namespace trakr

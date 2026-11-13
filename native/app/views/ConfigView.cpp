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
    switch (activeGroup_) {
        case Group::ProjectSettings:
            return {
                {"Performance presets", "Global"},
                {"Pattern divider", std::to_string(song.lpb)},
                {"Notes scale", "Chromatic"},
                {"Root note", "C"},
                {"Notes layout", "Standard 4-Row"},
                // configMenuDefs.h onOffStateTexts / limiterModeTexts.
                {"Anti-aliasing", "On"},
                {"Limiter mode", "Subtle"},
                {"Extra headroom", song.headroom ? "On" : "Off"},
                {"Snap to zero", "On"}};
        case Group::General:
            return {
                {"Pattern top info", "Tracks Only"},
                {"Recording options", "All"},
                {"Numbering mode", hexDisplay_ ? "Hexadecimal" : "Decimal"},
                {"Middle C", std::string("C-") + std::to_string(middleC_)},
                {"Clock sync correction", "0"}};
        case Group::Midi: {
            auto ports = MidiManager::instance().getPortNames();
            int cur = MidiManager::instance().getCurrentPort();
            std::string portName =
                (cur >= 0 && cur < static_cast<int>(ports.size())) ? ports[cur]
                                                                   : "None / Disconnected";
            return {
                {"MIDI in device", portName},
                {"Clock in", "Internal"},
                {"Clock out", "Enabled"},
                {"Transport in", "Enabled"},
                {"Transport out", "Enabled"},
                {"Notes in channel", "ALL"},
                {"Clock sync correction", "0"}};
        }
        case Group::Metronome:
            return {{"State", "Off"},
                    {"Volume", "50"},
                    {"Pre roll", "Off"},
                    {"Time signature num", "4"},
                    {"Time signature denum", "4"}};
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
    switch (activeGroup_) {
        case Group::ProjectSettings:
            if (selectedItem_ == 1) song.lpb = std::clamp(song.lpb + delta, 1, 16);
            else if (selectedItem_ == 7) song.headroom = (delta > 0);
            engineHost.publish(song);
            break;
        case Group::General:
            // Rows: 0 Pattern top info, 1 Recording options, 2 Numbering mode,
            // 3 Middle C, 4 Clock sync correction.
            if (selectedItem_ == 2) hexDisplay_ = !hexDisplay_;
            else if (selectedItem_ == 3) middleC_ = (delta > 0) ? 3 : 4;
            break;
        case Group::Midi:
            if (selectedItem_ == 0) {
                auto ports = MidiManager::instance().getPortNames();
                if (!ports.empty()) {
                    int cur = MidiManager::instance().getCurrentPort();
                    int total = static_cast<int>(ports.size()) + 1;
                    int next = ((cur + 1 + delta) % total + total) % total - 1;
                    if (next < 0) MidiManager::instance().closePort();
                    else MidiManager::instance().openPort(next);
                }
            }
            break;
        case Group::Metronome:
        case Group::About:
            break;
    }
}

std::vector<hw::ScreenButton> ConfigView::getScreenButtons(Song& song, EngineHost& engineHost,
                                                           Theme& theme, Screen& activeScreen,
                                                           bool firmwareParity) {
    if (firmwareParity) {
        // gConfigEdit.cpp bottom cLabels (specs/config.json): idle state shows
        // every cell blank except slot 7 "Enter". Editing stays available via
        // the keyboard (arrow keys adjust, PageUp/PageDown switch group).
        std::vector<hw::ScreenButton> btns(8);
        for (auto& b : btns) b = {"", "", "", "", hw::UnderlineColor::None, false, false, nullptr};
        btns[7] = {"Enter", "", "", "", hw::UnderlineColor::Green, true, false, [&]() {
            selectedPlace_ = 1;
            engineHost.publish(song);
        }};
        return btns;
    }
    std::vector<hw::ScreenButton> btns(8);
    btns[0] = {"Value -", "", "", "", hw::UnderlineColor::Green, true, false, [&]() { selectedPlace_ = 1; adjustSelectedValue(song, engineHost, theme, -1); }};
    btns[1] = {"Value +", "", "", "", hw::UnderlineColor::Green, true, false, [&]() { selectedPlace_ = 1; adjustSelectedValue(song, engineHost, theme, 1); }};
    btns[2] = {"Reset", "Default", "", "", hw::UnderlineColor::Dimmed, true, false, [&]() { song.masterGainDb = -3.5f; song.lpb = 4; engineHost.publish(song); }};
    btns[3] = {"Theme", "Classic", "", "", hw::UnderlineColor::Green, true, false, [&]() { theme = makeTrackerClassicTheme(); }};
    btns[4] = {"Format", hexDisplay_ ? "Hex" : "Dec", "", "", hw::UnderlineColor::Green, true, hexDisplay_, [&]() { hexDisplay_ = !hexDisplay_; }};
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

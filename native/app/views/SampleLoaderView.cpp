#include "SampleLoaderView.h"
#include "ScreenManager.h"
#include "FontLoader.h"
#include "BitmapText.h"
#include "imgui.h"
#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

// Sample Loader screen structural port of:
//   core/interface/sampleImporter/gSampleImport.cpp  (initDisplayControls)
//   core/libs/NewDisplay/{list,bar,frame,label,bgLabel,
//                         points,spectrum,progressCursor}.cpp
// Creation order in initDisplayControls(): frameControl, label[0..7], bgLabel,
// explorerLabelControl, instrumentLabelControl, explorerListControl,
// instrumentListControl, memoryBarControl, previewVolumeBarControl,
// progressCursor, pointsControl, spectrumControl[0], spectrumControl[1].
// NewDisplay composites controls in REVERSE creation order, so this body draws
// spectrum[1] -> spectrum[0] -> points -> progressCursor -> preview volume bar
// -> memory bar -> instrument list -> explorer list -> instrument label ->
// explorer label -> bgLabel/labels (app ButtonDeck chrome) -> frameControl.
// The importer crop controls are hidden on the default screen and the app has
// no crop page yet; they remain named stubs below.

namespace trakr {
namespace {

// Firmware geometry (device pixels, 800x480); gSampleImport.cpp.
constexpr int kChromeHeight = 28;          // app header; firmware body starts below
constexpr int kRedFrameWidth = 24;         // config.h; 24/16 device px
constexpr int kRows = 13, kRowH = 27;      // list linesCount / cList h_row
constexpr int kListX = 1, kListY = 29 + 32, kListW = (800 / 8) * 3 - 3, kListH = 394 - 32;
constexpr int kSlotsX = (800 / 8) * 3 + 1; // 301
constexpr int kFileLabelX = (800 / 16) * 3 - 1;      // 149
constexpr int kSlotLabelX = (800 / 16) * 9 - 1;      // 449
constexpr int kLabelY = 32, kLabelW = (800 / 8) * 3 - 3, kLabelH = 26;
constexpr int kBarY = 29, kBarH = 394, kBarW = 800 / 8 - 3;
constexpr int kPreviewX = (800 / 8) * 6 + 1; // 601
constexpr int kMemoryX = (800 / 8) * 7 + 1;  // 701
constexpr int kBottomShadow = 1, kBarShadow = 1; // Themes.cpp
constexpr int kPreviewVolume = 86;               // PREVIEW_VOLUME_DEFAULT

// framesPlaces[3][4]: 0 SD card, 1 instruments, 2 preview volume.
constexpr int kFramePlaces[3][4] = {
    {1, 31, (800 / 8) * 3 - 3, 391},
    {(800 / 8) * 3 + 1, 31, (800 / 8) * 3 - 3, 391},
    {(800 / 8) * 6 + 1, 31, 800 / 8 - 3, 391},
};

// Hidden importer crop page controls (cProgressCursor/cPoints/cSpectrum[2]).
[[maybe_unused]] constexpr int kWaveX = 100, kWaveW = 600;
[[maybe_unused]] constexpr int kWaveH = 0xFFFF / 300;         // MAX_16BIT / 300
[[maybe_unused]] constexpr int kWaveY = 75 + (300 - kWaveH) / 2;

// Loader colors mirrored verbatim from core/source/Themes.cpp (themeDefault).
// Kept file-local so this screen port does not contend on native/app/Theme.cpp;
// consolidation into Theme is deferred.
static const ImU32 kListActive[7] = {0xCD1818, 0xCD1818, 0x575757, 0x0A0A0A,
                                     0xFFFFFF, 0xFFFFFF, 0x000000};
static const ImU32 kListInactive[7] = {0xFFFFFF, 0xFFFFFF, 0x575757, 0x0A0A0A,
                                       0xFFFFFF, 0xFFFFFF, 0x000000};
static const ImU32 kMemNormal[5] = {0xFFFFFF, 0xCD1818, 0x30CB3F, 0x080808, 0x0A0A0A};
static const ImU32 kMemFull[5] = {0xCD1818, 0xCD1818, 0xCD1818, 0x080808, 0x0A0A0A};
static const ImU32 kActiveBar[5] = {0xFFFFFF, 0xFF0000, 0x00FF00, 0x080808, 0x0A0A0A};
static const ImU32 kFrameColors[2] = {0xCD1818, 0x3F3F3F};

constexpr ImU32 rgba(ImU32 c) {
    return IM_COL32((c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF, 0xFF);
}

// One pass of the firmware importer controls. Coordinates stay in firmware
// device pixels; px()/py() only map them into the app workspace.
struct LoaderControls {
    ImDrawList* dl{};
    ImVec2 origin{};
    float sx{1.0f}, sy{1.0f};
    const std::vector<std::string>* files{};
    const std::vector<std::string>* instruments{};
    int selectedFile{0}, selectedSlot{0}, place{0}, previewVolume{kPreviewVolume};
    bool filesActive{true}, memoryFull{false};
    int memoryUsed{0}, memoryCompare{0};
    int clickedFile{-1}, clickedSlot{-1};
    ImFont* light{};
    ImFont* bold{};
    float lightSize{14.0f}; // cList default Light14
    float boldSize{17.0f};  // controlStyleFont4
    float px(float x) const { return origin.x + x * sx; }
    float py(float y) const { return origin.y + (y - kChromeHeight) * sy; }
    float stroke() const { return std::max(1.0f, std::round(kRedFrameWidth / 16.0f * sy)); }
    void fill(float x, float y, float w, float h, ImU32 c) const {
        dl->AddRectFilled(ImVec2(px(x), py(y)), ImVec2(px(x + w), py(y + h)), c);
    }
    void outline(float x, float y, float w, float h, ImU32 c, float t) const {
        dl->AddRect(ImVec2(px(x), py(y)), ImVec2(px(x + w), py(y + h)), c, 0.0f, 0, t);
    }
    // controlStyleBottomShadow: solid bg fading to transparent over 12px.
    void shadow(float x, float y, float w, float h, ImU32 bg) const {
        const ImU32 clear = bg & 0x00FFFFFFu;
        dl->AddRectFilledMultiColor(ImVec2(px(x), py(y + h - 10)),
                                    ImVec2(px(x + w), py(y + h + 2)), bg, bg, clear, clear);
    }
    void text(ImFont* f, float size, float x, float yc, ImU32 c, const char* s) const {
        const ImVec2 ts = f->CalcTextSizeA(size, FLT_MAX, 0.0f, s);
        dl->AddText(f, size, ImVec2(px(x), py(yc) - ts.y * 0.5f), c, s);
    }
    // Firmware L4 bitmap text (exact glyphs the fwref reference uses).
    void textL4(float x, float yc, ImU32 c, const char* s, hw::FwFont ff) const {
        const float h = hw::fwTextHeight(ff) * sy;
        hw::fwDrawText(dl, ff, px(x), py(yc) - h * 0.5f, s, c, sx, sy);
    }
    void textCenterL4(float x, float yc, ImU32 c, const char* s, hw::FwFont ff) const {
        const float w = hw::fwTextWidth(ff, s) * sx;
        const float h = hw::fwTextHeight(ff) * sy;
        hw::fwDrawText(dl, ff, px(x) - w * 0.5f, py(yc) - h * 0.5f, s, c, sx, sy);
    }
    void textCenter(ImFont* f, float size, float x, float yc, ImU32 c, const char* s) const {
        const ImVec2 ts = f->CalcTextSizeA(size, FLT_MAX, 0.0f, s);
        dl->AddText(f, size, ImVec2(px(x) - ts.x * 0.5f, py(yc) - ts.y * 0.5f), c, s);
    }
    bool hit(const char* id, float x, float y, float w, float h) const {
        ImGui::SetCursorScreenPos(ImVec2(px(x), py(y)));
        return ImGui::InvisibleButton(id, ImVec2(w * sx, h * sy));
    }

    // cList::update(): explorerListControl / instrumentListControl.
    void listControl(const char* id, float x, int selected, const std::vector<std::string>& data,
                     const ImU32* colors, int* clicked) {
        const int length = static_cast<int>(data.size());
        const bool active = filesActive == (colors == kListActive);
        const int wBar = length > kRows ? kListW - 13 : kListW - 6;
        const ImU32 bg = rgba(colors[3]);
        fill(x, kListY, kListW, kListH, bg);
        if (kBottomShadow > 0) shadow(x, kListY, kListW, kListH, bg);

        // cList::setData(): selection frame row and first visible text row.
        int barPos = selected, textPos = 0;
        if (selected >= length - 2 && length > kRows) {
            barPos = kRows - (selected == length - 1 ? 1 : 2);
            textPos = length - kRows;
        } else if (selected > kRows - 2 && length > kRows) {
            barPos = kRows - 2;
            textPos = selected - (kRows - 2);
        }
        barPos = std::clamp(barPos, 0, std::max(0, std::min(kRows, length) - 1));
        const int y0 = kListY + 4;
        outline(x + 3, y0 + barPos * kRowH, wBar, kRowH,
                rgba(active ? colors[1] : colors[0]), stroke());
        dl->PushClipRect(ImVec2(px(x), py(y0)),
                         ImVec2(px(x + wBar), py(y0 + kRowH * kRows)), true);
        const int lines = std::min(length, kRows);
        for (int i = 0; i < lines; ++i) {
            const int row = i + textPos;
            if (row >= length) break;
            textL4(x + 13, y0 + kRowH / 2.0f + i * kRowH,
                   rgba(row == selected ? colors[5] : colors[4]), data[row].c_str(),
                   hw::FwFont::Light14);
        }
        dl->PopClipRect();
        if (length > kRows) {
            const int innerH = kListH - 14, yLength = (innerH * kRows) / length;
            const int thumbY = y0 + 6 + (selected * (innerH - yLength)) / length;
            fill(x + kListW - 6, y0, 3, innerH, rgba(colors[6]));
            fill(x + kListW - 5, thumbY, 2, yLength, rgba(colors[2]));
        }
        for (int i = 0; i < lines; ++i) {
            const int row = i + textPos;
            if (row >= length) break;
            char item[40];
            std::snprintf(item, sizeof(item), "##%s_%d", id, row);
            if (hit(item, x + 3, y0 + i * kRowH, wBar, kRowH)) *clicked = row;
        }
    }
    void explorerListControl() {
        listControl("loader_file", kListX, selectedFile, *files,
                    filesActive ? kListActive : kListInactive, &clickedFile);
    }
    void instrumentListControl() {
        listControl("loader_slot", kSlotsX, selectedSlot, *instruments,
                    filesActive ? kListInactive : kListActive, &clickedSlot);
    }

    // cLabel: explorerLabelControl/instrumentLabelControl. Header colors are
    // activateLabelsBorder()'s {list[4], list[3], list[0], list[5]}.
    void labelControl(float x, const char* title, const ImU32* colors) const {
        fill(x - kLabelW / 2.0f, kLabelY, kLabelW, kLabelH, rgba(colors[3]));
        textCenterL4(x, kLabelY + kLabelH / 2.0f, rgba(colors[4]), title, hw::FwFont::Bold17);
    }
    void explorerLabelControl() const {
        labelControl(kFileLabelX, "SD Card", filesActive ? kListActive : kListInactive);
    }
    void instrumentLabelControl() const {
        labelControl(kSlotLabelX, "Instruments", filesActive ? kListInactive : kListActive);
    }
    // cBar::update(): Background | Gradient | BottomShadow, Value_0_100 or
    // CompareTwoValues.
    void barControl(float x, const ImU32* colors, int value, int compare, bool compareMode) const {
        const int barWidth = kBarW - 20, barX = static_cast<int>(x) + (kBarW - barWidth) / 2;
        const int barY = kBarY + 10, barHeight = kBarH - 25;
        const ImU32 bg = rgba(colors[4]);
        fill(x, kBarY, kBarW, kBarH, bg);
        if (kBottomShadow > 0) shadow(x, kBarY, kBarW, kBarH, bg);
        outline(x + 10, kBarY + 11, kBarW - 20, kBarH - 25, rgba(colors[3]), 1.0f);
        if (kBarShadow > 0) {
            const ImU32 clear = bg & 0x00FFFFFFu, container = rgba(colors[3]);
            dl->AddRectFilledMultiColor(ImVec2(px(x + 10), py(kBarY + 11)),
                                        ImVec2(px(x + 18), py(kBarY + kBarH - 14)),
                                        clear, container, container, clear);
        }
        if (value < 0 || value > 100) return;
        const int fillY = barHeight - (barHeight * value) / 100;
        if (!compareMode) {
            fill(barX + 1, barY + fillY, barWidth - 2, barHeight - fillY, rgba(colors[0]));
            return;
        }
        if (compare < 0 || compare > 100) return;
        const int sub = compare - value, cmp = (barHeight * std::abs(sub)) / 100;
        const int top = barY + (sub < 0 ? fillY + cmp : fillY);
        fill(barX + 1, top, barWidth - 2, barY + barHeight - top, rgba(colors[0]));
        if (sub > 0) fill(barX, barY + fillY - cmp, barWidth - 1, cmp, rgba(colors[1]));
        else if (sub < 0) fill(barX + 1, barY + fillY, barWidth - 1, cmp, rgba(colors[2]));
    }
    void previewVolumeBarControl() const {
        barControl(kPreviewX, kActiveBar, previewVolume, 0, false);
    }
    void memoryBarControl() const {
        barControl(kMemoryX, memoryFull ? kMemFull : kMemNormal, memoryUsed, memoryCompare, true);
    }

    // Hidden importer crop page controls (showDefaultScreen hides them).
    void progressCursor() const {}
    void pointsControl() const {}
    void spectrumControl(int) const {}
    // cBgLabel / label[0..7]: bottom bar, app ButtonDeck chrome.
    void bgLabel() const {}
    void labels() const {}
    // cFrame::update(): selected place outline, frameColors[0].
    void frameControl() const {
        const int* p = kFramePlaces[std::clamp(place, 0, 2)];
        outline(p[0], p[1], p[2], p[3], rgba(kFrameColors[0]), stroke());
    }
    // cSampleImporter::showDefaultScreen(), reverse creation order.
    void showDefaultScreen() {
        fill(0, kChromeHeight, 800, 397, rgba(0x000000)); // Themes.cpp backgroundColor
        spectrumControl(1);
        spectrumControl(0);
        pointsControl();
        progressCursor();
        previewVolumeBarControl();
        memoryBarControl();
        instrumentListControl();
        explorerListControl();
        instrumentLabelControl();
        explorerLabelControl();
        bgLabel();
        labels();
        frameControl();
    }
};

} // namespace
SampleLoaderView::SampleLoaderView() {
    std::error_code ec;
    currentPath_ = std::filesystem::current_path(ec);
    refreshDirectory();
}

void SampleLoaderView::draw(bool* p_open) { (void)p_open; }
void SampleLoaderView::setParityState() {
    // Parity listing mirrors the canonical fwref loader scenario
    // (native/tools/fwref/scenario.json -> loader.files / selectedFile) so the
    // port renders the same SD-card contents and selection row as the reference.
    currentPath_ = "/samples/drums";
    dirEntries_ = {"/..", "909_Kick.wav", "909_Snare.wav", "909_Clap.wav",
                   "909_ClosedHat.wav", "909_OpenHat.wav", "909_LowTom.wav",
                   "909_MidTom.wav", "909_HiTom.wav", "909_Rimshot.wav",
                   "909_Cowbell.wav", "909_Crash.wav", "909_Ride.wav",
                   "/Acoustic", "/Percussion", "kick_01.wav", "snare_01.wav",
                   "hat_01.wav", "clap_01.wav", "tom_01.wav"};
    isDirectory_ = {true, false, false, false, false, false, false, false,
                    false, false, false, false, false, true, true, false,
                    false, false, false, false};
    selectedFileIdx_ = 1;
    selectedSlotIdx_ = 0;
    dedupeEntries();
}
std::vector<hw::ScreenButton> SampleLoaderView::getScreenButtons(
    Song& song, History& history, EngineHost& engineHost, int& selectedInstIdx,
    Screen& activeScreen, bool firmwareParity) {
    auto preview = [&]() { previewSelectedSlot(song, engineHost); };
    auto removeSlot = [&]() { deleteInstrumentSlot(song, history, engineHost, selectedSlotIdx_); };
    auto removeUnused = [&]() { deleteUnusedInstruments(song, history, engineHost); };
    auto importFile = [&]() { importSelectedFile(song, history, engineHost, selectedSlotIdx_); };
    std::vector<hw::ScreenButton> btns(8);
    const bool isAudition = auditionStartTime_ > 0.0;

    if (firmwareParity) {
        // gSampleImport.cpp bottom cLabels (specs/loader.json): Add, Add next,
        // Preview, Add, Import, Delete/unused, Preview Vol, Memory.
        const auto mem = memoryUsage(song);
        btns[0] = {"Add", "", "", "", hw::UnderlineColor::Green, true, false,
                   [&]() { addNextAvailableSlot(song, history, engineHost, selectedInstIdx); }};
        btns[1] = {"Add next", "", "", "", hw::UnderlineColor::Green, true, false,
                   [&]() { addNextAvailableSlot(song, history, engineHost, selectedInstIdx); }};
        btns[2] = {"Preview", "", "", "", hw::UnderlineColor::Green, true, isAudition, preview};
        btns[3] = {"Add", "", "", "", hw::UnderlineColor::Green, true, false,
                   [&]() { addNextAvailableSlot(song, history, engineHost, selectedInstIdx); }};
        btns[4] = {"Import", "", "", "", hw::UnderlineColor::Green, true, false, importFile};
        btns[5] = {"Delete", "unused", "", "", hw::UnderlineColor::Red, true, false, removeUnused};
        btns[6] = {"Preview Vol", "0.00 dB", "", "", hw::UnderlineColor::Green, false, false, nullptr};
        btns[7] = {"Memory", std::to_string(mem.used) + "%", "", "", hw::UnderlineColor::Green, false, false, nullptr};
        return btns;
    }
    btns[0] = {"Preview", "", "", "", hw::UnderlineColor::Green, true, isAudition, preview};
    btns[1] = {"Delete", "Slot", "", "", hw::UnderlineColor::Red, true, false, removeSlot};
    btns[2] = {"Delete", "Unused", "", "", hw::UnderlineColor::Red, true, false, removeUnused};
    btns[3] = {"Add Next", "Slot", "", "", hw::UnderlineColor::Green, true, false,
               [&]() { addNextAvailableSlot(song, history, engineHost, selectedInstIdx); }};
    btns[4] = {"Import", "Selected", "", "", hw::UnderlineColor::Green, true, false, importFile};
    btns[5] = {"Parent", "Folder", "", "", hw::UnderlineColor::Dimmed,
               currentPath_.has_parent_path(), false, [this]() { goParentDirectory(); }};
    btns[6] = {"Enter", "Dir", "", "", hw::UnderlineColor::Dimmed,
               (!dirEntries_.empty() && isDirectory_[selectedFileIdx_]), false,
               [this]() { enterSelectedDirectory(); }};
    btns[7] = {"Pattern", "Page", "", "", hw::UnderlineColor::Orange, true, false,
               [&activeScreen]() { activeScreen = Screen::Pattern; }};
    return btns;
}

void SampleLoaderView::update(Song& song, History& history, EngineHost& engineHost,
                              int& selectedInstIdx) {
    (void)history;
    advanceAudition(song, engineHost);
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                                   ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar;
    if (!ImGui::Begin("Sample Loader", nullptr, flags)) { ImGui::End(); return; }
    const ImVec2 avail = ImGui::GetContentRegionAvail();
    ImFont* light = getMonoLightFont();
    ImFont* bold = getMonoBoldFont();
    if (avail.x < 100.0f || avail.y < 100.0f || !light || !bold) { ImGui::End(); return; }

    // Firmware navigation: Left/Right switches the focused pane (red box),
    // Up/Down moves the selection inside it.
    const int nFiles = static_cast<int>(dirEntries_.size());
    if (ImGui::IsWindowHovered()) {
        if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) focusFiles_ = true;
        if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) focusFiles_ = false;
        if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
            if (focusFiles_) selectedFileIdx_ = std::max(0, selectedFileIdx_ - 1);
            else selectedSlotIdx_ = std::max(0, selectedSlotIdx_ - 1);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
            if (focusFiles_) selectedFileIdx_ = std::min(std::max(0, nFiles - 1), selectedFileIdx_ + 1);
            else selectedSlotIdx_ = std::min(47, selectedSlotIdx_ + 1);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Enter) && focusFiles_) enterSelectedDirectory();
    }

    std::vector<std::string> fileRows = fileLabels();
    std::vector<std::string> slotRows = slotLabels(song);
    const auto mem = memoryUsage(song);

    const ImVec2 origin = ImGui::GetCursorScreenPos();
    LoaderControls c;
    c.dl = ImGui::GetWindowDrawList();
    c.origin = origin;
    c.sx = avail.x / 800.0f;
    c.sy = avail.y / 397.0f;
    c.files = &fileRows;
    c.instruments = &slotRows;
    c.selectedFile = std::clamp(selectedFileIdx_, 0, std::max(0, nFiles - 1));
    c.selectedSlot = std::clamp(selectedSlotIdx_, 0, 47);
    c.place = focusFiles_ ? 0 : 1;
    c.filesActive = focusFiles_;
    c.memoryUsed = mem.used;
    c.memoryCompare = mem.compare;
    c.memoryFull = mem.full;
    c.previewVolume = kPreviewVolume;
    c.light = light;
    c.bold = bold;
    c.lightSize = 14.0f * c.sy; // controlStyleFont1 (cList default Light14)
    c.boldSize = 17.0f * c.sy;  // controlStyleFont4
    c.showDefaultScreen();      // cSampleImporter::showDefaultScreen()

    if (c.clickedFile >= 0) {
        selectedFileIdx_ = c.clickedFile;
        focusFiles_ = true;
        if (ImGui::IsMouseDoubleClicked(0)) enterSelectedDirectory();
    }
    if (c.clickedSlot >= 0) {
        selectedSlotIdx_ = c.clickedSlot;
        selectedInstIdx = c.clickedSlot;
        focusFiles_ = false;
    }
    ImGui::SetCursorScreenPos(origin);
    ImGui::End();
}

} // namespace trakr

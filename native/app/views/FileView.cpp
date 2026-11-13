#include "FileView.h"
#include "ScreenManager.h"
#include "../FontLoader.h"
#include "imgui.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <utility>

// File screen structural port of the project editor:
//   core/interface/projectEditor/gProjEdit.cpp  (initDisplayControls)
//   core/libs/NewDisplay/{list,img,frame,label,bgLabel}.cpp
// Creation order in initDisplayControls(): frameControl, label[0..7], bgLabel,
// browserLabelControl, recentLabelControl, fileListControl, recentListControl,
// coverImg, blankSpaceCover. NewDisplay composites controls in REVERSE creation
// order, so this body draws blankSpaceCover -> coverImg -> recentListControl ->
// fileListControl -> recentLabelControl -> browserLabelControl, then the
// bgLabel/label[8] app ButtonDeck chrome, then frameControl on top.
// The app File page is the project browser, so update() renders the
// showProjectBrowser() stage: fileListControl holds the project list, and
// coverImg stays hidden because hideProjectCover() is active there.

namespace trakr {
namespace {

const std::vector<std::string> kDefaultProjects = {
    "01. New Project",
    "02. Demo Song",
    "03. Album Sketch",
    "04. Live Set 01",
    "05. Bassline Idea",
    "06. Chiptune 01",
    "07. Drum Kit Test",
    "08. Untitled 07",
    "09. Untitled 08",
    "10. Untitled 09",
    "11. Untitled 10",
    "12. Untitled 11",
    "13. Untitled 12",
};

// --- Firmware geometry (device pixels, 800x480); gProjEdit.cpp --------------
constexpr int kChromeHeight = 28;      // app header; firmware body starts under it
constexpr int kRedFrameWidth = 24;     // config.h; 24/16 device px per stroke unit
constexpr int kListLines = 13;         // projectList.linesCount
constexpr int kRecentLines = 10;       // recentList.linesCount
constexpr int kListRowHeight = 27;     // list.cpp h_row
constexpr int kListX = 1;
constexpr int kListY = 29 + 32;        // 61
constexpr int kListW = (800 / 8) * 3 - 3;
constexpr int kListH = 394 - 32;
constexpr int kRecentX = (800 / 8) * 3 + 1; // 301
constexpr int kBrowserLabelX = (800 / 16) * 3;    // 150
constexpr int kRecentLabelX = (800 / 16) * 9;     // 450
constexpr int kLabelY = 32;
constexpr int kLabelW = (800 / 8) * 3 - 6;        // 294
constexpr int kLabelH = 26;
constexpr int kBlankX = 600 + 1;       // showProjectBrowser(): position 600 + 1
constexpr int kBlankY = 29;
constexpr int kBlankW = 200;           // kOpenProject: setControlSize(..., 200, -1)
constexpr int kBlankH = 394;
[[maybe_unused]] constexpr int kCoverCx = 400; // cImg: prop.x = 400, prop.y = 220
[[maybe_unused]] constexpr int kCoverCy = 220;

// gProjEdit.cpp framesPlaces[2][4]: 0 file list place, 1 recent list place.
constexpr int kFramePlaces[2][4] = {
    {0 + 1, 31, (800 / 8) * 3 - 3, 391},
    {(800 / 8) * 3 + 1, 31, (800 / 8) * 3 - 3, 391},
};

// File-screen colors mirrored verbatim from core/source/Themes.cpp
// (themeDefault). Kept file-local so parallel screen ports do not contend on
// native/app/Theme.cpp; consolidation into Theme is deferred.
// clang-format off
[[maybe_unused]] static const ImU32 kProjectActiveListColors[7] =
    {0xcd1818, 0xcd1818, 0x575757, 0x0A0A0A, 0xFFFFFF, 0x30cb3f, 0x000000};
[[maybe_unused]] static const ImU32 kProjectInactiveListColors[7] =
    {0xFFFFFF, 0xFFFFFF, 0x575757, 0x0A0A0A, 0xFFFFFF, 0x30cb3f, 0x000000};
static const ImU32 kBlankSpaceCoverLabelColors[4] = {0xFFFFFF, 0x0A0A0A, 0xFF0000, 0x000000};
static const ImU32 kTopBarLabelColors[4] = {0xFFFFFF, 0x080808, 0xFFFFFF, 0xFFFFFF};
[[maybe_unused]] static const ImU32 kActiveButtonLabelsColors[4] =
    {0x777777, 0x323132, 0xcd1818, 0x777777};
[[maybe_unused]] static const ImU32 kInactiveButtonLabelsColors[4] =
    {0x333333, 0x323132, 0xcd1818, 0x333333};
[[maybe_unused]] static const ImU32 kHighlightedButtonLabelsColors[4] =
    {0x777777, 0x222222, 0xcd1818, 0x777777};
static const ImU32 kFrameColors[2] = {0xcd1818, 0x3F3F3F};
[[maybe_unused]] static const ImU32 kBottomButtonsBgColors[3] = {0x777777, 0x0A0A0A, 0xcd1818};
constexpr ImU32 kBackgroundColor = 0x000000;
constexpr int kBottomScreenShadow = 1; // themeDefault.bottomScreenShadow
// clang-format on

constexpr ImU32 rgba(ImU32 c) {
    return IM_COL32((c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF, 0xFF);
}

// One pass of the firmware project-editor controls. Coordinates stay in
// firmware device pixels; px()/py() only map them into the app workspace.
struct FileControls {
    ImDrawList* dl{};
    ImVec2 origin{};
    float sx{1.0f}, sy{1.0f};
    std::vector<std::string>* projects{};
    int selected{0};
    int listStart{0};
    int clickedRow{-1};
    ImFont* font{};
    ImFont* boldFont{};
    float fontSize{14.0f};
    float boldSize{17.0f};

    float px(float x) const { return origin.x + x * sx; }
    float py(float y) const { return origin.y + (y - kChromeHeight) * sy; }
    float stroke() const { return std::max(1.0f, std::round(kRedFrameWidth / 16.0f * sy)); }

    void fill(float x, float y, float w, float h, ImU32 c) const {
        dl->AddRectFilled(ImVec2(px(x), py(y)), ImVec2(px(x + w), py(y + h)), c);
    }
    void frame(float x, float y, float w, float h, ImU32 c) const {
        dl->AddRect(ImVec2(px(x), py(y)), ImVec2(px(x + w), py(y + h)), c, 0.0f, 0, stroke());
    }
    void line(float x1, float y1, float x2, float y2, ImU32 c) const {
        dl->AddLine(ImVec2(px(x1), py(y1)), ImVec2(px(x2), py(y2)), c, stroke());
    }
    // API_CMD_TEXT(..., OPT_CENTERY): y is the row center.
    void text(float x, float yCenter, ImU32 c, const char* s) const {
        if (!s || !*s) return;
        dl->AddText(font, fontSize, ImVec2(px(x), py(yCenter) - fontSize * 0.5f), c, s);
    }
    void textCenter(float centerX, float yCenter, ImU32 c, const char* s) const {
        if (!s || !*s) return;
        const ImVec2 ts = boldFont->CalcTextSizeA(boldSize, FLT_MAX, 0.0f, s);
        dl->AddText(boldFont, boldSize,
                    ImVec2(px(centerX) - ts.x * 0.5f, py(yCenter) - boldSize * 0.5f), c, s);
    }
    bool hit(const char* id, float x, float y, float w, float h) const {
        ImGui::SetCursorScreenPos(ImVec2(px(x), py(y)));
        return ImGui::InvisibleButton(id, ImVec2(w * sx, h * sy));
    }
    // list.cpp controlStyleBackground + controlStyleBottomShadow.
    void listBackground(float x, float y, float w, float h, ImU32 bg) const {
        fill(x, y, w, h, bg);
        if (kBottomScreenShadow > 0) {
            const ImU32 clear = bg & 0x00FFFFFFu;
            dl->AddRectFilledMultiColor(ImVec2(px(x), py(y + h - 10)),
                                        ImVec2(px(x + w), py(y + h + 2)), bg, bg, clear, clear);
        }
    }
    // list.cpp scroll bar: colors[6] track, colors[2] thumb (two lines).
    void listScrollBar(float x, float y, float w, float h, int length, int start,
                       const ImU32* c) const {
        if (length <= kListLines) return;
        const int yLength = ((h - 14) * kListLines) / length;
        const int yPos = (y + 4) + 6 + (start * ((h - 14) - yLength)) / length;
        fill(x + (w - 6), y + 4 + 4, 3, h - 14, rgba(c[6]));
        line(x + (w - 5), yPos, x + (w - 5), yPos + yLength, rgba(c[2]));
        line(x + (w - 4), yPos, x + (w - 4), yPos + yLength, rgba(c[2]));
    }

    // cList::update(): fileListControl (projectList).
    void fileListControl() {
        const ImU32* c = kProjectActiveListColors;
        listBackground(kListX, kListY, kListW, kListH, rgba(c[3]));
        const int length = static_cast<int>(projects->size());
        const int lines = std::min(length, kListLines);
        const int y0 = kListY + 4; // list.cpp top margin
        const int wBar = (length > kListLines) ? kListW - 13 : kListW - 6;
        const int barPos = std::clamp(selected - listStart, 0, kListLines - 1);

        // Selection frame: selectionActive -> colors[1], else colors[0].
        if (barPos < lines) frame(kListX + 3, y0 + barPos * kListRowHeight, wBar,
                                  kListRowHeight, rgba(c[1]));

        dl->PushClipRect(ImVec2(px(kListX), py(y0)),
                         ImVec2(px(kListX + wBar), py(y0 + kListRowHeight * kListLines)), true);
        for (int i = 0; i < lines; ++i) {
            const int rowIdx = listStart + i;
            // list.cpp: selectTab -> colors[5], selected start row -> colors[5],
            // otherwise colors[4]. The app has no multi-select tab list.
            const ImU32 color = rgba(rowIdx == listStart ? c[5] : c[4]);
            text(kListX + 13, y0 + kListRowHeight / 2 + i * kListRowHeight, color,
                 (*projects)[rowIdx].c_str());
        }
        dl->PopClipRect();

        for (int i = 0; i < lines; ++i) {
            char id[32];
            std::snprintf(id, sizeof(id), "##file_row_%d", listStart + i);
            if (hit(id, kListX + 3, y0 + i * kListRowHeight, wBar, kListRowHeight))
                clickedRow = listStart + i;
        }
        listScrollBar(kListX, kListY, kListW, kListH, length, listStart, c);
    }

    // cList::update(): recentListControl (recentList). The desktop app keeps no
    // recent-project history, so the list is empty: background only.
    void recentListControl() {
        const ImU32* c = kProjectInactiveListColors;
        listBackground(kRecentX, kListY, kListW, kListH, rgba(c[3]));
        listScrollBar(kRecentX, kListY, kListW, kListH, 0, 0, c);
    }

    // cLabel::update(): browserLabelControl, style Background|CenterY|CenterX|Font4.
    void browserLabelControl() const {
        fill(kBrowserLabelX, kLabelY, kLabelW, kLabelH, rgba(kTopBarLabelColors[1]));
        textCenter(kBrowserLabelX + kLabelW / 2, kLabelY + kLabelH / 2,
                   rgba(kTopBarLabelColors[0]), "SD Card");
    }
    // cLabel::update(): recentLabelControl, same style as the browser label.
    void recentLabelControl() const {
        fill(kRecentLabelX, kLabelY, kLabelW, kLabelH, rgba(kTopBarLabelColors[1]));
        textCenter(kRecentLabelX + kLabelW / 2, kLabelY + kLabelH / 2,
                   rgba(kTopBarLabelColors[0]), "Recent");
    }

    // cImg::update(): coverImg (prop.x=400, prop.y=220, CenterX|CenterY). The
    // project browser hides it (gProjEdit.cpp hideProjectCover), and no cover
    // bitmap is loaded in RAM, so the control draws nothing.
    void coverImg() const {}

    // cLabel::update(): blankSpaceCover, style Background, colors[1].
    void blankSpaceCover() const {
        fill(kBlankX, kBlankY, kBlankW, kBlankH, rgba(kBlankSpaceCoverLabelColors[1]));
    }

    // cLabel label[0..7] and cBgLabel bgLabel are bottom ButtonDeck chrome.
    void labels() const {}
    void bgLabel() const {}

    // cFrame::update(): framesPlaces[place], frameColors[0].
    void frameControl(int place) const {
        const int* p = kFramePlaces[std::clamp(place, 0, 1)];
        frame(p[0], p[1], p[2], p[3], rgba(kFrameColors[0]));
    }

    // gProjEdit.cpp showProjectBrowser(): reverse creation order.
    void showProjectBrowser() {
        blankSpaceCover();
        coverImg();
        recentListControl();
        fileListControl();
        recentLabelControl();
        browserLabelControl();
        bgLabel();
        labels();
        frameControl(0); // selectedPlace = 0 (file list)
    }
};

} // namespace

FileView::FileView() {
    setParityState();
}

void FileView::setParityState() {
    projects_ = kDefaultProjects;
    selectedProject_ = 0;
    listStart_ = 0;
    projectName_ = "New Project";
}

void FileView::draw(bool* p_open) { (void)p_open; }

void FileView::moveSelection(int delta) {
    if (projects_.empty()) return;
    selectedProject_ =
        std::clamp(selectedProject_ + delta, 0, static_cast<int>(projects_.size()) - 1);
}

void FileView::selectProject(int index) {
    if (projects_.empty()) return;
    selectedProject_ = std::clamp(index, 0, static_cast<int>(projects_.size()) - 1);
}

void FileView::setActions(std::function<void()> onOpen, std::function<void()> onSave,
                          std::function<void()> onSaveAs, std::function<void()> onNew) {
    onOpen_ = std::move(onOpen);
    onSave_ = std::move(onSave);
    onSaveAs_ = std::move(onSaveAs);
    onNew_ = std::move(onNew);
}

std::vector<hw::ScreenButton> FileView::getScreenButtons(Song& song, History& history,
                                                         EngineHost& engineHost,
                                                         Screen& activeScreen,
                                                         bool& showExportDeck,
                                                         bool firmwareParity) {
    (void)firmwareParity;
    (void)activeScreen;
    std::vector<hw::ScreenButton> btns(8);

    // gProjEdit.cpp bottom cLabels (specs/file.json): New/Project, Open,
    // Import Mod, Save, Save As, Export, (blank), (blank).
    btns[0] = {"New", "Project", "", "", hw::UnderlineColor::Green, true, false, [&]() {
        song = makeDefaultSong();
        history.clear();
        engineHost.publish(song);
        projectName_ = "New Project";
        selectedProject_ = 0;
        listStart_ = 0;
        if (onNew_) onNew_();
    }};
    btns[1] = {"Open", "", "", "", hw::UnderlineColor::Green, true, false, [this]() {
        if (onOpen_) onOpen_();
    }};
    // No module-import path exists yet, so Import Mod is rendered like the
    // reference but left non-clickable instead of misrouting to Open.
    btns[2] = {"Import Mod", "", "", "", hw::UnderlineColor::Green, false, false, nullptr};
    btns[3] = {"Save", "", "", "", hw::UnderlineColor::Green, true, false, [this]() {
        if (onSave_) onSave_();
    }};
    btns[4] = {"Save As", "", "", "", hw::UnderlineColor::Green, true, false, [this]() {
        if (onSaveAs_) onSaveAs_();
    }};
    btns[5] = {"Export", "", "", "", hw::UnderlineColor::Green, true, false, [&]() {
        showExportDeck = true;
    }};
    btns[6] = {"", "", "", "", hw::UnderlineColor::None, false, false, nullptr};
    btns[7] = {"", "", "", "", hw::UnderlineColor::None, false, false, nullptr};
    return btns;
}

void FileView::update(Song& song, EngineHost& engineHost) {
    (void)song;
    (void)engineHost;

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar;
    if (!ImGui::Begin("File", nullptr, flags)) { ImGui::End(); return; }

    if (projects_.empty()) projects_.push_back("01. New Project");
    selectedProject_ = std::clamp(selectedProject_, 0, static_cast<int>(projects_.size()) - 1);
    const int maxStart = std::max(0, static_cast<int>(projects_.size()) - kListLines);
    listStart_ = std::clamp(listStart_, 0, maxStart);
    if (selectedProject_ < listStart_) listStart_ = selectedProject_;
    else if (selectedProject_ >= listStart_ + kListLines)
        listStart_ = selectedProject_ - kListLines + 1;
    listStart_ = std::clamp(listStart_, 0, maxStart);

    // Keyboard navigation across the project list (existing binding).
    if (ImGui::IsWindowHovered()) {
        if (ImGui::IsKeyPressed(ImGuiKey_UpArrow) && selectedProject_ > 0) --selectedProject_;
        if (ImGui::IsKeyPressed(ImGuiKey_DownArrow))
            selectedProject_ = std::min(static_cast<int>(projects_.size()) - 1, selectedProject_ + 1);
    }

    const ImVec2 avail = ImGui::GetContentRegionAvail();
    ImFont* font = getMonoFont();
    ImFont* boldFont = getMonoBoldFont();
    if (avail.x < 100.0f || avail.y < 100.0f || !font) { ImGui::End(); return; }

    const ImVec2 origin = ImGui::GetCursorScreenPos();
    FileControls c;
    c.dl = ImGui::GetWindowDrawList();
    c.origin = origin;
    c.sx = avail.x / 800.0f;
    c.sy = avail.y / 397.0f;
    c.projects = &projects_;
    c.selected = selectedProject_;
    c.listStart = listStart_;
    c.font = font;
    c.boldFont = boldFont;
    c.fontSize = 14.0f * c.sy; // cList font index 0: RobotoMonoLight14
    c.boldSize = 17.0f * c.sy; // controlStyleFont4: RobotoMonoBold17

    // Display clear (Themes.cpp backgroundColor); controls composite above it.
    c.fill(0.0f, static_cast<float>(kChromeHeight), 800.0f, 397.0f, rgba(kBackgroundColor));
    c.showProjectBrowser();

    if (c.clickedRow >= 0) selectedProject_ = c.clickedRow;
    ImGui::SetCursorScreenPos(origin);
    ImGui::End();
}

} // namespace trakr

#include "file.h"

#include "Song.h"
#include "EngineHost.h"
#include "views/FileView.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace trakr {
using namespace ::trakr::ui;

namespace {
constexpr int kListX = 1;
constexpr int kListY = 29 + 32;          // 61
constexpr int kListW = (800 / 8) * 3 - 3; // 297
constexpr int kListH = 394 - 32;          // 362
constexpr int kRecentX = (800 / 8) * 3 + 1;
constexpr int kLabelY = 32;
constexpr int kLabelW = (800 / 8) * 3 - 6; // 294
constexpr int kLabelH = 26;

// gProjEdit.cpp framesPlaces[2][4].
constexpr int kFramePlaces[2][4] = {
    {1, 31, kListW, 391},
    {kRecentX, 31, kListW, 391},
};
} // namespace

FileScreen::FileScreen() = default;

void FileScreen::build() {
    if (built_) return;

    std::vector<RectI> places;
    for (const auto& p : kFramePlaces) places.push_back(RectI{p[0], p[1], p[2], p[3]});
    frame_ = &display_.add<FrameControl>("frame", std::move(places), 0);

    // gProjEdit.cpp:55-65 creates these with controlStyleCenterX, so the
    // firmware x is the label centre; the firmware background spans
    // [x - w/2, x + w/2].
    browserLabel_ =
        &display_.add<LabelControl>("browserLabel", "SD Card", trakr::hw::FwFont::Bold17);
    browserLabel_->setRect(RectI{(800 / 16) * 3 - kLabelW / 2, kLabelY, kLabelW, kLabelH});
    browserLabel_->setColors(kTopBarLabelColors[0], kTopBarLabelColors[1],
                             kTopBarLabelColors[2]);
    recentLabel_ =
        &display_.add<LabelControl>("recentLabel", "Recent", trakr::hw::FwFont::Bold17);
    recentLabel_->setRect(RectI{(800 / 16) * 9 - kLabelW / 2, kLabelY, kLabelW, kLabelH});
    recentLabel_->setColors(kTopBarLabelColors[0], kTopBarLabelColors[1], kTopBarLabelColors[2]);

    fileList_ = &display_.add<ListControl>("fileList", std::vector<std::string>{}, 13);
    fileList_->setRect(RectI{kListX, kListY, kListW, kListH});
    fileList_->setColors(kProjectActiveListColors);
    // The firmware project browser always paints a selection row on the recent
    // list even when it holds no entries.
    recentList_ = &display_.add<ListControl>("recentList", std::vector<std::string>{""}, 10);
    recentList_->setRect(RectI{kRecentX, kListY, kListW, kListH});
    recentList_->setColors(kProjectInactiveListColors);

    // blankSpaceCover is created last, so the reverse composite paints it first.
    // gProjEdit.cpp:253-254 (kOpenProject): position (800/8)*6 + 1, size 200.
    blankCover_ = &display_.add<BlankCoverControl>("blankSpaceCover");
    blankCover_->setRect(RectI{(800 / 8) * 6 + 1, 29, 200, 394});

    fileList_->setOnSelect([this](int index) {
        if (view_) view_->selectProject(index);
    });

    built_ = true;
}

void FileScreen::syncFromModel(const FileView& view) {
    fileList_->setItems(view.projects());
    fileList_->setSelected(view.selectedProject());
    fileList_->setStart(-1);
}

void FileScreen::update(Song& song,
                        EngineHost& engineHost,
                        FileView& view,
                        const ImRect& workspace,
                        ui::LayoutProfile profile) {
    (void)song;
    (void)engineHost;
    build();
    view_ = &view;

    ImGui::SetCursorScreenPos(workspace.Min);
    ImGui::InvisibleButton("##gui_next_file_workspace", workspace.GetSize());
    const bool hovered = ImGui::IsItemHovered();
    const bool keyTarget =
        ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) || !ImGui::IsAnyItemActive();

    if (keyTarget && !ImGui::GetIO().WantTextInput) {
        if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) view.moveSelection(-1);
        if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) view.moveSelection(+1);
        if (ImGui::IsKeyPressed(ImGuiKey_Enter)) view.openSelected();
    }
    if (hovered) {
        const float wheel = ImGui::GetIO().MouseWheel;
        if (std::abs(wheel) > 0.01f) view.moveSelection((wheel > 0.0f) ? -1 : +1);
    }

    if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
        const LayoutContext lc = LayoutContext::fromWorkspace(workspace, profile);
        const ImVec2 device = lc.screenToDevice(ImGui::GetIO().MousePos);
        const int dx = static_cast<int>(std::floor(device.x));
        const int dy = static_cast<int>(std::floor(device.y));
        if (ui::Control* hit = display_.hitTest(dx, dy)) hit->onPress(dx, dy);
        if (ImGui::IsMouseDoubleClicked(0)) view.openSelected();
    }

    syncFromModel(view);
}

void FileScreen::render(ImDrawList* drawList,
                        const ImRect& workspace,
                        ui::LayoutProfile profile) const {
    if (!built_ || drawList == nullptr) return;
    const LayoutContext lc = LayoutContext::fromWorkspace(workspace, profile);
    Canvas canvas(drawList, lc);
    canvas.pushClip(RectI{0, kWorkspaceTop, kDeviceWidth, kWorkspaceSpanY});
    canvas.fillRect(RectI{0, 28, kDeviceWidth, 397}, IM_COL32(0, 0, 0, 255));
    display_.renderAll(canvas);
    canvas.popClip();
}

} // namespace trakr

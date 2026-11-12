#include "config.h"

#include "Song.h"
#include "EngineHost.h"
#include "Theme.h"
#include "views/ConfigView.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

namespace trakr {
using namespace ::trakr::ui;

namespace {
constexpr int kListX = 1;
constexpr int kListY = 29;
constexpr int kListW = 800 / 4 - 3;          // 197
constexpr int kListH = 394;
constexpr int kSubmenuX = (800 / 8) * 2 + 1; // 201
constexpr int kSubmenuW = 800 / 2 - 3;       // 397

// framesPlacesConfig[4][4].
constexpr int kFramePlaces[4][4] = {
    {1, 31, kListW, 391},
    {kSubmenuX, 31, kSubmenuW, 391},
    {(800 / 8) * 5 + 1, 31, kListW, 391},
    {(800 / 8) * 5 + 1, 31, 600 / 2 - 3, 391},
};
} // namespace

ConfigScreen::ConfigScreen() = default;

void ConfigScreen::build() {
    if (built_) return;

    std::vector<RectI> places;
    for (const auto& p : kFramePlaces) places.push_back(RectI{p[0], p[1], p[2], p[3]});
    frame_ = &display_.add<FrameControl>("frame", std::move(places), 0);

    std::vector<std::string> groups;
    for (int i = 0; i < ConfigView::kGroupCount; ++i)
        groups.emplace_back(ConfigView::groupName(i));
    baseList_ = &display_.add<ListControl>("configBasemenuList", std::move(groups), 14);
    baseList_->setRect(RectI{kListX, kListY, kListW, kListH});
    baseList_->setStart(0);

    submenuList_ = &display_.add<ParamValueListControl>("configSubmenuList", 14);
    submenuList_->setRect(RectI{kSubmenuX, kListY, kSubmenuW, kListH});

    // blankSpaceCover[0] ({601,29,198,394}) and [1] ({201,29,198,394}) are
    // created last so the reverse composite paints them first (under the lists).
    blankCoverA_ = &display_.add<BlankCoverControl>("blankSpaceCover0");
    blankCoverA_->setRect(RectI{(800 / 8) * 6 + 1, kListY, 800 / 4 - 2, kListH});
    blankCoverB_ = &display_.add<BlankCoverControl>("blankSpaceCover1");
    blankCoverB_->setRect(RectI{(800 / 8) * 2 + 1, kListY, 800 / 4 - 2, kListH});

    baseList_->setOnSelect([this](int index) {
        if (view_) view_->selectGroup(index);
    });
    submenuList_->setOnSelect([this](int index) {
        if (view_) view_->selectItem(index);
    });

    built_ = true;
}

void ConfigScreen::syncFromModel(const Song& song, const ConfigView& view) {
    baseList_->setSelected(static_cast<int>(view.activeGroup()));
    std::vector<ParamValueListControl::Row> rows;
    for (const auto& row : view.currentRows(song)) rows.emplace_back(row.name, row.value);
    submenuList_->setRows(std::move(rows));
    submenuList_->setSelected(view.selectedItem());
    // cParamValueList list->start tracks the selected row while it is edited.
    if (view.selectedPlace() == 1) submenuList_->setStart(view.selectedItem());
    frame_->setSelected(view.selectedPlace());
}

void ConfigScreen::update(Song& song,
                          EngineHost& engineHost,
                          Theme& theme,
                          ConfigView& view,
                          const ImRect& workspace,
                          ui::LayoutProfile profile) {
    build();
    view_ = &view;

    ImGui::SetCursorScreenPos(workspace.Min);
    ImGui::InvisibleButton("##gui_next_config_workspace", workspace.GetSize());
    const bool hovered = ImGui::IsItemHovered();
    const bool keyTarget =
        ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) || !ImGui::IsAnyItemActive();

    if (keyTarget && !ImGui::GetIO().WantTextInput) {
        if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) view.moveSelection(-1);
        if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) view.moveSelection(+1);
        if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) view.switchGroup(-1);
        if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) view.switchGroup(+1);
        if (ImGui::IsKeyPressed(ImGuiKey_Minus) || ImGui::IsKeyPressed(ImGuiKey_KeypadSubtract)) {
            view.setSelectedPlace(1);
            view.adjustSelectedValue(song, engineHost, theme, -1);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Equal) || ImGui::IsKeyPressed(ImGuiKey_KeypadAdd)) {
            view.setSelectedPlace(1);
            view.adjustSelectedValue(song, engineHost, theme, +1);
        }
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
    }

    syncFromModel(song, view);
}

void ConfigScreen::render(ImDrawList* drawList,
                          const ImRect& workspace,
                          ui::LayoutProfile profile) const {
    if (!built_ || drawList == nullptr) return;
    const LayoutContext lc = LayoutContext::fromWorkspace(workspace, profile);
    Canvas canvas(drawList, lc);
    canvas.pushClip(RectI{0, kWorkspaceTop, kDeviceWidth, kWorkspaceSpanY});
    display_.renderAll(canvas);
    canvas.popClip();
}

} // namespace trakr

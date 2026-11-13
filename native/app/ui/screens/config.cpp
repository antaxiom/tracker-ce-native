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

    // Item editor controls (gConfigEdit.cpp: configListControl at
    // (800/8)*4+1 width (800/8)*2-3, configLabel at (800/8)*2+15,27, and
    // descriptionTextBox at (800/8)*2+10,60 width 800/4-15 height 365).
    // Created before the blank covers so they paint on top of them.
    editList_ = &display_.add<ListControl>("configListControl", std::vector<std::string>{}, 14);
    editList_->setRect(RectI{(800 / 8) * 4 + 1, kListY, (800 / 8) * 2 - 3, kListH});
    editList_->setVisible(false);
    editorTitle_ = &display_.add<LabelControl>("configLabel", "", trakr::hw::FwFont::Regular14);
    editorTitle_->setRect(RectI{(800 / 8) * 2 + 15, 27, 800 / 4 - 18, 55});
    editorTitle_->setShowBackground(false);
    editorTitle_->setAlignLeft(true);
    editorTitle_->setVisible(false);
    editorDescription_ =
        &display_.add<TextBoxControl>("descriptionTextBox", trakr::hw::FwFont::Regular12, 3);
    editorDescription_->setRect(RectI{(800 / 8) * 2 + 10, 60, 800 / 4 - 15, 365});
    editorDescription_->setVisible(false);

    // blankSpaceCover[0] ({601,29,198,394}) and [1] ({201,29,198,394}) are
    // created last so the reverse composite paints them first (under the lists).
    blankCoverA_ = &display_.add<BlankCoverControl>("blankSpaceCover0");
    blankCoverA_->setRect(RectI{(800 / 8) * 6 + 1, kListY, 800 / 4 - 2, kListH});
    blankCoverB_ = &display_.add<BlankCoverControl>("blankSpaceCover1");
    blankCoverB_->setRect(RectI{(800 / 8) * 2 + 1, kListY, 800 / 4 - 2, kListH});

    baseList_->setOnSelect([this](int index) {
        if (!view_) return;
        if (view_->editing()) view_->cancelEditor();
        view_->selectGroup(index);
    });
    submenuList_->setOnSelect([this](int index) {
        if (view_) view_->selectItem(index);
    });
    editList_->setOnSelect([this](int index) {
        if (view_) view_->setEditorSelection(index);
    });

    built_ = true;
}

void ConfigScreen::syncFromModel(const Song& song, const ConfigView& view) {
    baseList_->setSelected(static_cast<int>(view.activeGroup()));
    std::vector<ParamValueListControl::Row> rows;
    if (view.editing()) {
        // gConfigEdit.cpp itemEditorOpen(): hideSubmenu() + showConfigItemLabel()
        // + showConfigItemDesctription() + showConfigList4().
        const auto& editor = view.editor();
        submenuList_->setVisible(false);
        baseList_->setSelected(static_cast<int>(view.activeGroup()));
        editList_->setVisible(true);
        editList_->setItems(editor.options);
        editList_->setStart(editor.selected);
        editList_->setSelected(editor.selected);
        editorTitle_->setVisible(true);
        editorTitle_->setText(editor.title);
        editorDescription_->setVisible(!editor.description.empty());
        editorDescription_->setText(editor.description);
        frame_->setSelected(1); // frameData.startPlace stays on the submenu place
        return;
    }

    submenuList_->setVisible(true);
    editList_->setVisible(false);
    editorTitle_->setVisible(false);
    editorDescription_->setVisible(false);
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

    // --config-edit <item>: the CLI requests an editor state for captures.
    if (!view.pendingEditor().empty()) {
        const std::string name = view.pendingEditor();
        view.clearPendingEditor();
        view.openEditorForName(name, song, engineHost);
    }

    ImGui::SetCursorScreenPos(workspace.Min);
    ImGui::InvisibleButton("##gui_next_config_workspace", workspace.GetSize());
    const bool hovered = ImGui::IsItemHovered();
    const bool keyTarget =
        ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) || !ImGui::IsAnyItemActive();

    if (keyTarget && !ImGui::GetIO().WantTextInput) {
        if (view.editing()) {
            if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) view.moveEditorSelection(-1);
            if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) view.moveEditorSelection(+1);
            if (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter))
                view.applyEditor(song, engineHost);
            // functLeft(): a left press closes the item editor (Cancel).
            if (ImGui::IsKeyPressed(ImGuiKey_Escape) || ImGui::IsKeyPressed(ImGuiKey_LeftArrow))
                view.cancelEditor();
        } else {
            if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) view.moveSelection(-1);
            if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) view.moveSelection(+1);
            if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) view.switchGroup(-1);
            if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) view.switchGroup(+1);
            if (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter))
                view.openEditor(song, engineHost);
            if (ImGui::IsKeyPressed(ImGuiKey_Minus) || ImGui::IsKeyPressed(ImGuiKey_KeypadSubtract)) {
                view.setSelectedPlace(1);
                view.adjustSelectedValue(song, engineHost, theme, -1);
            }
            if (ImGui::IsKeyPressed(ImGuiKey_Equal) || ImGui::IsKeyPressed(ImGuiKey_KeypadAdd)) {
                view.setSelectedPlace(1);
                view.adjustSelectedValue(song, engineHost, theme, +1);
            }
        }
    }

    if (hovered && !view.editing()) {
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
    // cLabel/cTextBox colors: configItemEditorTextLabelColors[0].
    const ImU32 editorText = ImGui::ColorConvertFloat4ToU32(theme.configItemEditorTextLabelColors[0]);
    editorTitle_->setColors(editorText, editorText, editorText);
    editorDescription_->setColor(editorText);
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

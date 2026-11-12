#include "perform.h"

#include "Song.h"
#include "EngineHost.h"
#include "views/PerformView.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace trakr {
using namespace ::trakr::ui;

namespace {
constexpr int kColumnCount = 12;
constexpr int kColumnX = 42;
constexpr int kColumnY = 34;
constexpr int kColumnW = 62;
constexpr int kColumnH = 174 + 210 + 5; // 389
constexpr int kColumnStride = 65;
constexpr int kFxNameY = 210 + 34;      // textLabel[i] posY

// gPerformance.cpp framesPlaces[12][4] verbatim.
constexpr int kFramePlaces[kColumnCount][4] = {
    {11, 31, 62, 391},          {11 + 65 * 1, 31, 62, 391},  {11 + 65 * 2, 31, 62, 391},
    {11 + 65 * 3, 31, 62, 391}, {11 + 65 * 4, 31, 62, 391},  {11 + 65 * 5, 31, 62, 391},
    {11 + 65 * 6, 31, 62, 391}, {11 + 65 * 7, 31, 62, 391},  {11 + 65 * 8, 31, 62, 391},
    {11 + 65 * 9, 31, 62, 391}, {11 + 65 * 10, 31, 62, 391}, {11 + 65 * 11, 31, 62, 391},
};
} // namespace

PerformScreen::PerformScreen() = default;

void PerformScreen::build() {
    if (built_) return;

    std::vector<RectI> places;
    places.reserve(kColumnCount);
    for (const auto& p : kFramePlaces) places.push_back(RectI{p[0], p[1], p[2], p[3]});
    frame_ = &display_.add<FrameControl>("frame", std::move(places), 0);
    frame_->setVisible(false);

    for (int i = 0; i < kColumnCount; ++i) {
        const int cx = kColumnX + kColumnStride * i;
        names_[i] = &display_.add<VerticalLabelControl>("textLabel" + std::to_string(i));
        names_[i]->setRect(RectI{cx, 0, 0, 0});
        names_[i]->setBottomY(kFxNameY);
        values_[i] = &display_.add<MultiLabelControl>("value1Label" + std::to_string(i));
        // cMultiLabel uses controlStyleCenterX: prop.x = 42 + 65*i is the centre.
        values_[i]->setRect(RectI{cx - kColumnW / 2, kColumnY, kColumnW, kColumnH});

        values_[i]->setOnTop([this, i]() {
            if (view_) view_->selectTop(i, *engineHost_);
        });
        values_[i]->setOnRow([this, i](int row) {
            if (view_) view_->selectRow(i, row, *engineHost_);
        });
    }

    built_ = true;
}

void PerformScreen::syncFromModel(const PerformView& view) {
    const bool editing = view.performanceEditPlace() >= 0;
    for (int i = 0; i < kColumnCount; ++i) {
        const PerformSlot12& slot = view.slots()[static_cast<size_t>(i)];
        MultiLabelState state;
        state.labels = slot.labels;
        state.activeRow = slot.activeRow;
        state.topText = "---";
        state.editing = editing;
        values_[i]->setState(state);
        names_[i]->setText(slot.name);
        names_[i]->setColor(kPerfFxNameTextColor);
    }
    const int place = view.performanceEditPlace();
    frame_->setVisible(place >= 0);
    if (place >= 0) frame_->setSelected(place);
}

void PerformScreen::update(Song& song,
                           History& history,
                           EngineHost& engineHost,
                           PerformView& view,
                           const ImRect& workspace,
                           ui::LayoutProfile profile) {
    (void)song;
    (void)history;
    build();
    view_ = &view;
    engineHost_ = &engineHost;

    ImGui::SetCursorScreenPos(workspace.Min);
    ImGui::InvisibleButton("##gui_next_perform_workspace", workspace.GetSize());

    if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
        const LayoutContext lc = LayoutContext::fromWorkspace(workspace, profile);
        const ImVec2 device = lc.screenToDevice(ImGui::GetIO().MousePos);
        const int dx = static_cast<int>(std::floor(device.x));
        const int dy = static_cast<int>(std::floor(device.y));
        if (ui::Control* hit = display_.hitTest(dx, dy)) hit->onPress(dx, dy);
    }

    syncFromModel(view);
}

void PerformScreen::render(ImDrawList* drawList,
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

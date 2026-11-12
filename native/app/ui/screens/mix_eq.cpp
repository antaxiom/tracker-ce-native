#include "mix_eq.h"

#include "Song.h"
#include "History.h"
#include "EngineHost.h"
#include "views/MixerView.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace trakr {
using namespace ::trakr::ui;

namespace {
constexpr int kColumnWidth = 800 / 8; // 100
constexpr int kEqBarY = 29;
constexpr int kEqBarW = 800 / 8 - 3;    // 97
constexpr int kEqBarH = 394;
constexpr int kCoverX = 800 / 8 * 5 + 2; // 502

constexpr int kFramePlaces[5][4] = {
    {1, 31, kEqBarW, 391},   {101, 31, kEqBarW, 391},
    {201, 31, kEqBarW, 391}, {301, 31, kEqBarW, 391},
    {401, 31, kEqBarW, 391},
};
} // namespace

MasterEqScreen::MasterEqScreen() = default;

void MasterEqScreen::build() {
    if (built_) return;

    std::vector<RectI> places;
    places.reserve(5);
    for (int i = 0; i < 5; ++i)
        places.push_back(RectI{kFramePlaces[i][0], kFramePlaces[i][1], kFramePlaces[i][2],
                               kFramePlaces[i][3]});
    frame_ = &display_.add<FrameControl>("frame", std::move(places), 0);

    for (int i = 0; i < 5; ++i) {
        band_[i] = &display_.add<BarControl>("eqBand" + std::to_string(i), i,
                                             BarControl::Mode::Fill);
        band_[i]->setRect(RectI{kColumnWidth * i + 2, kEqBarY, kEqBarW, kEqBarH});
        band_[i]->setOnPress([this](int column) {
            if (view_) view_->setSelectedTrack(column);
        });
    }
    cover_ = &display_.add<BlankCoverControl>("blankSpaceCover");
    cover_->setRect(RectI{kCoverX, kEqBarY, 800 - kCoverX, kEqBarH});

    built_ = true;
}

void MasterEqScreen::syncFromModel(const Song& song, const MixerView& view) {
    for (int i = 0; i < 5; ++i) {
        const int value = std::clamp(
            static_cast<int>(std::lround((song.masterEq[i] + 12.0f) / 24.0f * 100.0f)), 0, 100);
        band_[i]->setValue(value);
    }
    frame_->setSelected(std::clamp(view.selectedTrack(), 0, 4));
}

const char* MasterEqScreen::bandLabel(int band) {
    static const char* const kLabels[5] = {"115 Hz", "330 Hz", "990 Hz", "3000 Hz", "9900 Hz"};
    return (band >= 0 && band < 5) ? kLabels[band] : "";
}

void MasterEqScreen::update(Song& song,
                            History& history,
                            EngineHost& engineHost,
                            MixerView& view,
                            const ImRect& workspace,
                            ui::LayoutProfile profile,
                            bool shiftHeld) {
    (void)history;
    build();
    view_ = &view;

    ImGui::SetCursorScreenPos(workspace.Min);
    ImGui::InvisibleButton("##gui_next_master_eq_workspace", workspace.GetSize());
    const bool hovered = ImGui::IsItemHovered();
    const bool keyTarget =
        ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) || !ImGui::IsAnyItemActive();
    const bool fine = shiftHeld || ImGui::GetIO().KeyShift;

    if (keyTarget && !ImGui::GetIO().WantTextInput) {
        if (ImGui::IsKeyPressed(ImGuiKey_Tab)) view.togglePage();
        if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) view.selectNextTrack(-1);
        if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) view.selectNextTrack(+1);
        if (ImGui::IsKeyPressed(ImGuiKey_UpArrow))
            view.adjustSelectedParam(song, engineHost, +1, fine);
        if (ImGui::IsKeyPressed(ImGuiKey_DownArrow))
            view.adjustSelectedParam(song, engineHost, -1, fine);
        if (ImGui::IsKeyPressed(ImGuiKey_Delete) || ImGui::IsKeyPressed(ImGuiKey_Backspace))
            view.resetSelected(song, engineHost);
    }

    if (hovered) {
        const float wheel = ImGui::GetIO().MouseWheel;
        if (std::abs(wheel) > 0.01f)
            view.adjustSelectedParam(song, engineHost, (wheel > 0.0f) ? +1 : -1, fine);
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

void MasterEqScreen::render(ImDrawList* drawList,
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

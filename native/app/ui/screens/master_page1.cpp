#include "master_page1.h"

#include "Song.h"
#include "EngineHost.h"
#include "views/MixerView.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace trakr {
using namespace ::trakr::ui;

namespace {

constexpr int kColumnWidth = kDeviceWidth / 8; // 100
constexpr int kMasterBarY = 29;
constexpr int kMasterBarW = kDeviceWidth / 8 - 3; // 97
constexpr int kMasterBarH = 394;

constexpr int kFramePlaces[8][4] = {
    {1, 31, kMasterBarW, 391},   {101, 31, kMasterBarW, 391},
    {201, 31, kMasterBarW, 391}, {301, 31, kMasterBarW, 391},
    {401, 31, kMasterBarW, 391}, {501, 31, kMasterBarW, 391},
    {601, 31, kMasterBarW, 391}, {701, 31, kMasterBarW, 391},
};

const char* const kSaturationNames[5] = {
    "Hard Clip", "Soft Clip", "Saturate", "Overdrive", "Drive+",
};

} // namespace

MasterPage1Screen::MasterPage1Screen() = default;

void MasterPage1Screen::build() {
    if (built_) return;

    std::vector<RectI> places;
    places.reserve(8);
    for (int i = 0; i < 8; ++i) {
        places.push_back(RectI{kFramePlaces[i][0], kFramePlaces[i][1], kFramePlaces[i][2],
                               kFramePlaces[i][3]});
    }
    frame_ = &display_.add<FrameControl>("frame", std::move(places), 0);

    // showMasterScreen(): barControl[i] exists for all columns; only volume,
    // bass boost and space are shown. Columns 1..4 are covered by one label.
    for (int i = 0; i < 8; ++i) {
        auto& bar = display_.add<BarControl>("bar" + std::to_string(i), i, BarControl::Mode::Fill);
        bar.setRect(RectI{kColumnWidth * i + 2, kMasterBarY, kMasterBarW, kMasterBarH});
        bar.setVisible(i == 0 || i == 6 || i == 7);
        bars_[i] = &bar;
    }

    auto& cover = display_.add<BlankCoverControl>("blankSpaceCover");
    cover.setRect(RectI{kColumnWidth + 2, kMasterBarY, 4 * kColumnWidth - 3, kMasterBarH});

    std::vector<std::string> items(kSaturationNames, kSaturationNames + 5);
    saturationList_ = &display_.add<ListControl>("saturation", std::move(items), 5);
    saturationList_->setRect(RectI{5 * kColumnWidth + 2, kMasterBarY, kMasterBarW, kMasterBarH});

    for (int i = 0; i < 8; ++i) {
        bars_[i]->setOnPress([this](int column) {
            if (view_) view_->setSelectedTrack(column);
        });
    }
    saturationList_->setOnSelect([this](int index) {
        if (!song_ || !engineHost_ || !view_) return;
        song_->masterSaturationType = index;
        view_->setSelectedTrack(5);
        engineHost_->publish(*song_);
    });

    built_ = true;
}

void MasterPage1Screen::syncFromModel(const Song& song, const MixerView& view) {
    int volume = static_cast<int>(std::lround((song.masterGainDb + 51.5f) / 63.5f * 100.0f));
    volume = std::clamp(volume, 0, 100);
    if (std::abs(song.masterGainDb) < 0.005f) volume = 81; // MASTER_VOLUME_DEFAULT
    bars_[0]->setValue(volume);

    bars_[6]->setValue(std::clamp(static_cast<int>(std::lround(song.masterBassBoost * 100.0f)),
                                  0, 100));
    const int space8 = std::clamp(static_cast<int>(std::lround(song.masterSpace * 8.0f)), 0, 8);
    bars_[7]->setValue(space8 * 100 / 8);

    saturationList_->setSelected(std::clamp(song.masterSaturationType, 0, 4));
    frame_->setSelected(std::clamp(view.selectedTrack(), 0, 7));
}

void MasterPage1Screen::update(Song& song,
                               EngineHost& engineHost,
                               MixerView& view,
                               const ImRect& workspace,
                               ui::LayoutProfile profile,
                               bool shiftHeld) {
    build();
    song_ = &song;
    engineHost_ = &engineHost;
    view_ = &view;

    ImGui::SetCursorScreenPos(workspace.Min);
    ImGui::InvisibleButton("##gui_next_master_workspace", workspace.GetSize());
    const bool hovered = ImGui::IsItemHovered();
    const bool keyTarget =
        ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) || !ImGui::IsAnyItemActive();
    const bool fine = shiftHeld || ImGui::GetIO().KeyShift;

    if (keyTarget && !ImGui::GetIO().WantTextInput) {
        if (ImGui::IsKeyPressed(ImGuiKey_Tab)) view.togglePage();
        const int selected = view.selectedTrack();
        if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow, false))
            view.setSelectedTrack(std::max(0, selected - 1));
        if (ImGui::IsKeyPressed(ImGuiKey_RightArrow, false))
            view.setSelectedTrack(std::min(7, selected + 1));
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
        if (ui::Control* hit = display_.hitTest(static_cast<int>(std::floor(device.x)),
                                                static_cast<int>(std::floor(device.y)))) {
            hit->onPress(static_cast<int>(std::floor(device.x)),
                         static_cast<int>(std::floor(device.y)));
        }
    }

    syncFromModel(song, view);
}

void MasterPage1Screen::render(ImDrawList* drawList,
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

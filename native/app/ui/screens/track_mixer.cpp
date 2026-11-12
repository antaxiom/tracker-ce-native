#include "track_mixer.h"

#include "Song.h"
#include "History.h"
#include "EngineHost.h"
#include "views/MixerView.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace trakr {
using namespace ::trakr::ui;

namespace {
constexpr int kColumnWidth = 800 / 8;      // 100
constexpr int kMixerBarY = 64;
constexpr int kMixerBarH = 359;
constexpr int kMixerBarW = 800 / 16 - 3;   // 47
constexpr int kTrackCount = 8;
} // namespace

TrackMixerScreen::TrackMixerScreen() = default;

void TrackMixerScreen::build() {
    if (built_) return;

    for (int i = 0; i < kTrackCount; ++i) {
        name_[i] = &display_.add<LabelControl>("trackName" + std::to_string(i), "",
                                              trakr::hw::FwFont::Bold17);
        name_[i]->setRect(RectI{kColumnWidth * i + 800 / 16 - 48, 32, 96, 26});
        name_[i]->setColors(kTopBarLabelColors[0], kTopBarLabelColors[1],
                            kTopBarLabelColors[2]);

        // showLevelBar(): level meter sits right of the gain fader.
        level_[i] = &display_.add<BarControl>("trackLevel" + std::to_string(i), i,
                                              BarControl::Mode::Fill);
        level_[i]->setRect(RectI{kColumnWidth * i + 2 + 800 / 16, kMixerBarY, kMixerBarW,
                                 kMixerBarH});
        level_[i]->setValue(0);
        level_[i]->setOnPress([this](int column) {
            if (view_) view_->setSelectedTrack(column);
        });

        // showTrackVolumeBar(): gain fader, mute uses inactiveBarColors.
        volume_[i] = &display_.add<BarControl>("trackVolume" + std::to_string(i), i,
                                               BarControl::Mode::Fill);
        volume_[i]->setRect(RectI{kColumnWidth * i + 2, kMixerBarY, kMixerBarW, kMixerBarH});
        volume_[i]->setOnPress([this](int column) {
            if (view_) view_->setSelectedTrack(column);
        });
    }

    built_ = true;
}

void TrackMixerScreen::syncFromModel(const Song& song, const MixerView& view) {
    for (int i = 0; i < kTrackCount; ++i) {
        const bool has = i < static_cast<int>(song.tracks.size());
        if (has) {
            const auto& track = song.tracks[static_cast<size_t>(i)];
            name_[i]->setText(track.name.empty() ? "Track " + std::to_string(i + 1)
                                                 : track.name);
            volume_[i]->setValue(MixerView::gainBarValue(track.gainDb));
            volume_[i]->setInactive(track.mute);
        } else {
            name_[i]->setText("Track " + std::to_string(i + 1));
            volume_[i]->setValue(0);
            volume_[i]->setInactive(false);
        }
        level_[i]->setValue(0); // showLevelBar feeds live trackLevel; idle is empty
    }
    (void)view;
}

void TrackMixerScreen::update(Song& song,
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
    ImGui::InvisibleButton("##gui_next_track_mixer_workspace", workspace.GetSize());
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

void TrackMixerScreen::render(ImDrawList* drawList,
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

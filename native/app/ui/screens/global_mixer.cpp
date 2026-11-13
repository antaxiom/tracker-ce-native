#include "global_mixer.h"

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
constexpr int kColumnWidth = 800 / 8;      // 100
constexpr int kAuxNameY = 32;
constexpr int kAuxNameH = 26;
constexpr int kAuxNameW = 800 / 8 - 4;     // 96
constexpr int kAuxBarY = 64;
constexpr int kAuxBarH = 359;
constexpr int kAuxCount = 4;

// gMasterParams.cpp initDisplayControls(): three 32px bars per aux column -
// volume fader at x + 2 and the two level bars at x + 34 / x + 66.
constexpr int kAuxVolumeW = 32;

constexpr int kFramePlaces[4][4] = {
    {1, 31, kColumnWidth - 2, 391},
    {kColumnWidth + 1, 31, kColumnWidth - 2, 391},
    {kColumnWidth * 2 + 1, 31, kColumnWidth - 2, 391},
    {kColumnWidth * 3 + 1, 31, kColumnWidth - 2, 391},
};

const char* auxLabel(int aux) {
    static const char* const kLabels[4] = {"Delay", "Reverb", "Dry Mix", "Line In"};
    return (aux >= 0 && aux < 4) ? kLabels[aux] : "";
}
} // namespace

GlobalMixerScreen::GlobalMixerScreen() = default;

void GlobalMixerScreen::build() {
    if (built_) return;

    std::vector<RectI> places;
    places.reserve(kAuxCount);
    for (int i = 0; i < kAuxCount; ++i)
        places.push_back(RectI{kFramePlaces[i][0], kFramePlaces[i][1], kFramePlaces[i][2],
                               kFramePlaces[i][3]});
    frame_ = &display_.add<FrameControl>("frame", std::move(places), 0);

    for (int i = 0; i < kAuxCount; ++i) {
        // gMasterParams.cpp: the aux names are controlStyleCenterX labels on
        // topBarLabelColors, so x is the centre.
        name_[i] = &display_.add<LabelControl>("auxName" + std::to_string(i), auxLabel(i),
                                              trakr::hw::FwFont::Bold17);
        name_[i]->setRect(RectI{kColumnWidth * i + 800 / 16 - kAuxNameW / 2, kAuxNameY,
                                kAuxNameW, kAuxNameH});
        name_[i]->setColors(kTopBarLabelColors[0], kTopBarLabelColors[1],
                            kTopBarLabelColors[2]);

        volume_[i] = &display_.add<BarControl>("auxVolume" + std::to_string(i), i,
                                               BarControl::Mode::Fill);
        volume_[i]->setRect(RectI{kColumnWidth * i + 2, kAuxBarY, kAuxVolumeW, kAuxBarH});
        volume_[i]->setOnPress([this](int column) {
            if (view_) view_->setSelectedTrack(column);
        });

        levelL_[i] = &display_.add<BarControl>("auxLevelL" + std::to_string(i), i,
                                               BarControl::Mode::Fill);
        levelL_[i]->setRect(RectI{kColumnWidth * i + 2 + 32, kAuxBarY, kAuxVolumeW, kAuxBarH});
        levelL_[i]->setOnPress([this](int column) {
            if (view_) view_->setSelectedTrack(column);
        });

        levelR_[i] = &display_.add<BarControl>("auxLevelR" + std::to_string(i), i,
                                               BarControl::Mode::Fill);
        levelR_[i]->setRect(RectI{kColumnWidth * i + 2 + 64, kAuxBarY, kAuxVolumeW, kAuxBarH});
        levelR_[i]->setOnPress([this](int column) {
            if (view_) view_->setSelectedTrack(column);
        });
    }

    built_ = true;
}

void GlobalMixerScreen::syncFromModel(const Song& song, const MixerView& view,
                                      const EngineHost& engineHost) {
    for (int i = 0; i < kAuxCount; ++i) {
        // Muted buses use the inactive palettes (gMasterParams.cpp showGlobalMixer).
        const bool muted = song.auxMute[i];
        name_[i]->setColors(muted ? kInactiveLabelsColors[0] : kActiveLabelsColors[0],
                            kActiveLabelsColors[1], kActiveLabelsColors[2]);
        volume_[i]->setValue(std::clamp(song.auxVolume[i], 0, 100));
        volume_[i]->setInactive(muted);

        const MeterLevels meters = engineHost.getAuxMeters(i);
        auto toBar = [](float peak) {
            // Linear peak -> the mixer's dB fader scale (silence at 0).
            if (peak <= 0.0000001f) return 0;
            const float db = 20.0f * std::log10(peak);
            return std::clamp(faderValueForGainDb(db), 0, 100);
        };
        levelL_[i]->setValue(toBar(meters.peakL));
        levelR_[i]->setValue(toBar(meters.peakR));
    }
    frame_->setSelected(std::clamp(view.selectedTrack(), 0, kAuxCount - 1));
}

void GlobalMixerScreen::update(Song& song,
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
    ImGui::InvisibleButton("##gui_next_global_mixer_workspace", workspace.GetSize());
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
            view.toggleAuxMute(song, engineHost, view.selectedTrack());
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
        if (dx >= 0 && dx < 800) view.setSelectedTrack(std::clamp(dx / kColumnWidth, 0, 3));
    }

    syncFromModel(song, view, engineHost);
}

void GlobalMixerScreen::render(ImDrawList* drawList, const ImRect& workspace,
                               ui::LayoutProfile profile) const {
    const LayoutContext lc = LayoutContext::fromWorkspace(workspace, profile);
    Canvas canvas(drawList, lc);
    display_.renderAll(canvas);
}

} // namespace trakr

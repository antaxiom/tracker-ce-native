#include "song.h"

#include "Command.h"
#include "History.h"
#include "Song.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

namespace trakr {
using namespace ::trakr::ui;

namespace {
constexpr int kColumnWidth = kDeviceWidth / 8; // 100
constexpr int kTempoX = kColumnWidth * 7 + 1;  // 701
constexpr int kTempoW = kColumnWidth - 3;     // 97
constexpr int kTempoY = 29;
constexpr int kTempoH = 394;
constexpr int kPlayerX = 1;
constexpr int kPlayerW = kColumnWidth * 7 - 3; // 697

// gSongEdit.cpp framesPlaces[3][4].
constexpr int kFramePlaces[3][4] = {
    {1, 31, 500 - 3, 391},
    {kColumnWidth * 5 + 1, 31, kDeviceWidth / 4 - 3, 391},
    {kTempoX, 31, kTempoW, 391},
};
} // namespace

SongScreen::SongScreen() = default;

void SongScreen::build() {
    if (built_) return;

    std::vector<RectI> places;
    places.reserve(3);
    for (const auto& p : kFramePlaces) places.push_back(RectI{p[0], p[1], p[2], p[3]});
    frame_ = &display_.add<FrameControl>("frame", std::move(places), 2);
    frame_->setVisible(false);

    tempoBar_ = &display_.add<BarControl>("tempoBar", 7, BarControl::Mode::Fill);
    tempoBar_->setRect(RectI{kTempoX, kTempoY, kTempoW, kTempoH});

    player_ = &display_.add<SongPlayerControl>("songPlayer");
    player_->setRect(RectI{kPlayerX, kTempoY, kPlayerW, kTempoH});

    player_->setOnSelect([this](int slot) {
        if (currentOrderIdx_) *currentOrderIdx_ = slot;
    });

    built_ = true;
}

void SongScreen::syncFromModel(const Song& song, EngineHost& engineHost, int currentOrderIdx) {
    SongPlayerState state;
    state.song = &song;
    state.selectedSlot = currentOrderIdx;
    state.trackCount = static_cast<int>(song.tracks.size());
    state.playhead = engineHost.getPlayhead();
    state.playing = state.playhead.playing;
    state.listActive = true; // the app song list is always the selected place
    state.editMode = false;
    state.startNumbersFrom1 = true;
    player_->setState(state);

    tempoBar_->setValue(std::clamp(static_cast<int>(song.bpm * 100.0f / 800.0f), 0, 100));
    // showDefaultScreen() leaves the frame hidden (frameData.startPlace = 2);
    // it appears only when a place is activated. The app song screen is always
    // on the list place, so no frame is drawn.
    frame_->setVisible(false);
}

void SongScreen::update(Song& song,
                        History& history,
                        EngineHost& engineHost,
                        int& currentOrderIdx,
                        const ImRect& workspace,
                        ui::LayoutProfile profile) {
    build();
    if (song.order.empty()) song.order.push_back(0);

    song_ = &song;
    history_ = &history;
    engineHost_ = &engineHost;
    currentOrderIdx_ = &currentOrderIdx;
    currentOrderIdx = std::clamp(currentOrderIdx, 0, static_cast<int>(song.order.size()) - 1);

    ImGui::SetCursorScreenPos(workspace.Min);
    ImGui::InvisibleButton("##gui_next_song_workspace", workspace.GetSize());
    const bool hovered = ImGui::IsItemHovered();
    const bool keyTarget =
        ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) || !ImGui::IsAnyItemActive();

    auto commitOrder = [&](std::vector<int> next) {
        if (next == song.order) return;
        history.execute(std::make_unique<SetOrderCommand>(std::move(next), song.order), song);
        engineHost.publish(song);
    };

    if (keyTarget && !ImGui::GetIO().WantTextInput) {
        if (ImGui::IsKeyPressed(ImGuiKey_UpArrow) && currentOrderIdx > 0) --currentOrderIdx;
        if (ImGui::IsKeyPressed(ImGuiKey_DownArrow) &&
            currentOrderIdx + 1 < static_cast<int>(song.order.size())) {
            ++currentOrderIdx;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow) || ImGui::IsKeyPressed(ImGuiKey_Minus) ||
            ImGui::IsKeyPressed(ImGuiKey_KeypadSubtract)) {
            if (song.order[static_cast<size_t>(currentOrderIdx)] > 0) {
                std::vector<int> next = song.order;
                next[static_cast<size_t>(currentOrderIdx)]--;
                commitOrder(std::move(next));
            }
        }
        if (ImGui::IsKeyPressed(ImGuiKey_RightArrow) || ImGui::IsKeyPressed(ImGuiKey_Equal) ||
            ImGui::IsKeyPressed(ImGuiKey_KeypadAdd)) {
            if (song.order[static_cast<size_t>(currentOrderIdx)] + 1 <
                static_cast<int>(song.patterns.size())) {
                std::vector<int> next = song.order;
                next[static_cast<size_t>(currentOrderIdx)]++;
                commitOrder(std::move(next));
            }
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Insert)) {
            std::vector<int> next = song.order;
            next.insert(next.begin() + currentOrderIdx + 1,
                        song.order[static_cast<size_t>(currentOrderIdx)]);
            commitOrder(std::move(next));
            ++currentOrderIdx;
        }
        if ((ImGui::IsKeyPressed(ImGuiKey_Delete) || ImGui::IsKeyPressed(ImGuiKey_Backspace)) &&
            song.order.size() > 1) {
            std::vector<int> next = song.order;
            next.erase(next.begin() + currentOrderIdx);
            commitOrder(std::move(next));
            currentOrderIdx = std::max(0, currentOrderIdx - 1);
        }
    }

    if (hovered) {
        const float wheel = ImGui::GetIO().MouseWheel;
        const int n = static_cast<int>(song.order.size());
        if (std::abs(wheel) > 0.01f) {
            currentOrderIdx =
                std::clamp(currentOrderIdx - static_cast<int>(std::round(wheel)), 0, n - 1);
        }
    }

    if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
        const LayoutContext lc = LayoutContext::fromWorkspace(workspace, profile);
        const ImVec2 device = lc.screenToDevice(ImGui::GetIO().MousePos);
        const int dx = static_cast<int>(std::floor(device.x));
        const int dy = static_cast<int>(std::floor(device.y));
        if (ui::Control* hit = display_.hitTest(dx, dy)) hit->onPress(dx, dy);
    }

    syncFromModel(song, engineHost, currentOrderIdx);
}

void SongScreen::render(ImDrawList* drawList,
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

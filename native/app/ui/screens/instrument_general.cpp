#include "instrument_general.h"

#include "Song.h"
#include "History.h"
#include "EngineHost.h"
#include "views/InstrumentView.h"

#include <cmath>
#include <string>
#include <vector>

namespace trakr {
using namespace ::trakr::ui;
using ui::BarControl;
using ui::BlankCoverControl;
using ui::BgLabelControl;
using ui::Canvas;
using ui::FrameControl;
using ui::LabelControl;
using ui::LayoutContext;
using ui::ListControl;
using ui::RectI;

namespace {

constexpr int kColumnWidth = kDeviceWidth / 8; // 100

// gInstrEdit.cpp refreshFramePosition() for the Parameters page: 7 places
// (position 7 / "More" is not framed; activateLabelsBorder refuses it).
constexpr int kFramePlaces[7][4] = {
    {1, 31, kBarWidth, 391},   {101, 31, kBarWidth, 391}, {201, 31, kBarWidth, 391},
    {301, 31, kBarWidth, 391}, {401, 31, kBarWidth, 391}, {501, 31, kBarWidth, 391},
    {601, 31, kBarWidth, 391},
};

} // namespace

InstrumentGeneralScreen::InstrumentGeneralScreen() = default;

void InstrumentGeneralScreen::build() {
    if (built_) return;

    std::vector<RectI> places;
    places.reserve(7);
    for (int i = 0; i < 7; ++i) {
        places.push_back(RectI{kFramePlaces[i][0], kFramePlaces[i][1], kFramePlaces[i][2],
                               kFramePlaces[i][3]});
    }
    frame_ = &display_.add<FrameControl>("frame", std::move(places), 0);

    // Firmware creates {label[i], barControl[i]} pairs, then bgLabel and the
    // lists. The bottom labels and bgLabel are app chrome (ScreenButtons) in
    // this slice, so only the bars and the workspace controls are built here.
    for (int i = 0; i < 8; ++i) {
        const auto mode = (i >= 1 && i <= 3) ? BarControl::Mode::LeftRight : BarControl::Mode::Fill;
        auto& bar = display_.add<BarControl>("bar" + std::to_string(i), i, mode);
        bar.setRect(RectI{kColumnWidth * i + 1, kBarY, kBarWidth, kBarHeight});
        bar.setVisible(i != 4 && i != 7); // showInstrumentParams hides bar 4 and 7
        bars_[i] = &bar;
    }

    std::vector<std::string> filterItems = {"Disabled", "Low-pass", "High-pass", "Band-pass"};
    filterList_ = &display_.add<ListControl>("filter", std::move(filterItems), 5);
    filterList_->setRect(RectI{kColumnWidth * 4 + 1, kBarY, kBarWidth, kBarHeight});

    auto& cover = display_.add<BlankCoverControl>("blankSpaceCover");
    cover.setRect(RectI{kColumnWidth * 7 + 1, kBarY, kBarWidth, kBarHeight});

    for (int i = 0; i < 8; ++i) {
        bars_[i]->setOnPress([this](int column) {
            if (view_) view_->setSelectedColumn(column);
        });
    }
    filterList_->setOnSelect([this](int index) {
        if (!song_ || !view_ || !engineHost_ || !selectedInstIdx_) return;
        auto& inst = song_->instruments[static_cast<size_t>(*selectedInstIdx_)];
        inst.filterType = static_cast<Instrument::FilterType>(index);
        view_->setSelectedColumn(4);
        engineHost_->publish(*song_);
    });

    built_ = true;
}

void InstrumentGeneralScreen::syncFromModel(const Song& song, int selectedInstIdx,
                                            const InstrumentView& view) {
    if (selectedInstIdx < 0 || selectedInstIdx >= static_cast<int>(song.instruments.size())) return;
    const Instrument& inst = song.instruments[static_cast<size_t>(selectedInstIdx)];

    bars_[0]->setValue(inst.volume);
    bars_[1]->setValue(inst.panning * 2);
    bars_[2]->setValue((inst.tune * 100) / 24);
    bars_[3]->setValue(inst.finetune);
    bars_[5]->setValue(inst.cutoff);
    bars_[6]->setValue(inst.resonance);

    const bool filterOn = inst.filterType != Instrument::FilterType::None;
    bars_[5]->setInactive(!filterOn);
    bars_[6]->setInactive(!filterOn);

    filterList_->setSelected(static_cast<int>(inst.filterType));
    frame_->setSelected(std::min(view.selectedColumn(), 6));
}

void InstrumentGeneralScreen::update(Song& song,
                                     History& history,
                                     EngineHost& engineHost,
                                     int& selectedInstIdx,
                                     InstrumentView& view,
                                     const ImRect& workspace,
                                     ui::LayoutProfile profile,
                                     bool shiftHeld) {
    (void)history;
    build();
    song_ = &song;
    engineHost_ = &engineHost;
    view_ = &view;
    selectedInstIdx_ = &selectedInstIdx;

    if (song.instruments.empty()) return;
    if (selectedInstIdx < 0) selectedInstIdx = 0;
    if (selectedInstIdx >= static_cast<int>(song.instruments.size()))
        selectedInstIdx = static_cast<int>(song.instruments.size()) - 1;

    ImGui::SetCursorScreenPos(workspace.Min);
    ImGui::InvisibleButton("##gui_next_instrument_workspace", workspace.GetSize());
    const bool hovered = ImGui::IsItemHovered();
    const bool keyTarget =
        ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) || !ImGui::IsAnyItemActive();
    const bool fine = shiftHeld || ImGui::GetIO().KeyShift;

    if (keyTarget && !ImGui::GetIO().WantTextInput) {
        const int col = view.selectedColumn();
        if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow, false)) view.setSelectedColumn(col - 1);
        if (ImGui::IsKeyPressed(ImGuiKey_RightArrow, false)) view.setSelectedColumn(col + 1);
        if (ImGui::IsKeyPressed(ImGuiKey_P) || ImGui::IsKeyPressed(ImGuiKey_F2)) {
            view.setSelectedColumn(1);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
            view.adjustSelectedParam(song, engineHost, selectedInstIdx, +1, fine);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
            view.adjustSelectedParam(song, engineHost, selectedInstIdx, -1, fine);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Delete) || ImGui::IsKeyPressed(ImGuiKey_Backspace)) {
            view.resetSelectedParam(song, engineHost, selectedInstIdx);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Space)) {
            engineHost.previewNoteOn(-1, selectedInstIdx, 60, 0.9f);
        }
        if (ImGui::IsKeyReleased(ImGuiKey_Space)) engineHost.previewNoteOff(-1);
    }

    if (hovered) {
        const float wheel = ImGui::GetIO().MouseWheel;
        if (std::abs(wheel) > 0.01f) {
            view.adjustSelectedParam(song, engineHost, selectedInstIdx,
                                     (wheel > 0.0f) ? +1 : -1, fine);
        }
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Left, 4.0f)) {
            const float dy = ImGui::GetIO().MouseDelta.y;
            if (std::abs(dy) > 1.5f) {
                view.adjustSelectedParam(song, engineHost, selectedInstIdx,
                                         (dy < 0.0f) ? +1 : -1, fine);
            }
        }
    }

    if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
        const LayoutContext lc = LayoutContext::fromWorkspace(workspace, profile);
        const ImVec2 device = lc.screenToDevice(ImGui::GetIO().MousePos);
        const int dx = static_cast<int>(std::floor(device.x));
        const int dy = static_cast<int>(std::floor(device.y));
        if (ui::Control* hit = display_.hitTest(dx, dy)) hit->onPress(dx, dy);
    }

    syncFromModel(song, selectedInstIdx, view);
}

void InstrumentGeneralScreen::render(ImDrawList* drawList,
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

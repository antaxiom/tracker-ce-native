#include "instrument_pages.h"

#include "Song.h"
#include "History.h"
#include "EngineHost.h"
#include "views/InstrumentView.h"
#include "views/InstrumentBars.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace trakr {
using namespace ::trakr::ui;

namespace {
constexpr int kColumnWidth = 800 / 8; // 100
constexpr int kPageBarY = 29;
constexpr int kPageBarW = 800 / 8 - 3;    // 97
constexpr int kPageBarH = 394;

// gInstrEdit.cpp framesPlaces[8][4].
constexpr int kFramePlaces[8][4] = {
    {1, 31, kPageBarW, 391},   {101, 31, kPageBarW, 391}, {201, 31, kPageBarW, 391},
    {301, 31, kPageBarW, 391}, {401, 31, kPageBarW, 391}, {501, 31, kPageBarW, 391},
    {601, 31, kPageBarW, 391}, {701, 31, kPageBarW, 391},
};
// framesPlacesAutomation[7][4]: destination spans two columns.
constexpr int kFramePlacesAutomation[7][4] = {
    {1, 31, 2 * kColumnWidth - 3, 391}, {201, 31, kPageBarW, 391}, {301, 31, kPageBarW, 391},
    {401, 31, kPageBarW, 391},              {501, 31, kPageBarW, 391}, {601, 31, kPageBarW, 391},
    {701, 31, kPageBarW, 391},
};
} // namespace

InstrumentPagesScreen::InstrumentPagesScreen() = default;

void InstrumentPagesScreen::build() {
    if (built_) return;

    std::vector<RectI> general;
    for (const auto& p : kFramePlaces) general.push_back(RectI{p[0], p[1], p[2], p[3]});
    frameGeneral_ = &display_.add<FrameControl>("frameGeneral", std::move(general), 0);
    std::vector<RectI> automation;
    for (const auto& p : kFramePlacesAutomation)
        automation.push_back(RectI{p[0], p[1], p[2], p[3]});
    frameAutomation_ = &display_.add<FrameControl>("frameAutomation", std::move(automation), 0);

    for (int i = 0; i < 8; ++i) {
        // Effects/Automation columns are all Value_0_100 fill bars (the
        // LeftRight bars live on the General page's own screen).
        bars_[i] = &display_.add<BarControl>("pageBar" + std::to_string(i), i,
                                             BarControl::Mode::Fill);
        bars_[i]->setRect(RectI{kColumnWidth * i + 1, kPageBarY, kPageBarW, kPageBarH});
        bars_[i]->setVisible(false);
        bars_[i]->setOnPress([this](int column) {
            if (view_) view_->setSelectedColumn(column);
        });
    }

    std::vector<std::string> dests(kAutoDests, kAutoDests + 6);
    destList_ = &display_.add<ListControl>("automationDest", std::move(dests), 6);
    destList_->setRect(RectI{1, kPageBarY, 2 * kColumnWidth - 3, kPageBarH});
    std::vector<std::string> types(kAutoTypes, kAutoTypes + 3);
    typeList_ = &display_.add<ListControl>("automationType", std::move(types), 5);
    typeList_->setRect(RectI{201, kPageBarY, kPageBarW, kPageBarH});
    std::vector<std::string> shapes(kLfoShapes, kLfoShapes + 5);
    shapeList_ = &display_.add<ListControl>("automationShape", std::move(shapes), 5);
    shapeList_->setRect(RectI{301, kPageBarY, kPageBarW, kPageBarH});
    std::vector<std::string> speeds(kLfoSpeeds, kLfoSpeeds + kNumSpeeds);
    speedList_ = &display_.add<ListControl>("automationSpeed", std::move(speeds), 14);
    speedList_->setRect(RectI{401, kPageBarY, kPageBarW, kPageBarH});

    effectsCover_ = &display_.add<BlankCoverControl>("effectsCover");
    effectsCover_->setRect(RectI{401, kPageBarY, 397, kPageBarH});
    effectsCover_->setVisible(false);

    destList_->setOnSelect([this](int index) {
        if (!song_ || !view_ || !selectedInstIdx_) return;
        view_->setSelectedAutoDest(index);
        view_->setSelectedColumn(0);
    });
    typeList_->setOnSelect([this](int index) {
        if (!song_ || !view_ || !selectedInstIdx_ || !engineHost_) return;
        auto& inst = song_->instruments[static_cast<size_t>(*selectedInstIdx_)];
        inst.automation[static_cast<size_t>(view_->selectedAutoDest())].type =
            static_cast<Instrument::AutoType>(index);
        view_->setSelectedColumn(2);
        engineHost_->publish(*song_);
    });
    shapeList_->setOnSelect([this](int index) {
        if (!song_ || !view_ || !selectedInstIdx_ || !engineHost_) return;
        auto& inst = song_->instruments[static_cast<size_t>(*selectedInstIdx_)];
        inst.automation[static_cast<size_t>(view_->selectedAutoDest())].shape =
            static_cast<Instrument::AutoShape>(index);
        view_->setSelectedColumn(3);
        engineHost_->publish(*song_);
    });
    speedList_->setOnSelect([this](int index) {
        if (!song_ || !view_ || !selectedInstIdx_ || !engineHost_) return;
        auto& inst = song_->instruments[static_cast<size_t>(*selectedInstIdx_)];
        inst.automation[static_cast<size_t>(view_->selectedAutoDest())].speedStepIdx = index;
        view_->setSelectedColumn(4);
        engineHost_->publish(*song_);
    });

    built_ = true;
}

void InstrumentPagesScreen::syncFromModel(const Song& song, int selectedInstIdx,
                                          const InstrumentView& view) {
    if (selectedInstIdx < 0 || selectedInstIdx >= static_cast<int>(song.instruments.size()))
        return;
    const Instrument& inst = song.instruments[static_cast<size_t>(selectedInstIdx)];
    const bool effects = view.subPage() == InstrumentView::SubPage::Effects;

    for (auto* bar : bars_) bar->setVisible(false);
    frameGeneral_->setVisible(false);
    frameAutomation_->setVisible(false);
    destList_->setVisible(false);
    typeList_->setVisible(false);
    shapeList_->setVisible(false);
    speedList_->setVisible(false);
    effectsCover_->setVisible(false);

    if (effects) {
        bars_[0]->setValue(inst.overdrive);
        bars_[1]->setValue(std::clamp(((inst.bitDepth - 4) * 100) / 12, 0, 100));
        bars_[2]->setValue(inst.reverbSend);
        bars_[3]->setValue(inst.delaySend);
        for (int i = 0; i < 4; ++i) bars_[i]->setVisible(true);
        effectsCover_->setVisible(true);
        frameGeneral_->setVisible(view.selectedColumn() <= 3);
        frameGeneral_->setSelected(view.selectedColumn());
        return;
    }

    const int dest = std::clamp(view.selectedAutoDest(), 0, 5);
    const auto& slot = inst.automation[static_cast<size_t>(dest)];
    const bool enabled = slot.type != Instrument::AutoType::Off;
    const bool lfo = slot.type == Instrument::AutoType::Lfo;

    destList_->setSelected(dest);
    typeList_->setSelected(static_cast<int>(slot.type));
    destList_->setVisible(true);
    typeList_->setVisible(true);

    if (lfo) {
        shapeList_->setSelected(static_cast<int>(slot.shape));
        shapeList_->setVisible(true);
        speedList_->setSelected(std::clamp(slot.speedStepIdx, 0, kNumSpeeds - 1));
        speedList_->setVisible(true);
        bars_[5]->setValue(std::clamp(static_cast<int>(std::lround(slot.amount * 100.0f)), 0, 100));
        bars_[5]->setVisible(true);
    } else {
        const int attack =
            std::clamp(static_cast<int>(std::lround(slot.attackSec * 10.0f)), 0, 100);
        const int decay = std::clamp(static_cast<int>(std::lround(slot.decaySec * 10.0f)), 0, 100);
        const int sustain = std::clamp(static_cast<int>(std::lround(slot.sustain * 100.0f)), 0, 100);
        const int release =
            std::clamp(static_cast<int>(std::lround(slot.releaseSec * 10.0f)), 0, 100);
        const int amount = std::clamp(static_cast<int>(std::lround(slot.amount * 100.0f)), 0, 100);
        bars_[3]->setValue(attack);
        bars_[4]->setValue(decay);
        bars_[5]->setValue(sustain);
        bars_[6]->setValue(release);
        bars_[7]->setValue(amount);
        for (int i = 3; i <= 7; ++i) {
            bars_[i]->setVisible(true);
            bars_[i]->setInactive(!enabled);
        }
    }

    const int col = view.selectedColumn();
    frameAutomation_->setVisible(true);
    frameAutomation_->setSelected(col <= 1 ? 0 : col - 1);
}

void InstrumentPagesScreen::update(Song& song,
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
    ImGui::InvisibleButton("##gui_next_instrument_pages_workspace", workspace.GetSize());
    const bool hovered = ImGui::IsItemHovered();
    const bool keyTarget =
        ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) || !ImGui::IsAnyItemActive();
    const bool fine = shiftHeld || ImGui::GetIO().KeyShift;

    if (keyTarget && !ImGui::GetIO().WantTextInput) {
        if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow, false)) view.setSelectedColumn(view.selectedColumn() - 1);
        if (ImGui::IsKeyPressed(ImGuiKey_RightArrow, false)) view.setSelectedColumn(view.selectedColumn() + 1);
        if (ImGui::IsKeyPressed(ImGuiKey_UpArrow))
            view.adjustSelectedParam(song, engineHost, selectedInstIdx, +1, fine);
        if (ImGui::IsKeyPressed(ImGuiKey_DownArrow))
            view.adjustSelectedParam(song, engineHost, selectedInstIdx, -1, fine);
        if (ImGui::IsKeyPressed(ImGuiKey_Delete) || ImGui::IsKeyPressed(ImGuiKey_Backspace))
            view.resetSelectedParam(song, engineHost, selectedInstIdx);
        if (ImGui::IsKeyPressed(ImGuiKey_Space))
            engineHost.previewNoteOn(-1, selectedInstIdx, 60, 0.9f);
        if (ImGui::IsKeyReleased(ImGuiKey_Space)) engineHost.previewNoteOff(-1);
    }

    if (hovered) {
        const float wheel = ImGui::GetIO().MouseWheel;
        if (std::abs(wheel) > 0.01f)
            view.adjustSelectedParam(song, engineHost, selectedInstIdx, (wheel > 0.0f) ? +1 : -1,
                                     fine);
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

void InstrumentPagesScreen::render(ImDrawList* drawList,
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

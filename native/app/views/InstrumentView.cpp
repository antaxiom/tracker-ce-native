#include "InstrumentView.h"
#include "InstrumentBars.h"
#include "../FontLoader.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

// Polyend Tracker Instrument Parameters screen.
// Firmware reference: core/interface/instrumentEditor/gInstrEdit.cpp
// Structural template: native/app/views/{OrderListView,ConfigView,PerformView}.cpp
// Geometry lives in firmware device pixels in InstrumentBars.cpp and is mapped
// once through px()/py(); this file owns input, model edits and the app footer.

namespace trakr {

InstrumentView::InstrumentView() = default;
void InstrumentView::draw(bool* p_open) { (void)p_open; }

void InstrumentView::setSelectedColumn(int col) {
    selectedCol_ = std::clamp(col, 0, 7);
}

void InstrumentView::togglePage() {
    // Page key toggles the secondary Effects page; Automation is reached from
    // the Effects page's own Automation button, matching the firmware.
    subPage_ = (subPage_ == SubPage::General) ? SubPage::Effects : SubPage::General;
    selectedCol_ = 0;
}

void InstrumentView::adjustSelectedParam(Song& song, EngineHost& engineHost,
                                         int selectedInstIdx, int delta, bool fine) {
    (void)fine;
    if (selectedInstIdx < 0 || selectedInstIdx >= static_cast<int>(song.instruments.size())) return;
    auto& inst = song.instruments[selectedInstIdx];
    if (subPage_ == SubPage::General) {
        switch (selectedCol_) {
            case 0: inst.volume = std::clamp(inst.volume + delta, 0, 100); break;
            case 1: inst.panning = std::clamp(inst.panning + delta, -50, 50); break;
            case 2: inst.tune = std::clamp(inst.tune + delta, -24, 24); break;
            case 3: inst.finetune = std::clamp(inst.finetune + delta, -100, 100); break;
            // Up moves the visible list selection up (previous item), matching
            // the on-screen list and the arrow-key convention.
            case 4: {
                int idx = static_cast<int>(inst.filterType);
                idx = (idx + (delta > 0 ? 3 : 1)) % 4;
                inst.filterType = static_cast<Instrument::FilterType>(idx);
                break;
            }
            case 5: inst.cutoff = std::clamp(inst.cutoff + delta, 0, 100); break;
            case 6: inst.resonance = std::clamp(inst.resonance + delta, 0, 100); break;
            case 7: break; // More is a page button, not an adjustable value
        }
    } else if (subPage_ == SubPage::Effects) {
        switch (selectedCol_) {
            case 0: inst.overdrive = std::clamp(inst.overdrive + delta, 0, 100); break;
            case 1: inst.bitDepth = std::clamp(inst.bitDepth + delta, 4, 16); break;
            case 2: inst.reverbSend = std::clamp(inst.reverbSend + delta, 0, 100); break;
            case 3: inst.delaySend = std::clamp(inst.delaySend + delta, 0, 100); break;
            case 7: break; // Back is a page button, not an adjustable value
        }
    } else if (subPage_ == SubPage::Automation) {
        auto& slot = inst.automation[selectedAutoDest_];
        // Up selects the previous row in the visible list, Down the next one.
        if (selectedCol_ <= 1) selectedAutoDest_ = (selectedAutoDest_ + (delta > 0 ? 5 : 1)) % 6;
        else if (selectedCol_ == 2) slot.type = static_cast<Instrument::AutoType>(
            (static_cast<int>(slot.type) + (delta > 0 ? 2 : 1)) % 3);
        else if (slot.type == Instrument::AutoType::Lfo) {
            if (selectedCol_ == 3) slot.shape = static_cast<Instrument::AutoShape>(
                (static_cast<int>(slot.shape) + (delta > 0 ? 1 : 4)) % 5);
            else if (selectedCol_ == 4) slot.speedStepIdx = std::clamp(slot.speedStepIdx + delta, 0, kNumSpeeds - 1);
            else if (selectedCol_ == 7) slot.amount = std::clamp(slot.amount + delta * 0.02f, 0.0f, 1.0f);
        } else if (slot.type == Instrument::AutoType::Envelope) {
            if (selectedCol_ == 3) slot.attackSec = std::clamp(slot.attackSec + delta * 0.05f, 0.001f, 10.0f);
            else if (selectedCol_ == 4) slot.decaySec = std::clamp(slot.decaySec + delta * 0.05f, 0.001f, 10.0f);
            else if (selectedCol_ == 5) slot.sustain = std::clamp(slot.sustain + delta * 0.02f, 0.0f, 1.0f);
            else if (selectedCol_ == 6) slot.releaseSec = std::clamp(slot.releaseSec + delta * 0.05f, 0.001f, 10.0f);
            else if (selectedCol_ == 7) slot.amount = std::clamp(slot.amount + delta * 0.02f, 0.0f, 1.0f);
        }
    }
    engineHost.publish(song);
}

void InstrumentView::resetSelectedParam(Song& song, EngineHost& engineHost, int selectedInstIdx) {
    if (selectedInstIdx < 0 || selectedInstIdx >= static_cast<int>(song.instruments.size())) return;
    auto& inst = song.instruments[selectedInstIdx];
    if (subPage_ == SubPage::General) {
        switch (selectedCol_) {
            case 0: inst.volume = 50; break;
            case 1: inst.panning = 0; break;
            case 2: inst.tune = 0; break;
            case 3: inst.finetune = 0; break;
            case 4: inst.filterType = Instrument::FilterType::None; break;
            case 5: inst.cutoff = 100; break;
            case 6: inst.resonance = 0; break;
            case 7: break;
        }
    } else if (subPage_ == SubPage::Effects) {
        switch (selectedCol_) {
            case 0: inst.overdrive = 0; break;
            case 1: inst.bitDepth = 16; break;
            case 2: inst.reverbSend = 0; break;
            case 3: inst.delaySend = 0; break;
        }
    }
    engineHost.publish(song);
}

std::vector<hw::ScreenButton> InstrumentView::getScreenButtons(Song& song, EngineHost& engineHost,
                                                               int selectedInstIdx) {
    std::vector<hw::ScreenButton> btns(8);
    if (selectedInstIdx < 0 || selectedInstIdx >= static_cast<int>(song.instruments.size())) return btns;
    auto& inst = song.instruments[selectedInstIdx];
    char buf[32];

    if (subPage_ == SubPage::General) {
        // Firmware bottom-bar values (gInstrEdit.cpp showParams*): bare numbers,
        // "0.00 dB", filter name. Labels are name/value pairs; no underlines.
        if (inst.volume == 0) std::snprintf(buf, sizeof(buf), "-inf dB");
        else std::snprintf(buf, sizeof(buf), "%.2f dB", (inst.volume - 50) * 0.48f);
        btns[0] = {"Volume", buf, "", "", hw::UnderlineColor::None, true, false, [this]() { selectedCol_ = 0; }};
        std::snprintf(buf, sizeof(buf), "%d", inst.panning);
        btns[1] = {"Panning", buf, "", "", hw::UnderlineColor::None, true, false, [this]() { selectedCol_ = 1; }};
        std::snprintf(buf, sizeof(buf), "%d", inst.tune);
        btns[2] = {"Tune", buf, "", "", hw::UnderlineColor::None, true, false, [this]() { selectedCol_ = 2; }};
        std::snprintf(buf, sizeof(buf), "%d", inst.finetune);
        btns[3] = {"Finetune", buf, "", "", hw::UnderlineColor::None, true, false, [this]() { selectedCol_ = 3; }};
        btns[4] = {"Filter Type", kFilterTypes[static_cast<int>(inst.filterType)], "", "", hw::UnderlineColor::None, true, false, [this]() { selectedCol_ = 4; }};
        std::snprintf(buf, sizeof(buf), "%d", inst.cutoff);
        btns[5] = {"Cutoff", buf, "", "", (inst.filterType == Instrument::FilterType::None)
                                                   ? hw::UnderlineColor::Dimmed : hw::UnderlineColor::None,
                   true, false, [this]() { selectedCol_ = 5; }};
        std::snprintf(buf, sizeof(buf), "%d", inst.resonance);
        btns[6] = {"Resonance", buf, "", "", (inst.filterType == Instrument::FilterType::None)
                                                      ? hw::UnderlineColor::Dimmed : hw::UnderlineColor::None,
                   true, false, [this]() { selectedCol_ = 6; }};
        btns[7] = {"More", " ", "", "", hw::UnderlineColor::None, true, false, [this]() { subPage_ = SubPage::Effects; selectedCol_ = 0; }};
    } else if (subPage_ == SubPage::Effects) {
        std::snprintf(buf, sizeof(buf), "%d%%", inst.overdrive);
        btns[0] = {"Overdrive", buf, "", "", hw::UnderlineColor::Green, true, (selectedCol_ == 0), [this]() { selectedCol_ = 0; }};
        std::snprintf(buf, sizeof(buf), "%d", inst.bitDepth);
        btns[1] = {"Bit depth", buf, "", "", hw::UnderlineColor::Green, true, (selectedCol_ == 1), [this]() { selectedCol_ = 1; }};
        std::snprintf(buf, sizeof(buf), (inst.reverbSend == 0) ? "-inf dB" : "%d%%", inst.reverbSend);
        btns[2] = {"Reverb send", buf, "", "", hw::UnderlineColor::Green, true, (selectedCol_ == 2), [this]() { selectedCol_ = 2; }};
        std::snprintf(buf, sizeof(buf), (inst.delaySend == 0) ? "-inf dB" : "%d%%", inst.delaySend);
        btns[3] = {"Delay send", buf, "", "", hw::UnderlineColor::Green, true, (selectedCol_ == 3), [this]() { selectedCol_ = 3; }};
        btns[4] = {"Automation", "Page", "", "", hw::UnderlineColor::Orange, true, false, [this]() { subPage_ = SubPage::Automation; selectedCol_ = 0; }};
        btns[5] = {"Reset", "Param", "", "", hw::UnderlineColor::Dimmed, true, false, [&inst, &song, &engineHost, this]() {
            if (selectedCol_ == 0) inst.overdrive = 0; else if (selectedCol_ == 1) inst.bitDepth = 16;
            else if (selectedCol_ == 2) inst.reverbSend = 0; else if (selectedCol_ == 3) inst.delaySend = 0;
            engineHost.publish(song);
        }};
        btns[6] = {"General", "Page", "", "", hw::UnderlineColor::Green, true, false, [this]() { subPage_ = SubPage::General; selectedCol_ = 0; }};
        btns[7] = {"Back", "", "", "", hw::UnderlineColor::Dimmed, true, false, [this]() { subPage_ = SubPage::General; selectedCol_ = 0; }};
    } else {
        auto& slot = inst.automation[selectedAutoDest_];
        btns[0] = {"Destination", kAutoDests[selectedAutoDest_], "", "", hw::UnderlineColor::Green, true, (selectedCol_ <= 1), [this]() { selectedCol_ = 0; }};
        btns[1] = {"General", "Page", "", "", hw::UnderlineColor::Green, true, false, [this]() { subPage_ = SubPage::General; selectedCol_ = 0; }};
        btns[2] = {"Type", kAutoTypes[static_cast<int>(slot.type)], "", "", hw::UnderlineColor::Green, true, (selectedCol_ == 2), [this]() { selectedCol_ = 2; }};
        if (slot.type == Instrument::AutoType::Lfo) {
            btns[3] = {"Shape", kLfoShapes[static_cast<int>(slot.shape)], "", "", hw::UnderlineColor::Green, true, (selectedCol_ == 3), [this]() { selectedCol_ = 3; }};
            int sIdx = std::clamp(slot.speedStepIdx, 0, kNumSpeeds - 1);
            btns[4] = {"Speed", kLfoSpeeds[sIdx], "", "", hw::UnderlineColor::Green, true, (selectedCol_ == 4), [this]() { selectedCol_ = 4; }};
            btns[5] = {"", "", "", "", hw::UnderlineColor::None, false, false, nullptr};
            btns[6] = {"", "", "", "", hw::UnderlineColor::None, false, false, nullptr};
            std::snprintf(buf, sizeof(buf), "%d%%", static_cast<int>(slot.amount * 100.0f));
            btns[7] = {"Amount", buf, "", "", hw::UnderlineColor::Green, true, (selectedCol_ == 7), [this]() { selectedCol_ = 7; }};
        } else if (slot.type == Instrument::AutoType::Envelope) {
            std::snprintf(buf, sizeof(buf), "%.2fs", slot.attackSec);
            btns[3] = {"Attack", buf, "", "", hw::UnderlineColor::Green, true, (selectedCol_ == 3), [this]() { selectedCol_ = 3; }};
            std::snprintf(buf, sizeof(buf), "%.2fs", slot.decaySec);
            btns[4] = {"Decay", buf, "", "", hw::UnderlineColor::Green, true, (selectedCol_ == 4), [this]() { selectedCol_ = 4; }};
            std::snprintf(buf, sizeof(buf), "%d%%", static_cast<int>(slot.sustain * 100.0f));
            btns[5] = {"Sustain", buf, "", "", hw::UnderlineColor::Green, true, (selectedCol_ == 5), [this]() { selectedCol_ = 5; }};
            std::snprintf(buf, sizeof(buf), "%.2fs", slot.releaseSec);
            btns[6] = {"Release", buf, "", "", hw::UnderlineColor::Green, true, (selectedCol_ == 6), [this]() { selectedCol_ = 6; }};
            std::snprintf(buf, sizeof(buf), "%d%%", static_cast<int>(slot.amount * 100.0f));
            btns[7] = {"Amount", buf, "", "", hw::UnderlineColor::Green, true, (selectedCol_ == 7), [this]() { selectedCol_ = 7; }};
        } else {
            for (int k = 3; k < 8; ++k) btns[k] = {"", "", "", "", hw::UnderlineColor::None, false, false, nullptr};
        }
    }
    return btns;
}

void InstrumentView::update(Song& song, History& history, EngineHost& engineHost,
                            int& selectedInstIdx) {
    (void)history;
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                                   ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar;
    if (!ImGui::Begin("Instrument Parameters", nullptr, flags)) { ImGui::End(); return; }
    if (selectedInstIdx < 0 && !song.instruments.empty()) selectedInstIdx = 0;
    if (selectedInstIdx >= static_cast<int>(song.instruments.size()))
        selectedInstIdx = static_cast<int>(song.instruments.size()) - 1;

    if (selectedInstIdx >= 0 && selectedInstIdx < static_cast<int>(song.instruments.size())) {
        auto& inst = song.instruments[selectedInstIdx];
        const bool isShift = ImGui::GetIO().KeyShift || shiftHeld_;
        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) || !ImGui::IsAnyItemActive()) {
            if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) selectedCol_ = std::max(0, selectedCol_ - 1);
            if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) selectedCol_ = std::min(7, selectedCol_ + 1);
            if (ImGui::IsKeyPressed(ImGuiKey_P) || ImGui::IsKeyPressed(ImGuiKey_F2)) selectedCol_ = 1;
            if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) adjustSelectedParam(song, engineHost, selectedInstIdx, 1, isShift);
            if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) adjustSelectedParam(song, engineHost, selectedInstIdx, -1, isShift);
            if (ImGui::IsKeyPressed(ImGuiKey_Delete) || ImGui::IsKeyPressed(ImGuiKey_Backspace))
                resetSelectedParam(song, engineHost, selectedInstIdx);
            if (ImGui::IsKeyPressed(ImGuiKey_Space)) engineHost.previewNoteOn(-1, selectedInstIdx, 60, 0.9f);
            if (ImGui::IsKeyReleased(ImGuiKey_Space)) engineHost.previewNoteOff(-1);
        }

        if (ImGui::IsWindowHovered()) {
            const float wheel = ImGui::GetIO().MouseWheel;
            if (std::abs(wheel) > 0.01f)
                adjustSelectedParam(song, engineHost, selectedInstIdx, (wheel > 0) ? 1 : -1, isShift);
            if (ImGui::IsMouseDragging(0, 4.0f)) {
                const float dy = ImGui::GetIO().MouseDelta.y;
                if (std::abs(dy) > 1.5f)
                    adjustSelectedParam(song, engineHost, selectedInstIdx, (dy < 0) ? 1 : -1, isShift);
            }
        }

        const ImVec2 avail = ImGui::GetContentRegionAvail();
        ImFont* light = getMonoLightFont();
        ImFont* bold = getMonoBoldFont();
        if (avail.x < 100.0f || avail.y < 100.0f || !light) { ImGui::End(); return; }
        const ImVec2 origin = ImGui::GetCursorScreenPos();

        InstrumentScreenState state;
        state.dl = ImGui::GetWindowDrawList();
        state.origin = origin;
        state.sx = avail.x / 800.0f;
        state.sy = avail.y / 397.0f;
        state.inst = &inst;
        state.page = (subPage_ == SubPage::General) ? 0 : (subPage_ == SubPage::Effects) ? 1 : 2;
        state.selectedCol = selectedCol_;
        state.selectedDest = selectedAutoDest_;
        state.light = light;
        state.bold = bold;
        state.lightSize = 14.0f * state.sy; // controlStyleFont1: Light 14
        state.boldSize = 17.0f * state.sy;  // controlStyleFont2/4: Bold 17
        drawInstrumentScreen(state);

        bool changed = false;
        if (state.clickedFilter >= 0) {
            inst.filterType = static_cast<Instrument::FilterType>(state.clickedFilter);
            selectedCol_ = 4;
            changed = true;
        }
        if (state.clickedDest >= 0) {
            selectedAutoDest_ = std::clamp(state.clickedDest, 0, 5);
            selectedCol_ = 0;
        }
        if (state.clickedType >= 0) {
            inst.automation[selectedAutoDest_].type = static_cast<Instrument::AutoType>(state.clickedType);
            selectedCol_ = 2;
            changed = true;
        }
        if (state.clickedShape >= 0) {
            inst.automation[selectedAutoDest_].shape = static_cast<Instrument::AutoShape>(state.clickedShape);
            selectedCol_ = 3;
            changed = true;
        }
        if (state.clickedSpeed >= 0) {
            inst.automation[selectedAutoDest_].speedStepIdx = std::clamp(state.clickedSpeed, 0, kNumSpeeds - 1);
            selectedCol_ = 4;
            changed = true;
        }
        if (state.clickedCol >= 0) {
            selectedCol_ = std::clamp(state.clickedCol, 0, 7);
            if (subPage_ == SubPage::Effects && selectedCol_ == 7) {
                subPage_ = SubPage::General;
                selectedCol_ = 0;
            }
        }
        if (changed) engineHost.publish(song);
        ImGui::SetCursorScreenPos(origin);
        if (ImGui::IsMouseReleased(0)) engineHost.previewNoteOff(-1);
    }
    ImGui::End();
}

} // namespace trakr

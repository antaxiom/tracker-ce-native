#include "FillModal.h"
#include "Command.h"
#include "../kit/Widgets.h"
#include "imgui.h"
#include <algorithm>

namespace trakr {

void FillModal::draw(bool* p_open, Pattern& pat, int patIdx, int track, int selectedInst,
                     History& history, Song& song, float footerHeight,
                     bool hasSelection, const SelectionRange& sel) {
    if (!p_open || !*p_open) {
        lastMinRow_ = lastMaxRow_ = lastMinTrack_ = lastMaxTrack_ = -1;
        return;
    }

    // Always adapt range and target whenever selection is active or changes
    if (hasSelection) {
        if (sel.minRow() != lastMinRow_ || sel.maxRow() != lastMaxRow_ ||
            sel.minTrack() != lastMinTrack_ || sel.maxTrack() != lastMaxTrack_) {
            config_.startRow = sel.minRow();
            config_.endRow = sel.maxRow();
            lastMinRow_ = sel.minRow();
            lastMaxRow_ = sel.maxRow();
            lastMinTrack_ = sel.minTrack();
            lastMaxTrack_ = sel.maxTrack();
        }
    } else if (lastMinRow_ != -1) {
        config_.startRow = 0;
        config_.endRow = std::max(0, pat.rows - 1);
        lastMinRow_ = lastMaxRow_ = lastMinTrack_ = lastMaxTrack_ = -1;
    }

    const ImGuiViewport* vp = ImGui::GetMainViewport();
    if (!vp) return;

    // Centered Fill Modal (Tracker firmware parity: gPatternEdit.cpp line 567 showFillPopup)
    const float modalW = std::clamp(vp->WorkSize.x * 0.75f, 540.0f, 760.0f);
    const float modalH = 175.0f;
    ImVec2 pos(vp->WorkPos.x + (vp->WorkSize.x - modalW) * 0.5f,
               vp->WorkPos.y + std::max(20.0f, (vp->WorkSize.y - footerHeight - modalH) * 0.42f));
    ImVec2 size(modalW, modalH);

    ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(size);
    ImGui::SetNextWindowViewport(vp->ID);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration |
                            ImGuiWindowFlags_NoMove |
                            ImGuiWindowFlags_NoSavedSettings;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 4.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.5f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 6.0f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.08f, 0.09f, 0.12f, 0.98f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.0f, 0.91f, 0.75f, 1.0f)); // Teal accent border

    if (ImGui::Begin("##FillPopupModal", nullptr, flags)) {
        char headerSubtitle[64];
        if (hasSelection) {
            snprintf(headerSubtitle, sizeof(headerSubtitle), "ADAPTED TO SELECTION: T%02d..T%02d | R%02d..%02d",
                     sel.minTrack() + 1, sel.maxTrack() + 1, config_.startRow, config_.endRow);
        } else {
            snprintf(headerSubtitle, sizeof(headerSubtitle), "TRACK T%02d | FILL (F7)", track + 1);
        }
        kit::PanelHeader("STEP FILL GENERATOR", headerSubtitle);
        ImGui::Spacing();

        ImGui::Columns(4, "##FillCols", false);
        ImGui::SetColumnWidth(0, 240);
        ImGui::SetColumnWidth(1, 220);
        ImGui::SetColumnWidth(2, 280);

        // Column 1: Mode & Target
        ImGui::TextColored(ImVec4(0.84f, 0.44f, 0.13f, 1.0f), "MODE & TARGET");
        const char* const modes[] = {"Euclidean Rhythm", "Random in Scale", "Linear Ramp", "Arpeggio"};
        int currentMode = static_cast<int>(config_.mode);
        ImGui::SetNextItemWidth(180);
        if (ImGui::Combo("##Mode", &currentMode, modes, IM_ARRAYSIZE(modes))) {
            config_.mode = static_cast<FillMode>(currentMode);
        }
        if (hasSelection) {
            ImGui::TextColored(ImVec4(0.0f, 0.9f, 0.85f, 1.0f), "Target: Selection (%d tracks)", sel.trackCount());
        } else {
            ImGui::Text("Target Track: T%02d", track + 1);
        }
        ImGui::SetNextItemWidth(120);
        ImGui::DragInt("Base Note", &config_.baseNote, 1.0f, 0, 119);

        // Column 2: Range & Steps
        ImGui::NextColumn();
        ImGui::TextColored(ImVec4(0.84f, 0.44f, 0.13f, 1.0f), "PATTERN RANGE");
        ImGui::SetNextItemWidth(120);
        ImGui::DragInt("Start Row", &config_.startRow, 1.0f, 0, pat.rows - 1);
        ImGui::SetNextItemWidth(120);
        ImGui::DragInt("End Row", &config_.endRow, 1.0f, 0, pat.rows - 1);
        config_.startRow = std::clamp(config_.startRow, 0, pat.rows - 1);
        config_.endRow = std::clamp(config_.endRow, 0, pat.rows - 1);

        // Column 3: Mode Parameters
        ImGui::NextColumn();
        ImGui::TextColored(ImVec4(0.84f, 0.44f, 0.13f, 1.0f), "PARAMETERS");
        if (config_.mode == FillMode::Euclidean) {
            int rangeRows = std::max(1, std::abs(config_.endRow - config_.startRow) + 1);
            ImGui::SetNextItemWidth(140);
            ImGui::SliderInt("Hits", &config_.hits, 1, rangeRows);
            ImGui::SetNextItemWidth(140);
            ImGui::SliderInt("Rotation", &config_.rotation, 0, rangeRows - 1);
        } else if (config_.mode == FillMode::RandomInScale || config_.mode == FillMode::Arpeggio) {
            const char* const roots[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
            ImGui::SetNextItemWidth(60);
            ImGui::Combo("Root", &config_.rootNote, roots, IM_ARRAYSIZE(roots));
            ImGui::SameLine();
            const auto& scales = getAvailableScales();
            std::vector<const char*> scaleNames;
            for (const auto& s : scales) scaleNames.push_back(s.name);
            int currentScale = static_cast<int>(config_.scale);
            ImGui::SetNextItemWidth(110);
            if (ImGui::Combo("Scale", &currentScale, scaleNames.data(), static_cast<int>(scaleNames.size()))) {
                config_.scale = static_cast<ScaleType>(currentScale);
            }
            ImGui::SetNextItemWidth(140);
            ImGui::SliderInt("Octaves", &config_.octaveRange, 1, 4);
        } else {
            ImGui::SetNextItemWidth(140);
            ImGui::SliderInt("Vol Start", &config_.volumeStart, 0, 64);
            ImGui::SetNextItemWidth(140);
            ImGui::SliderInt("Vol End", &config_.volumeEnd, 0, 64);
        }

        // Column 4: Hardware Actions
        ImGui::NextColumn();
        ImGui::Spacing();
        if (kit::FlatButton("APPLY FILL (ENTER)", ImVec2(160, 32)) || ImGui::IsKeyPressed(ImGuiKey_Enter, false)) {
            config_.instrument = selectedInst;
            int minT = hasSelection ? sel.minTrack() : track;
            int maxT = hasSelection ? sel.maxTrack() : track;
            for (int t = minT; t <= maxT; ++t) {
                if (t >= 0 && t < static_cast<int>(pat.tracks.size())) {
                    auto oldTrack = pat.tracks[t];
                    Pattern patCopy = pat;
                    applyFill(patCopy, t, config_);
                    history.execute(std::make_unique<SetTrackCommand>(patIdx, t, patCopy.tracks[t], oldTrack), song);
                }
            }
            *p_open = false;
        }
        ImGui::Spacing();
        if (kit::FlatButton("CLOSE (ESC)", ImVec2(160, 26)) || ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
            *p_open = false;
        }

        ImGui::Columns(1);
    }
    ImGui::End();

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(3);
}

} // namespace trakr

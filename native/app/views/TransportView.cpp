#include "TransportView.h"
#include "../kit/Widgets.h"
#include "imgui.h"
#include "imgui_internal.h"
#include <filesystem>

namespace trakr {

void TransportView::draw(bool* p_open) {
    (void)p_open;
}

void TransportView::update(Song& song, History& history, EngineHost& engineHost,
                          int& octave, int& editStep, bool& recordMode,
                          bool* pShowPattern, bool* pShowInstruments,
                          bool* pShowSample, bool* pShowMixer,
                          bool* pShowOrder,
                          const std::string& currentFilePath) {
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 5.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 0.0f));
    if (ImGui::Begin("Transport", nullptr, flags)) {
        playing_ = engineHost.getPlayhead().playing;
        static bool s_loopInitialized = false;
        if (!s_loopInitialized) {
            engineHost.setPatternLoop(loopPattern_);
            s_loopInitialized = true;
        }

        // Polyend Hardware Badge: Project Name
        std::string projLabel = "PROJECT: UNTITLED";
        if (!currentFilePath.empty()) {
            projLabel = "PROJECT: " + std::filesystem::path(currentFilePath).stem().string();
        }
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.13f, 0.16f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.92f, 0.50f, 0.10f, 1.0f)); // Polyend Amber
        ImGui::Button(projLabel.c_str(), ImVec2(140, 24));
        ImGui::PopStyleColor(2);
        ImGui::SameLine();

        // Polyend Hardware Badge: Pattern Index
        int patIdx = engineHost.getPlayhead().orderIndex;
        char patBuf[16];
        snprintf(patBuf, sizeof(patBuf), "PAT %02d", patIdx);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.13f, 0.16f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.00f, 0.91f, 0.85f, 1.0f)); // Mint Cyan
        ImGui::Button(patBuf, ImVec2(60, 24));
        ImGui::PopStyleColor(2);
        ImGui::SameLine();

        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();

        // Illuminated Hardware Transport Buttons
        // Stop
        if (kit::FlatButton("STOP", ImVec2(50, 24), !playing_)) {
            engineHost.stopPlayback();
        }
        ImGui::SameLine();

        // Play (Neon Green when active)
        if (playing_) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.00f, 0.85f, 0.40f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
        }
        if (ImGui::Button("PLAY", ImVec2(62, 24))) {
            if (playing_) engineHost.stopPlayback();
            else engineHost.play(engineHost.getPlayhead().orderIndex, 0);
        }
        if (playing_) ImGui::PopStyleColor(2);
        ImGui::SameLine();

        // Record (Polyend Red when armed)
        kit::ToggleButton("REC", &recordMode, ImVec4(0.80f, 0.09f, 0.09f, 1.0f), ImVec2(44, 24));
        if (ImGui::IsItemHovered()) kit::Tooltip("Toggle Pattern Record / Edit Mode (Ctrl+R)");
        ImGui::SameLine();

        // Loop pattern
        if (kit::ToggleButton("LOOP", &loopPattern_, ImVec4(0.00f, 0.70f, 0.90f, 1.0f), ImVec2(46, 24))) {
            engineHost.setPatternLoop(loopPattern_);
        }
        ImGui::SameLine();

        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();

        // BPM & LPB
        ImGui::AlignTextToFramePadding();
        ImGui::TextColored(ImVec4(0.92f, 0.50f, 0.10f, 1.0f), "BPM");
        ImGui::SameLine();
        int bpmInt = static_cast<int>(song.bpm);
        int oldBpm = bpmInt;
        if (kit::ValueBox("##bpm", &bpmInt, 40, 300, false, ImVec2(42, 22)) && bpmInt != oldBpm) {
            history.execute(std::make_unique<SetSongParamCommand>(
                static_cast<float>(bpmInt), song.bpm, song.lpb, song.lpb), song);
            engineHost.publish(song);
        }
        ImGui::SameLine();

        ImGui::TextColored(ImVec4(0.92f, 0.50f, 0.10f, 1.0f), "LPB");
        ImGui::SameLine();
        int lpbVal = song.lpb;
        int oldLpb = lpbVal;
        if (kit::ValueBox("##lpb", &lpbVal, 1, 32, false, ImVec2(32, 22)) && lpbVal != oldLpb) {
            history.execute(std::make_unique<SetSongParamCommand>(
                song.bpm, song.bpm, lpbVal, song.lpb), song);
            engineHost.publish(song);
        }
        ImGui::SameLine();

        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();

        // Octave & Edit Step
        ImGui::TextColored(ImVec4(0.55f, 0.60f, 0.70f, 1.0f), "OCT");
        ImGui::SameLine();
        kit::ValueBox("##oct", &octave, 0, 8, false, ImVec2(28, 22));
        ImGui::SameLine();

        ImGui::TextColored(ImVec4(0.55f, 0.60f, 0.70f, 1.0f), "STEP");
        ImGui::SameLine();
        kit::ValueBox("##step", &editStep, 0, 16, false, ImVec2(28, 22));
        if (ImGui::IsItemHovered()) kit::Tooltip("Cursor advance step (0-16)");
        ImGui::SameLine();

        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();

        // Master Output Peak Meters
        ImGui::TextColored(ImVec4(0.92f, 0.50f, 0.10f, 1.0f), "OUT");
        if (ImGui::IsItemHovered()) kit::Tooltip("Master Output Peak Meters (L/R)");
        ImGui::SameLine(0, 4);

        MeterLevels masterMeters = engineHost.getMasterMeters();
        masterPeakL_ = masterMeters.peakL;
        masterPeakR_ = masterMeters.peakR;

        kit::Meter(masterPeakL_, -1.0f, ImVec2(7, 22), true);
        if (ImGui::IsItemHovered()) kit::Tooltip("Left Channel Peak Level");
        ImGui::SameLine(0, 2);
        kit::Meter(masterPeakR_, -1.0f, ImVec2(7, 22), true);
        if (ImGui::IsItemHovered()) kit::Tooltip("Right Channel Peak Level");
    }
    ImGui::End();
    ImGui::PopStyleVar(2);
}

} // namespace trakr

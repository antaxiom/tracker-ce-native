#include "ExportDeck.h"
#include "../kit/Widgets.h"
#include "Render.h"
#include "PolyendExport.h"
#include "imgui.h"
#include <filesystem>

namespace trakr {

void ExportDeck::draw(bool* p_open, const Song& song, float footerHeight) {
    if (!p_open || !*p_open) return;

    const ImGuiViewport* vp = ImGui::GetMainViewport();
    if (!vp) return;

    const float deckHeight = 150.0f;
    ImVec2 pos(vp->WorkPos.x, vp->WorkPos.y + vp->WorkSize.y - footerHeight - deckHeight);
    ImVec2 size(vp->WorkSize.x, deckHeight);

    ImGui::SetNextWindowPos(pos);
    ImGui::SetNextWindowSize(size);
    ImGui::SetNextWindowViewport(vp->ID);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration |
                            ImGuiWindowFlags_NoMove |
                            ImGuiWindowFlags_NoSavedSettings;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 8.0f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.06f, 0.06f, 0.08f, 0.98f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.84f, 0.44f, 0.13f, 1.0f));

    if (ImGui::Begin("##TrackerExportDeck", nullptr, flags)) {
        kit::PanelHeader("AUDIO & STEM EXPORT", "TRACKER PROJECT WORKFLOW");

        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.70f, 0.75f, 0.80f, 1.0f), "Choose export destination format:");
        ImGui::Spacing();

        // 1. Render Stems
        if (kit::FlatButton("RENDER 8 STEMS (WAV)", ImVec2(195, 30))) {
            std::filesystem::create_directories("./exports/stems");
            if (renderStemsToWav(song, "./exports/stems/stem")) {
                statusMessage_ = "[OK] Successfully rendered 8 Track Stems to ./exports/stems/";
            } else {
                statusMessage_ = "[ERR] Error rendering stems";
            }
        }
        ImGui::SameLine(0, 10);

        // 2. Render Master Mix
        if (kit::FlatButton("RENDER MASTER MIX", ImVec2(180, 30))) {
            std::filesystem::create_directories("./exports");
            if (renderToWav(song, "./exports/master_mix.wav")) {
                statusMessage_ = "[OK] Successfully rendered Master Mix to ./exports/master_mix.wav";
            } else {
                statusMessage_ = "[ERR] Error rendering master mix";
            }
        }
        ImGui::SameLine(0, 10);

        // 3. Export Tracker SD Project
        if (kit::FlatButton("EXPORT PROJECT SD CARD", ImVec2(200, 30))) {
            std::filesystem::create_directories("./exports/tracker_project");
            if (io::exportPolyendProject("./exports/tracker_project", song)) {
                statusMessage_ = "[OK] Exported Tracker Project (project.mt + patterns + samples) to ./exports/tracker_project/";
            } else {
                statusMessage_ = "[ERR] Error exporting project";
            }
        }
        ImGui::SameLine(0, 15);

        // Close
        if (kit::FlatButton("CLOSE (ESC)", ImVec2(110, 30)) || ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
            *p_open = false;
        }

        if (!statusMessage_.empty()) {
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.00f, 0.91f, 0.75f, 1.0f), "%s", statusMessage_.c_str());
        }
    }
    ImGui::End();

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(3);
}

} // namespace trakr

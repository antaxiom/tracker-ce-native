#include "SoftKeys.h"
#include "imgui_internal.h"
#include <algorithm>

namespace trakr {

void renderPolyendSoftKeys(const std::vector<SoftKeyDef>& keys, float height) {
    if (keys.empty()) return;

    const ImGuiViewport* vp = ImGui::GetMainViewport();
    if (!vp) return;

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration |
                            ImGuiWindowFlags_NoMove |
                            ImGuiWindowFlags_NoScrollWithMouse |
                            ImGuiWindowFlags_NoSavedSettings |
                            ImGuiWindowFlags_NoNavFocus;

    ImVec2 barPos(vp->WorkPos.x, vp->WorkPos.y + vp->WorkSize.y - height);
    ImVec2 barSize(vp->WorkSize.x, height);

    ImGui::SetNextWindowPos(barPos);
    ImGui::SetNextWindowSize(barSize);
    ImGui::SetNextWindowViewport(vp->ID);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4.0f, 2.0f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.04f, 0.04f, 0.05f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.24f, 0.24f, 0.28f, 1.0f));

    if (ImGui::Begin("##SoftKeysBar", nullptr, flags)) {
        ImGui::BringWindowToDisplayFront(ImGui::GetCurrentWindow());
        size_t count = std::min<size_t>(keys.size(), 8);
        const float totalW = ImGui::GetContentRegionAvail().x;
        const float spacing = 4.0f;
        const float btnW = std::max(40.0f, (totalW - spacing * (count - 1)) / static_cast<float>(count));
        const float btnH = height - 6.0f;

        for (size_t i = 0; i < count; ++i) {
            const auto& k = keys[i];
            ImGui::PushID(static_cast<int>(i));

            if (k.active) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.84f, 0.44f, 0.13f, 1.0f)); // Polyend Orange
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.92f, 0.52f, 0.20f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.75f, 0.38f, 0.10f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
            } else {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.10f, 0.11f, 0.13f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.18f, 0.20f, 0.24f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.24f, 0.26f, 0.32f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.75f, 0.78f, 0.82f, 1.0f));
            }

            char labelBuf[64];
            snprintf(labelBuf, sizeof(labelBuf), "%s %s", k.keyLabel.c_str(), k.title.c_str());

            if (ImGui::Button(labelBuf, ImVec2(btnW, btnH))) {
                if (k.onClick) k.onClick();
            }

            ImGui::PopStyleColor(4);
            ImGui::PopID();

            if (i + 1 < count) ImGui::SameLine(0, spacing);
        }
    }
    ImGui::End();

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(3);
}

} // namespace trakr

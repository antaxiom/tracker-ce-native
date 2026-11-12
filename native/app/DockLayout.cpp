#include "DockLayout.h"
#include "imgui_internal.h"
#include <initializer_list>

namespace trakr {

void buildDefaultDockLayout(ImGuiID dockspace_id, const ImVec2& viewportSize) {
    ImGuiDockNodeFlags flags = ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_DockSpace;
    ImGui::DockBuilderRemoveNode(dockspace_id);
    ImGui::DockBuilderAddNode(dockspace_id, flags);
    ImGui::DockBuilderSetNodeSize(dockspace_id, viewportSize);

    ImGuiID dock_main = dockspace_id;
    ImGuiID dock_top = ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Up, 0.055f, nullptr, &dock_main);
    ImGuiID dock_bottom = ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Down, 0.30f, nullptr, &dock_main);
    ImGuiID dock_left = ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Left, 0.14f, nullptr, &dock_main);
    ImGuiID dock_right = ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Right, 0.22f, nullptr, &dock_main);

    if (ImGuiDockNode* topNode = ImGui::DockBuilderGetNode(dock_top)) {
        topNode->LocalFlags |= ImGuiDockNodeFlags_NoTabBar;
    }

    for (const char* w : {"Pattern Editor", "Kit Gallery"}) ImGui::DockBuilderDockWindow(w, dock_main);
    ImGui::DockBuilderDockWindow("Transport", dock_top);
    ImGui::DockBuilderDockWindow("Order List", dock_left);
    ImGui::DockBuilderDockWindow("Instruments", dock_right);
    for (const char* w : {"Sample View", "Mixer"}) ImGui::DockBuilderDockWindow(w, dock_bottom);

    ImGui::DockBuilderFinish(dockspace_id);
}

} // namespace trakr

#include "HwWidgets.h"
#include "imgui_internal.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace trakr::hw {

bool BarColumn(const char* label,
               float* value,
               float minVal,
               float maxVal,
               const char* format,
               const ImVec2& size,
               bool selected,
               bool recordMode,
               bool isBipolar) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;

    ImGuiContext& g = *GImGui;
    const ImGuiID id = window->GetID(label);

    ImVec2 pos = window->DC.CursorPos;
    ImRect totalRect(pos, ImVec2(pos.x + size.x, pos.y + size.y));
    ImGui::ItemSize(totalRect);
    if (!ImGui::ItemAdd(totalRect, id)) return false;

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(totalRect, id, &hovered, &held);

    bool changed = false;
    float range = (maxVal - minVal > 0.001f) ? (maxVal - minVal) : 1.0f;

    // Handle mouse drag / click
    if (held || (hovered && g.IO.MouseDown[0])) {
        float mouseY = g.IO.MousePos.y;
        float barTop = totalRect.Min.y + 22.0f;
        float barBottom = totalRect.Max.y - 24.0f;
        float barH = std::max(10.0f, barBottom - barTop);

        float norm = 1.0f - std::clamp((mouseY - barTop) / barH, 0.0f, 1.0f);
        float newVal = minVal + norm * range;
        if (std::abs(*value - newVal) > 0.001f) {
            *value = newVal;
            changed = true;
        }
    }

    // Mouse wheel edit
    if (hovered && std::abs(g.IO.MouseWheel) > 0.01f) {
        float step = range / 100.0f;
        *value = std::clamp(*value + g.IO.MouseWheel * step, minVal, maxVal);
        changed = true;
    }

    ImDrawList* dl = window->DrawList;

    // Column background
    ImU32 colBg = selected ? IM_COL32(18, 20, 25, 255) : IM_COL32(12, 13, 16, 255);
    dl->AddRectFilled(totalRect.Min, totalRect.Max, colBg, 2.0f);

    // Label on top
    ImVec2 lblSize = ImGui::CalcTextSize(label);
    dl->AddText(ImVec2(totalRect.Min.x + (size.x - lblSize.x) * 0.5f, totalRect.Min.y + 4.0f),
                IM_COL32(180, 180, 180, 255), label);

    // Bar track
    float barW = std::max(12.0f, size.x * 0.35f);
    float barX = totalRect.Min.x + (size.x - barW) * 0.5f;
    float barTop = totalRect.Min.y + 22.0f;
    float barBottom = totalRect.Max.y - 24.0f;
    float barH = barBottom - barTop;

    ImRect trackRect(ImVec2(barX, barTop), ImVec2(barX + barW, barBottom));
    dl->AddRectFilled(trackRect.Min, trackRect.Max, IM_COL32(22, 24, 30, 255), 1.0f);

    // Bar fill
    float normVal = std::clamp((*value - minVal) / range, 0.0f, 1.0f);
    ImU32 barCol = selected ? IM_COL32(254, 247, 73, 255) : IM_COL32(220, 220, 220, 255);

    if (isBipolar) {
        float midY = barTop + barH * 0.5f;
        dl->AddLine(ImVec2(barX - 2.0f, midY), ImVec2(barX + barW + 2.0f, midY), IM_COL32(100, 100, 100, 255), 1.0f);
        float fillY = barBottom - normVal * barH;
        if (fillY < midY) {
            dl->AddRectFilled(ImVec2(barX, fillY), ImVec2(barX + barW, midY), barCol, 1.0f);
        } else {
            dl->AddRectFilled(ImVec2(barX, midY), ImVec2(barX + barW, fillY), barCol, 1.0f);
        }
    } else {
        float fillY = barBottom - normVal * barH;
        dl->AddRectFilled(ImVec2(barX, fillY), ImVec2(barX + barW, barBottom), barCol, 1.0f);
    }

    // Value box at bottom
    char valBuf[32];
    std::snprintf(valBuf, sizeof(valBuf), format ? format : "%.1f", *value);
    ImVec2 valSize = ImGui::CalcTextSize(valBuf);
    dl->AddText(ImVec2(totalRect.Min.x + (size.x - valSize.x) * 0.5f, totalRect.Max.y - 18.0f),
                IM_COL32(240, 240, 240, 255), valBuf);

    // Selection frame (Green for navigate, Red for edit)
    if (selected) {
        ImU32 frameCol = recordMode ? IM_COL32(205, 24, 24, 255) : IM_COL32(48, 203, 63, 255);
        dl->AddRect(totalRect.Min, totalRect.Max, frameCol, 2.0f, 0, 1.5f);
    }

    return changed;
}

bool JogValueEdit(const char* label,
                  int* value,
                  int minVal,
                  int maxVal,
                  bool shiftHeld,
                  const ImVec2& size) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;

    ImGuiContext& g = *GImGui;
    const ImGuiID id = window->GetID(label);

    ImVec2 pos = window->DC.CursorPos;
    ImRect rect(pos, ImVec2(pos.x + size.x, pos.y + size.y));
    ImGui::ItemSize(rect);
    if (!ImGui::ItemAdd(rect, id)) return false;

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(rect, id, &hovered, &held);

    bool changed = false;
    int oldVal = *value;

    if (hovered && std::abs(g.IO.MouseWheel) > 0.01f) {
        int delta = (g.IO.MouseWheel > 0) ? 1 : -1;
        if (!shiftHeld) delta *= 4;
        *value = std::clamp(*value + delta, minVal, maxVal);
        changed = (*value != oldVal);
    }

    if (held && std::abs(g.IO.MouseDelta.y) > 0.5f) {
        int delta = (g.IO.MouseDelta.y < 0) ? 1 : -1;
        *value = std::clamp(*value + delta, minVal, maxVal);
        changed = (*value != oldVal);
    }

    ImDrawList* dl = window->DrawList;
    dl->AddRectFilled(rect.Min, rect.Max, hovered ? IM_COL32(32, 35, 42, 255) : IM_COL32(20, 22, 26, 255), 2.0f);
    dl->AddRect(rect.Min, rect.Max, IM_COL32(50, 52, 60, 255), 2.0f, 0, 1.0f);

    char buf[32];
    std::snprintf(buf, sizeof(buf), "%d", *value);
    ImVec2 txtSz = ImGui::CalcTextSize(buf);
    dl->AddText(ImVec2(rect.Min.x + (size.x - txtSz.x) * 0.5f, rect.Min.y + (size.y - txtSz.y) * 0.5f),
                IM_COL32(240, 240, 240, 255), buf);

    return changed;
}

int ModalList(const char* title,
              const std::vector<std::string>& items,
              int selectedIndex,
              const ImVec2& pos,
              const ImVec2& size) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    if (!dl) return selectedIndex;

    ImRect r(pos, ImVec2(pos.x + size.x, pos.y + size.y));
    dl->AddRectFilled(r.Min, r.Max, IM_COL32(14, 15, 18, 250), 3.0f);
    dl->AddRect(r.Min, r.Max, IM_COL32(70, 72, 80, 255), 3.0f, 0, 1.5f);

    float fontH = ImGui::GetFontSize();
    float itemH = fontH + 8.0f;
    float curY = r.Min.y + 6.0f;

    int newSel = selectedIndex;
    ImVec2 mPos = ImGui::GetIO().MousePos;
    bool clicked = ImGui::GetIO().MouseClicked[0];

    for (size_t i = 0; i < items.size(); ++i) {
        ImRect itemRect(ImVec2(r.Min.x + 3.0f, curY), ImVec2(r.Max.x - 3.0f, curY + itemH));
        bool hov = itemRect.Contains(mPos);
        if (hov && clicked) {
            newSel = static_cast<int>(i);
        }

        if (static_cast<int>(i) == selectedIndex) {
            dl->AddRectFilled(itemRect.Min, itemRect.Max, IM_COL32(0, 232, 190, 255), 2.0f);
            dl->AddText(ImVec2(itemRect.Min.x + 8.0f, curY + 4.0f), IM_COL32(0, 0, 0, 255), items[i].c_str());
        } else {
            if (hov) dl->AddRectFilled(itemRect.Min, itemRect.Max, IM_COL32(35, 38, 46, 255), 2.0f);
            dl->AddText(ImVec2(itemRect.Min.x + 8.0f, curY + 4.0f), IM_COL32(220, 220, 220, 255), items[i].c_str());
        }
        curY += itemH + 2.0f;
    }

    return newSel;
}

} // namespace trakr::hw


#define IMGUI_DEFINE_MATH_OPERATORS
#include "Widgets.h"
#include "imgui_internal.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace trakr::kit {

bool FlatButton(const char* label, const ImVec2& size, bool active) {
    ImGui::PushStyleColor(ImGuiCol_Button, active ? ImGui::GetColorU32(ImGuiCol_ButtonActive) : ImGui::GetColorU32(ImGuiCol_Button));
    bool clicked = ImGui::Button(label, size);
    ImGui::PopStyleColor();
    return clicked;
}

bool ToggleButton(const char* label, bool* v, const ImVec4& activeColor, const ImVec2& size) {
    if (!v) return false;
    bool active = *v;
    ImU32 col = active ? (activeColor.w > 0.0f ? ImGui::ColorConvertFloat4ToU32(activeColor) : ImGui::GetColorU32(ImGuiCol_ButtonActive))
                       : ImGui::GetColorU32(ImGuiCol_Button);
    ImGui::PushStyleColor(ImGuiCol_Button, col);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, col);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, col);
    bool clicked = ImGui::Button(label, size);
    if (clicked) *v = !(*v);
    ImGui::PopStyleColor(3);
    return clicked;
}

bool Knob(const char* label, float* v, float v_min, float v_max, const char* format, float radius) {
    if (!v) return false;
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);

    const float diameter = radius * 2.0f;
    const ImVec2 labelSize = ImGui::CalcTextSize(label, nullptr, true);
    const ImVec2 totalSize(std::max(diameter, labelSize.x), diameter + labelSize.y + style.ItemInnerSpacing.y);

    const ImRect bb(window->DC.CursorPos, ImVec2(window->DC.CursorPos.x + totalSize.x, window->DC.CursorPos.y + totalSize.y));
    ImGui::ItemSize(bb);
    if (!ImGui::ItemAdd(bb, id)) return false;

    bool hovered, held;
    bool valueChanged = false;
    bool pressed = ImGui::ButtonBehavior(ImRect(bb.Min, ImVec2(bb.Min.x + diameter, bb.Min.y + diameter)), id, &hovered, &held);
    (void)pressed;

    if (held) {
        float dragDelta = -g.IO.MouseDelta.y;
        if (g.IO.KeyShift) dragDelta *= 0.1f;
        float range = (v_max - v_min);
        float deltaVal = (dragDelta / 150.0f) * range;
        float oldVal = *v;
        *v = std::clamp(*v + deltaVal, v_min, v_max);
        if (*v != oldVal) valueChanged = true;
    }

    const ImVec2 center(bb.Min.x + diameter * 0.5f, bb.Min.y + diameter * 0.5f);
    const float norm = std::clamp((*v - v_min) / (v_max - v_min), 0.0f, 1.0f);
    constexpr float kAngleMin = -M_PI * 0.75f;
    constexpr float kAngleMax = M_PI * 0.75f;
    const float angle = kAngleMin + norm * (kAngleMax - kAngleMin);

    ImDrawList* drawList = window->DrawList;
    drawList->AddCircleFilled(center, radius, ImGui::GetColorU32(ImGuiCol_FrameBg));
    drawList->AddCircle(center, radius, ImGui::GetColorU32(held ? ImGuiCol_ButtonActive : (hovered ? ImGuiCol_Border : ImGuiCol_FrameBg)));

    // Indicator needle
    const ImVec2 needleEnd(center.x + std::sin(angle) * (radius * 0.8f), center.y - std::cos(angle) * (radius * 0.8f));
    drawList->AddLine(center, needleEnd, ImGui::GetColorU32(ImGuiCol_SliderGrabActive), 2.0f);

    // Label below
    ImVec2 labelPos(bb.Min.x + (totalSize.x - labelSize.x) * 0.5f, bb.Min.y + diameter + style.ItemInnerSpacing.y);
    drawList->AddText(labelPos, ImGui::GetColorU32(ImGuiCol_TextDisabled), label);

    if (hovered) {
        char valBuf[32];
        std::snprintf(valBuf, sizeof(valBuf), format, *v);
        Tooltip(valBuf);
    }

    return valueChanged;
}

bool MiniSlider(const char* label, float* v, float v_min, float v_max, const ImVec2& size, const char* format) {
    if (!v) return false;
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;

    const ImGuiID id = window->GetID(label);
    const ImVec2 realSize = ImGui::CalcItemSize(size, ImGui::GetContentRegionAvail().x, 18.0f);
    const ImRect bb(window->DC.CursorPos, ImVec2(window->DC.CursorPos.x + realSize.x, window->DC.CursorPos.y + realSize.y));
    ImGui::ItemSize(bb);
    if (!ImGui::ItemAdd(bb, id)) return false;

    bool hovered, held;
    ImGui::ButtonBehavior(bb, id, &hovered, &held);

    bool changed = false;
    if (held) {
        float norm = (GImGui->IO.MousePos.x - bb.Min.x) / bb.GetWidth();
        norm = std::clamp(norm, 0.0f, 1.0f);
        float newVal = v_min + norm * (v_max - v_min);
        if (*v != newVal) {
            *v = newVal;
            changed = true;
        }
    }

    float fillWidth = bb.GetWidth() * std::clamp((*v - v_min) / (v_max - v_min), 0.0f, 1.0f);
    ImDrawList* drawList = window->DrawList;
    drawList->AddRectFilled(bb.Min, bb.Max, ImGui::GetColorU32(ImGuiCol_FrameBg));
    drawList->AddRectFilled(bb.Min, ImVec2(bb.Min.x + fillWidth, bb.Max.y), ImGui::GetColorU32(held ? ImGuiCol_ButtonActive : ImGuiCol_SliderGrab));
    drawList->AddRect(bb.Min, bb.Max, ImGui::GetColorU32(hovered ? ImGuiCol_Border : ImGuiCol_Separator));

    char buf[32];
    std::snprintf(buf, sizeof(buf), format, *v);
    ImVec2 txtSize = ImGui::CalcTextSize(buf);
    ImVec2 txtPos(bb.Min.x + (bb.GetWidth() - txtSize.x) * 0.5f, bb.Min.y + (bb.GetHeight() - txtSize.y) * 0.5f);
    drawList->AddText(txtPos, ImGui::GetColorU32(ImGuiCol_Text), buf);

    return changed;
}

bool MiniSliderV(const char* label, float* v, float v_min, float v_max, const ImVec2& size) {
    if (!v) return false;
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;

    const ImGuiID id = window->GetID(label);
    const ImVec2 realSize = ImGui::CalcItemSize(size, ImGui::GetContentRegionAvail().x, 18.0f);
    const ImRect bb(window->DC.CursorPos, ImVec2(window->DC.CursorPos.x + realSize.x, window->DC.CursorPos.y + realSize.y));
    ImGui::ItemSize(bb);
    if (!ImGui::ItemAdd(bb, id)) return false;

    bool hovered, held;
    ImGui::ButtonBehavior(bb, id, &hovered, &held);

    bool changed = false;
    if (held) {
        float norm = 1.0f - (GImGui->IO.MousePos.y - bb.Min.y) / bb.GetHeight();
        norm = std::clamp(norm, 0.0f, 1.0f);
        float newVal = v_min + norm * (v_max - v_min);
        if (*v != newVal) {
            *v = newVal;
            changed = true;
        }
    }

    float fillHeight = bb.GetHeight() * std::clamp((*v - v_min) / (v_max - v_min), 0.0f, 1.0f);
    ImDrawList* drawList = window->DrawList;
    drawList->AddRectFilled(bb.Min, bb.Max, ImGui::GetColorU32(ImGuiCol_FrameBg));
    drawList->AddRectFilled(ImVec2(bb.Min.x, bb.Max.y - fillHeight), bb.Max, ImGui::GetColorU32(held ? ImGuiCol_ButtonActive : ImGuiCol_SliderGrab));
    drawList->AddRect(bb.Min, bb.Max, ImGui::GetColorU32(hovered ? ImGuiCol_Border : ImGuiCol_Separator));

    return changed;
}

bool ValueBox(const char* label, int* v, int v_min, int v_max, bool hex, const ImVec2& size) {
    if (!v) return false;
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;

    const ImGuiID id = window->GetID(label);
    const ImVec2 realSize = ImGui::CalcItemSize(size, ImGui::GetContentRegionAvail().x, 18.0f);
    const ImRect bb(window->DC.CursorPos, ImVec2(window->DC.CursorPos.x + realSize.x, window->DC.CursorPos.y + realSize.y));
    ImGui::ItemSize(bb);
    if (!ImGui::ItemAdd(bb, id)) return false;

    bool hovered, held;
    ImGui::ButtonBehavior(bb, id, &hovered, &held);

    bool changed = false;
    if (hovered && GImGui->IO.MouseWheel != 0.0f) {
        int newVal = std::clamp(*v + (GImGui->IO.MouseWheel > 0 ? 1 : -1), v_min, v_max);
        if (*v != newVal) { *v = newVal; changed = true; }
    } else if (held) {
        float delta = -GImGui->IO.MouseDelta.y;
        if (std::abs(delta) >= 1.0f) {
            int step = GImGui->IO.KeyShift ? 16 : 1;
            int newVal = std::clamp(*v + (delta > 0 ? step : -step), v_min, v_max);
            if (*v != newVal) { *v = newVal; changed = true; }
        }
    } else if (hovered && ImGui::IsMouseReleased(ImGuiMouseButton_Left) && GImGui->IO.MouseDragMaxDistanceSqr[0] < 4.0f) {
        int newVal = (*v >= v_max) ? v_min : (*v + 1);
        if (*v != newVal) { *v = newVal; changed = true; }
    } else if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
        int newVal = (*v <= v_min) ? v_max : (*v - 1);
        if (*v != newVal) { *v = newVal; changed = true; }
    }

    ImDrawList* drawList = window->DrawList;
    drawList->AddRectFilled(bb.Min, bb.Max, ImGui::GetColorU32(held ? ImGuiCol_FrameBgActive : ImGuiCol_FrameBg));
    drawList->AddRect(bb.Min, bb.Max, ImGui::GetColorU32(hovered ? ImGuiCol_Border : ImGuiCol_Separator));

    char buf[16];
    if (hex) {
        std::snprintf(buf, sizeof(buf), "%02X", *v & 0xFF);
    } else {
        std::snprintf(buf, sizeof(buf), "%d", *v);
    }
    ImVec2 txtSize = ImGui::CalcTextSize(buf);
    ImVec2 txtPos(bb.Min.x + (bb.GetWidth() - txtSize.x) * 0.5f, bb.Min.y + (bb.GetHeight() - txtSize.y) * 0.5f);
    drawList->AddText(txtPos, ImGui::GetColorU32(ImGuiCol_Text), buf);

    return changed;
}

void Meter(float peak, float decayPeak, const ImVec2& size, bool vertical) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return;

    const ImRect bb(window->DC.CursorPos, ImVec2(window->DC.CursorPos.x + size.x, window->DC.CursorPos.y + size.y));
    ImGui::ItemSize(bb);
    if (!ImGui::ItemAdd(bb, window->GetID(&peak))) return;

    ImDrawList* drawList = window->DrawList;
    drawList->AddRectFilled(bb.Min, bb.Max, ImGui::GetColorU32(ImGuiCol_FrameBg));

    const float norm = std::clamp(peak, 0.0f, 1.2f);
    const float clipThreshold = 1.0f;

    if (vertical) {
        const float activeH = bb.GetHeight() * std::min(1.0f, norm);
        ImVec2 fillMin(bb.Min.x, bb.Max.y - activeH);

        ImU32 col = (norm > clipThreshold) ? IM_COL32(240, 50, 50, 255)
                    : (norm > 0.8f) ? IM_COL32(230, 200, 40, 255) : IM_COL32(50, 200, 80, 255);
        drawList->AddRectFilled(fillMin, bb.Max, col);

        if (decayPeak >= 0.0f) {
            float decayH = bb.GetHeight() * std::clamp(decayPeak, 0.0f, 1.0f);
            drawList->AddLine(ImVec2(bb.Min.x, bb.Max.y - decayH), ImVec2(bb.Max.x, bb.Max.y - decayH), IM_COL32(255, 255, 255, 220), 1.0f);
        }
    } else {
        const float activeW = bb.GetWidth() * std::min(1.0f, norm);
        ImU32 col = (norm > clipThreshold) ? IM_COL32(240, 50, 50, 255)
                    : (norm > 0.8f) ? IM_COL32(230, 200, 40, 255) : IM_COL32(50, 200, 80, 255);
        drawList->AddRectFilled(bb.Min, ImVec2(bb.Min.x + activeW, bb.Max.y), col);
    }

    drawList->AddRect(bb.Min, bb.Max, ImGui::GetColorU32(ImGuiCol_Border));
}

void PanelHeader(const char* title, const char* rightLabel) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    const ImGuiStyle& style = GImGui->Style;
    float h = ImGui::GetTextLineHeight() + style.FramePadding.y * 2.0f;
    ImVec2 pos = window->DC.CursorPos;
    ImVec2 size(ImGui::GetContentRegionAvail().x, h);
    ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));

    ImGui::ItemSize(bb);
    if (!ImGui::ItemAdd(bb, window->GetID(title))) return;

    ImDrawList* drawList = window->DrawList;
    drawList->AddRectFilled(bb.Min, bb.Max, ImGui::GetColorU32(ImGuiCol_TitleBgActive));
    drawList->AddLine(ImVec2(bb.Min.x, bb.Max.y), bb.Max, ImGui::GetColorU32(ImGuiCol_Border));

    drawList->AddText(ImVec2(bb.Min.x + style.FramePadding.x, bb.Min.y + style.FramePadding.y),
                      ImGui::GetColorU32(ImGuiCol_Text), title);

    if (rightLabel) {
        ImVec2 rSize = ImGui::CalcTextSize(rightLabel);
        drawList->AddText(ImVec2(bb.Max.x - rSize.x - style.FramePadding.x, bb.Min.y + style.FramePadding.y),
                          ImGui::GetColorU32(ImGuiCol_TextDisabled), rightLabel);
    }
}

bool Splitter(bool vertical, float thickness, float* size1, float* size2, float min1, float min2) {
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    ImGuiID id = window->GetID("##Splitter");
    ImRect bb;
    bb.Min = window->DC.CursorPos + (vertical ? ImVec2(*size1, 0.0f) : ImVec2(0.0f, *size1));
    bb.Max = bb.Min + ImGui::CalcItemSize(vertical ? ImVec2(thickness, -1.0f) : ImVec2(-1.0f, thickness), 0.0f, 0.0f);
    return ImGui::SplitterBehavior(bb, id, vertical ? ImGuiAxis_X : ImGuiAxis_Y, size1, size2, min1, min2, 0.0f);
}

bool ListBox(const char* label, int* current_item, const std::vector<std::string>& items, int visible_items, float width) {
    if (!current_item) return false;
    float w = width > 0.0f ? width : ImGui::GetContentRegionAvail().x;
    float h = ImGui::GetTextLineHeightWithSpacing() * visible_items + GImGui->Style.FramePadding.y * 2.0f;

    bool changed = false;
    if (ImGui::BeginListBox(label, ImVec2(w, h))) {
        for (int i = 0; i < static_cast<int>(items.size()); ++i) {
            const bool is_selected = (*current_item == i);
            if (ImGui::Selectable(items[i].c_str(), is_selected)) { *current_item = i; changed = true; }
            if (is_selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndListBox();
    }
    return changed;
}

void WaveformView(const char* str_id, const float* samples, size_t count,
                  int* startMarker, int* loopStartMarker, int* loopEndMarker, int* endMarker,
                  const ImVec2& size, const std::vector<int>* slices, double playheadPos) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return;

    ImVec2 realSize = ImGui::CalcItemSize(size, ImGui::GetContentRegionAvail().x, 80.0f);
    ImRect bb(window->DC.CursorPos, window->DC.CursorPos + realSize);
    ImGui::ItemSize(bb);
    if (!ImGui::ItemAdd(bb, window->GetID(str_id))) return;

    ImDrawList* drawList = window->DrawList;
    drawList->AddRectFilled(bb.Min, bb.Max, ImGui::GetColorU32(ImGuiCol_FrameBg));
    drawList->AddRect(bb.Min, bb.Max, ImGui::GetColorU32(ImGuiCol_Border));

    const float midY = bb.Min.y + bb.GetHeight() * 0.5f;
    drawList->AddLine(ImVec2(bb.Min.x, midY), ImVec2(bb.Max.x, midY), IM_COL32(80, 90, 105, 180));

    if (samples && count > 1) {
        const float w = bb.GetWidth();
        const float hHalf = bb.GetHeight() * 0.45f;
        const int numCols = static_cast<int>(w);

        for (int x = 0; x < numCols; ++x) {
            size_t idx0 = static_cast<size_t>((static_cast<double>(x) / numCols) * count);
            size_t idx1 = static_cast<size_t>((static_cast<double>(x + 1) / numCols) * count);
            idx1 = std::clamp(idx1, idx0 + 1, count);

            float minVal = 0.0f, maxVal = 0.0f;
            for (size_t s = idx0; s < idx1; ++s) {
                minVal = std::min(minVal, samples[s]);
                maxVal = std::max(maxVal, samples[s]);
            }

            float px = bb.Min.x + x;
            drawList->AddLine(ImVec2(px, midY + minVal * hHalf), ImVec2(px, midY + maxVal * hHalf), IM_COL32(34, 187, 238, 255));
        }
    }

    if (slices && count > 0) {
        int sIdx = 1;
        for (int sFrame : *slices) {
            float norm = static_cast<float>(sFrame) / static_cast<float>(count);
            float x = bb.Min.x + norm * bb.GetWidth();
            drawList->AddLine(ImVec2(x, bb.Min.y), ImVec2(x, bb.Max.y), IM_COL32(0, 220, 255, 170), 1.0f);
            char sTag[8]; snprintf(sTag, sizeof(sTag), "S%d", sIdx++);
            drawList->AddText(ImVec2(x + 2, bb.Max.y - 14.0f), IM_COL32(0, 220, 255, 200), sTag);
        }
    }

    bool hovered, held;
    ImGui::ButtonBehavior(bb, window->GetID(str_id), &hovered, &held);
    if (held && count > 0) {
        float norm = std::clamp((GImGui->IO.MousePos.x - bb.Min.x) / bb.GetWidth(), 0.0f, 1.0f);
        int frame = static_cast<int>(norm * static_cast<float>(count));
        int* targets[] = { startMarker, loopStartMarker, loopEndMarker, endMarker };
        int* closest = nullptr;
        int minD = 999999999;
        for (int* m : targets) {
            if (m) { int d = std::abs(*m - frame); if (d < minD) { minD = d; closest = m; } }
        }
        if (closest) *closest = frame;
    }

    auto drawMarker = [&](int* marker, ImU32 col, const char* label, float offsetY) {
        if (!marker || count == 0) return;
        float norm = static_cast<float>(*marker) / static_cast<float>(count);
        float x = bb.Min.x + norm * bb.GetWidth();
        drawList->AddLine(ImVec2(x, bb.Min.y), ImVec2(x, bb.Max.y), col, 1.5f);
        ImVec2 tSz = ImGui::CalcTextSize(label);
        drawList->AddRectFilled(ImVec2(x + 1, bb.Min.y + offsetY), ImVec2(x + tSz.x + 4, bb.Min.y + offsetY + tSz.y + 1), IM_COL32(14, 16, 20, 210), 2.0f);
        drawList->AddText(ImVec2(x + 2, bb.Min.y + offsetY), col, label);
    };

    drawMarker(startMarker, IM_COL32(50, 220, 100, 255), "S", 2.0f);
    drawMarker(loopStartMarker, IM_COL32(230, 180, 40, 255), "LS", 18.0f);
    drawMarker(loopEndMarker, IM_COL32(230, 180, 40, 255), "LE", 18.0f); drawMarker(endMarker, IM_COL32(240, 70, 70, 255), "E", 2.0f);

    if (playheadPos >= 0.0 && count > 0) {
        float norm = static_cast<float>(playheadPos) / static_cast<float>(count);
        if (norm >= 0.0f && norm <= 1.0f) {
            float x = bb.Min.x + norm * bb.GetWidth();
            drawList->AddLine(ImVec2(x, bb.Min.y), ImVec2(x, bb.Max.y), IM_COL32(255, 255, 255, 240), 2.0f);
            drawList->AddTriangleFilled(ImVec2(x - 4, bb.Min.y), ImVec2(x + 4, bb.Min.y), ImVec2(x, bb.Min.y + 6), IM_COL32(255, 255, 255, 255));
        }
    }
}

void Tooltip(const char* text) {
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
        ImGui::BeginTooltip();
        ImGui::TextUnformatted(text);
        ImGui::EndTooltip();
    }
}

} // namespace trakr::kit

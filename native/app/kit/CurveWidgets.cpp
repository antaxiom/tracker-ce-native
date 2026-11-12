#include "CurveWidgets.h"
#include "imgui_internal.h"
#include <algorithm>
#include <cmath>

namespace trakr::kit {

void AdsrPlot(const char* str_id, float attackMs, float decayMs, float sustain, float releaseMs,
              const ImVec2& size) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return;

    ImVec2 realSize = ImGui::CalcItemSize(size, ImGui::GetContentRegionAvail().x, 70.0f);
    ImRect bb(window->DC.CursorPos, ImVec2(window->DC.CursorPos.x + realSize.x, window->DC.CursorPos.y + realSize.y));
    ImGui::ItemSize(bb);
    if (!ImGui::ItemAdd(bb, window->GetID(str_id))) return;

    ImDrawList* dl = window->DrawList;
    dl->AddRectFilled(bb.Min, bb.Max, IM_COL32(16, 18, 22, 255), 3.0f);
    dl->AddRect(bb.Min, bb.Max, IM_COL32(45, 52, 64, 255), 3.0f);

    const float padX = 8.0f, padY = 6.0f;
    const float w = bb.GetWidth() - padX * 2.0f;
    const float h = bb.GetHeight() - padY * 2.0f;
    const float baseLine = bb.Max.y - padY;
    const float topY = bb.Min.y + padY;

    // Time scaling: allocate proportional visual segments
    float aNorm = std::clamp(std::log10(std::max(1.0f, attackMs)) / 3.3f, 0.08f, 0.40f);
    float dNorm = std::clamp(std::log10(std::max(1.0f, decayMs)) / 3.7f, 0.08f, 0.35f);
    float rNorm = std::clamp(std::log10(std::max(1.0f, releaseMs)) / 3.7f, 0.08f, 0.35f);
    float sumNorm = aNorm + dNorm + rNorm;
    float scale = (sumNorm > 0.75f) ? (0.75f / sumNorm) : 1.0f;
    float wA = w * (aNorm * scale);
    float wD = w * (dNorm * scale);
    float wR = w * (rNorm * scale);
    float wS = std::max(15.0f, w - (wA + wD + wR));

    float sLevel = std::clamp(sustain, 0.0f, 1.0f);
    float susY = baseLine - sLevel * h;

    ImVec2 p0(bb.Min.x + padX, baseLine);
    ImVec2 pA(p0.x + wA, topY);
    ImVec2 pD(pA.x + wD, susY);
    ImVec2 pS(pD.x + wS, susY);
    ImVec2 pR(pS.x + wR, baseLine);

    // Fill polygon under curve
    ImVec2 poly[6] = { p0, pA, pD, pS, pR, ImVec2(pR.x, baseLine) };
    dl->AddConvexPolyFilled(poly, 6, IM_COL32(235, 130, 25, 45));

    // Glow curve
    const ImU32 colLine = IM_COL32(245, 145, 30, 255);
    dl->AddLine(p0, pA, colLine, 2.0f);
    dl->AddLine(pA, pD, colLine, 2.0f);
    dl->AddLine(pD, pS, colLine, 2.0f);
    dl->AddLine(pS, pR, colLine, 2.0f);

    // Nodes
    const ImU32 colNode = IM_COL32(0, 230, 215, 255);
    dl->AddCircleFilled(pA, 3.0f, colNode);
    dl->AddCircleFilled(pD, 3.0f, colNode);
    dl->AddCircleFilled(pS, 3.0f, colNode);
}

void FilterResponsePlot(const char* str_id, int filterType, float cutoffHz, float resonance,
                        const ImVec2& size) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return;

    ImVec2 realSize = ImGui::CalcItemSize(size, ImGui::GetContentRegionAvail().x, 70.0f);
    ImRect bb(window->DC.CursorPos, ImVec2(window->DC.CursorPos.x + realSize.x, window->DC.CursorPos.y + realSize.y));
    ImGui::ItemSize(bb);
    if (!ImGui::ItemAdd(bb, window->GetID(str_id))) return;

    ImDrawList* dl = window->DrawList;
    dl->AddRectFilled(bb.Min, bb.Max, IM_COL32(16, 18, 22, 255), 3.0f);
    dl->AddRect(bb.Min, bb.Max, IM_COL32(45, 52, 64, 255), 3.0f);

    const float padX = 8.0f, padY = 6.0f;
    const float w = bb.GetWidth() - padX * 2.0f;
    const float h = bb.GetHeight() - padY * 2.0f;
    const float midY = bb.Min.y + padY + h * 0.5f;

    // Cutoff frequency normalized on log scale: 20 Hz to 20,000 Hz
    float normCutoff = std::clamp((std::log10(std::max(20.0f, cutoffHz)) - std::log10(20.0f)) / 3.0f, 0.0f, 1.0f);
    float qPeak = std::clamp((resonance - 0.707f) * 0.25f, 0.0f, 0.45f);

    const int steps = 40;
    ImVec2 pts[steps + 1];
    for (int i = 0; i <= steps; ++i) {
        float fx = static_cast<float>(i) / steps;
        float x = bb.Min.x + padX + fx * w;
        float resp = 0.0f; // in normalized response: 0 = 0dB (midY), positive = boost, negative = cut

        if (filterType == 1) { // Lowpass
            float dist = fx - normCutoff;
            if (dist < 0.0f) {
                resp = std::exp(dist * 6.0f) * qPeak;
            } else {
                resp = qPeak * std::exp(-dist * 12.0f) - dist * 1.6f;
            }
        } else if (filterType == 2) { // Highpass
            float dist = fx - normCutoff;
            if (dist > 0.0f) {
                resp = std::exp(-dist * 6.0f) * qPeak;
            } else {
                resp = qPeak * std::exp(dist * 12.0f) + dist * 1.6f;
            }
        } else if (filterType == 3) { // Bandpass
            float dist = std::abs(fx - normCutoff);
            resp = (1.0f + qPeak * 1.5f) * std::exp(-dist * 8.0f) - 0.7f;
        }

        resp = std::clamp(resp, -0.9f, 0.8f);
        float y = midY - resp * (h * 0.48f);
        pts[i] = ImVec2(x, y);
    }

    const ImU32 colPlot = (filterType == 0) ? IM_COL32(120, 130, 145, 200) : IM_COL32(0, 220, 210, 255);
    for (int i = 0; i < steps; ++i) {
        dl->AddLine(pts[i], pts[i + 1], colPlot, 2.0f);
    }

    // Cutoff frequency indicator vertical marker
    if (filterType > 0) {
        float cx = bb.Min.x + padX + normCutoff * w;
        dl->AddLine(ImVec2(cx, bb.Min.y + padY), ImVec2(cx, bb.Max.y - padY), IM_COL32(235, 130, 25, 180), 1.0f);
    }
}

} // namespace trakr::kit


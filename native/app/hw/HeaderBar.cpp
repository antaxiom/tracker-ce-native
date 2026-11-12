#include "HeaderBar.h"
#include "Song.h"
#include "EngineHost.h"
#include "Theme.h"
#include "FontLoader.h"
#include "imgui_internal.h"
#include <cstdio>
#include <cmath>
#include <algorithm>

// Polyend Tracker TitleBar Parity
// Firmware reference: core/interface/titleBar.cpp, songTimer.cpp, core/source/Themes.cpp (titleBarColors = {grey_08, white})

namespace trakr::hw {

HeaderBar::HeaderBar() {
    timerBuf_[0] = '\0';
    rightBuf_[0] = '\0';
    patStepBuf_[0] = '\0';
}

void HeaderBar::render(const ImRect& rect,
                       const char* pageTitle,
                       const trakr::Song& song,
                       const trakr::EngineHost& engineHost,
                       int currentOrderIdx,
                       int& selectedInstIdx,
                       bool recordMode,
                       const trakr::Theme& theme,
                       float scale,
                       int cursorRow,
                       const char* rightLabelOverride,
                       bool firmwareChrome,
                       bool showInstrumentRight,
                       bool showBottomSeparator,
                       bool showTransport) {
    (void)theme;

    // The header window is exactly 28 device px tall. ImGui's default minimum
    // window size (32px) would extend it into the workspace and paint over the
    // first rows of the LCD, so clamp the min size for this window only.
    struct WindowMinSizeGuard {
        ImVec2 saved;
        WindowMinSizeGuard() : saved(ImGui::GetStyle().WindowMinSize) {
            ImGui::GetStyle().WindowMinSize = ImVec2(1.0f, 1.0f);
        }
        ~WindowMinSizeGuard() { ImGui::GetStyle().WindowMinSize = saved; }
    } minSizeGuard;

    ImGui::SetNextWindowPos(rect.Min);
    ImGui::SetNextWindowSize(rect.GetSize());
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    if (!ImGui::Begin("##TrackerHeaderBar", nullptr,
                      ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                      ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar |
                      ImGuiWindowFlags_NoScrollWithMouse)) {
        ImGui::End();
        ImGui::PopStyleVar(2);
        return;
    }
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    if (!drawList) {
        ImGui::End();
        ImGui::PopStyleVar(2);
        return;
    }

    // Header background: grey_08 #080808 (Themes.cpp titleBarColors[0])
    drawList->AddRectFilled(rect.Min, rect.Max, IM_COL32(8, 8, 8, 255));

    // Bottom separator line: White #FFFFFF (Themes.cpp titleBarColors[1]).
    // Firmware draws it at posY+28 (row 28, outside the label background); the
    // window clip ends at row 27, so firmware chrome draws it in the foreground.
    if (!firmwareChrome && showBottomSeparator) {
        drawList->AddRectFilled(ImVec2(rect.Min.x, rect.Max.y - 1.0f * scale),
                                ImVec2(rect.Max.x, rect.Max.y),
                                IM_COL32(255, 255, 255, 255));
    }

    ImFont* font = getMonoBoldFont();
    float fontH = font->LegacySize;
    float centerY = rect.Min.y + 2.5f * scale;

    // 1. Left Label: Page Title (e.g. "Pattern 1", "Performance FX")
    float leftX = rect.Min.x + 9.0f * scale;
    ImU32 textWhite = IM_COL32(255, 255, 255, 255);
    drawList->AddText(font, fontH, ImVec2(leftX, centerY), textWhite, pageTitle ? pageTitle : "Pattern 1");

    if (recordMode) {
        float titleW = font->CalcTextSizeA(fontH, FLT_MAX, 0.0f, pageTitle ? pageTitle : "Pattern 1").x;
        ImVec2 recPos(leftX + titleW + 8.0f * scale, centerY + 1.0f);
        drawList->AddRectFilled(recPos, ImVec2(recPos.x + 28.0f * scale, recPos.y + fontH - 2.0f),
                                IM_COL32(205, 24, 24, 255), 2.0f);
        drawList->AddText(font, fontH, ImVec2(recPos.x + 3.0f * scale, recPos.y - 1.0f),
                          IM_COL32(255, 255, 255, 255), "REC");
    }

    bool isSongScreen = (pageTitle && std::string_view(pageTitle) == "Song");
    const bool hasOverride = (rightLabelOverride != nullptr && rightLabelOverride[0] != '\0');
    if (hasOverride) {
        // File page: title bar shows the current project name on the right
        // instead of transport/pattern info (firmware titleBar setRightLabel).
        ImVec2 rightTextSize = font->CalcTextSizeA(fontH, FLT_MAX, 0.0f, rightLabelOverride);
        float rightX = firmwareChrome
                         ? rect.Min.x + 790.0f * scale - rightTextSize.x
                         : rect.Min.x + 700.0f * scale - rightTextSize.x * 0.5f;
        drawList->AddText(font, fontH, ImVec2(rightX, centerY), textWhite, rightLabelOverride);
    } else {
        // Center transport (songTimer.cpp: "0m:00s  ||  P1.001"). The firmware
        // only composes this label on screens whose spec carries it (Pattern);
        // other screens must not show it. Gate explicitly, not on firmwareChrome.
        float centerX = 0.0f;
        float centerRight = 0.0f;
        bool drewTransport = false;
        if (showTransport && !isSongScreen) {
            drewTransport = true;
            static double s_playTimeSeconds = 0.0;
            const auto& playhead = engineHost.getPlayhead();
            if (playhead.playing) {
                s_playTimeSeconds += ImGui::GetIO().DeltaTime;
            } else {
                s_playTimeSeconds = 0.0;
            }
            uint32_t totalSecs = static_cast<uint32_t>(s_playTimeSeconds);
            uint32_t mins = totalSecs / 60;
            uint32_t secs = totalSecs % 60;

            // Keep the transport slot as two spaces; the play/loop glyph is drawn
            // as vector art (RobotoMono has no U+25B6/U+21BB -> tofu otherwise).
            const char* transportGlyph = playhead.playing ? "  " : "||";
            int curPat = (currentOrderIdx >= 0 && currentOrderIdx < static_cast<int>(song.order.size()))
                             ? song.order[currentOrderIdx]
                             : 0;
            int curStep = playhead.playing ? playhead.row : cursorRow;

            std::snprintf(timerBuf_, sizeof(timerBuf_), "%3um:%02us %2s P%d.%03d ",
                          mins, secs, transportGlyph, curPat + 1, curStep + 1);

            centerX = rect.Min.x + 280.0f * scale;
            centerRight = centerX + font->CalcTextSizeA(fontH, FLT_MAX, 0.0f, timerBuf_).x;
            ImU32 timerCol = playhead.playing ? IM_COL32(0, 232, 190, 255) : textWhite;
            drawList->AddText(font, fontH, ImVec2(centerX, centerY), timerCol, timerBuf_);

            if (playhead.playing) {
                char prefix[24];
                std::snprintf(prefix, sizeof(prefix), "%um:%02us  ", mins, secs);
                const float slotX = centerX + font->CalcTextSizeA(fontH, FLT_MAX, 0.0f, prefix).x;
                const float slotW = font->CalcTextSizeA(fontH, FLT_MAX, 0.0f, "  ").x;
                const float cy = centerY + fontH * 0.5f;
                const float h2 = fontH * 0.28f;
                if (engineHost.isPatternLoop()) {
                    const ImVec2 c(slotX + slotW * 0.5f, cy);
                    const float r = h2 * 1.4f;
                    drawList->PathArcTo(c, r, IM_PI * 0.35f, IM_PI * 1.95f, 20);
                    drawList->PathStroke(timerCol, 0, 1.6f * scale);
                    const float ax = c.x + r * std::cos(IM_PI * 1.95f);
                    const float ay = c.y + r * std::sin(IM_PI * 1.95f);
                    drawList->AddTriangleFilled(ImVec2(ax - 2.5f * scale, ay - 3.0f * scale),
                                                ImVec2(ax + 3.0f * scale, ay - 0.5f * scale),
                                                ImVec2(ax - 1.5f * scale, ay + 3.0f * scale), timerCol);
                } else {
                    drawList->AddTriangleFilled(ImVec2(slotX + slotW * 0.32f, cy - h2),
                                                ImVec2(slotX + slotW * 0.32f, cy + h2),
                                                ImVec2(slotX + slotW * 0.74f, cy), timerCol);
                }
            }
        }

        // Right label (titleBar.cpp showCurrentInstrumentOnRightLabel). Only the
        // screens whose reference carries a rightLabel pass showInstrumentRight.
        if (showInstrumentRight) {
            rightBuf_[0] = '\0';
            if (selectedInstIdx >= 0 && selectedInstIdx < static_cast<int>(song.instruments.size())) {
                std::snprintf(rightBuf_, sizeof(rightBuf_), "%d. %s",
                              selectedInstIdx + 1,
                              song.instruments[selectedInstIdx].name.c_str());
            } else {
                std::snprintf(rightBuf_, sizeof(rightBuf_), "%d. Empty slot", selectedInstIdx + 1);
            }

            ImVec2 rightTextSize = font->CalcTextSizeA(fontH, FLT_MAX, 0.0f, rightBuf_);
            float rightX = rect.Min.x + 790.0f * scale - rightTextSize.x;
            if (!drewTransport || rightX > centerRight + 10.0f) {
                ImRect instRect(ImVec2(rightX - 4.0f, rect.Min.y + 2.0f), ImVec2(rect.Max.x - 4.0f, rect.Max.y - 2.0f));
                bool hov = instRect.Contains(ImGui::GetIO().MousePos);
                if (hov && !song.instruments.empty()) {
                    if (ImGui::IsMouseClicked(0)) {
                        selectedInstIdx = (selectedInstIdx + 1) % static_cast<int>(song.instruments.size());
                    } else if (ImGui::IsMouseClicked(1)) {
                        selectedInstIdx = (selectedInstIdx + static_cast<int>(song.instruments.size()) - 1) % static_cast<int>(song.instruments.size());
                    } else if (ImGui::GetIO().MouseWheel != 0.0f) {
                        int d = (ImGui::GetIO().MouseWheel > 0.0f) ? -1 : 1;
                        selectedInstIdx = std::clamp(selectedInstIdx + d, 0, static_cast<int>(song.instruments.size()) - 1);
                    }
                }
                drawList->AddText(font, fontH, ImVec2(rightX, centerY), hov ? IM_COL32(254, 247, 73, 255) : textWhite, rightBuf_);
            }
        }

        auto masterMeters = engineHost.getMasterMeters();
        if (drewTransport && (masterMeters.peakL > 1.0f || masterMeters.peakR > 1.0f)) {
            drawList->AddLine(ImVec2(centerX - 40.0f, rect.Min.y),
                              ImVec2(centerX + 40.0f, rect.Min.y), IM_COL32(205, 24, 24, 255), 2.0f);
        }
    }

    ImGui::End();
    ImGui::PopStyleVar(2);

    if (firmwareChrome) {
        ImGui::GetForegroundDrawList()->AddRectFilled(
            ImVec2(rect.Min.x, rect.Max.y),
            ImVec2(rect.Max.x, rect.Max.y + 1.0f * scale),
            IM_COL32(255, 255, 255, 255));
    }
}

} // namespace trakr::hw

#include "HeaderBar.h"
#include "Song.h"
#include "EngineHost.h"
#include "Theme.h"
#include "FontLoader.h"
#include "BitmapText.h"
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

    // Bottom separator line: White #FFFFFF (Themes.cpp titleBarColors[1]),
    // drawn at device row 28 (topBarLabel posY+height), i.e. just outside the
    // 28px header window, so it goes to the foreground list for every screen.
    if (showBottomSeparator) {
        ImGui::GetForegroundDrawList()->AddRectFilled(
            ImVec2(rect.Min.x, rect.Max.y), ImVec2(rect.Max.x, rect.Max.y + 1.0f * scale),
            IM_COL32(255, 255, 255, 255));
    }

    ImFont* font = getMonoBoldFont();
    float fontH = font->LegacySize;
    float centerY = rect.Min.y + 2.5f * scale;

    // titleBar.cpp labels use controlStyleFont4 = the Bold17 L4 bitmap font at
    // y centre 13; the bitmap cell top is device row 4 (ink 8..19).
    constexpr float kTitleCellTop = 2.0f;
    auto bWidth = [](const char* t) {
        return static_cast<float>(trakr::hw::fwTextWidth(trakr::hw::FwFont::Bold17, t));
    };
    auto bText = [&](float devX, const char* t, ImU32 col) {
        trakr::hw::fwDrawTextOn(drawList, trakr::hw::FwFont::Bold17,
                                rect.Min.x + devX * scale,
                                rect.Min.y + kTitleCellTop * scale, t, col,
                                IM_COL32(8, 8, 8, 255), scale, scale);
    };
    ImU32 textWhite = IM_COL32(255, 255, 255, 255);

    // 1. Left Label: Page Title (e.g. "Pattern 1", "Performance FX")
    const char* titleText = pageTitle ? pageTitle : "Pattern 1";
    bText(9.0f, titleText, textWhite);

    if (recordMode) {
        float titleW = bWidth(titleText) * scale;
        ImVec2 recPos(rect.Min.x + 9.0f * scale + titleW + 8.0f * scale, centerY + 1.0f);
        drawList->AddRectFilled(recPos, ImVec2(recPos.x + 28.0f * scale, recPos.y + fontH - 2.0f),
                                IM_COL32(205, 24, 24, 255), 2.0f);
        drawList->AddText(font, fontH, ImVec2(recPos.x + 3.0f * scale, recPos.y - 1.0f),
                          IM_COL32(255, 255, 255, 255), "REC");
    }


    // Center transport (songTimer.cpp: "0m:00s  ||  P1.001").
    // Interface::activateModule() shows songTimer on every module except the
    // pattern editor (interface.cpp:417-421), the Song screen included; the
    // File page keeps it too, beside the right-aligned project name.
    float centerX = 0.0f;
    float centerRight = 0.0f;
    bool drewTransport = false;
    if (showTransport) {
        drewTransport = true;
        // Song time comes from the audio clock (cSongTimer), never from
        // accumulated frame deltas, so any frame rate shows the same value.
        const auto& playhead = engineHost.getPlayhead();
        const uint32_t totalSecs = static_cast<uint32_t>(playhead.songSeconds);
        uint32_t mins = totalSecs / 60;
        uint32_t secs = totalSecs % 60;

        // songTimer.cpp: stopped state shows the Bold17 stop-icon pair
        // ("\x18\x19"); playing uses "\x1A\x1B" (song) / "\x1C\x1D"
        // (pattern). The icons exist only in the L4 Bold17 bitmap font.
        const char* transportGlyph = playhead.playing ? "\x1C\x1D" : "\x18\x19";
        int curPat = (currentOrderIdx >= 0 && currentOrderIdx < static_cast<int>(song.order.size()))
                         ? song.order[currentOrderIdx]
                         : 0;
        int curStep = playhead.playing ? playhead.row : cursorRow;

        // songTimer.cpp:144 timerFormat "%3im:%02is %2s P%i.%03i%2s".
        std::snprintf(timerBuf_, sizeof(timerBuf_), "%3um:%02us %2s P%d.%03d%2s",
                      mins, secs, transportGlyph, curPat + 1, curStep + 1, "");

        centerX = rect.Min.x + 280.0f * scale;
        centerRight = centerX + bWidth(timerBuf_) * scale;
        // songTimerLabelColors[0] (themeDefault) is white.
        ImU32 timerCol = textWhite;
        bText(280.0f, timerBuf_, timerCol);
    }

    const bool hasOverride = (rightLabelOverride != nullptr && rightLabelOverride[0] != '\0');
    if (hasOverride) {
        // File page: title bar shows the current project name on the right
        // instead of transport/pattern info (firmware titleBar setRightLabel).
        // titleBar.cpp: rightLabel prop.x = 790 with controlStyleRightX, so the
        // name is right-aligned to the bar edge, not centred.
        bText(790.0f - bWidth(rightLabelOverride), rightLabelOverride, textWhite);
    } else {
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

            const float rightW = bWidth(rightBuf_) * scale;
            float rightX = rect.Min.x + 790.0f * scale - rightW;
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
                bText(790.0f - bWidth(rightBuf_), rightBuf_,
                      hov ? IM_COL32(254, 247, 73, 255) : textWhite);
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

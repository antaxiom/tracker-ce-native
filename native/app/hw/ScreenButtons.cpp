#include "ScreenButtons.h"
#include "FontLoader.h"
#include "TextFit.h"
#include "imgui_internal.h"
#include <algorithm>

// Polyend Tracker Bottom Screen Buttons Parity
// Firmware reference: core/libs/NewDisplay/bgLabel.cpp, label.cpp, frame.cpp, core/source/Themes.cpp

namespace trakr::hw {

ScreenButtons::ScreenButtons() {
    buttons_.resize(8);
}

void ScreenButtons::setButtons(const std::vector<ScreenButton>& buttons) {
    buttons_ = buttons;
    if (buttons_.size() < 8) {
        buttons_.resize(8);
    }
}

int ScreenButtons::renderFirmware(const ImRect& rect, bool shiftHeld, float scale) {
    if (rect.GetWidth() <= 0.0f || rect.GetHeight() <= 0.0f) return -1;

    ImGui::SetNextWindowPos(rect.Min);
    ImGui::SetNextWindowSize(rect.GetSize());
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    if (!ImGui::Begin("##TrackerScreenButtons", nullptr,
                      ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                      ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar |
                      ImGuiWindowFlags_NoScrollWithMouse)) {
        ImGui::End();
        ImGui::PopStyleVar(2);
        return -1;
    }

    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImGuiIO& io = ImGui::GetIO();

    // bgLabel (bgLabel.cpp): x=1..797, y=425..478, grey_0A; separators at 100i.
    dl->AddRectFilled(ImVec2(rect.Min.x + 1.0f * scale, rect.Min.y),
                      ImVec2(rect.Min.x + 798.0f * scale, rect.Min.y + 54.0f * scale),
                      IM_COL32(10, 10, 10, 255));

    const float btnW = rect.GetWidth() / 8.0f;
    int clickedIdx = -1;
    ImFont* nameFont = getMonoFont();
    ImFont* valueFont = getMonoLightFont();

    for (int i = 0; i < 8; ++i) {
        const auto& btn = buttons_[i];
        const float bx = rect.Min.x + i * btnW;
        const float cx = bx + btnW * 0.5f;
        ImRect bRect(ImVec2(bx, rect.Min.y), ImVec2(bx + btnW, rect.Max.y));
        if (bRect.Contains(io.MousePos) && io.MouseClicked[0] && btn.enabled) {
            clickedIdx = i;
            if (btn.onClick) btn.onClick();
        }

        if (i > 0) {
            dl->AddRectFilled(ImVec2(bx, rect.Min.y + 13.0f * scale),
                              ImVec2(bx + 1.0f * scale, rect.Min.y + 55.0f * scale),
                              IM_COL32(119, 119, 119, 255));
        }

        const std::string& topStr = (shiftHeld && !btn.shiftLabel.empty()) ? btn.shiftLabel : btn.label;
        const std::string& btmStr = (shiftHeld && !btn.shiftValue.empty()) ? btn.shiftValue : btn.value;
        const bool twoLine = !btmStr.empty();

        const bool dimmed = (btn.underline == UnderlineColor::Dimmed);
        ImU32 topCol = dimmed ? IM_COL32(0x33, 0x33, 0x33, 255) : IM_COL32(0x77, 0x77, 0x77, 255);
        ImU32 btmCol = dimmed ? IM_COL32(0x57, 0x57, 0x57, 255) : IM_COL32(255, 255, 255, 255);

        const float pad = 2.0f * scale;
        if (!topStr.empty()) {
            float centerTopY = rect.Min.y + (twoLine ? 16.0f : 26.0f) * scale;
            ui::drawTextFit(dl, nameFont, bx + pad, bx + btnW - pad, centerTopY,
                            topCol, topStr.c_str(), true, 8.0f * scale, 7.0f * scale,
                            "ScreenButtons");
        }

        if (btn.label == "More" && btmStr == " ") {
            // label.cpp: count=2, spacing=13, y = posY+height-16 = 463 (bar top 425)
            const float dotsY = rect.Min.y + 38.0f * scale;
            dl->AddRectFilled(ImVec2(cx - 7.0f * scale, dotsY - 1.0f * scale),
                              ImVec2(cx - 4.0f * scale, dotsY + 2.0f * scale),
                              IM_COL32(0x77, 0x77, 0x77, 255));
            dl->AddRectFilled(ImVec2(cx + 6.0f * scale, dotsY - 1.0f * scale),
                              ImVec2(cx + 9.0f * scale, dotsY + 2.0f * scale),
                              IM_COL32(0x22, 0x22, 0x22, 255));
        } else if (twoLine) {
            float centerBtmY = rect.Min.y + 36.0f * scale;
            // gInstrEdit.cpp:762-765: the Filter Type value uses Font3
            // (Regular14) and activeButtonLabelsColors[3] = grey_77.
            const bool filterType = (btn.label == "Filter Type");
            ui::drawTextFit(dl, filterType ? nameFont : valueFont, bx + pad, bx + btnW - pad,
                            centerBtmY, filterType ? topCol : btmCol, btmStr.c_str(), true,
                            10.0f * scale, 7.0f * scale, "ScreenButtons");
        }
    }

    ImGui::End();
    ImGui::PopStyleVar(2);
    return clickedIdx;
}

int ScreenButtons::render(const ImRect& rect, bool shiftHeld, float scale) {
    if (firmwareMode_) return renderFirmware(rect, shiftHeld, scale);
    if (rect.GetWidth() <= 0.0f || rect.GetHeight() <= 0.0f) return -1;

    ImGui::SetNextWindowPos(rect.Min);
    ImGui::SetNextWindowSize(rect.GetSize());
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    if (!ImGui::Begin("##TrackerScreenButtons", nullptr,
                      ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                      ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar |
                      ImGuiWindowFlags_NoScrollWithMouse)) {
        ImGui::End();
        ImGui::PopStyleVar(2);
        return -1;
    }

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    if (!drawList) {
        ImGui::End();
        ImGui::PopStyleVar(2);
        return -1;
    }

    ImGuiIO& io = ImGui::GetIO();
    ImVec2 mousePos = io.MousePos;
    bool mouseClicked = io.MouseClicked[0];

    // Background bar: grey_0A #0A0A0A (bgLabel.cpp)
    drawList->AddRectFilled(rect.Min, rect.Max, IM_COL32(10, 10, 10, 255));

    float btnW = rect.GetWidth() / 8.0f;
    float btnH = rect.GetHeight();
    int clickedIdx = -1;

    for (size_t i = 0; i < 8; ++i) {
        const auto& btn = buttons_[i];
        float bx = rect.Min.x + i * btnW;
        float cx = bx + btnW * 0.5f;
        ImRect bRect(ImVec2(bx, rect.Min.y), ImVec2(bx + btnW, rect.Max.y));

        bool hovered = bRect.Contains(mousePos);
        if (hovered && mouseClicked && btn.enabled) {
            clickedIdx = static_cast<int>(i);
            if (btn.onClick) {
                btn.onClick();
            }
        }

        bool isMoreBtn = (btn.label == "More");
        bool isSolidOrange = (btn.active && btn.underline == UnderlineColor::Orange);
        bool isPerformMode = (!buttons_.empty() && buttons_[0].value.rfind("Patt:", 0) == 0);

        // Background fill
        if (isSolidOrange) {
            // Armed track in Perform mode: solid orange-red fill matching FT812ScreensCore.cpp (y=424..479)
            drawList->PushClipRectFullScreen();
            drawList->AddRectFilled(ImVec2(bx + 1.0f * scale, rect.Min.y - 1.0f * scale),
                                    ImVec2(bx + 1.0f * scale + 98.0f * scale, rect.Min.y + 54.0f * scale),
                                    IM_COL32(230, 74, 25, 255));
            drawList->PopClipRect();
        } else if (hovered && btn.enabled) {
            drawList->AddRectFilled(bRect.Min, bRect.Max, IM_COL32(22, 24, 28, 255));
        }

        // Vertical divider between buttons: 1px line at bx = i * 100 from y = 438..479 in #777777
        bool prevSolid = (i > 0 && buttons_[i - 1].active && buttons_[i - 1].underline == UnderlineColor::Orange);
        if (i > 0 && !isSolidOrange && !prevSolid) {
            drawList->AddRectFilled(ImVec2(bx, rect.Min.y + 13.0f * scale),
                                    ImVec2(bx + 1.0f * scale, rect.Min.y + 55.0f * scale),
                                    IM_COL32(119, 119, 119, 255));
        }

        // Active frame outline: 2px outline at bx + 1, 425, 98, 54 in #CD1818.
        // Pattern mirrors framesPlaces[0] = {1,423,99,55}: the 1.5px border
        // straddles x=0..101 / y=422..479.
        if (btn.active && !isSolidOrange) {
            float ox = patternChrome_ ? bx : bx + 1.0f * scale;
            float oy = patternChrome_ ? rect.Min.y - 3.0f * scale : rect.Min.y;
            float ow = (patternChrome_ ? 101.0f : 98.0f) * scale;
            float oh = (patternChrome_ ? 57.0f : 54.0f) * scale;
            float ot = 2.0f * scale;
            ImU32 oCol = IM_COL32(205, 24, 24, 255);
            // The pattern frame starts above the button window (y=422), so it
            // must be drawn in the foreground list to escape the window clip.
            ImDrawList* fdl = patternChrome_ ? ImGui::GetForegroundDrawList() : drawList;
            fdl->AddRectFilled(ImVec2(ox, oy), ImVec2(ox + ow, oy + ot), oCol);
            fdl->AddRectFilled(ImVec2(ox, oy + oh - ot), ImVec2(ox + ow, oy + oh), oCol);
            fdl->AddRectFilled(ImVec2(ox, oy), ImVec2(ox + ot, oy + oh), oCol);
            fdl->AddRectFilled(ImVec2(ox + ow - ot, oy), ImVec2(ox + ow, oy + oh), oCol);
        }

        const std::string& topStr = (shiftHeld && !btn.shiftLabel.empty()) ? btn.shiftLabel : btn.label;
        const std::string& btmStr = (shiftHeld && !btn.shiftValue.empty()) ? btn.shiftValue : btn.value;
        bool twoLine = (!btmStr.empty());

        bool isDimSingle = (btmStr.empty() && btn.underline == UnderlineColor::Red && !btn.active);
        ImU32 topCol = isSolidOrange ? IM_COL32(255, 255, 255, 255)
                                     : (!btn.enabled || isDimSingle ? IM_COL32(119, 119, 119, 255)
                                                                    : IM_COL32(255, 255, 255, 255));
        ImU32 btmCol = isSolidOrange ? IM_COL32(255, 255, 255, 255)
                                     : (btn.enabled ? IM_COL32(255, 255, 255, 255)
                                                    : IM_COL32(119, 119, 119, 255));

        ImFont* font = getMonoBoldFont();
        float fontSz = font->LegacySize;
        (void)fontSz;

        // Text must stay inside this button cell: long second-page option
        // names (Destination, Reverb send, Bit depth, ...) shrink to fit
        // instead of bleeding into the neighbouring button.
        const float textPad = 2.0f * scale;
        const float textX0 = bx + textPad;
        const float textX1 = bx + btnW - textPad;

        // 1. Top label: centered at y = 440 (two-line) or y = 452 (single-line)
        float xOffset = (isPerformMode && !isSolidOrange) ? 1.0f * scale : -0.5f * scale;
        if (!topStr.empty() && patternChrome_) {
            // gPatternEdit labels: Font3 (Regular14, ~7.1px advance) in
            // activeLabelsColors[0] = grey_77 (grey_33 when disabled).
            ImFont* lf = getMonoLightFont();
            const float fs = 15.2f * scale;
            const float adv = 7.1f * scale;
            const float w = static_cast<float>(topStr.size()) * adv;
            const float sx = cx - w * 0.5f;
            const float cy = rect.Min.y + (twoLine ? 15.0f : 27.0f) * scale;
            const ImVec2 sample = lf->CalcTextSizeA(fs, FLT_MAX, 0.0f, "Xg");
            const float dy = cy - sample.y * 0.5f;
            ImU32 tc = btn.enabled ? IM_COL32(0x77, 0x77, 0x77, 255) : IM_COL32(0x33, 0x33, 0x33, 255);
            char ch[2] = {0, 0};
            for (size_t k = 0; k < topStr.size(); ++k) {
                ch[0] = topStr[k];
                drawList->AddText(lf, fs, ImVec2(sx + static_cast<float>(k) * adv, dy), tc, ch);
            }
        } else if (!topStr.empty()) {
            float centerTopY = rect.Min.y + (isPerformMode ? 16.0f : (twoLine ? 15.0f : 27.0f)) * scale;
            ui::drawTextFit(drawList, font, textX0 + xOffset, textX1 + xOffset, centerTopY,
                            topCol, topStr.c_str(), true, 10.0f * scale, 7.0f * scale,
                            "ScreenButtons");
        }

        // 2. Bottom value: centered at y = 461
        float centerBtmY = rect.Min.y + (isPerformMode ? 38.0f : 36.0f) * scale;
        if (twoLine && !btmStr.empty()) {
            ui::drawTextFit(drawList, font, textX0 + xOffset, textX1 + xOffset, centerBtmY,
                            btmCol, btmStr.c_str(), true, 10.0f * scale, 7.0f * scale,
                            "ScreenButtons");
        }

        // 3. Coloured 80px underline indicator at y = 474 (cx - 40 to cx + 40)
        if (btn.underline != UnderlineColor::None && !isSolidOrange && !isMoreBtn) {
            ImU32 uCol = 0;
            switch (btn.underline) {
                case UnderlineColor::Green:  uCol = IM_COL32(48, 203, 63, 255); break;  // #30CB3F
                case UnderlineColor::Red:    uCol = IM_COL32(205, 24, 24, 255); break;  // #CD1818
                case UnderlineColor::Orange: uCol = IM_COL32(214, 112, 34, 255); break; // #D67022
                case UnderlineColor::Dimmed: uCol = 0; break;
                default: break;
            }
            if (uCol != 0) {
                // Pattern labels (gPatternEdit.cpp) carry a 1px line at y=474;
                // the other screens use the 2px 473..474 underline.
                float lineY = rect.Min.y + (patternChrome_ ? 49.0f : 48.0f) * scale;
                float lineH = (patternChrome_ ? 1.0f : 2.0f) * scale;
                float halfW = 40.0f * scale;
                drawList->AddRectFilled(ImVec2(cx - halfW, lineY),
                                        ImVec2(cx + halfW + 1.0f * scale, lineY + lineH),
                                        uCol);
            }
        }
    }

    ImGui::End();
    ImGui::PopStyleVar(2);

    return clickedIdx;
}

bool ScreenButtons::handleKeyShortcuts(bool shiftHeld) {
    (void)shiftHeld;
    static const ImGuiKey keys[8] = {
        ImGuiKey_F1, ImGuiKey_F2, ImGuiKey_F3, ImGuiKey_F4,
        ImGuiKey_F5, ImGuiKey_F6, ImGuiKey_F7, ImGuiKey_F8
    };

    for (size_t i = 0; i < 8; ++i) {
        if (ImGui::IsKeyPressed(keys[i], false)) {
            if (i < buttons_.size() && buttons_[i].enabled && buttons_[i].onClick) {
                buttons_[i].onClick();
                return true;
            }
        }
    }
    return false;
}

} // namespace trakr::hw

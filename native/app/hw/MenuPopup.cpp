#include "MenuPopup.h"
#include <algorithm>

namespace trakr::hw {

static const char* kModuleNames[12] = {
    "Pattern", "Instrument", "Sample Play", "Master",
    "Song", "Sample Load", "File", "Config",
    "Perform", "Sample Edit", "Sample Rec", "Close"
};

bool MenuPopup::render(const ImRect& displayRect,
                       trakr::Screen& activeScreen,
                       bool& showMenuPopup,
                       bool& showExportDeck,
                       trakr::ScreenContext& scrCtx,
                       float scale) {
    if (!showMenuPopup) {
        openFrameCount_ = 0;
        return false;
    }
    openFrameCount_++;
    (void)showExportDeck;

    ImDrawList* dl = ImGui::GetForegroundDrawList();
    if (!dl) return false;

    // Semi-transparent backdrop
    dl->AddRectFilled(displayRect.Min, displayRect.Max, IM_COL32(0, 0, 0, 160));

    float w = 440.0f * scale;
    float h = 330.0f * scale;
    float x = displayRect.Min.x + (displayRect.GetWidth() - w) * 0.5f;
    float y = displayRect.Min.y + (displayRect.GetHeight() - h) * 0.5f;
    ImRect popupRect(ImVec2(x, y), ImVec2(x + w, y + h));

    // Outer frame (Themes.cpp miniBoxMenuColors: bg black, frame one_true_red #CD1818)
    dl->AddRectFilled(popupRect.Min, popupRect.Max, IM_COL32(0, 0, 0, 255), 4.0f);
    dl->AddRect(popupRect.Min, popupRect.Max, IM_COL32(205, 24, 24, 255), 4.0f, 0, 2.0f);

    // Title header
    dl->AddText(ImVec2(x + 14.0f * scale, y + 10.0f * scale), IM_COL32(200, 200, 200, 255), "MENU");
    dl->AddLine(ImVec2(x + 10.0f * scale, y + 28.0f * scale),
                ImVec2(x + w - 10.0f * scale, y + 28.0f * scale), IM_COL32(60, 60, 60, 255));

    ImGuiIO& io = ImGui::GetIO();
    if (ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
        showMenuPopup = false;
        return true;
    }
    if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow, false))  selectedBox_ = (selectedBox_ % 4 == 0) ? selectedBox_ + 3 : selectedBox_ - 1;
    if (ImGui::IsKeyPressed(ImGuiKey_RightArrow, false)) selectedBox_ = (selectedBox_ % 4 == 3) ? selectedBox_ - 3 : selectedBox_ + 1;
    if (ImGui::IsKeyPressed(ImGuiKey_UpArrow, false))    selectedBox_ = (selectedBox_ < 4) ? selectedBox_ + 8 : selectedBox_ - 4;
    if (ImGui::IsKeyPressed(ImGuiKey_DownArrow, false))  selectedBox_ = (selectedBox_ >= 8) ? selectedBox_ - 8 : selectedBox_ + 4;
    if (io.MouseWheel != 0.0f) selectedBox_ = std::clamp(selectedBox_ - (io.MouseWheel > 0 ? 1 : -1), 0, 11);

    bool activate = ImGui::IsKeyPressed(ImGuiKey_Enter, false) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, false);

    // 4 Columns x 3 Rows
    const int kCols = 4, kRows = 3;
    float startX = x + 12.0f * scale;
    float startY = y + 36.0f * scale;
    float pad = 8.0f * scale;
    float boxW = (w - 24.0f * scale - (kCols - 1) * pad) / kCols;
    float boxH = (h - 48.0f * scale - (kRows - 1) * pad) / kRows;

    int triggeredBox = -1;
    for (int r = 0; r < kRows; ++r) {
        for (int c = 0; c < kCols; ++c) {
            int idx = r * kCols + c;
            float bx = startX + c * (boxW + pad);
            float by = startY + r * (boxH + pad);
            ImRect bRect(ImVec2(bx, by), ImVec2(bx + boxW, by + boxH));

            bool hov = bRect.Contains(io.MousePos);
            if (hov) selectedBox_ = idx;
            if (hov && ImGui::IsMouseClicked(0)) triggeredBox = idx;

            bool isSel = (selectedBox_ == idx);
            // miniBoxMenuColors: box-bg grey_22 (#222222), box-sel grey_32 (#323232)
            ImU32 bBg = isSel ? IM_COL32(50, 50, 50, 255) : IM_COL32(34, 34, 34, 255);
            ImU32 bBorder = isSel ? IM_COL32(205, 24, 24, 255) : IM_COL32(65, 65, 65, 255);

            dl->AddRectFilled(bRect.Min, bRect.Max, bBg, 3.0f);
            dl->AddRect(bRect.Min, bRect.Max, bBorder, 3.0f, 0, isSel ? 1.5f : 1.0f);

            ImVec2 txtSz = ImGui::CalcTextSize(kModuleNames[idx]);
            dl->AddText(ImVec2(bx + (boxW - txtSz.x) * 0.5f, by + (boxH - txtSz.y) * 0.5f),
                        isSel ? IM_COL32(255, 255, 255, 255) : IM_COL32(185, 185, 185, 255),
                        kModuleNames[idx]);
        }
    }

    if (activate) triggeredBox = selectedBox_;

    // Handle outside click (only after the opening frame)
    if (openFrameCount_ > 1 && ImGui::IsMouseClicked(0) && !popupRect.Contains(io.MousePos)) {
        showMenuPopup = false;
        openFrameCount_ = 0;
        return true;
    }

    if (triggeredBox >= 0) {
        switch (triggeredBox) {
            case 0: activeScreen = trakr::Screen::Pattern; break;
            case 1: activeScreen = trakr::Screen::Instrument; break;
            case 2:
                activeScreen = trakr::Screen::Sample;
                scrCtx.sampleView.setSubPage(SampleView::SubPage::Playback);
                break;
            case 3: activeScreen = trakr::Screen::Master; break;
            case 4: activeScreen = trakr::Screen::Song; break;
            case 5: activeScreen = trakr::Screen::SampleLoader; break;
            case 6: activeScreen = trakr::Screen::File; break;
            case 7: activeScreen = trakr::Screen::Config; break;
            case 8: activeScreen = trakr::Screen::Perform; break;
            case 9:
                activeScreen = trakr::Screen::Sample;
                scrCtx.sampleView.setSubPage(SampleView::SubPage::Editor);
                break;
            case 10:
                activeScreen = trakr::Screen::Sample;
                scrCtx.sampleView.setSubPage(SampleView::SubPage::Playback);
                scrCtx.recordMode = true;
                break;
            case 11: break; // Close
        }
        showMenuPopup = false;
        openFrameCount_ = 0;
        return true;
    }

    return false;
}

} // namespace trakr::hw

#include "Display.h"
#include <algorithm>
#include <cmath>

namespace trakr::hw {

Display::Display() {
    screenRect_ = ImRect(ImVec2(0, 0), ImVec2(kLogicalWidth, kLogicalHeight));
    headerRect_ = ImRect(ImVec2(0, 0), ImVec2(kLogicalWidth, kHeaderHeight));
    workspaceRect_ = ImRect(ImVec2(0, kHeaderHeight), ImVec2(kLogicalWidth, kLogicalHeight - kScreenButtonsHeight));
    buttonsRect_ = ImRect(ImVec2(0, kLogicalHeight - kScreenButtonsHeight), ImVec2(kLogicalWidth, kLogicalHeight));
    deckRect_ = ImRect(ImVec2(0, kLogicalHeight), ImVec2(kLogicalWidth, kLogicalHeight + kDeckHeight));
}

void Display::computeLayout(const ImVec2& availPos, const ImVec2& availSize, bool showDeck, bool wideMode) {
    showDeck_ = showDeck;
    wideMode_ = wideMode;

    if (availSize.x <= 10.0f || availSize.y <= 10.0f) {
        return;
    }

    if (wideMode_) {
        // Desktop Wide mode: stretch screen width to fill viewport, screen buttons scale horizontally
        scale_ = 1.0f;
        float deckH = showDeck_ ? kDeckHeight : 0.0f;
        float screenH = std::max(kLogicalHeight, availSize.y - deckH);

        screenRect_.Min = availPos;
        screenRect_.Max = ImVec2(availPos.x + availSize.x, availPos.y + screenH);

        headerRect_.Min = screenRect_.Min;
        headerRect_.Max = ImVec2(screenRect_.Max.x, screenRect_.Min.y + kHeaderHeight);

        buttonsRect_.Min = ImVec2(screenRect_.Min.x, screenRect_.Max.y - kScreenButtonsHeight);
        buttonsRect_.Max = screenRect_.Max;

        workspaceRect_.Min = ImVec2(screenRect_.Min.x, headerRect_.Max.y);
        workspaceRect_.Max = ImVec2(screenRect_.Max.x, buttonsRect_.Min.y);

        if (showDeck_) {
            deckRect_.Min = ImVec2(screenRect_.Min.x, screenRect_.Max.y);
            deckRect_.Max = ImVec2(screenRect_.Max.x, availPos.y + availSize.y);
        } else {
            deckRect_ = ImRect(ImVec2(0, 0), ImVec2(0, 0));
        }
    } else {
        // Fixed 800x480 aspect ratio mode: letterboxed and centered
        float logicalTotalH = kLogicalHeight + (showDeck_ ? kDeckHeight : 0.0f);
        float scaleX = availSize.x / kLogicalWidth;
        float scaleY = availSize.y / logicalTotalH;
        scale_ = std::min(scaleX, scaleY);
        if (scale_ < 0.65f) scale_ = 0.65f;

        float screenW = kLogicalWidth * scale_;
        float screenH = kLogicalHeight * scale_;
        float deckH = (showDeck_ ? kDeckHeight : 0.0f) * scale_;
        float totalH = screenH + deckH;

        float originX = availPos.x + std::floor((availSize.x - screenW) * 0.5f);
        float originY = availPos.y + std::floor((availSize.y - totalH) * 0.5f);

        screenRect_.Min = ImVec2(originX, originY);
        screenRect_.Max = ImVec2(originX + screenW, originY + screenH);

        headerRect_.Min = screenRect_.Min;
        headerRect_.Max = ImVec2(screenRect_.Max.x, screenRect_.Min.y + kHeaderHeight * scale_);

        buttonsRect_.Min = ImVec2(screenRect_.Min.x, screenRect_.Max.y - kScreenButtonsHeight * scale_);
        buttonsRect_.Max = screenRect_.Max;

        workspaceRect_.Min = ImVec2(screenRect_.Min.x, headerRect_.Max.y);
        workspaceRect_.Max = ImVec2(screenRect_.Max.x, buttonsRect_.Min.y);

        if (showDeck_) {
            deckRect_.Min = ImVec2(originX, screenRect_.Max.y);
            deckRect_.Max = ImVec2(originX + screenW, originY + totalH);
        } else {
            deckRect_ = ImRect(ImVec2(0, 0), ImVec2(0, 0));
        }
    }
}

ImVec2 Display::canvasToScreen(const ImVec2& pt) const {
    return ImVec2(screenRect_.Min.x + pt.x * scale_, screenRect_.Min.y + pt.y * scale_);
}

ImVec2 Display::screenToCanvas(const ImVec2& pt) const {
    if (scale_ <= 0.001f) return ImVec2(0, 0);
    return ImVec2((pt.x - screenRect_.Min.x) / scale_, (pt.y - screenRect_.Min.y) / scale_);
}

void Display::renderBezel() const {
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    if (!drawList) return;

    // Inner screen background (pure black #000000 matching Tracker screen)
    drawList->AddRectFilled(screenRect_.Min, screenRect_.Max, IM_COL32(0, 0, 0, 255));

    // Screen bezel: subtle 1px border on top/left/right only (flat bottom below screen buttons, no drop shadow)
    drawList->AddLine(screenRect_.Min, ImVec2(screenRect_.Max.x, screenRect_.Min.y), IM_COL32(40, 42, 46, 255), 1.0f);
    drawList->AddLine(screenRect_.Min, ImVec2(screenRect_.Min.x, screenRect_.Max.y), IM_COL32(40, 42, 46, 255), 1.0f);
    drawList->AddLine(ImVec2(screenRect_.Max.x, screenRect_.Min.y), screenRect_.Max, IM_COL32(40, 42, 46, 255), 1.0f);
}

} // namespace trakr::hw


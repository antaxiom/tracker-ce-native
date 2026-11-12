#pragma once

// Polyend Tracker Display Canvas Parity
// Reference: tracker-manual-1.7.0.txt line 700 (1.7 Page Layout)
// Hardware reference: 800x480 physical display, 28px header, 56px screen buttons.

#include "imgui.h"
#include "imgui_internal.h"

namespace trakr::hw {

class Display {
public:
    static constexpr float kLogicalWidth = 800.0f;
    static constexpr float kLogicalHeight = 480.0f;
    static constexpr float kHeaderHeight = 28.0f;
    static constexpr float kScreenButtonsHeight = 55.0f;
    static constexpr float kWorkspaceHeight = 397.0f; // kLogicalHeight - kHeaderHeight - kScreenButtonsHeight
    static constexpr float kDeckHeight = 260.0f; // Height of physical button/pad deck when visible

    Display();

    // Recompute layout bounding boxes based on available host window space
    void computeLayout(const ImVec2& availPos, const ImVec2& availSize, bool showDeck, bool wideMode);

    // Bounding rectangles in screen pixel space
    const ImRect& screenRect() const { return screenRect_; }
    const ImRect& headerRect() const { return headerRect_; }
    const ImRect& workspaceRect() const { return workspaceRect_; }
    const ImRect& buttonsRect() const { return buttonsRect_; }
    const ImRect& deckRect() const { return deckRect_; }

    float scale() const { return scale_; }
    bool isWideMode() const { return wideMode_; }
    bool isDeckVisible() const { return showDeck_; }

    // Coordinate conversions
    ImVec2 canvasToScreen(const ImVec2& pt) const;
    ImVec2 screenToCanvas(const ImVec2& pt) const;

    // Renders letterboxing / pillarboxing outer bezel
    void renderBezel() const;

private:
    ImRect screenRect_{};
    ImRect headerRect_{};
    ImRect workspaceRect_{};
    ImRect buttonsRect_{};
    ImRect deckRect_{};

    float scale_{1.0f};
    bool wideMode_{false};
    bool showDeck_{true};
};

} // namespace trakr::hw


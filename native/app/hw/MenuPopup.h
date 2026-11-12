#pragma once

// Polyend Tracker Menu Popup
// Firmware reference: core/interface/core/interfaceMenuPopup.cpp
// Colors: core/source/Themes.cpp miniBoxMenuColors

#include "ScreenManager.h"
#include "imgui.h"

namespace trakr::hw {

class MenuPopup {
public:
    MenuPopup() = default;

    // Renders the 440x330 4x3 module popup overlay
    // Returns true if an item was selected or popup should close
    bool render(const ImRect& displayRect,
                trakr::Screen& activeScreen,
                bool& showMenuPopup,
                bool& showExportDeck,
                trakr::ScreenContext& scrCtx,
                float scale = 1.0f);

    int selectedIndex() const { return selectedBox_; }
    void setSelectedIndex(int idx) { selectedBox_ = (idx >= 0 && idx < 12) ? idx : 0; }

private:
    int selectedBox_{0};
    int openFrameCount_{0};
};

} // namespace trakr::hw

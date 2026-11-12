#pragma once

// Polyend Tracker Header Bar Parity
// Firmware reference: core/interface/core/titleBar.cpp & songTimer.cpp
// Manual reference: tracker-manual-1.7.0.txt line 700 (1.7 Page Layout)

#include "imgui.h"
#include "imgui_internal.h"
#include <string>

namespace trakr {
    class Song;
    class EngineHost;
    class Theme;
}

namespace trakr::hw {

class HeaderBar {
public:
    HeaderBar();

    // Renders the Tracker header bar within the given screen rect
    void render(const ImRect& rect,
                const char* pageTitle,
                const trakr::Song& song,
                const trakr::EngineHost& engineHost,
                int currentOrderIdx,
                int& selectedInstIdx,
                bool recordMode,
                const trakr::Theme& theme,
                float scale = 1.0f,
                int cursorRow = 0,
                const char* rightLabelOverride = nullptr,
                bool firmwareChrome = false,
                bool showInstrumentRight = true,
                bool showBottomSeparator = true,
                bool showTransport = true);

private:
    char timerBuf_[64];
    char rightBuf_[64];
    char patStepBuf_[32];
};

} // namespace trakr::hw

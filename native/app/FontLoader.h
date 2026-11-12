#pragma once

#include "imgui.h"

namespace trakr {

struct AppFonts {
    ImFont* mono{nullptr};
    ImFont* sans{nullptr};
    ImFont* monoLarge{nullptr};
    ImFont* monoBold{nullptr};
    ImFont* monoLight{nullptr};
};

AppFonts loadFonts(float dpiScale = 1.0f, bool usePolyendFont = false, bool firmwareMetrics = false);

ImFont* getMonoFont();
ImFont* getMonoBoldFont();
ImFont* getMonoLightFont();

} // namespace trakr


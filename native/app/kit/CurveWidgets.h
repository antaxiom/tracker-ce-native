#pragma once

#include "imgui.h"

namespace trakr::kit {

// Visual graphical ADSR curve plot
void AdsrPlot(const char* str_id, float attackMs, float decayMs, float sustain, float releaseMs,
              const ImVec2& size = ImVec2(-1, 80));

// Visual graphical SVF Filter frequency response curve plot
void FilterResponsePlot(const char* str_id, int filterType, float cutoffHz, float resonance,
                        const ImVec2& size = ImVec2(-1, 80));

} // namespace trakr::kit


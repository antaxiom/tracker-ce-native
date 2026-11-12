#pragma once

// Polyend Tracker Custom Hardware Widgets Parity
// Firmware reference: core/interface/core/commonControls.h (cBar, cList)
// Supports vertical BarColumn widget, JogValueEdit, and authentic ModalList.

#include "imgui.h"
#include <string>
#include <vector>

namespace trakr::hw {

// Vertical parameter column (Volume, Pan, Cutoff, etc.)
// Displays vertical fill bar, center-zero line for bipolar params,
// and green (navigate) / red (edit) selection frame.
bool BarColumn(const char* label,
               float* value,
               float minVal,
               float maxVal,
               const char* format,
               const ImVec2& size,
               bool selected,
               bool recordMode,
               bool isBipolar = false);

// Jog dial value editor (drag up/down or mouse wheel with fine resolution on Shift)
bool JogValueEdit(const char* label,
                  int* value,
                  int minVal,
                  int maxVal,
                  bool shiftHeld,
                  const ImVec2& size = ImVec2(50, 22));

// Modal popup selection list
int ModalList(const char* title,
              const std::vector<std::string>& items,
              int selectedIndex,
              const ImVec2& pos,
              const ImVec2& size);

} // namespace trakr::hw


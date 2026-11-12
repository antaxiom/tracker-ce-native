#pragma once

#include "imgui.h"
#include <string>
#include <vector>

namespace trakr::kit {

// 1. FlatButton: Crisp flat button with 1px border
bool FlatButton(const char* label, const ImVec2& size = ImVec2(0, 0), bool active = false);

// 2. ToggleButton: Latching toggle button (ideal for Mute / Solo)
bool ToggleButton(const char* label, bool* v, const ImVec4& activeColor = ImVec4(0, 0, 0, 0), const ImVec2& size = ImVec2(0, 0));

// 3. Knob: Compact rotary control with arc indicator
bool Knob(const char* label, float* v, float v_min, float v_max, const char* format = "%.2f", float radius = 16.0f);

// 4. MiniSlider: Horizontal or vertical bar with embedded value label
bool MiniSlider(const char* label, float* v, float v_min, float v_max, const ImVec2& size = ImVec2(90, 18), const char* format = "%.2f");
bool MiniSliderV(const char* label, float* v, float v_min, float v_max, const ImVec2& size = ImVec2(18, 90));

// 5. ValueBox: Click-drag or double-click to type numeric cell (supports decimal and hex)
bool ValueBox(const char* label, int* v, int v_min, int v_max, bool hex = false, const ImVec2& size = ImVec2(50, 18));

// 6. Meter: Audio peak meter with decay line and clip LED
void Meter(float peak, float decayPeak = -1.0f, const ImVec2& size = ImVec2(8, 80), bool vertical = true);

// 7. PanelHeader: Sleek panel banner
void PanelHeader(const char* title, const char* rightLabel = nullptr);

// 8. Splitter: Draggable layout separator
bool Splitter(bool vertical, float thickness, float* size1, float* size2, float min1, float min2);

// 9. ListBox: Fast virtualized list
bool ListBox(const char* label, int* current_item, const std::vector<std::string>& items, int visible_items = 6, float width = -1.0f);

// 10. WaveformView: Audio waveform with start/loop markers
void WaveformView(const char* str_id, const float* samples, size_t count,
                  int* startMarker = nullptr, int* loopStartMarker = nullptr,
                  int* loopEndMarker = nullptr, int* endMarker = nullptr,
                  const ImVec2& size = ImVec2(-1, 90),
                  const std::vector<int>* slices = nullptr,
                  double playheadPos = -1.0);

// 11. Tooltip: Simple hover tooltip
void Tooltip(const char* text);

} // namespace trakr::kit


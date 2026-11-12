#pragma once

// Trakr semantic UI design system.
//
// Values mirror the firmware themes and control metrics so the Classic layout
// profile reproduces the original look. They are tokens, not raster output:
// controls draw through Canvas, which snaps to device pixels at any scale.
//
// Sources:
//   core/source/Themes.cpp                  (themeDefault palette)
//   core/interface/instrumentEditor/gInstrEdit.cpp (geometry)
//   core/libs/NewDisplay/{bar,list,label,frame,bgLabel}.cpp (control metrics)

#include "imgui.h"
#include <cstdint>

namespace trakr::ui {

// Device-space surface (firmware 800x480 LCD).
constexpr int kDeviceWidth = 800;
constexpr int kDeviceHeight = 480;
constexpr int kHeaderHeight = 28;
constexpr int kButtonsTop = 424;
constexpr int kButtonsHeight = 56;
constexpr int kWorkspaceTop = kHeaderHeight;
constexpr int kWorkspaceHeight = kButtonsTop - kWorkspaceTop; // 396
constexpr int kWorkspaceSpanY = 397; // legacy view scale, keeps y mapping identical

// Control metrics (firmware device pixels).
constexpr int kBarY = 29;
constexpr int kBarWidth = kDeviceWidth / 8 - 3; // 97
constexpr int kBarHeight = 394;
constexpr int kListRowHeight = 27;
constexpr int kListTopInset = 4;
constexpr int kLineWidth1px = 16;  // EVE LINE_WIDTH units
constexpr int kFrameLineWidth = 24; // cFrame; 1.5 device px -> 2px raster

// themeDefault, mirrored verbatim from core/source/Themes.cpp.
constexpr ImU32 kBlack = IM_COL32(0x00, 0x00, 0x00, 255);
constexpr ImU32 kWhite = IM_COL32(0xFF, 0xFF, 0xFF, 255);
constexpr ImU32 kRed = IM_COL32(0xFF, 0x00, 0x00, 255);
constexpr ImU32 kGreen = IM_COL32(0x00, 0xFF, 0x00, 255);
constexpr ImU32 kOneTrueRed = IM_COL32(0xCD, 0x18, 0x18, 255);
constexpr ImU32 kOneTrueGreen = IM_COL32(0x30, 0xCB, 0x3F, 255);
constexpr ImU32 kGrey08 = IM_COL32(0x08, 0x08, 0x08, 255);
constexpr ImU32 kGrey0A = IM_COL32(0x0A, 0x0A, 0x0A, 255);
constexpr ImU32 kGrey12 = IM_COL32(0x12, 0x12, 0x12, 255);
constexpr ImU32 kGrey22 = IM_COL32(0x22, 0x22, 0x22, 255);
constexpr ImU32 kGrey32 = IM_COL32(0x32, 0x31, 0x32, 255);
constexpr ImU32 kGrey33 = IM_COL32(0x33, 0x33, 0x33, 255);
constexpr ImU32 kGrey3F = IM_COL32(0x3F, 0x3F, 0x3F, 255);
constexpr ImU32 kGrey44 = IM_COL32(0x44, 0x43, 0x44, 255);
constexpr ImU32 kGrey57 = IM_COL32(0x57, 0x57, 0x57, 255);
constexpr ImU32 kGrey77 = IM_COL32(0x77, 0x77, 0x77, 255);
constexpr ImU32 kOrange = IM_COL32(0xD6, 0x70, 0x22, 255);

// themeDefault.activeBarColors / inactiveBarColors
// 0 fill, 1 compare-up, 2 compare-down, 3 container, 4 background
constexpr ImU32 kActiveBarColors[5] = {kWhite, kRed, kGreen, kGrey08, kGrey0A};
constexpr ImU32 kInactiveBarColors[5] = {kGrey22, kRed, kGreen, kGrey08, kGrey0A};

// themeDefault.activeListColors
// frameNormal, frameSelect, scrollBar, listBg, fontNormal, fontSelection, scrollTrack
constexpr ImU32 kActiveListColors[7] = {kWhite, kWhite, kGrey57, kGrey0A,
                                        kWhite, kWhite, kBlack};

// themeDefault.frameColors
constexpr ImU32 kFrameColors[2] = {kOneTrueRed, kGrey3F};

// themeDefault.blankSpaceCoverLabelColors
constexpr ImU32 kBlankSpaceCoverColors[4] = {kWhite, kGrey0A, kRed, kBlack};

// themeDefault.activeLabelsColors / inactiveLabelsColors
constexpr ImU32 kActiveLabelsColors[4] = {kGrey77, kGrey32, kOneTrueRed, kWhite};
constexpr ImU32 kInactiveLabelsColors[4] = {kGrey33, kGrey32, kOneTrueRed, kGrey57};

// themeDefault.bottomButtonsBgColors: text, background, frame
constexpr ImU32 kBottomButtonsBg[3] = {kGrey77, kGrey0A, kOneTrueRed};

} // namespace trakr::ui

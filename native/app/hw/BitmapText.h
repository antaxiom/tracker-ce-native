#pragma once

// Firmware L4 bitmap-font text rendering.
// The Polyend firmware draws text from embedded 4-bit-per-pixel Roboto Mono
// bitmaps (core/libs/fonts/*_L4.cpp), decoded here and blitted into an
// ImDrawList. This reproduces the exact glyph pixels the fwref reference uses,
// which a TTF rasterizer cannot match at the sub-pixel level.

#include "imgui.h"

namespace trakr::hw {

enum class FwFont {
    Light14,    // RobotoMono-Light_14_L4  (cList default / Font1)
    Light17,    // RobotoMono-Light_17_L4
    Regular12,  // RobotoMono-Regular_12_L4
    Regular14,  // RobotoMono-Regular_14_L4
    Bold17      // RobotoMono-Bold_17_L4   (controlStyleFont4)
};

// Width in firmware device pixels (sum of per-char advances).
int fwTextWidth(FwFont font, const char* text);
// Cell height in firmware device pixels.
int fwTextHeight(FwFont font);

// Draw `text` with its top-left at screen-space (screenX, screenYTop), scaling
// each device pixel to (sx, sy). `color` carries the glyph colour; its alpha
// scales the per-pixel 4-bit coverage.
void fwDrawText(ImDrawList* dl, FwFont font, float screenX, float screenYTop,
                const char* text, ImU32 color, float sx, float sy);

// Same, but pre-blends every glyph pixel against an opaque background with the
// emulator's integer blend ((src*a + dst*(255-a)) / 255) and draws the result
// opaque. GPU alpha blending rounds differently, which shows up as a 1-value
// fringe on every antialiased glyph edge in the visual-regression diff. Use
// only where the background is known to be flat `bg`.
void fwDrawTextOn(ImDrawList* dl, FwFont font, float screenX, float screenYTop,
                  const char* text, ImU32 color, ImU32 bg, float sx, float sy);

} // namespace trakr::hw

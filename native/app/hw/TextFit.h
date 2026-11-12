#pragma once

// Shared text fitting for the 800x480 firmware-parity UI.
//
// Firmware labels are drawn into fixed cells (100px screen buttons, 100px
// instrument columns, browser rows, ...). Long option names must never bleed
// into the neighbouring cell, so every fixed-cell label goes through this
// helper: it measures the string at the firmware advance, shrinks the glyphs
// proportionally when the text would overflow, clips as a last resort, and
// can report offenders when TRAKR_LAYOUT_AUDIT=1.

#include "imgui.h"

namespace trakr::ui {

struct FittedText {
    int chars = 0;
    float advance = 0.0f;
    float width = 0.0f;
    float fontSize = 0.0f;
    bool scaled = false;
    bool clipped = false;
};

// Measure how text fits in maxWidth using a firmware advance (10px bold,
// 8px regular at 1:1). Returns the advance and font size to use.
FittedText fitText(const char* text, float maxWidth, float preferredAdvance,
                   float preferredFontSize, float minAdvance = 7.0f);

// Draw text centred vertically at centerY and horizontally either centred or
// left-aligned inside [x0, x1]. The glyph size follows the fitted advance so
// text never overfills its cell.
void drawTextFit(ImDrawList* dl, ImFont* font, float x0, float x1, float centerY,
                 ImU32 col, const char* text, bool centerX,
                 float preferredAdvance, float minAdvance = 7.0f,
                 const char* auditTag = nullptr);

// Audit helpers (no-ops unless TRAKR_LAYOUT_AUDIT is set).
bool layoutAuditEnabled();
void resetLayoutAudit();
int layoutAuditCount();
void dumpLayoutAudit();

} // namespace trakr::ui

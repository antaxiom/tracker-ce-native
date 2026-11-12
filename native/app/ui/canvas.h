#pragma once

// Host-agnostic drawing surface for semantic UI controls.
//
// Controls draw in firmware device pixels through Canvas. Canvas maps to the
// host draw list with device-pixel snapped edges and L4 bitmap text, so the
// same control code renders crisply at any integer scale.

#include "layout.h"
#include "hw/BitmapText.h"
#include "imgui.h"
#include <string_view>

namespace trakr::ui {

class Canvas {
public:
    Canvas(ImDrawList* drawList, const LayoutContext& layout)
        : dl_(drawList), lc_(layout) {}

    ImDrawList* drawList() const { return dl_; }
    const LayoutContext& layout() const { return lc_; }

    void fillRect(const RectI& r, ImU32 color) const;
    // Inset band stroke (kept for panels that draw inside their rect).
    void strokeRect(const RectI& r, ImU32 color, float deviceThickness = 2.0f) const;
    // EVE-faithful centred stroke: half the width lies outside the rect edge,
    // matching API_BEGIN(LINES) with square caps (cFrame, cList selection).
    void strokeRectCentered(const RectI& r, ImU32 color, float deviceThickness = 1.5f) const;
    void vGradient(const RectI& r, ImU32 top, ImU32 bottom) const;
    void hGradient(const RectI& r, ImU32 left, ImU32 right) const;

    void text(trakr::hw::FwFont font, int x, int yTop, ImU32 color, std::string_view s) const;
    void textCentered(trakr::hw::FwFont font, const RectI& r, ImU32 color,
                      std::string_view s) const;
    int textWidth(trakr::hw::FwFont font, std::string_view s) const;
    int textHeight(trakr::hw::FwFont font) const;

    void pushClip(const RectI& r);
    void popClip();
    int clipDepth() const { return clipDepth_; }

private:
    ImDrawList* dl_{};
    LayoutContext lc_{};
    int clipDepth_{0};
};

} // namespace trakr::ui

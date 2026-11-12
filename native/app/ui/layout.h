#pragma once

// Device-space layout for the semantic UI.
//
// Controls always think in firmware device pixels (800x480). LayoutContext maps
// that space into a host rectangle. Classic uses the exact firmware span; Studio
// resolves the same controls into a different host rectangle.

#include "design.h"
#include "imgui.h"
#include "imgui_internal.h"
#include <algorithm>
#include <cmath>

namespace trakr::ui {

enum class LayoutProfile {
    Classic, // 800x480 firmware geometry
    Studio,  // flexible, same controls, host-sized
};

struct RectI {
    int x{0}, y{0}, w{0}, h{0};

    int right() const { return x + w; }
    int bottom() const { return y + h; }
    bool empty() const { return w <= 0 || h <= 0; }
    bool contains(int px, int py) const {
        return px >= x && px < right() && py >= y && py < bottom();
    }
    RectI inset(int d) const { return RectI{x + d, y + d, w - 2 * d, h - 2 * d}; }
};

struct LayoutContext {
    ImVec2 origin{0.0f, 0.0f};
    float scaleX{1.0f};
    float scaleY{1.0f};
    int deviceTop{kWorkspaceTop};
    LayoutProfile profile{LayoutProfile::Classic};

    float screenX(float x) const { return origin.x + x * scaleX; }
    float screenY(float y) const { return origin.y + (y - deviceTop) * scaleY; }

    // Snapped host edge for filled geometry: avoids seams between adjacent
    // device-pixel bands when the host scale is fractional.
    float edgeX(float x) const { return std::round(screenX(x)); }
    float edgeY(float y) const { return std::round(screenY(y)); }

    ImVec2 toScreen(int x, int y) const { return ImVec2(edgeX(static_cast<float>(x)),
                                                         edgeY(static_cast<float>(y))); }
    ImVec2 toScreenF(float x, float y) const { return ImVec2(screenX(x), screenY(y)); }

    ImVec2 screenToDevice(const ImVec2& p) const {
        const float sx = (scaleX > 0.0f) ? scaleX : 1.0f;
        const float sy = (scaleY > 0.0f) ? scaleY : 1.0f;
        return ImVec2((p.x - origin.x) / sx, (p.y - origin.y) / sy + deviceTop);
    }

    // Classic host rect: workspace occupies device y = 28..425. Classic uses an
    // integer scale (crisp device pixels) centered in the host rect; Studio may
    // use a fit scale until it grows a reflow layout.
    static LayoutContext fromWorkspace(const ImRect& rect, LayoutProfile profile) {
        LayoutContext lc;
        float scale = 0.0f;
        if (profile == LayoutProfile::Classic) {
            scale = std::floor(rect.GetHeight() / static_cast<float>(kWorkspaceSpanY));
            if (scale < 1.0f)
                scale = rect.GetHeight() / static_cast<float>(kWorkspaceSpanY);
        } else {
            scale = std::min(rect.GetWidth() / static_cast<float>(kDeviceWidth),
                             rect.GetHeight() / static_cast<float>(kWorkspaceSpanY));
        }
        if (scale <= 0.0f) scale = 1.0f;

        lc.scaleX = scale;
        lc.scaleY = scale;
        lc.origin.y = rect.Min.y;
        lc.origin.x = rect.Min.x +
                      std::floor((rect.GetWidth() - kDeviceWidth * scale) * 0.5f);
        lc.deviceTop = kWorkspaceTop;
        lc.profile = profile;
        return lc;
    }
};

} // namespace trakr::ui

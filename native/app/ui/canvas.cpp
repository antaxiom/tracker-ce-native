#include "canvas.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace trakr::ui {
namespace {

ImVec2 minScreen(const LayoutContext& lc, const RectI& r) {
    return ImVec2(lc.edgeX(static_cast<float>(r.x)),
                  lc.edgeY(static_cast<float>(r.y)));
}

ImVec2 maxScreen(const LayoutContext& lc, const RectI& r) {
    return ImVec2(lc.edgeX(static_cast<float>(r.right())),
                  lc.edgeY(static_cast<float>(r.bottom())));
}

} // namespace

void Canvas::fillRect(const RectI& r, ImU32 color) const {
    if (!dl_ || r.empty()) return;
    const ImVec2 a = minScreen(lc_, r);
    const ImVec2 b = maxScreen(lc_, r);
    if (b.x <= a.x || b.y <= a.y) return;
    dl_->AddRectFilled(a, b, color);
}

void Canvas::strokeRect(const RectI& r, ImU32 color, float deviceThickness) const {
    if (!dl_ || r.empty()) return;
    const float x0 = lc_.edgeX(static_cast<float>(r.x));
    const float y0 = lc_.edgeY(static_cast<float>(r.y));
    const float x1 = lc_.edgeX(static_cast<float>(r.right()));
    const float y1 = lc_.edgeY(static_cast<float>(r.bottom()));
    if (x1 <= x0 || y1 <= y0) return;

    const int t = std::max(1, static_cast<int>(std::lround(deviceThickness * lc_.scaleY)));
    const float ft = static_cast<float>(t);

    dl_->AddRectFilled(ImVec2(x0, y0), ImVec2(x1, std::min(y1, y0 + ft)), color);
    dl_->AddRectFilled(ImVec2(x0, std::max(y0, y1 - ft)), ImVec2(x1, y1), color);
    dl_->AddRectFilled(ImVec2(x0, std::min(y1, y0 + ft)), ImVec2(std::min(x1, x0 + ft), std::max(y0, y1 - ft)), color);
    dl_->AddRectFilled(ImVec2(std::max(x0, x1 - ft), std::min(y1, y0 + ft)), ImVec2(x1, std::max(y0, y1 - ft)), color);
}

void Canvas::strokeRectCentered(const RectI& r, ImU32 color, float deviceThickness) const {
    if (!dl_ || r.empty()) return;
    const float x0 = lc_.edgeX(static_cast<float>(r.x));
    const float y0 = lc_.edgeY(static_cast<float>(r.y));
    const float x1 = lc_.edgeX(static_cast<float>(r.right()));
    const float y1 = lc_.edgeY(static_cast<float>(r.bottom()));
    if (x1 <= x0 || y1 <= y0) return;

    const int t = std::max(1, static_cast<int>(std::lround(deviceThickness * lc_.scaleY)));
    const float lo = static_cast<float>(t / 2);
    const float hi = static_cast<float>(t - t / 2);

    const float left = x0 - lo;
    const float right = x1 + hi;
    const float top = y0 - lo;
    const float bottom = y1 + hi;

    dl_->AddRectFilled(ImVec2(left, top), ImVec2(right, top + static_cast<float>(t)), color);
    dl_->AddRectFilled(ImVec2(left, bottom - static_cast<float>(t)), ImVec2(right, bottom), color);
    dl_->AddRectFilled(ImVec2(left, top + static_cast<float>(t)),
                       ImVec2(left + static_cast<float>(t), bottom - static_cast<float>(t)), color);
    dl_->AddRectFilled(ImVec2(right - static_cast<float>(t), top + static_cast<float>(t)),
                       ImVec2(right, bottom - static_cast<float>(t)), color);
}

void Canvas::vGradient(const RectI& r, ImU32 top, ImU32 bottom) const {
    if (!dl_ || r.empty()) return;
    const ImVec2 a = minScreen(lc_, r);
    const ImVec2 b = maxScreen(lc_, r);
    if (b.x <= a.x || b.y <= a.y) return;
    dl_->AddRectFilledMultiColor(a, b, top, top, bottom, bottom);
}

void Canvas::hGradient(const RectI& r, ImU32 left, ImU32 right) const {
    if (!dl_ || r.empty()) return;
    const ImVec2 a = minScreen(lc_, r);
    const ImVec2 b = maxScreen(lc_, r);
    if (b.x <= a.x || b.y <= a.y) return;
    dl_->AddRectFilledMultiColor(a, b, left, right, right, left);
}

void Canvas::line(int x1, int y1, int x2, int y2, ImU32 color, float deviceThickness) const {
    if (!dl_) return;
    // EVE LINE_WIDTH rasterises hard-edged segments (the emulator fills
    // [min(x), y - t/2, |dx| + 1, t] for orthogonal lines), while ImGui's
    // anti-aliased stroke path adds a texture fringe that spills up to a pixel
    // outside the segment and past the edges of the control it belongs to.
    // Draw orthogonal segments as exact device-pixel rects and disable the AA
    // fringe for diagonal ones so lines never overdraw their boxes.
    const int t = std::max(1, static_cast<int>(std::lround(deviceThickness * lc_.scaleY)));
    const ImVec2 a(lc_.screenX(static_cast<float>(x1)), lc_.screenY(static_cast<float>(y1)));
    const ImVec2 b(lc_.screenX(static_cast<float>(x2)), lc_.screenY(static_cast<float>(y2)));
    const int minX = static_cast<int>(std::lround(std::min(a.x, b.x)));
    const int maxX = static_cast<int>(std::lround(std::max(a.x, b.x)));
    const int minY = static_cast<int>(std::lround(std::min(a.y, b.y)));
    const int maxY = static_cast<int>(std::lround(std::max(a.y, b.y)));

    if (y1 == y2 && minY == maxY) {
        // Endpoints are inclusive along the segment, like the emulator.
        dl_->AddRectFilled(ImVec2(static_cast<float>(minX), static_cast<float>(minY - t / 2)),
                           ImVec2(static_cast<float>(maxX + 1),
                                  static_cast<float>(minY - t / 2 + t)),
                           color);
        return;
    }
    if (x1 == x2 && minX == maxX) {
        dl_->AddRectFilled(ImVec2(static_cast<float>(minX - t / 2), static_cast<float>(minY)),
                           ImVec2(static_cast<float>(minX - t / 2 + t),
                                  static_cast<float>(maxY + 1)),
                           color);
        return;
    }

    dl_->PushDrawFlag(ImDrawFlags_AALines, false);
    dl_->AddLine(a, b, color, static_cast<float>(t));
    dl_->PopDrawFlag();
}

void Canvas::text(trakr::hw::FwFont font, int x, int yTop, ImU32 color,
                  std::string_view s) const {
    if (!dl_ || s.empty() || s.front() == '\0') return;
    const std::string tmp(s);
    trakr::hw::fwDrawText(dl_, font, lc_.screenX(static_cast<float>(x)),
                          lc_.screenY(static_cast<float>(yTop)), tmp.c_str(), color,
                          lc_.scaleX, lc_.scaleY);
}

void Canvas::textCentered(trakr::hw::FwFont font, const RectI& r, ImU32 color,
                          std::string_view s) const {
    const int w = textWidth(font, s);
    const int h = textHeight(font);
    const int x = r.x + (r.w - w) / 2;
    const int y = r.y + (r.h - h) / 2;
    text(font, x, y, color, s);
}

void Canvas::textVCentered(trakr::hw::FwFont font, int x, int centerY, ImU32 color,
                           std::string_view s) const {
    const int h = textHeight(font);
    text(font, x, centerY - h / 2, color, s);
}

void Canvas::textVertical270(trakr::hw::FwFont font, int centerX, int bottomY, ImU32 color,
                             std::string_view s) const {
    if (!dl_ || s.empty()) return;
    // Glyphs are rasterised at the device origin and then rotated into place,
    // matching cLabel's LOADIDENTITY/ROTATE(270) matrix. CMD_NUMBER/TEXT advance
    // by the per-character width reported by the bitmap font.
    const float centerScreenX = lc_.screenX(static_cast<float>(centerX));
    const float heightPx = static_cast<float>(trakr::hw::fwTextHeight(font));
    const float xOffset = heightPx * 0.5f * lc_.scaleX + 0.5f;
    float curY = lc_.screenY(static_cast<float>(bottomY));
    for (char ch : s) {
        if (ch != ' ') {
            const char glyph[2] = {ch, '\0'};
            const int v0 = dl_->VtxBuffer.Size;
            trakr::hw::fwDrawText(dl_, font, lc_.screenX(0.0f), lc_.screenY(0.0f), glyph, color,
                                  lc_.scaleX, lc_.scaleY);
            for (int i = v0; i < dl_->VtxBuffer.Size; ++i) {
                const float rx = dl_->VtxBuffer[i].pos.x - lc_.origin.x;
                const float ry = dl_->VtxBuffer[i].pos.y - lc_.origin.y;
                dl_->VtxBuffer[i].pos.x = (centerScreenX - xOffset) + ry;
                dl_->VtxBuffer[i].pos.y = curY - rx;
            }
        }
        const char chStr[2] = {ch, '\0'};
        curY -= (static_cast<float>(trakr::hw::fwTextWidth(font, chStr)) + 1.0f) * lc_.scaleY;
    }
}

int Canvas::textWidth(trakr::hw::FwFont font, std::string_view s) const {
    const std::string tmp(s);
    return trakr::hw::fwTextWidth(font, tmp.c_str());
}

int Canvas::textHeight(trakr::hw::FwFont font) const {
    return trakr::hw::fwTextHeight(font);
}

void Canvas::pushClip(const RectI& r) {
    if (!dl_) return;
    RectI c = r;
    c.x = std::max(c.x, 0);
    c.y = std::max(c.y, lc_.deviceTop);
    c.w = std::min(c.right(), kDeviceWidth) - c.x;
    c.h = std::min(c.bottom(), kDeviceHeight) - c.y;
    if (c.w < 0) c.w = 0;
    if (c.h < 0) c.h = 0;
    const ImVec2 a = minScreen(lc_, c);
    const ImVec2 b = maxScreen(lc_, c);
    dl_->PushClipRect(a, ImVec2(std::max(a.x, b.x), std::max(a.y, b.y)), true);
    ++clipDepth_;
}

void Canvas::popClip() {
    if (!dl_ || clipDepth_ <= 0) return;
    dl_->PopClipRect();
    --clipDepth_;
}

} // namespace trakr::ui

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

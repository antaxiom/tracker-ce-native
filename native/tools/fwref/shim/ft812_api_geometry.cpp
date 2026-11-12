// Geometry helpers shared by the FT812 immediate-mode shim. Kept in their own
// translation unit so ft812_api.cpp stays focused on API state and commands.

#include "FT812.h"
#include "fwref_internal.h"
#include "audit.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace
{

using trakr::emulator::ColorRGB;
using trakr::emulator::FT812DisplayEmulator;
using trakr::emulator::FirmwareFont;

} // namespace

namespace fwref
{

void drawMatrixGlyph(FT812DisplayEmulator* target,
                     const FwrefMatrix& mtx,
                     int16_t x,
                     int16_t y,
                     uint8_t ch,
                     ColorRGB col,
                     FirmwareFont font)
{
    if (target == nullptr)
        return;
    const int tx = mtx.active ? (mtx.tx >> 16) : 0;
    const int ty = mtx.active ? (mtx.ty >> 16) : 0;
    const int q = mtx.active ? mtx.quarterTurns() : 0;
    if (mtx.active && q != 0 && q != 3)
        fwref::auditCall("API_MATRIX_ROTATION", fwref::AuditClass::Approximation,
                         "only 0/270 degree rotations implemented");
    if (q == 3)
    {
        const int h = target->fontHeight(font);
        const char text[2] = { static_cast<char>(ch), 0 };
        target->drawTextVertical270(static_cast<int16_t>(x + tx + h / 2),
                                    static_cast<int16_t>(y + ty), text, col, font);
        return;
    }
    target->drawGlyph(static_cast<int16_t>(x + tx), static_cast<int16_t>(y + ty), ch, col, font);
}

void drawThickSegment(FT812DisplayEmulator* target,
                      int x0,
                      int y0,
                      int x1,
                      int y1,
                      int thickness,
                      ColorRGB col,
                      bool extendCaps)
{
    if (target == nullptr)
        return;
    int half = thickness / 2;
    if (y0 == y1)
    {
        int a = std::min(x0, x1);
        int b = std::max(x0, x1);
        if (extendCaps)
        {
            a -= half;
            b += half;
        }
        target->drawRectFilled(a, y0 - half, b - a + 1, thickness, col);
        return;
    }
    if (x0 == x1)
    {
        int a = std::min(y0, y1);
        int b = std::max(y0, y1);
        if (extendCaps)
        {
            a -= half;
            b += half;
        }
        target->drawRectFilled(x0 - half, a, thickness, b - a + 1, col);
        return;
    }

    // Generic Bresenham-ish thick line (not used by these screens).
    int dx = std::abs(x1 - x0);
    int dy = std::abs(y1 - y0);
    int steps = std::max(dx, dy);
    if (steps == 0)
        return;
    for (int i = 0; i <= steps; ++i)
    {
        int x = x0 + (x1 - x0) * i / steps;
        int y = y0 + (y1 - y0) * i / steps;
        target->drawRectFilled(x - half, y - half, thickness, thickness, col);
    }
}

} // namespace fwref

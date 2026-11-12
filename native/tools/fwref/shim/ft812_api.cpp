// Immediate-mode FT812 API shim for the unmodified NewDisplay controls. EVE
// draws a vertex pair when its second vertex arrives, using the colour in
// effect at that vertex.

#include "FT812.h"
#include "fwref_internal.h"
#include "audit.h"
#include "displayStructs.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include "matrix.h"

namespace
{

struct GState
{
    uint32_t color = 0xFFFFFF;
    uint8_t alpha = 255;
    uint16_t lineWidth = 16;
    uint16_t pointSize = 16;
    uint16_t scissorX = 0;
    uint16_t scissorY = 0;
    uint16_t scissorW = 800;
    uint16_t scissorH = 480;
    uint8_t srcBlend = SRC_ALPHA;
    uint8_t dstBlend = ONE_MINUS_SRC_ALPHA;
    uint8_t bitmapHandle = 0;
    uint8_t cell = 0;
    uint32_t numberBase = 10; // API_CMD_SETBASE (cSongPlayer list numbers)

    FwrefMatrix mtx;
};
using fwref::Vertex;

GState g;
std::vector<GState> gStack;
uint8_t gPrim = 0;
std::vector<Vertex> gVerts;
using trakr::emulator::BlendMode;
using trakr::emulator::ColorRGB;
using trakr::emulator::FT812DisplayEmulator;
using trakr::emulator::FirmwareFont;

trakr::emulator::FT812DisplayEmulator* gTarget = nullptr;
void applyClipAndBlend()
{
    if (gTarget == nullptr)
        return;
    gTarget->setClipRect(g.scissorX, g.scissorY, g.scissorX + g.scissorW, g.scissorY + g.scissorH);
    BlendMode mode = BlendMode::SrcAlphaOneMinusSrcAlpha;
    if (g.srcBlend == SRC_ALPHA && g.dstBlend == ZERO)
        mode = BlendMode::SrcAlphaZero;
    else if (g.srcBlend == SRC_ALPHA && g.dstBlend == ONE)
        mode = BlendMode::SrcAlphaOne;
    gTarget->setBlendMode(mode);
}

ColorRGB toColor(uint32_t rgb, uint8_t alpha)
{
    return ColorRGB::fromHex(rgb & 0xFFFFFF, alpha);
}
FirmwareFont fontFromHandle(int handle)
{
    int idx = handle - 1;
    if (idx < 0 || idx > 4)
        idx = 0;
    return static_cast<FirmwareFont>(idx);
}
} // namespace

namespace fwref
{

void setTarget(trakr::emulator::FT812DisplayEmulator* emu)
{
    gTarget = emu;
    g = GState{};
    gStack.clear();
    gVerts.clear();
    gPrim = 0;
}

trakr::emulator::FT812DisplayEmulator* target()
{
    return gTarget;
}

} // namespace fwref

// ---------------------------------------------------------------------------
// Graphics registers / utils
// ---------------------------------------------------------------------------

void API_COLOR(uint32_t RGB)
{
    g.color = RGB & 0x00FFFFFF;
}

void API_COLOR_RGB(uint8_t R, uint8_t G, uint8_t B)
{
    g.color = (static_cast<uint32_t>(R) << 16) | (static_cast<uint32_t>(G) << 8) | B;
}

void API_COLOR_A(uint8_t alpha)
{
    g.alpha = alpha;
}

void API_LINE_WIDTH(uint16_t width)
{
    g.lineWidth = width;
}

void API_POINT_SIZE(uint16_t size)
{
    g.pointSize = size;
}

void API_BLEND_FUNC(uint8_t src, uint8_t dst)
{
    g.srcBlend = src;
    g.dstBlend = dst;
}

void API_SCISSOR_XY(uint16_t x, uint16_t y)
{
    g.scissorX = x;
    g.scissorY = y;
}

void API_SCISSOR_SIZE(uint16_t width, uint16_t height)
{
    g.scissorW = width;
    g.scissorH = height;
}

void API_SAVE_CONTEXT(void)
{
    gStack.push_back(g);
}

void API_RESTORE_CONTEXT(void)
{
    if (!gStack.empty())
    {
        g = gStack.back();
        gStack.pop_back();
    }
}

void API_BITMAP_HANDLE(uint8_t handle)
{
    g.bitmapHandle = handle;
}

void API_CELL(uint8_t cell)
{
    g.cell = cell;
}

void API_VERTEX2F(int16_t x, int16_t y)
{
    // EVE draws a CELL selected by API_CELL at a VERTEX2F when the bitmap x
    // coordinate does not fit VERTEX2II's uint8 range (firmware controls use
    // this for x > 511, see displayControls.cpp:68-74 and tracker.cpp:1259-1267).
    if (gTarget != nullptr && gPrim == BITMAPS && g.cell != 0 && g.bitmapHandle != 0)
    {
        applyClipAndBlend();
        fwref::drawMatrixGlyph(gTarget, g.mtx, x, y, g.cell, toColor(g.color, g.alpha),
                               fontFromHandle(g.bitmapHandle));
        g.cell = 0;
        return;
    }
    gVerts.push_back({ x, y, g.color, g.alpha });
}

void API_VERTEX2II(uint16_t x, uint16_t y, uint8_t handle, uint8_t cell)
{
    if (gTarget == nullptr)
        return;
    applyClipAndBlend();
    if (gPrim == BITMAPS || gPrim == 0)
    {
        fwref::drawMatrixGlyph(gTarget, g.mtx, static_cast<int16_t>(x),
                               static_cast<int16_t>(y), cell, toColor(g.color, g.alpha),
                               fontFromHandle(handle));
    }
}

void API_BEGIN(uint8_t prim)
{
    gPrim = prim;
    gVerts.clear();
}

void API_END(void)
{
    if (gTarget == nullptr)
    {
        gVerts.clear();
        gPrim = 0;
        return;
    }

    applyClipAndBlend();

    if (gPrim == RECTS)
    {
        for (size_t i = 0; i + 1 < gVerts.size(); i += 2)
        {
            const Vertex& a = gVerts[i];
            const Vertex& b = gVerts[i + 1];
            int x0 = std::min<int>(a.x, b.x);
            int y0 = std::min<int>(a.y, b.y);
            int x1 = std::max<int>(a.x, b.x);
            int y1 = std::max<int>(a.y, b.y);
            gTarget->drawRectFilled(x0, y0, x1 - x0, y1 - y0, toColor(b.color, b.alpha));
        }
    }
    else if (gPrim == LINES)
    {
        for (size_t i = 0; i + 1 < gVerts.size(); i += 2)
        {
            const Vertex& a = gVerts[i];
            const Vertex& b = gVerts[i + 1];
            int t = std::max<int>(1, (g.lineWidth + 8) / 16);
            fwref::drawThickSegment(gTarget, a.x, a.y, b.x, b.y, t, toColor(b.color, b.alpha), false);
        }
    }
    else if (gPrim == LINE_STRIP)
    {
        // EVE draws every strip segment as a rectangle centred on the path and
        // extending half the line width past both endpoints (square caps), so
        // joined corners overhang by that half width exactly as the hardware
        // does. Keeping extendCaps=true here is the hardware-faithful choice.
        int t = std::max<int>(1, (g.lineWidth + 8) / 16);
        for (size_t i = 0; i + 1 < gVerts.size(); ++i)
        {
            const Vertex& a = gVerts[i];
            const Vertex& b = gVerts[i + 1];
            fwref::drawThickSegment(gTarget, a.x, a.y, b.x, b.y, t, toColor(b.color, b.alpha), true);
        }
    }
    else if (gPrim == FTPOINTS)
    {
        int d = std::max<int>(1, (g.pointSize + 8) / 16);
        for (const Vertex& v : gVerts)
        {
            int x = v.x - d / 2;
            int y = v.y - d / 2;
            gTarget->drawRectFilled(x, y, d, d, toColor(v.color, v.alpha));
        }
    }
    else if (gPrim == BITMAPS)
    {
        // BITMAPS vertices are only used with API_CELL in these controls. Any
        // non-cell bitmap vertex would be silently dropped, so audit it.
        if (!gVerts.empty())
            fwref::auditCall("API_BITMAPS_VERTEX", fwref::AuditClass::PixelAffecting,
                             "bitmap vertex without CELL");
    }

    gVerts.clear();
    gPrim = 0;
}

// ---------------------------------------------------------------------------
// Co-processor commands
// ---------------------------------------------------------------------------

void API_CMD_GRADIENT(int16_t x0, int16_t y0, uint32_t rgb0, int16_t x1, int16_t y1, uint32_t rgb1)
{
    if (gTarget == nullptr)
        return;
    applyClipAndBlend();
    gTarget->drawLinearGradient(x0, y0, toColor(rgb0, 255), x1, y1, toColor(rgb1, 255));
}

void API_CMD_TEXT(int16_t x, int16_t y, int16_t font, uint16_t options, const char* string)
{
    API_CMD_TEXT(x, y, font, options, string, 0);
}

void API_CMD_TEXT(int16_t x,
                  int16_t y,
                  int16_t font,
                  uint16_t options,
                  const char* string,
                  uint32_t length)
{
    if (gTarget == nullptr || string == nullptr)
        return;
    constexpr uint16_t kSupportedTextOptions = 512u | 1024u | 2048u; // centerX|centerY|rightX
    if ((options & static_cast<uint16_t>(~kSupportedTextOptions)) != 0)
        fwref::auditCall("API_CMD_TEXT_OPTIONS", fwref::AuditClass::PixelAffecting,
                         "unsupported option bits");
    for (const unsigned char* p = reinterpret_cast<const unsigned char*>(string); *p; ++p)
    {
        if (*p >= 128)
        {
            fwref::auditCall("API_CMD_TEXT_NON_ASCII", fwref::AuditClass::PixelAffecting,
                             "non-ASCII glyph");
            break;
        }
    }
    applyClipAndBlend();
    gTarget->drawTextCmd(x, y, std::string(string), toColor(g.color, g.alpha),
                         fontFromHandle(font), options, length);
}

// EVE CMD_SETBASE selects the base used by CMD_NUMBER. cSongPlayer switches
// between decimal and hexadecimal with it around songPlayer.cpp:238-245.
void API_CMD_SETBASE(uint32_t base)
{
    g.numberBase = (base == 16) ? 16 : 10;
}

// EVE CMD_NUMBER formats `n` in the current base and draws it with the same
// text options as CMD_TEXT. cSongPlayer draws one playlist/slot entry per row
// (songPlayer.cpp:304-308 and 477-549).
void API_CMD_NUMBER(int16_t x, int16_t y, int16_t font, uint16_t options, int32_t n)
{
    if (gTarget == nullptr)
        return;
    char buf[32];
    if (g.numberBase == 16)
        std::snprintf(buf, sizeof(buf), "%X", static_cast<uint32_t>(n));
    else if (g.numberBase == 10)
        std::snprintf(buf, sizeof(buf), "%d", n);
    else
    {
        fwref::auditCall("API_CMD_NUMBER_BASE", fwref::AuditClass::Approximation,
                         "only base 10/16 implemented");
        std::snprintf(buf, sizeof(buf), "%d", n);
    }
    applyClipAndBlend();
    gTarget->drawTextCmd(x, y, std::string(buf), toColor(g.color, g.alpha),
                         fontFromHandle(font), options, 0);
}

// Matrix state accessor for the CMD_* wrappers in ft812_api_stubs.cpp.
namespace fwref
{
FwrefMatrix& matrixState() { return g.mtx; }
} // namespace fwref

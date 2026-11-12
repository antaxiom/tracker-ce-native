#ifndef FWREF_INTERNAL_H_
#define FWREF_INTERNAL_H_

// Shared plumbing between the fwref host shims. This is not part of the public
// render API; it only wires the unmodified firmware control code to the
// FT812 rasterizer.

#include "FT812Emulator.h"
#include "shim/matrix.h"

#include <cstdint>
#include <vector>

namespace fwref
{

struct Vertex
{
    int16_t x;
    int16_t y;
    uint32_t color;
    uint8_t alpha;
};

void setTarget(trakr::emulator::FT812DisplayEmulator* emu);
trakr::emulator::FT812DisplayEmulator* target();

// Immediate-mode matrix state owned by ft812_api.cpp; the CMD_* wrappers live
// in ft812_api_stubs.cpp to keep both shim translation units small.
FwrefMatrix& matrixState();

void initFonts();

// Geometry helpers (ft812_api_geometry.cpp).
void drawMatrixGlyph(trakr::emulator::FT812DisplayEmulator* target,
                     const FwrefMatrix& mtx,
                     int16_t x,
                     int16_t y,
                     uint8_t ch,
                     trakr::emulator::ColorRGB col,
                     trakr::emulator::FirmwareFont font);

void drawThickSegment(trakr::emulator::FT812DisplayEmulator* target,
                      int x0,
                      int y0,
                      int x1,
                      int y1,
                      int thickness,
                      trakr::emulator::ColorRGB col,
                      bool extendCaps);

} // namespace fwref

#endif // FWREF_INTERNAL_H_

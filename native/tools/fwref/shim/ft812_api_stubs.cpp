// Host no-ops for FT812 commands that do not affect the reference framebuffer.
// Stateful commands (colour, scissor, blend, geometry, text, number, matrix)
// stay in ft812_api.cpp.

#include "FT812.h"
#include "fwref_internal.h"

// EVE matrix stack commands (cLabel rotate-around-centre idiom).
void API_CMD_LOADIDENTITY(void) { fwref::matrixState().loadIdentity(); }
void API_CMD_TRANSLATE(int32_t tx, int32_t ty) { fwref::matrixState().translate(tx, ty); }
void API_CMD_ROTATE(int32_t a) { fwref::matrixState().rotate(a); }
void API_CMD_SETMATRIX(void) { fwref::matrixState().setMatrix(); }

void API_CLEAR_COLOR_RGB(uint8_t, uint8_t, uint8_t) {}
void API_CLEAR_COLOR(uint32_t) {}
void API_CLEAR(uint8_t, uint8_t, uint8_t) {}
void API_BITMAP_SOURCE(uint32_t) {}
void API_BITMAP_LAYOUT(uint8_t, uint16_t, uint16_t) {}
void API_BITMAP_SIZE(uint8_t, uint8_t, uint8_t, uint16_t, uint16_t) {}
void API_TAG(uint8_t) {}
void API_ALPHA_FUNC(uint8_t, uint8_t) {}
void API_STENCIL_FUNC(uint8_t, uint8_t, uint8_t) {}
void API_STENCIL_OP(uint8_t, uint8_t) {}
void API_CLEAR_COLOR_A(uint8_t) {}
void API_CLEAR_STENCIL(uint8_t) {}
void API_CLEAR_TAG(uint8_t) {}
void API_STENCIL_MASK(uint8_t) {}
void API_TAG_MASK(uint8_t) {}
void API_CALL(uint16_t) {}
void API_JUMP(uint16_t) {}
void API_COLOR_MASK(uint8_t, uint8_t, uint8_t, uint8_t) {}
void API_RETURN(void) {}
void API_MACRO(uint8_t) {}
void API_DISPLAY(void) {}
void API_CMD_SETBITMAP(uint32_t, uint16_t, uint16_t, uint16_t) {}
void API_CMD_DLSTART(void) {}
void API_CMD_SWAP(void) {}
void API_CMD_MEMCPY(uint32_t, uint32_t, uint32_t) {}
void API_CMD_APPEND(uint32_t, uint32_t) {}
void API_VERTEX_FORMAT(uint8_t) {}
void API_VERTEX2II_UNUSED() {}

// Host shims for FT812 commands. Commands that affect the reference framebuffer
// must be implemented; this file contains the ones the current control set does
// not need. Each call is audited so an unsupported path cannot degrade silently
// (FW-1). Ignored-safe commands are recorded but never fail a strict render.

#include "FT812.h"
#include "audit.h"
#include "fwref_internal.h"

using fwref::AuditClass;

// EVE matrix stack commands (cLabel rotate-around-centre idiom).
void API_CMD_LOADIDENTITY(void) { fwref::matrixState().loadIdentity(); }
void API_CMD_TRANSLATE(int32_t tx, int32_t ty) { fwref::matrixState().translate(tx, ty); }
void API_CMD_ROTATE(int32_t a) { fwref::matrixState().rotate(a); }
void API_CMD_SETMATRIX(void) { fwref::matrixState().setMatrix(); }

// Pixel-affecting but unsupported by the current shim.
void API_CLEAR_COLOR_RGB(uint8_t, uint8_t, uint8_t) { fwref::auditCall("API_CLEAR_COLOR_RGB", AuditClass::PixelAffecting); }
void API_CLEAR_COLOR(uint32_t) { fwref::auditCall("API_CLEAR_COLOR", AuditClass::PixelAffecting); }
void API_CLEAR(uint8_t, uint8_t, uint8_t) { fwref::auditCall("API_CLEAR", AuditClass::PixelAffecting); }
void API_BITMAP_SOURCE(uint32_t) { fwref::auditCall("API_BITMAP_SOURCE", AuditClass::IgnoredSafe, "bitmap state is consumed only by audited image draws"); }
void API_BITMAP_LAYOUT(uint8_t, uint16_t, uint16_t) { fwref::auditCall("API_BITMAP_LAYOUT", AuditClass::IgnoredSafe, "bitmap state is consumed only by audited image draws"); }
void API_BITMAP_SIZE(uint8_t, uint8_t, uint8_t, uint16_t, uint16_t) { fwref::auditCall("API_BITMAP_SIZE", AuditClass::IgnoredSafe, "bitmap state is consumed only by audited image draws"); }
void API_ALPHA_FUNC(uint8_t, uint8_t) { fwref::auditCall("API_ALPHA_FUNC", AuditClass::PixelAffecting); }
void API_COLOR_MASK(uint8_t, uint8_t, uint8_t, uint8_t) { fwref::auditCall("API_COLOR_MASK", AuditClass::PixelAffecting); }
void API_CMD_SETBITMAP(uint32_t, uint16_t, uint16_t, uint16_t) { fwref::auditCall("API_CMD_SETBITMAP", AuditClass::IgnoredSafe, "font handles are preloaded; image draws are audited"); }
void API_VERTEX_FORMAT(uint8_t) { fwref::auditCall("API_VERTEX_FORMAT", AuditClass::PixelAffecting); }

// Ignored-safe: no effect on the framebuffer for the current display-list model.
void API_TAG(uint8_t) { fwref::auditCall("API_TAG", AuditClass::IgnoredSafe); }
void API_STENCIL_FUNC(uint8_t, uint8_t, uint8_t) { fwref::auditCall("API_STENCIL_FUNC", AuditClass::IgnoredSafe); }
void API_STENCIL_OP(uint8_t, uint8_t) { fwref::auditCall("API_STENCIL_OP", AuditClass::IgnoredSafe); }
void API_CLEAR_COLOR_A(uint8_t) { fwref::auditCall("API_CLEAR_COLOR_A", AuditClass::IgnoredSafe); }
void API_CLEAR_STENCIL(uint8_t) { fwref::auditCall("API_CLEAR_STENCIL", AuditClass::IgnoredSafe); }
void API_CLEAR_TAG(uint8_t) { fwref::auditCall("API_CLEAR_TAG", AuditClass::IgnoredSafe); }
void API_STENCIL_MASK(uint8_t) { fwref::auditCall("API_STENCIL_MASK", AuditClass::IgnoredSafe); }
void API_TAG_MASK(uint8_t) { fwref::auditCall("API_TAG_MASK", AuditClass::IgnoredSafe); }
void API_CALL(uint16_t) { fwref::auditCall("API_CALL", AuditClass::IgnoredSafe); }
void API_JUMP(uint16_t) { fwref::auditCall("API_JUMP", AuditClass::IgnoredSafe); }
void API_RETURN(void) { fwref::auditCall("API_RETURN", AuditClass::IgnoredSafe); }
void API_MACRO(uint8_t) { fwref::auditCall("API_MACRO", AuditClass::IgnoredSafe); }
void API_DISPLAY(void) { fwref::auditCall("API_DISPLAY", AuditClass::IgnoredSafe); }
void API_CMD_DLSTART(void) { fwref::auditCall("API_CMD_DLSTART", AuditClass::IgnoredSafe); }
void API_CMD_SWAP(void) { fwref::auditCall("API_CMD_SWAP", AuditClass::IgnoredSafe); }
void API_CMD_MEMCPY(uint32_t, uint32_t, uint32_t) { fwref::auditCall("API_CMD_MEMCPY", AuditClass::IgnoredSafe); }
void API_CMD_APPEND(uint32_t, uint32_t) { fwref::auditCall("API_CMD_APPEND", AuditClass::IgnoredSafe); }
void API_VERTEX2II_UNUSED() { fwref::auditCall("API_VERTEX2II_UNUSED", AuditClass::IgnoredSafe); }

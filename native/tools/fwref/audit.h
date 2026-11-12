#pragma once

// fwref conformance auditing (FW-1).
//
// The shim previously degraded silently: unsupported EVE commands were no-ops.
// Audit calls make every such path visible. In strict mode any pixel-affecting
// or approximated call is a failure; ignored-safe commands are recorded but do
// not fail the render.

#include <string>

namespace fwref
{

enum class AuditClass
{
    IgnoredSafe,     // tags, stencil, display-list control: no framebuffer effect
    PixelAffecting,  // unsupported call that can change pixels
    Approximation,   // implemented but not exact (e.g. non-quarter matrix)
};

void setStrict(bool strict);
bool strict();

void auditCall(const char* api, AuditClass cls, const char* detail = nullptr);

void resetAudit();
int auditFailures();
int auditWarnings();
bool auditFailed();
void printAuditReport(const char* context);

} // namespace fwref

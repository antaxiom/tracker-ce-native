#include "audit.h"

#include <cstdio>
#include <set>
#include <string>

namespace fwref
{
namespace
{

bool gStrict = false;
int gFailures = 0;
int gWarnings = 0;
std::set<std::string> gSeen;

const char* className(AuditClass cls)
{
    switch (cls)
    {
        case AuditClass::IgnoredSafe: return "ignored-safe";
        case AuditClass::PixelAffecting: return "UNSUPPORTED";
        case AuditClass::Approximation: return "APPROXIMATE";
    }
    return "unknown";
}

void printOnce(bool first, AuditClass cls, const char* api, const char* detail)
{
    if (!first) return;
    if (detail)
        std::fprintf(stderr, "fwref audit: %s %s (%s)\n", className(cls), api, detail);
    else
        std::fprintf(stderr, "fwref audit: %s %s\n", className(cls), api);
}

} // namespace

void setStrict(bool strict) { gStrict = strict; }
bool strict() { return gStrict; }

void auditCall(const char* api, AuditClass cls, const char* detail)
{
    const std::string key = std::string(api) + "|" + (detail ? detail : "");
    const bool first = gSeen.insert(key).second;

    if (cls == AuditClass::PixelAffecting)
        ++gFailures;
    else if (cls == AuditClass::Approximation)
        ++gWarnings;

    if (cls != AuditClass::IgnoredSafe)
        printOnce(first, cls, api, detail);
}

void resetAudit()
{
    gFailures = 0;
    gWarnings = 0;
    gSeen.clear();
}

int auditFailures() { return gFailures; }
int auditWarnings() { return gWarnings; }

void printAuditReport(const char* context)
{
    std::printf("fwref audit [%s]: %d unsupported pixel-affecting call(s), %d approximation(s)\n",
                context ? context : "render", gFailures, gWarnings);
    for (const std::string& key : gSeen)
        std::printf("  seen: %s\n", key.c_str());
}

} // namespace fwref

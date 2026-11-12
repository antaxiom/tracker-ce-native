#include "TextFit.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <set>
#include <string>

namespace trakr::ui {

namespace {

struct AuditEntry {
    std::string tag;
    std::string text;
    int boxWidth;
    int advance;
    bool clipped;
    bool operator<(const AuditEntry& o) const {
        if (tag != o.tag) return tag < o.tag;
        if (text != o.text) return text < o.text;
        if (boxWidth != o.boxWidth) return boxWidth < o.boxWidth;
        if (advance != o.advance) return advance < o.advance;
        return clipped < o.clipped;
    }
};

std::set<AuditEntry>& auditEntries() {
    static std::set<AuditEntry> entries;
    return entries;
}

void recordAudit(const char* tag, const char* text, float boxWidth,
                 const FittedText& fit) {
    if (!tag) return;
    if (!layoutAuditEnabled()) return;
    if (!fit.scaled && !fit.clipped) return;
    AuditEntry e;
    e.tag = tag ? tag : "?";
    e.text = text ? text : "";
    e.boxWidth = static_cast<int>(std::lround(boxWidth));
    e.advance = static_cast<int>(std::lround(fit.advance * 10.0f));
    e.clipped = fit.clipped;
    auto inserted = auditEntries().insert(e);
    if (inserted.second) {
        std::fprintf(stderr, "[LAYOUT] %s '%s' box=%dpx advance=%.1fpx %s\n",
                     e.tag.c_str(), e.text.c_str(), e.boxWidth,
                     fit.advance, fit.clipped ? "CLIPPED" : "shrunk");
    }
}

} // namespace

bool layoutAuditEnabled() {
    static const bool enabled = [] {
        const char* v = std::getenv("TRAKR_LAYOUT_AUDIT");
        return v && v[0] != '\0' && std::strcmp(v, "0") != 0;
    }();
    return enabled;
}

void resetLayoutAudit() { auditEntries().clear(); }

int layoutAuditCount() { return static_cast<int>(auditEntries().size()); }

void dumpLayoutAudit() {
    if (auditEntries().empty()) {
        std::fprintf(stderr, "[LAYOUT] no overflow/shrink events\n");
        return;
    }
    std::fprintf(stderr, "[LAYOUT] %zu fitted label(s):\n", auditEntries().size());
    for (const auto& e : auditEntries()) {
        std::fprintf(stderr, "[LAYOUT]   %s '%s' box=%dpx advance=%.1fpx %s\n",
                     e.tag.c_str(), e.text.c_str(), e.boxWidth,
                     e.advance / 10.0f, e.clipped ? "CLIPPED" : "shrunk");
    }
}

FittedText fitText(const char* text, float maxWidth, float preferredAdvance,
                   float preferredFontSize, float minAdvance) {
    FittedText out;
    if (!text || !*text) return out;
    out.chars = static_cast<int>(std::strlen(text));
    out.advance = preferredAdvance;
    out.fontSize = preferredFontSize;
    const float needed = static_cast<float>(out.chars) * preferredAdvance;
    if (needed <= maxWidth || maxWidth <= 0.0f) {
        out.width = needed;
        return out;
    }

    const float perChar = maxWidth / static_cast<float>(out.chars);
    if (perChar >= minAdvance) {
        out.advance = perChar;
        out.fontSize = preferredFontSize * (perChar / preferredAdvance);
        out.width = maxWidth;
        out.scaled = true;
        return out;
    }

    out.advance = minAdvance;
    out.fontSize = preferredFontSize * (minAdvance / preferredAdvance);
    out.chars = std::max(1, static_cast<int>(std::floor(maxWidth / minAdvance)));
    out.width = static_cast<float>(out.chars) * minAdvance;
    out.clipped = true;
    out.scaled = true;
    return out;
}

void drawTextFit(ImDrawList* dl, ImFont* font, float x0, float x1, float centerY,
                 ImU32 col, const char* text, bool centerX,
                 float preferredAdvance, float minAdvance, const char* auditTag) {
    if (!dl || !font || !text || !*text) return;
    const float boxWidth = x1 - x0;
    if (boxWidth <= 0.0f) return;

    FittedText fit = fitText(text, boxWidth, preferredAdvance, font->LegacySize, minAdvance);
    recordAudit(auditTag, text, boxWidth, fit);

    const float startX = centerX ? (x0 + x1 - fit.width) * 0.5f : x0;
    const ImVec2 sample = font->CalcTextSizeA(fit.fontSize, FLT_MAX, 0.0f, "Xg");
    const float drawY = centerY - sample.y * 0.5f;

    dl->PushClipRect(ImVec2(x0, centerY - font->LegacySize), ImVec2(x1, centerY + font->LegacySize), true);
    char buf[2] = {0, 0};
    for (int i = 0; i < fit.chars; ++i) {
        buf[0] = text[i];
        dl->AddText(font, fit.fontSize, ImVec2(startX + i * fit.advance, drawY), col, buf);
    }
    dl->PopClipRect();
}

} // namespace trakr::ui

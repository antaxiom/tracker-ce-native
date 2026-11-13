#include "BitmapText.h"

#include <cstring>
#include <vector>

#include "NewDisplay/CompressionRLE.h"
#include "RobotoMono-Bold_17_L4.h"
#include "RobotoMono-Light_14_L4.h"
#include "RobotoMono-Light_17_L4.h"
#include "RobotoMono-Regular_12_L4.h"
#include "RobotoMono-Regular_14_L4.h"

namespace trakr::hw {
namespace {

// Decoded L4 font: 148-byte header (widths[128], format, linestride, width,
// height) followed by per-glyph 4-bit bitmaps of linestride*height bytes.
struct GlyphFont {
    uint8_t widths[128]{};
    int linestride{0}, width{0}, height{0}, firstChar{0};
    std::vector<uint8_t> data;
};

GlyphFont decode(const CompressedData* comp, int compLen) {
    GlyphFont f;
    f.data.assign(32768, 0);
    decompress(comp, compLen, f.data.data());
    std::memcpy(f.widths, f.data.data(), 128);
    f.linestride = f.data[132];
    f.width = f.data[136];
    f.height = f.data[140];
    f.firstChar = 0;
    while (f.firstChar < 127 && f.widths[f.firstChar] == 0)
        ++f.firstChar;
    return f;
}

const GlyphFont& glyphFont(FwFont font) {
    static GlyphFont fonts[5];
    static bool init = false;
    if (!init) {
        init = true;
        fonts[0] = decode(Roboto_Mono_Light_14_L4_compressed, 3202);
        fonts[1] = decode(Roboto_Mono_Light_17_L4_compressed, 3553);
        fonts[2] = decode(Roboto_Mono_Regular_12_L4_compressed, 3041);
        fonts[3] = decode(Roboto_Mono_Regular_14_L4_compressed, 3487);
        fonts[4] = decode(Roboto_Mono_Bold_17_L4_compressed, 4884);
    }
    return fonts[static_cast<int>(font)];
}

} // namespace

int fwTextWidth(FwFont font, const char* text) {
    const GlyphFont& f = glyphFont(font);
    int w = 0;
    for (const unsigned char* p = reinterpret_cast<const unsigned char*>(text); p && *p; ++p)
        if (*p < 128) w += f.widths[*p];
    return w;
}

int fwTextHeight(FwFont font) { return glyphFont(font).height; }

void fwDrawText(ImDrawList* dl, FwFont font, float screenX, float screenYTop,
                const char* text, ImU32 color, float sx, float sy) {
    if (!dl || !text) return;
    const GlyphFont& f = glyphFont(font);
    const int glyphBytes = f.linestride * f.height;
    const int baseA = static_cast<int>((color >> IM_COL32_A_SHIFT) & 0xFF);
    float dx = screenX;
    for (const unsigned char* p = reinterpret_cast<const unsigned char*>(text); *p; ++p) {
        const unsigned char ch = *p;
        if (ch >= f.firstChar && ch < 128 && f.widths[ch] > 0) {
            const uint8_t* glyph = f.data.data() + 148 + (ch - f.firstChar) * glyphBytes;
            for (int r = 0; r < f.height; ++r) {
                const uint8_t* row = glyph + r * f.linestride;
                auto nibbleAt = [row](int c) {
                    const uint8_t bv = row[c / 2];
                    return static_cast<uint8_t>((c % 2 == 0) ? ((bv >> 4) & 0x0F) : (bv & 0x0F));
                };
                int c = 0;
                while (c < f.width) {
                    const uint8_t nib = nibbleAt(c);
                    const int a = (nib * 17 * baseA) / 255;
                    if (nib == 0 || a <= 0) {
                        ++c;
                        continue;
                    }
                    int runEnd = c + 1;
                    while (runEnd < f.width) {
                        const uint8_t next = nibbleAt(runEnd);
                        if (next == 0 || (next * 17 * baseA) / 255 != a) break;
                        ++runEnd;
                    }
                    const ImU32 col = (color & ~IM_COL32_A_MASK) | (static_cast<ImU32>(a) << IM_COL32_A_SHIFT);
                    dl->AddRectFilled(ImVec2(dx + c * sx, screenYTop + r * sy),
                                      ImVec2(dx + runEnd * sx, screenYTop + (r + 1) * sy), col);
                    c = runEnd;
                }
            }
        }
        if (ch < 128) dx += f.widths[ch] * sx;
    }
}

} // namespace trakr::hw

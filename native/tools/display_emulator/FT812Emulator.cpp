#include "FT812Emulator.h"
#include "NewDisplay/CompressionRLE.h"
#include "RobotoMono-Light_14_L4.h"
#include "RobotoMono-Light_17_L4.h"
#include "RobotoMono-Regular_14_L4.h"
#include "RobotoMono-Bold_17_L4.h"
#include "RobotoMono-Regular_12_L4.h"
#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#include "imstb_truetype.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>

namespace trakr::emulator {

namespace {
void decodeFont(const CompressedData* comp, int compLen, DecodedL4Font& out) {
    out.data.assign(32768, 0);
    decompress(comp, compLen, out.data.data());
    std::memcpy(out.widths, out.data.data(), 128);
    out.format = out.data[128];
    out.linestride = out.data[132];
    out.width = out.data[136];
    out.height = out.data[140];
    out.firstChar = 0;
    while (out.firstChar < 127 && out.widths[out.firstChar] == 0) {
        out.firstChar++;
    }
}

[[maybe_unused]] std::string resolveFontPath(const std::string& rel) {
    const std::vector<std::string> candidates = {
        rel,
        "native/" + rel,
        "../" + rel,
        "../../" + rel,
        "native/assets/fonts/" + rel,
        "assets/fonts/" + rel
    };
    for (const auto& c : candidates) {
        if (std::filesystem::exists(c)) return c;
    }
    return "";
}

[[maybe_unused]] bool loadTtfIntoDecodedFont(const std::string& path, float font_size, int targetAdvance, int cellW, int cellH, DecodedL4Font& out) {
    if (path.empty()) return false;
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    std::vector<uint8_t> ttf((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    f.close();

    stbtt_fontinfo font;
    if (!stbtt_InitFont(&font, ttf.data(), stbtt_GetFontOffsetForIndex(ttf.data(), 0))) return false;

    float scale = stbtt_ScaleForPixelHeight(&font, font_size);
    int ascent, descent, lineGap;
    stbtt_GetFontVMetrics(&font, &ascent, &descent, &lineGap);
    int baseline = static_cast<int>(std::round(ascent * scale));

    out.format = 2; // L4
    out.width = cellW;
    out.height = cellH;
    out.linestride = (cellW + 1) / 2;
    out.firstChar = 32;
    for (int i = 0; i < 128; ++i) {
        out.widths[i] = (i >= 32) ? targetAdvance : 0;
    }

    int glyphBytes = out.linestride * out.height;
    int totalChars = 128 - out.firstChar;
    out.data.assign(148 + totalChars * glyphBytes, 0);

    for (int ch = out.firstChar; ch < 128; ++ch) {
        int cIdx = ch - out.firstChar;
        uint8_t* glyphDst = out.data.data() + 148 + cIdx * glyphBytes;

        int x0, y0, x1, y1;
        stbtt_GetCodepointBitmapBox(&font, ch, scale, scale, &x0, &y0, &x1, &y1);
        int gw = x1 - x0;
        int gh = y1 - y0;
        if (gw > 0 && gh > 0) {
            std::vector<uint8_t> bmp(gw * gh);
            stbtt_MakeCodepointBitmap(&font, bmp.data(), gw, gh, gw, scale, scale, ch);

            int startX = x0;
            int startY = baseline + y0;
            for (int r = 0; r < gh; ++r) {
                int py = startY + r;
                if (py < 0 || py >= cellH) continue;
                for (int c = 0; c < gw; ++c) {
                    int px = startX + c;
                    if (px < 0 || px >= cellW) continue;
                    uint8_t alpha = bmp[r * gw + c];
                    uint8_t nib = alpha >> 4;
                    if (nib > 0) {
                        int byteIdx = py * out.linestride + (px / 2);
                        if (px % 2 == 0) {
                            glyphDst[byteIdx] = (glyphDst[byteIdx] & 0x0F) | (nib << 4);
                        } else {
                            glyphDst[byteIdx] = (glyphDst[byteIdx] & 0xF0) | (nib & 0x0F);
                        }
                    }
                }
            }
        }
    }
    return true;
}
} // namespace

FT812DisplayEmulator::FT812DisplayEmulator() : fb_(kWidth * kHeight, 0xFF000000) {
    initFirmwareFonts();
}

void FT812DisplayEmulator::initFirmwareFonts() {
    // Use the firmware's embedded L4 font bitmaps and per-character advances.
    // Loading the TTF with a forced advance made wide glyphs overlap and made
    // list cells (e.g. Song "Pattern 1") bleed past their control width.
    decodeFont(Roboto_Mono_Light_14_L4_compressed,
               sizeof(Roboto_Mono_Light_14_L4_compressed) / sizeof(CompressedData), fonts_[0]);
    decodeFont(Roboto_Mono_Light_17_L4_compressed,
               sizeof(Roboto_Mono_Light_17_L4_compressed) / sizeof(CompressedData), fonts_[1]);
    decodeFont(Roboto_Mono_Regular_14_L4_compressed,
               sizeof(Roboto_Mono_Regular_14_L4_compressed) / sizeof(CompressedData), fonts_[2]);
    decodeFont(Roboto_Mono_Bold_17_L4_compressed,
               sizeof(Roboto_Mono_Bold_17_L4_compressed) / sizeof(CompressedData), fonts_[3]);
    decodeFont(Roboto_Mono_Regular_12_L4_compressed,
               sizeof(Roboto_Mono_Regular_12_L4_compressed) / sizeof(CompressedData), fonts_[4]);
}

void FT812DisplayEmulator::clear(ColorRGB col) {
    uint32_t val = (static_cast<uint32_t>(col.a) << 24) |
                   (static_cast<uint32_t>(col.b) << 16) |
                   (static_cast<uint32_t>(col.g) << 8) |
                   static_cast<uint32_t>(col.r);
    std::fill(fb_.begin(), fb_.end(), val);
}

void FT812DisplayEmulator::blendPixel(int x, int y, ColorRGB col) {
    if (x < clipX0_ || x >= clipX1_ || y < clipY0_ || y >= clipY1_) return;
    if (x < 0 || x >= kWidth || y < 0 || y >= kHeight || col.a == 0) return;
    uint32_t& dst = fb_[y * kWidth + x];
    if (blendMode_ == BlendMode::SrcAlphaZero) {
        if (col.a == 255) {
            dst = (0xFF000000u) | (static_cast<uint32_t>(col.b) << 16) |
                  (static_cast<uint32_t>(col.g) << 8) | static_cast<uint32_t>(col.r);
        } else {
            dst = (0xFF000000u) | (static_cast<uint32_t>((col.b * col.a) / 255) << 16) |
                  (static_cast<uint32_t>((col.g * col.a) / 255) << 8) |
                  static_cast<uint32_t>((col.r * col.a) / 255);
        }
        return;
    }
    if (blendMode_ == BlendMode::SrcAlphaOne) {
        uint8_t ar = dst & 0xFF;
        uint8_t ag = (dst >> 8) & 0xFF;
        uint8_t ab = (dst >> 16) & 0xFF;
        auto add = [](uint8_t d, uint8_t s, uint32_t a) {
            const uint32_t v = static_cast<uint32_t>(d) + (static_cast<uint32_t>(s) * a) / 255u;
            return static_cast<uint8_t>(v > 255u ? 255u : v);
        };
        dst = 0xFF000000u | (static_cast<uint32_t>(add(ab, col.b, col.a)) << 16) |
              (static_cast<uint32_t>(add(ag, col.g, col.a)) << 8) |
              static_cast<uint32_t>(add(ar, col.r, col.a));
        return;
    }
    if (col.a == 255) {
        dst = (0xFF000000u) | (static_cast<uint32_t>(col.b) << 16) |
              (static_cast<uint32_t>(col.g) << 8) | static_cast<uint32_t>(col.r);
        return;
    }
    uint8_t dr = dst & 0xFF;
    uint8_t dg = (dst >> 8) & 0xFF;
    uint8_t db = (dst >> 16) & 0xFF;
    uint32_t a = col.a;
    uint32_t ia = 255 - a;
    uint8_t or_ = static_cast<uint8_t>((col.r * a + dr * ia) / 255);
    uint8_t og  = static_cast<uint8_t>((col.g * a + dg * ia) / 255);
    uint8_t ob  = static_cast<uint8_t>((col.b * a + db * ia) / 255);
    dst = 0xFF000000u | (static_cast<uint32_t>(ob) << 16) |
          (static_cast<uint32_t>(og) << 8) | static_cast<uint32_t>(or_);
}

void FT812DisplayEmulator::setClipRect(int x0, int y0, int x1, int y1) {
    clipX0_ = std::clamp(x0, 0, kWidth);
    clipY0_ = std::clamp(y0, 0, kHeight);
    clipX1_ = std::clamp(x1, 0, kWidth);
    clipY1_ = std::clamp(y1, 0, kHeight);
}

void FT812DisplayEmulator::resetClip() {
    clipX0_ = 0;
    clipY0_ = 0;
    clipX1_ = kWidth;
    clipY1_ = kHeight;
}

void FT812DisplayEmulator::drawLinearGradient(int x0, int y0, ColorRGB c0,
                                              int x1, int y1, ColorRGB c1) {
    // EVE CMD_GRADIENT: fills the active scissor, interpolating along the
    // (x0,y0)->(x1,y1) axis. Projection is clamped to the segment.
    int dx = x1 - x0;
    int dy = y1 - y0;
    int len2 = dx * dx + dy * dy;
    for (int y = clipY0_; y < clipY1_; ++y) {
        for (int x = clipX0_; x < clipX1_; ++x) {
            float t = 0.0f;
            if (len2 > 0) {
                t = static_cast<float>((x - x0) * dx + (y - y0) * dy) /
                    static_cast<float>(len2);
                t = std::clamp(t, 0.0f, 1.0f);
            }
            ColorRGB c{
                static_cast<uint8_t>(c0.r + (c1.r - c0.r) * t),
                static_cast<uint8_t>(c0.g + (c1.g - c0.g) * t),
                static_cast<uint8_t>(c0.b + (c1.b - c0.b) * t),
                255
            };
            blendPixel(x, y, c);
        }
    }
}

void FT812DisplayEmulator::drawRectFilled(int x, int y, int w, int h, ColorRGB col) {
    int x0 = std::clamp(x, 0, kWidth), x1 = std::clamp(x + w, 0, kWidth);
    int y0 = std::clamp(y, 0, kHeight), y1 = std::clamp(y + h, 0, kHeight);
    for (int r = y0; r < y1; ++r) {
        for (int c = x0; c < x1; ++c) {
            blendPixel(c, r, col);
        }
    }
}

void FT812DisplayEmulator::drawVerticalGradient(int x, int y, int w, int h, ColorRGB topCol, ColorRGB btmCol) {
    if (h <= 0) return;
    for (int row = 0; row < h; ++row) {
        float t = static_cast<float>(row) / static_cast<float>(std::max(1, h - 1));
        ColorRGB c{
            static_cast<uint8_t>(topCol.r + (btmCol.r - topCol.r) * t),
            static_cast<uint8_t>(topCol.g + (btmCol.g - topCol.g) * t),
            static_cast<uint8_t>(topCol.b + (btmCol.b - topCol.b) * t),
            255
        };
        drawRectFilled(x, y + row, w, 1, c);
    }
}

void FT812DisplayEmulator::drawRectOutline(int x, int y, int w, int h, ColorRGB col, int thickness) {
    drawRectFilled(x, y, w, thickness, col);
    drawRectFilled(x, y + h - thickness, w, thickness, col);
    drawRectFilled(x, y, thickness, h, col);
    drawRectFilled(x + w - thickness, y, thickness, h, col);
}

void FT812DisplayEmulator::drawLine(int x0, int y0, int x1, int y1, ColorRGB col, int thickness) {
    if (y0 == y1) {
        drawRectFilled(std::min(x0, x1), y0 - thickness / 2, std::abs(x1 - x0) + 1, thickness, col);
        return;
    }
    if (x0 == x1) {
        drawRectFilled(x0 - thickness / 2, std::min(y0, y1), thickness, std::abs(y1 - y0) + 1, col);
        return;
    }
}

int FT812DisplayEmulator::measureTextWidth(const std::string& text, FirmwareFont fontId) const {
    const auto& f = fonts_[static_cast<int>(fontId)];
    int w = 0;
    for (unsigned char ch : text) {
        if (ch < 128) w += f.widths[ch];
    }
    return w;
}

int FT812DisplayEmulator::fontHeight(FirmwareFont fontId) const {
    return fonts_[static_cast<int>(fontId)].height;
}

void FT812DisplayEmulator::drawText(int x, int y, const std::string& text, ColorRGB col,
                                    FirmwareFont fontId, bool centerX, bool centerY) {
    uint16_t options = 0;
    if (centerX) options |= 512;
    if (centerY) options |= 1024;
    drawTextCmd(x, y, text, col, fontId, options, 0);
}

void FT812DisplayEmulator::drawGlyph(int x, int y, unsigned char ch, ColorRGB col,
                                     FirmwareFont fontId) {
    const auto& f = fonts_[static_cast<int>(fontId)];
    if (ch < f.firstChar || ch >= 128 || f.widths[ch] == 0) return;
    int charIdx = ch - f.firstChar;
    int glyphBytes = f.linestride * f.height;
    const uint8_t* glyph = f.data.data() + 148 + charIdx * glyphBytes;
    for (int r = 0; r < f.height; ++r) {
        for (int c = 0; c < f.width; ++c) {
            uint8_t byteVal = glyph[r * f.linestride + (c / 2)];
            uint8_t nib = (c % 2 == 0) ? ((byteVal >> 4) & 0x0F) : (byteVal & 0x0F);
            if (nib > 0) {
                ColorRGB pxCol = col;
                pxCol.a = static_cast<uint8_t>((nib * 17 * col.a) / 255);
                blendPixel(x + c, y + r, pxCol);
            }
        }
    }
}

void FT812DisplayEmulator::drawTextCmd(int x, int y, const std::string& text, ColorRGB col,
                                       FirmwareFont fontId, uint16_t options, uint32_t length) {
    const auto& f = fonts_[static_cast<int>(fontId)];
    std::string s = text;
    if (length > 0 && length < s.size()) s.resize(length);

    int w = 0;
    for (unsigned char ch : s) {
        if (ch < 128) w += f.widths[ch];
    }

    int drawX = x;
    if (options & 2048) {
        drawX = x - w; // OPT_RIGHTX
    } else if (options & 512) {
        drawX = x - w / 2; // OPT_CENTERX
    }
    int drawY = (options & 1024) ? (y - f.height / 2) : y; // OPT_CENTERY

    for (unsigned char ch : s) {
        drawGlyph(drawX, drawY, ch, col, fontId);
        if (ch < 128) drawX += f.widths[ch];
    }
}

void FT812DisplayEmulator::drawTextClipped(int x, int y, const std::string& text, ColorRGB col,
                                           FirmwareFont fontId, bool centerX, bool centerY,
                                           int clipX0, int clipX1) {
    const auto& f = fonts_[static_cast<int>(fontId)];
    int drawX = centerX ? (x - measureTextWidth(text, fontId) / 2) : x;
    int drawY = centerY ? (y - f.height / 2) : y;
    int glyphBytes = f.linestride * f.height;

    for (unsigned char ch : text) {
        if (ch >= f.firstChar && ch < 128 && f.widths[ch] > 0) {
            int charIdx = ch - f.firstChar;
            const uint8_t* glyph = f.data.data() + 148 + charIdx * glyphBytes;
            for (int r = 0; r < f.height; ++r) {
                for (int c = 0; c < f.width; ++c) {
                    int px = drawX + c;
                    if (px < clipX0 || px >= clipX1) continue;
                    uint8_t byteVal = glyph[r * f.linestride + (c / 2)];
                    uint8_t nib = (c % 2 == 0) ? ((byteVal >> 4) & 0x0F) : (byteVal & 0x0F);
                    if (nib > 0) {
                        ColorRGB pxCol = col;
                        pxCol.a = static_cast<uint8_t>((nib * 17 * col.a) / 255);
                        blendPixel(px, drawY + r, pxCol);
                    }
                }
            }
        }
        if (ch < 128) drawX += f.widths[ch];
    }
}

void FT812DisplayEmulator::drawTextVertical270(int centerX, int startY, const std::string& text,
                                               ColorRGB col, FirmwareFont fontId) {
    // Matches core/libs/NewDisplay/label.cpp lines 199-256 (controlStyleVerticalText)
    const auto& f = fonts_[static_cast<int>(fontId)];
    int baseX = centerX - f.height / 2;
    int curY = startY;
    int glyphBytes = f.linestride * f.height;

    for (unsigned char ch : text) {
        if (ch >= f.firstChar && ch < 128 && f.widths[ch] > 0) {
            int charIdx = ch - f.firstChar;
            const uint8_t* glyph = f.data.data() + 148 + charIdx * glyphBytes;
            for (int r = 0; r < f.height; ++r) {
                for (int c = 0; c < f.width; ++c) {
                    uint8_t byteVal = glyph[r * f.linestride + (c / 2)];
                    uint8_t nib = (c % 2 == 0) ? ((byteVal >> 4) & 0x0F) : (byteVal & 0x0F);
                    if (nib > 0) {
                        ColorRGB pxCol = col;
                        pxCol.a = static_cast<uint8_t>((nib * 17 * col.a) / 255);
                        // 270 degree rotation: (c, r) -> (baseX + r, curY - c)
                        blendPixel(baseX + r, curY - c, pxCol);
                    }
                }
            }
        }
        // cSongPlayer/label.cpp advance by font->data[ch] + 1 (letter spacing).
        if (ch < 128) curY -= f.widths[ch] + 1;
    }
}

namespace {
uint32_t crc32Update(uint32_t crc, const uint8_t* data, size_t len) {
    static uint32_t table[256];
    static bool init = false;
    if (!init) {
        for (uint32_t i = 0; i < 256; ++i) {
            uint32_t c = i;
            for (int k = 0; k < 8; ++k) c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
            table[i] = c;
        }
        init = true;
    }
    for (size_t i = 0; i < len; ++i) crc = table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    return crc;
}

void putBe32(std::vector<uint8_t>& out, uint32_t v) {
    out.push_back(static_cast<uint8_t>(v >> 24));
    out.push_back(static_cast<uint8_t>(v >> 16));
    out.push_back(static_cast<uint8_t>(v >> 8));
    out.push_back(static_cast<uint8_t>(v));
}

void pngChunk(std::vector<uint8_t>& out, const char type[4], const std::vector<uint8_t>& data) {
    putBe32(out, static_cast<uint32_t>(data.size()));
    std::vector<uint8_t> typed(type, type + 4);
    typed.insert(typed.end(), data.begin(), data.end());
    out.insert(out.end(), typed.begin(), typed.end());
    uint32_t crc = crc32Update(0xFFFFFFFFu, typed.data(), typed.size()) ^ 0xFFFFFFFFu;
    putBe32(out, crc);
}
} // namespace

bool FT812DisplayEmulator::savePng(const std::string& path) const {
    // Deterministic PNG writer (stored deflate blocks, no external tools).
    std::vector<uint8_t> raw;
    raw.reserve(kHeight * (1 + kWidth * 4));
    for (int y = 0; y < kHeight; ++y) {
        raw.push_back(0); // filter: none
        for (int x = 0; x < kWidth; ++x) {
            uint32_t px = fb_[static_cast<size_t>(y) * kWidth + x];
            raw.push_back(static_cast<uint8_t>(px & 0xFF));
            raw.push_back(static_cast<uint8_t>((px >> 8) & 0xFF));
            raw.push_back(static_cast<uint8_t>((px >> 16) & 0xFF));
            raw.push_back(static_cast<uint8_t>((px >> 24) & 0xFF));
        }
    }

    std::vector<uint8_t> z;
    z.push_back(0x78);
    z.push_back(0x01);
    size_t pos = 0;
    while (pos < raw.size()) {
        size_t n = std::min<size_t>(65535, raw.size() - pos);
        bool final = (pos + n == raw.size());
        z.push_back(final ? 1 : 0);
        z.push_back(static_cast<uint8_t>(n & 0xFF));
        z.push_back(static_cast<uint8_t>((n >> 8) & 0xFF));
        z.push_back(static_cast<uint8_t>((~n) & 0xFF));
        z.push_back(static_cast<uint8_t>(((~n) >> 8) & 0xFF));
        z.insert(z.end(), raw.begin() + static_cast<std::ptrdiff_t>(pos),
                 raw.begin() + static_cast<std::ptrdiff_t>(pos + n));
        pos += n;
    }
    uint32_t a = 1, b = 0;
    for (uint8_t byte : raw) {
        a = (a + byte) % 65521;
        b = (b + a) % 65521;
    }
    putBe32(z, (b << 16) | a);

    std::vector<uint8_t> png = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
    std::vector<uint8_t> ihdr;
    putBe32(ihdr, kWidth);
    putBe32(ihdr, kHeight);
    ihdr.push_back(8); // bit depth
    ihdr.push_back(6); // RGBA
    ihdr.push_back(0);
    ihdr.push_back(0);
    ihdr.push_back(0);
    pngChunk(png, "IHDR", ihdr);
    pngChunk(png, "IDAT", z);
    pngChunk(png, "IEND", {});

    std::ofstream out(path, std::ios::binary);
    if (!out) return false;
    out.write(reinterpret_cast<const char*>(png.data()), static_cast<std::streamsize>(png.size()));
    return static_cast<bool>(out);
}


void drawTitleBar(FT812DisplayEmulator& emu, const std::string& title,
                  const std::string& transport, const std::string& inst) {
    emu.drawRectFilled(0, 0, 800, 28, ColorRGB::fromHex(0x080808));
    emu.drawLine(0, 27, 800, 27, ColorRGB::fromHex(0xFFFFFF), 1);
    emu.drawText(9, 14, title, ColorRGB::fromHex(0xFFFFFF), FirmwareFont::RobotoMonoBold17, false, true);
    emu.drawText(400, 14, transport, ColorRGB::fromHex(0xFFFFFF), FirmwareFont::RobotoMonoBold17, true, true);
    emu.drawText(700, 14, inst, ColorRGB::fromHex(0xFFFFFF), FirmwareFont::RobotoMonoBold17, true, true);
}

void drawScreenButtonsBar(FT812DisplayEmulator& emu, const char* top[8], const char* btm[8],
                          const uint32_t lineCol[8], int activeIdx) {
    emu.drawRectFilled(0, 425, 800, 55, ColorRGB::fromHex(0x0A0A0A));
    for (int i = 0; i < 8; ++i) {
        int bx = i * 100, cx = bx + 50;
        if (i > 0) emu.drawLine(bx, 438, bx, 479, ColorRGB::fromHex(0x777777), 1);
        if (activeIdx == i) {
            emu.drawRectOutline(bx + 1, 425, 98, 54, ColorRGB::fromHex(0xCD1818), 2);
        }
        bool twoLine = (btm && btm[i] && btm[i][0] != '\0');
        if (top && top[i] && top[i][0] != '\0') {
            uint32_t tCol = (lineCol && lineCol[i] == 0xCD1818 && !twoLine) ? 0x777777 : 0xFFFFFF;
            emu.drawText(cx, twoLine ? 440 : 452, top[i], ColorRGB::fromHex(tCol), FirmwareFont::RobotoMonoBold17, true, true);
        }
        if (twoLine) {
            emu.drawText(cx, 461, btm[i], ColorRGB::fromHex(0xFFFFFF), FirmwareFont::RobotoMonoBold17, true, true);
        }
        if (lineCol && lineCol[i] != 0) {
            emu.drawLine(cx - 40, 474, cx + 40, 474, ColorRGB::fromHex(lineCol[i]), 2);
        }
    }
}

bool renderOriginalReferenceScreen(const std::string& screenName, const std::string& outPngPath) {
    FT812DisplayEmulator emu;
    emu.clear(ColorRGB::fromHex(0x000000));
    bool ok = renderCoreScreen(screenName, emu);
    if (!ok) {
        ok = renderExtraScreen(screenName, emu);
    }
    if (!ok) return false;
    return emu.savePng(outPngPath);
}

} // namespace trakr::emulator

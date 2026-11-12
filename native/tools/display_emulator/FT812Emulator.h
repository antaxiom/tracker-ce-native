#pragma once

// FT812 / EVE Graphics Controller Display Emulator
// Emulates the 800x480 FT812 display processor & L4 RLE fonts used in core/libs/NewDisplay
// Reference: core/libs/NewDisplay/display.cpp, tracker.cpp, multiLabel.cpp, label.cpp, bgLabel.cpp

#include <cstdint>
#include <vector>
#include <string>

namespace trakr::emulator {

struct ColorRGB {
    uint8_t r{0};
    uint8_t g{0};
    uint8_t b{0};
    uint8_t a{255};

    static ColorRGB fromHex(uint32_t hex, uint8_t alpha = 255) {
        ColorRGB c;
        c.r = static_cast<uint8_t>((hex >> 16) & 0xFF);
        c.g = static_cast<uint8_t>((hex >> 8) & 0xFF);
        c.b = static_cast<uint8_t>(hex & 0xFF);
        c.a = alpha;
        return c;
    }
};

// Firmware font table order (core/libs/NewDisplay/display.cpp fontsData1):
// 0=Light14, 1=Light17, 2=Regular14, 3=Bold17, 4=Regular12
enum class FirmwareFont {
    RobotoMonoLight14 = 0,
    RobotoMonoLight17 = 1,
    RobotoMonoRegular14 = 2,
    RobotoMonoBold17 = 3,
    RobotoMonoRegular12 = 4
};

enum class BlendMode {
    SrcAlphaOneMinusSrcAlpha,
    SrcAlphaZero,
    SrcAlphaOne // EVE SRC_ALPHA, ONE (additive; cPoints/cLineIndicator regions)
};

struct DecodedL4Font {
    uint8_t widths[128]{};
    uint8_t format{0};
    uint8_t linestride{0};
    uint8_t width{0};
    uint8_t height{0};
    uint8_t firstChar{0};
    std::vector<uint8_t> data;
};

class FT812DisplayEmulator {
public:
    static constexpr int kWidth = 800;
    static constexpr int kHeight = 480;

    FT812DisplayEmulator();
    ~FT812DisplayEmulator() = default;

    void clear(ColorRGB col = {0, 0, 0, 255});
    void blendPixel(int x, int y, ColorRGB col);
    void drawRectFilled(int x, int y, int w, int h, ColorRGB col);
    void drawVerticalGradient(int x, int y, int w, int h, ColorRGB topCol, ColorRGB btmCol);
    void drawRectOutline(int x, int y, int w, int h, ColorRGB col, int thickness = 1);
    void drawLine(int x0, int y0, int x1, int y1, ColorRGB col, int thickness = 1);

    // EVE-like raster state used by the source-true firmware reference (fwref).
    void setBlendMode(BlendMode mode) { blendMode_ = mode; }
    BlendMode blendMode() const { return blendMode_; }
    void setClipRect(int x0, int y0, int x1, int y1);
    void resetClip();
    void drawLinearGradient(int x0, int y0, ColorRGB c0, int x1, int y1, ColorRGB c1);

    int measureTextWidth(const std::string& text, FirmwareFont fontId = FirmwareFont::RobotoMonoRegular14) const;
    int fontHeight(FirmwareFont fontId = FirmwareFont::RobotoMonoRegular14) const;

    // Horizontal FT812 L4 bitmap glyph rendering
    void drawText(int x, int y, const std::string& text, ColorRGB col,
                  FirmwareFont fontId = FirmwareFont::RobotoMonoRegular14, bool centerX = false, bool centerY = false);

    // EVE CMD_TEXT clone: options supports OPT_CENTERX / OPT_CENTERY / OPT_RIGHTX.
    // length == 0 draws up to the first NUL, otherwise at most `length` characters.
    void drawTextCmd(int x, int y, const std::string& text, ColorRGB col, FirmwareFont fontId,
                     uint16_t options, uint32_t length = 0);

    void drawGlyph(int x, int y, unsigned char ch, ColorRGB col, FirmwareFont fontId);

    // Same as drawText, but pixels outside [clipX0, clipX1) are discarded.
    // Firmware list/label controls clip text to their control rectangle.
    void drawTextClipped(int x, int y, const std::string& text, ColorRGB col,
                         FirmwareFont fontId, bool centerX, bool centerY,
                         int clipX0, int clipX1);

    // Vertical 270-degree rotated FT812 L4 glyph rendering (label.cpp controlStyleVerticalText)
    void drawTextVertical270(int centerX, int startY, const std::string& text, ColorRGB col,
                             FirmwareFont fontId = FirmwareFont::RobotoMonoBold17);

    // Save current 800x480 framebuffer to a 32-bit RGBA PNG file
    bool savePng(const std::string& path) const;

    const std::vector<uint32_t>& framebuffer() const { return fb_; }

private:
    std::vector<uint32_t> fb_; // 800x480 RGBA32 pixels
    DecodedL4Font fonts_[5];
    BlendMode blendMode_ = BlendMode::SrcAlphaOneMinusSrcAlpha;
    int clipX0_ = 0;
    int clipY0_ = 0;
    int clipX1_ = kWidth;
    int clipY1_ = kHeight;
    void initFirmwareFonts();
};

// Generates an exact 1:1 original firmware reference frame for any screen
// ("perform", "pattern", "song", "master", "instrument", "sample", "loader", "config", "file")
bool renderOriginalReferenceScreen(const std::string& screenName, const std::string& outPngPath);
void drawTitleBar(FT812DisplayEmulator& emu, const std::string& title,
                  const std::string& transport = "0m:00s  ||  P1.001",
                  const std::string& inst = "1. 909 Kick");
void drawScreenButtonsBar(FT812DisplayEmulator& emu, const char* top[8], const char* btm[8],
                          const uint32_t lineCol[8], int activeIdx = -1);

bool renderCoreScreen(const std::string& screenName, FT812DisplayEmulator& emu);
bool renderExtraScreen(const std::string& screenName, FT812DisplayEmulator& emu);

} // namespace trakr::emulator

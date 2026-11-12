// Phase-1 oracle gate.
//
// Creates a cBar using the exact firmware properties from
// core/interface/instrumentEditor/gInstrEdit.cpp:163-172 and asserts the
// pixels produced by the unmodified bar.cpp through the FT812 API shim.

#include "fwref.h"

#include "FT812Emulator.h"
#include "barControl.h"
#include "display.h"
#include "fwref_internal.h"

#include <cstdio>
#include <string>
#include <vector>

namespace
{

uint32_t rgbOf(uint32_t px)
{
    return px & 0xFFFFFF;
}

std::string hex(uint32_t v)
{
    char buf[16];
    std::snprintf(buf, sizeof(buf), "#%06X", v & 0xFFFFFF);
    return buf;
}

} // namespace

namespace fwref
{

bool runOracleSelfTest(std::string& error)
{
    using trakr::emulator::ColorRGB;
    using trakr::emulator::FT812DisplayEmulator;

    FT812DisplayEmulator emu;
    emu.clear(ColorRGB::fromHex(0x000000));
    emu.resetClip();

    initFonts();
    setTarget(&emu);
    display.destroyAll();

    strControlProperties prop;
    prop.x = 1;
    prop.y = 29;
    prop.w = 97; // 800 / 8 - 3
    prop.h = 394;
    prop.value = 50;
    prop.colors = display.getTheme().activeBarColors;
    prop.style = controlStyleValue_0_100 | controlStyleBackground | controlStyleGradient |
                 controlStyleBottomShadow | controlStyleShow;

    hControl handle = display.createControl<cBar>(&prop);
    if (handle < 0)
    {
        error = "createControl<cBar> failed";
        return false;
    }
    display.setControlValue(handle, 50);
    display.showControl(handle);
    display.renderAll();

    const auto& fb = emu.framebuffer();
    auto px = [&](int x, int y) { return rgbOf(fb[static_cast<size_t>(y) * 800 + x]); };

    if (px(2, 30) != 0x0A0A0A)
    {
        error = "bar background at (2,30) is " + hex(px(2, 30)) + ", expected #0A0A0A";
        return false;
    }

    // controlStyleGradient: 8px black -> #080808 at x=11..18. Pixel 12 is
    // 1/8 of the way along, so #010101 (the plan's #080808 sample at (12,41)
    // ignored this gradient).
    if (px(12, 41) != 0x010101)
    {
        error = "bar left gradient at (12,41) is " + hex(px(12, 41)) + ", expected #010101";
        return false;
    }

    // The container is #080808; sample away from the 8px left gradient.
    if (px(40, 41) != 0x080808)
    {
        error = "bar container at (40,41) is " + hex(px(40, 41)) + ", expected #080808";
        return false;
    }

    // value 50 -> barHeight=369, barFillY=369-184=185, fill top = 39+185 = 224
    int firstWhiteY = -1;
    for (int y = 29; y < 424; ++y)
    {
        if (px(50, y) == 0xFFFFFF)
        {
            firstWhiteY = y;
            break;
        }
    }
    if (firstWhiteY != 224)
    {
        error = "bar fill top at x=50 is " + std::to_string(firstWhiteY) + ", expected 224";
        return false;
    }

    int whiteCount = 0;
    for (int x = 0; x < 100; ++x)
        if (px(x, 300) == 0xFFFFFF)
            whiteCount++;
    if (whiteCount != 75)
    {
        error = "bar fill width is " + std::to_string(whiteCount) + "px, expected 75px";
        return false;
    }
    if (px(50, 300) != 0xFFFFFF)
    {
        error = "bar fill at (50,300) is " + hex(px(50, 300)) + ", expected #FFFFFF";
        return false;
    }

    for (int y = 29; y < 424; ++y)
    {
        for (int x = 1; x < 799; ++x)
        {
            uint32_t c = px(x, y);
            int r = static_cast<int>(c & 0xFF);
            int g = static_cast<int>((c >> 8) & 0xFF);
            int b = static_cast<int>((c >> 16) & 0xFF);
            if (g > r + 16 && g > b + 16)
            {
                error = "unexpected green pixel in bar at (" + std::to_string(x) + "," +
                        std::to_string(y) + ") = " + hex(c);
                return false;
            }
        }
    }

    return true;
}

} // namespace fwref

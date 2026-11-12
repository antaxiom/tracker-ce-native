// Host font decode for the fwref oracle.
//
// Uses the same compressed Roboto Mono L4 blobs and the same metadata offsets
// as core/libs/NewDisplay/display.cpp:236-266. No glyph or metric logic is
// duplicated: `decompress()` is the firmware implementation.

#include "displayStructs.h"
#include "NewDisplay/CompressionRLE.h"

#include <string.h>
#include <algorithm>
#include <vector>

#include "RobotoMono-Light_14_L4.h"
#include "RobotoMono-Light_17_L4.h"
#include "RobotoMono-Regular_14_L4.h"
#include "RobotoMono-Bold_17_L4.h"
#include "RobotoMono-Regular_12_L4.h"

namespace fwref
{

void initFonts();

} // namespace fwref

const uint8_t displayFontCount = 5;
strFont fonts[displayFontCount];

namespace
{
struct FontBlob
{
    const CompressedData* data;
    int count;
};

const FontBlob kFonts[displayFontCount] = {
    { Roboto_Mono_Light_14_L4_compressed,
      sizeof(Roboto_Mono_Light_14_L4_compressed) / sizeof(CompressedData) },
    { Roboto_Mono_Light_17_L4_compressed,
      sizeof(Roboto_Mono_Light_17_L4_compressed) / sizeof(CompressedData) },
    { Roboto_Mono_Regular_14_L4_compressed,
      sizeof(Roboto_Mono_Regular_14_L4_compressed) / sizeof(CompressedData) },
    { Roboto_Mono_Bold_17_L4_compressed,
      sizeof(Roboto_Mono_Bold_17_L4_compressed) / sizeof(CompressedData) },
    { Roboto_Mono_Regular_12_L4_compressed,
      sizeof(Roboto_Mono_Regular_12_L4_compressed) / sizeof(CompressedData) },
};

bool gFontsReady = false;
} // namespace

namespace fwref
{

void initFonts()
{
    if (gFontsReady)
        return;

    std::vector<uint8_t> buffer(65536, 0);

    for (uint8_t i = 0; i < displayFontCount; ++i)
    {
        std::fill(buffer.begin(), buffer.end(), 0);
        decompress(kFonts[i].data, kFonts[i].count, buffer.data());

        memcpy(fonts[i].data, buffer.data(), 148);
        fonts[i].format = buffer[128];
        fonts[i].linestride = buffer[132];
        fonts[i].width = buffer[136];
        fonts[i].height = buffer[140];

        uint16_t charNum = 0;
        for (int cha = 0; cha < 128; ++cha)
        {
            if (buffer[static_cast<size_t>(cha)])
                charNum++;
        }

        fonts[i].size = static_cast<uint32_t>(charNum) * fonts[i].linestride * fonts[i].height + 148;
        fonts[i].handle = i + 1;
        fonts[i].address = (i == 0) ? 0 : fonts[i - 1].address + fonts[i - 1].size;
        int rest = fonts[i].address % 4;
        fonts[i].address = fonts[i].address + (4 - rest);
        fonts[i].source = fonts[i].address + 148;
    }

    gFontsReady = true;
}

} // namespace fwref

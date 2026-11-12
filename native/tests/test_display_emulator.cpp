#include <catch2/catch_test_macros.hpp>
#include "FT812Emulator.h"
#include "fwref.h"
#include <cstdio>

TEST_CASE("FT812 Original Display Emulator: Framebuffer & Reference Screenshot", "[emulator][ft812]") {
    using namespace trakr::emulator;

    FT812DisplayEmulator emu;
    emu.clear(ColorRGB{0, 0, 0, 255});
    REQUIRE(emu.framebuffer().size() == 800 * 480);

    // Verify firmware L4 fonts decoded properly
    REQUIRE(emu.fontHeight(FirmwareFont::RobotoMonoBold17) == 22);
    REQUIRE(emu.measureTextWidth("Performance FX", FirmwareFont::RobotoMonoBold17) > 80);

    // Draw header and verify pixel values
    emu.drawRectFilled(0, 0, 800, 28, ColorRGB{8, 8, 8, 255});
    uint32_t px = emu.framebuffer()[10 * 800 + 10];
    REQUIRE((px & 0xFF) == 8);

    // Generate authentic FT812 reference screenshots for all menu screens
    REQUIRE(renderOriginalReferenceScreen("perform", "/tmp/emu_perform.png"));
    REQUIRE(renderOriginalReferenceScreen("pattern", "/tmp/emu_pattern.png"));
    REQUIRE(renderOriginalReferenceScreen("song", "/tmp/emu_song.png"));
    REQUIRE(renderOriginalReferenceScreen("instrument", "/tmp/emu_instrument.png"));
    REQUIRE(renderOriginalReferenceScreen("sample", "/tmp/emu_sample.png"));
    REQUIRE(renderOriginalReferenceScreen("master", "/tmp/emu_master.png"));
    REQUIRE(renderOriginalReferenceScreen("config", "/tmp/emu_config.png"));
    REQUIRE(renderOriginalReferenceScreen("loader", "/tmp/emu_loader.png"));
    REQUIRE(renderOriginalReferenceScreen("file", "/tmp/emu_file.png"));
}

TEST_CASE("FT812 reference dispatcher: Instrument/Master use the fwref oracle", "[emulator][fwref]") {
    using namespace trakr::emulator;

    auto rgb = [](uint32_t px) {
        return ((px & 0xFF) << 16) | (((px >> 8) & 0xFF) << 8) | ((px >> 16) & 0xFF);
    };

    REQUIRE(renderOriginalReferenceScreen("instrument", "/tmp/emu_instrument.png"));
    REQUIRE(renderOriginalReferenceScreen("master", "/tmp/emu_master.png"));

    FT812DisplayEmulator master;
    REQUIRE(fwref::renderMasterPage1(master, fwref::defaultComparisonScenario().master));
    // Source-true contract: 81% volume fill starts at y=110; red frame on column 0.
    REQUIRE(rgb(master.framebuffer()[110 * 800 + 50]) == 0xFFFFFF);
    REQUIRE(rgb(master.framebuffer()[200 * 800 + 1]) == 0xCD1818);
}

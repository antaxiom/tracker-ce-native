#include <catch2/catch_test_macros.hpp>

#include "FT812Emulator.h"
#include "fwref.h"

using namespace trakr::emulator;

namespace
{
uint32_t rgbOf(uint32_t px)
{
    uint32_t r = px & 0xFF;
    uint32_t g = (px >> 8) & 0xFF;
    uint32_t b = (px >> 16) & 0xFF;
    return (r << 16) | (g << 8) | b;
}
} // namespace

TEST_CASE("fwref oracle: unmodified firmware cBar self-test", "[fwref][oracle]")
{
    std::string error;
    REQUIRE(fwref::runOracleSelfTest(error));
}

TEST_CASE("fwref Instrument General reference pixels", "[fwref][instrument]")
{
    FT812DisplayEmulator emu;
    auto scenario = fwref::defaultComparisonScenario();
    REQUIRE(fwref::renderInstrumentGeneral(emu, scenario.instrument));

    const auto& fb = emu.framebuffer();
    auto px = [&](int x, int y) { return rgbOf(fb[static_cast<size_t>(y) * 800 + x]); };

    REQUIRE(px(1, 200) == 0xCD1818);   // selection frame
    REQUIRE(px(50, 35) == 0x0A0A0A);   // bar background
    REQUIRE(px(40, 41) == 0x080808);   // bar container
    REQUIRE(px(50, 300) == 0xFFFFFF);  // volume fill (white, no green)
    REQUIRE(px(50, 223) != 0xFFFFFF);  // fill starts at 224
    REQUIRE(px(50, 224) == 0xFFFFFF);
    REQUIRE(px(550, 40) == 0xFFFFFF);  // cutoff at 100%
    REQUIRE(px(404, 60) == 0xFFFFFF);  // filter list selection frame
    REQUIRE(px(404, 52) != 0xFFFFFF);  // frame starts at row 59, not 51
    REQUIRE(px(404, 59) == 0xFFFFFF);
    REQUIRE(px(404, 88) == 0xFFFFFF);
    REQUIRE(px(404, 95) != 0xFFFFFF);
    REQUIRE(px(744, 463) == 0x777777); // More dot 0
    REQUIRE(px(757, 463) == 0x222222); // More dot 1
    REQUIRE(px(100, 450) == 0x777777); // bottom separator
    REQUIRE(px(400, 28) == 0xFFFFFF);  // title bar underline
}

TEST_CASE("fwref Pattern reference pixels", "[fwref][pattern]")
{
    FT812DisplayEmulator emu;
    auto scenario = fwref::defaultComparisonScenario();
    REQUIRE(fwref::renderPattern(emu, scenario.pattern));

    const auto& fb = emu.framebuffer();
    auto px = [&](int x, int y) { return rgbOf(fb[static_cast<size_t>(y) * 800 + x]); };

    REQUIRE(px(450, 110) == 0x0E0E0E);  // step divider row 61 (stepDevider 4)
    REQUIRE(px(450, 140) == 0x080808);  // row 62 is not a divider
    REQUIRE(px(50, 220) == 0x30CB3F);   // green cursor param highlight
    REQUIRE(px(100, 210) == 0x30CB3F);  // green cursor frame
    REQUIRE(px(700, 210) == 0xFEF749);  // track end line at row 1
    REQUIRE(px(100, 423) == 0xCD1818);  // action frame top
    REQUIRE(px(50, 474) == 0x30CB3F);   // action underline
    REQUIRE(px(500, 450) == 0x777777);  // bottom separator
    REQUIRE(px(0, 28) == 0xFFFFFF);     // title bar underline edge
    REQUIRE(px(400, 10) == 0x080808);   // title bar bg
}

TEST_CASE("fwref Master 1/3 reference pixels", "[fwref][master]")
{
    FT812DisplayEmulator emu;
    auto scenario = fwref::defaultComparisonScenario();
    REQUIRE(fwref::renderMasterPage1(emu, scenario.master));

    const auto& fb = emu.framebuffer();
    auto px = [&](int x, int y) { return rgbOf(fb[static_cast<size_t>(y) * 800 + x]); };

    REQUIRE(px(1, 200) == 0xCD1818);  // selection frame
    REQUIRE(px(50, 35) == 0x0A0A0A);  // volume bar background
    REQUIRE(px(50, 109) != 0xFFFFFF); // 81% fill starts at 110
    REQUIRE(px(50, 110) == 0xFFFFFF);
    REQUIRE(px(300, 200) == 0x0A0A0A); // blank space cover over columns 1..4
    REQUIRE(px(505, 33) == 0xFFFFFF);  // saturation list selection frame
    REQUIRE(px(640, 300) == 0x080808); // bass boost empty bar container
}

TEST_CASE("fwref Master 2/3 Track Mixer reference pixels", "[fwref][master2]")
{
    FT812DisplayEmulator emu;
    auto scenario = fwref::defaultComparisonScenario();
    REQUIRE(fwref::renderMasterPage2(emu, scenario.master));

    const auto& fb = emu.framebuffer();
    auto px = [&](int x, int y) { return rgbOf(fb[static_cast<size_t>(y) * 800 + x]); };

    REQUIRE(px(5, 35) == 0x080808);   // track-name label background
    REQUIRE(px(20, 200) == 0xFFFFFF); // track volume fill (100%)
    REQUIRE(px(80, 200) == 0x080808); // level bar container (level 0)
    REQUIRE(px(410, 440) == 0x0A0A0A); // bottom bar
}

TEST_CASE("fwref Master 3/3 Global Mixer reference pixels", "[fwref][master3]")
{
    FT812DisplayEmulator emu;
    auto scenario = fwref::defaultComparisonScenario();
    REQUIRE(fwref::renderMasterPage3(emu, scenario.master));

    const auto& fb = emu.framebuffer();
    auto px = [&](int x, int y) { return rgbOf(fb[static_cast<size_t>(y) * 800 + x]); };

    REQUIRE(px(20, 200) == 0xFFFFFF);  // delay volume fill
    REQUIRE(px(60, 200) == 0x0A0A0A);  // delay level container
    REQUIRE(px(410, 440) == 0x0A0A0A); // bottom bar
}

TEST_CASE("fwref Master EQ reference pixels", "[fwref][masterEq]")
{
    FT812DisplayEmulator emu;
    auto scenario = fwref::defaultComparisonScenario();
    REQUIRE(fwref::renderMasterEq(emu, scenario.master));

    const auto& fb = emu.framebuffer();
    auto px = [&](int x, int y) { return rgbOf(fb[static_cast<size_t>(y) * 800 + x]); };

    REQUIRE(px(1, 200) == 0xCD1818);   // selection frame on band 0
    REQUIRE(px(50, 35) == 0x0A0A0A);   // band bar background above the fill
    REQUIRE(px(50, 60) == 0x080808);   // gradient ramp below the bar top
    REQUIRE(px(50, 300) == 0xFFFFFF);  // 0 dB band fill (bar centre)
    REQUIRE(px(250, 300) == 0xFFFFFF); // every band sits at 0 dB by default
    REQUIRE(px(520, 300) == 0x0A0A0A); // blank space cover over columns 5..7
    REQUIRE(px(710, 300) == 0x0A0A0A);
    REQUIRE(px(410, 440) == 0x0A0A0A); // bottom bar
}

TEST_CASE("fwref Config reference pixels", "[fwref][config]")
{
    FT812DisplayEmulator emu;
    auto scenario = fwref::defaultComparisonScenario();
    REQUIRE(fwref::renderConfig(emu, scenario.config));

    const auto& fb = emu.framebuffer();
    auto px = [&](int x, int y) { return rgbOf(fb[static_cast<size_t>(y) * 800 + x]); };

    REQUIRE(px(1, 200) == 0xCD1818);   // frame on the basemenu place
    REQUIRE(px(50, 200) == 0x0A0A0A);  // basemenu list background
    REQUIRE(px(400, 200) == 0x0A0A0A); // submenu list background
    REQUIRE(px(700, 200) == 0x0A0A0A); // blank space cover
}

TEST_CASE("fwref Perform reference pixels", "[fwref][perform]")
{
    FT812DisplayEmulator emu;
    auto scenario = fwref::defaultComparisonScenario();
    REQUIRE(fwref::renderPerform(emu, scenario.perform));

    const auto& fb = emu.framebuffer();
    auto px = [&](int x, int y) { return rgbOf(fb[static_cast<size_t>(y) * 800 + x]); };

    REQUIRE(px(20, 50) == 0x0A0A0A);   // top value cell background
    REQUIRE(px(13, 300) == 0x0A0A0A);  // value column background
    REQUIRE(px(410, 440) == 0x0A0A0A); // bottom bar
}

TEST_CASE("fwref Song reference pixels", "[fwref][song]")
{
    FT812DisplayEmulator emu;
    auto scenario = fwref::defaultComparisonScenario();
    REQUIRE(fwref::renderSong(emu, scenario.song));

    const auto& fb = emu.framebuffer();
    auto px = [&](int x, int y) { return rgbOf(fb[static_cast<size_t>(y) * 800 + x]); };

    REQUIRE(px(400, 10) == 0x080808);   // title bar background
    REQUIRE(px(1, 200) == 0x0A0A0A);    // cSongPlayer background
    REQUIRE(px(7, 100) == 0xFFFFFF);    // pattern 1, track 0 active block
    REQUIRE(px(67, 100) == 0x444344);   // pattern 1, muted track 2 block
    REQUIRE(px(277, 100) == 0x444344);  // pattern 1, muted MIDI track 1 block
    REQUIRE(px(127, 100) == 0x121212);  // pattern 1, empty track 4 block
    REQUIRE(px(7, 237) == 0xFFFFFF);    // pattern 2, track 0 block
    REQUIRE(px(7, 345) == 0x121212);    // pattern 3, empty track 0 block
    REQUIRE(px(7, 104) == 0x30CB3F);    // progress line
    REQUIRE(px(491, 91) == 0x30CB3F);   // active playlist selection frame
    REQUIRE(px(720, 100) == 0x080808);  // tempo bar container
    REQUIRE(px(720, 400) == 0xFFFFFF);  // tempo bar fill (value 16)
    REQUIRE(px(100, 450) == 0x777777);  // bottom separator
}

TEST_CASE("fwref Sample Loader reference pixels", "[fwref][loader]")
{
    FT812DisplayEmulator emu;
    auto scenario = fwref::defaultComparisonScenario();
    REQUIRE(fwref::renderLoader(emu, scenario.loader));

    const auto& fb = emu.framebuffer();
    auto px = [&](int x, int y) { return rgbOf(fb[static_cast<size_t>(y) * 800 + x]); };

    REQUIRE(px(400, 10) == 0x080808);   // title bar background
    REQUIRE(px(1, 200) == 0xCD1818);    // file-browser frame
    REQUIRE(px(100, 300) == 0x0A0A0A);  // explorer cList background
    REQUIRE(px(400, 300) == 0x0A0A0A);  // instrument cList background
    REQUIRE(px(620, 90) == 0x080808);   // preview volume container
    REQUIRE(px(620, 100) == 0xFFFFFF);  // preview volume fill (86%)
    REQUIRE(px(720, 350) == 0x080808);  // memory bar container
    REQUIRE(px(720, 370) == 0xFFFFFF);  // memory fill (12%)
    REQUIRE(px(750, 362) == 0xCD1818);  // memory compare delta (14% vs 12%)
    REQUIRE(px(400, 450) == 0x777777);  // bottom separator
}

TEST_CASE("fwref Sample Playback reference pixels", "[fwref][sample]")
{
    FT812DisplayEmulator emu;
    auto scenario = fwref::defaultComparisonScenario();
    REQUIRE(fwref::renderSample(emu, scenario.sample));

    const auto& fb = emu.framebuffer();
    auto px = [&](int x, int y) { return rgbOf(fb[static_cast<size_t>(y) * 800 + x]); };

    REQUIRE(px(400, 10) == 0x080808);   // title bar background
    REQUIRE(px(100, 225) == 0xFFFFFF);  // cSpectrum waveform peak
    REQUIRE(px(300, 140) == 0x0A0A0A);  // cPoints loop region fill (additive)
    REQUIRE(px(40, 116) == 0xFFFFFF);   // start point marker line
    REQUIRE(px(580, 116) == 0xFFFFFF);  // end point marker line
    REQUIRE(px(150, 340) == 0xFFFFFF);  // loop point marker triangle
    REQUIRE(px(180, 225) == 0xCD1818);  // cProgressCursor
    REQUIRE(px(180, 115) == 0x000000);  // cursor does not enter title bar
    REQUIRE(px(620, 100) == 0x0A0A0A);  // play-mode cList background
    REQUIRE(px(1, 424) == 0xCD1818);    // action-button frame place 0
}

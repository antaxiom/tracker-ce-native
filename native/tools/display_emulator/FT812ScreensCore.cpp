#include "FT812Emulator.h"
#include "fwref.h"
#include <string>

// Original Firmware Reference Screens (Core: Perform, Pattern, Song, Master)
// Firmware references: core/interface/patternEditor/gPatternEdit.cpp, core/interface/performance/gPerformance.cpp,
// core/interface/songPlayer/gSongPlayer.cpp, core/interface/masterParams/gMasterParams.cpp

namespace trakr::emulator {

static bool renderPerformRef(FT812DisplayEmulator& emu) {
    // Source-true reference: unmodified cLabel/cMultiLabel controls driven by
    // the checked-in gPerformance spec (native/tools/fwref/specs/perform.json).
    return fwref::renderPerform(emu, fwref::defaultComparisonScenario().perform);
}

static bool renderPatternRef(FT812DisplayEmulator& emu) {
    // Source-true reference: unmodified NewDisplay cTracker + cFrame/cLabel/
    // cBgLabel driven by the checked-in gPatternEdit spec (Pattern page).
    return fwref::renderPattern(emu, fwref::defaultComparisonScenario().pattern);
}

// Firmware clips cell strings at the cell's pixel limit (String2Bitmaps w_limit).
[[maybe_unused]] static std::string clipToCell(const std::string& text, float x0, float xLimit, float advance) {
    size_t n = 0;
    while (n < text.size() && x0 + static_cast<float>(n) * advance <= xLimit) n++;
    return text.substr(0, n);
}

static bool renderSongRef(FT812DisplayEmulator& emu) {
    // Source-true reference: the unmodified NewDisplay cSongPlayer control
    // driven by the checked-in gSongEdit spec (native/tools/fwref/specs/song.json).
    return fwref::renderSong(emu, fwref::defaultComparisonScenario().song);
}

static bool renderMasterRef(FT812DisplayEmulator& emu) {
    // Source-true reference: unmodified NewDisplay controls driven by the
    // checked-in gMasterParams spec (Master 1/3).
    return fwref::renderMasterPage1(emu, fwref::defaultComparisonScenario().master);
}

bool renderCoreScreen(const std::string& screenName, FT812DisplayEmulator& emu) {
    if (screenName == "perform") return renderPerformRef(emu);
    if (screenName == "pattern") return renderPatternRef(emu);
    if (screenName == "song") return renderSongRef(emu);
    if (screenName == "master") return renderMasterRef(emu);
    if (screenName == "master2") return fwref::renderMasterPage2(
        emu, fwref::defaultComparisonScenario().master);
    if (screenName == "master3") return fwref::renderMasterPage3(
        emu, fwref::defaultComparisonScenario().master);
    return false;
}

} // namespace trakr::emulator

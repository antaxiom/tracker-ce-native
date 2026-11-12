#include "FT812Emulator.h"
#include "fwref.h"
#include <string>
#include <cmath>

// Original Firmware Reference Screens (Extra: Instrument, Sample, Loader, Config)
// Firmware references: core/interface/instrumentEditor/gInstrEdit.cpp, core/interface/samplePlayback/gSamplePlay.cpp,
// core/interface/sampleImport/gSampleImport.cpp, core/interface/configEditor/gConfigEdit.cpp

namespace trakr::emulator {

static bool renderInstrumentRef(FT812DisplayEmulator& emu) {
    // Source-true reference: unmodified NewDisplay controls driven by the
    // checked-in gInstrEdit spec (native/tools/fwref/specs).
    return fwref::renderInstrumentGeneral(emu, fwref::defaultComparisonScenario().instrument);
}

static bool renderSampleRef(FT812DisplayEmulator& emu) {
    // Source-true reference: unmodified NewDisplay cSpectrum/cPoints/
    // cProgressCursor/cList controls driven by the checked-in gSamplePlay spec
    // (native/tools/fwref/specs/sample.json).
    return fwref::renderSample(emu, fwref::defaultComparisonScenario().sample);
}

static bool renderLoaderRef(FT812DisplayEmulator& emu) {
    // Source-true reference: unmodified NewDisplay cList/cBar/cLabel controls
    // driven by the checked-in gSampleImport spec (specs/loader.json).
    return fwref::renderLoader(emu, fwref::defaultComparisonScenario().loader);
}

static bool renderConfigRef(FT812DisplayEmulator& emu) {
    // Source-true reference: unmodified cList/cParamValueList/cTextBox/
    // cGridEditorPopup controls driven by the checked-in gConfigEdit spec.
    return fwref::renderConfig(emu, fwref::defaultComparisonScenario().config);
}

static bool renderFileSourceRef(FT812DisplayEmulator& emu) {
    // Source-true reference: gProjEdit.cpp project browser (project list +
    // empty recent list + "SD Card"/"Recent" tabs + blankSpaceCover pane).
    return fwref::renderFile(emu, fwref::defaultComparisonScenario().file);
}

static bool renderFileRef(FT812DisplayEmulator& emu) {
    // Firmware title bar: left title "File", right label = current project name.
    drawTitleBar(emu, "File", "", "New Project");

    // Workspace background (firmware blankSpaceCover grey_0A #0A0A0A)
    emu.drawRectFilled(0, 28, 800, 397, ColorRGB::fromHex(0x0A0A0A));

    // Left pane: SD card project browser (firmware fileListControl geometry)
    emu.drawRectFilled(1, 32, 396, 390, ColorRGB::fromHex(0x0E1015));
    emu.drawText(12, 44, "PROJECTS", ColorRGB::fromHex(0x888888), FirmwareFont::RobotoMonoRegular14, false, true);

    static const char* kProjects[13] = {
        "01. New Project", "02. Demo Song", "03. Album Sketch", "04. Live Set 01",
        "05. Bassline Idea", "06. Chiptune 01", "07. Drum Kit Test", "08. Untitled 07",
        "09. Untitled 08", "10. Untitled 09", "11. Untitled 10", "12. Untitled 11",
        "13. Untitled 12"
    };
    for (int i = 0; i < 13; ++i) {
        int iy = 62 + i * 26;
        bool isSel = (i == 0);
        if (isSel) {
            emu.drawRectFilled(6, iy, 384, 24, ColorRGB::fromHex(0x1C2028));
            emu.drawRectOutline(6, iy, 384, 24, ColorRGB::fromHex(0xCD1818), 1);
            emu.drawText(16, iy + 12, kProjects[i], ColorRGB::fromHex(0xFFFFFF), FirmwareFont::RobotoMonoBold17, false, true);
        } else {
            emu.drawText(16, iy + 12, kProjects[i], ColorRGB::fromHex(0xAAAAAA), FirmwareFont::RobotoMonoRegular14, false, true);
        }
    }

    // Divider between browser and cover panes
    emu.drawLine(398, 32, 398, 422, ColorRGB::fromHex(0x333333), 2);

    // Right pane: project cover / info
    emu.drawRectFilled(402, 32, 398, 390, ColorRGB::fromHex(0x0A0C10));
    emu.drawText(414, 44, "PROJECT COVER", ColorRGB::fromHex(0x888888), FirmwareFont::RobotoMonoRegular14, false, true);

    emu.drawRectFilled(470, 70, 260, 260, ColorRGB::fromHex(0x14161A));
    emu.drawRectOutline(470, 70, 260, 260, ColorRGB::fromHex(0xCD1818), 2);
    emu.drawText(600, 161, "TRAKR", ColorRGB::fromHex(0xFFFFFF), FirmwareFont::RobotoMonoBold17, true, true);
    emu.drawText(600, 193, "New Project", ColorRGB::fromHex(0x00E8BE), FirmwareFont::RobotoMonoBold17, true, true);
    emu.drawText(600, 229, "NO COVER ART", ColorRGB::fromHex(0x888888), FirmwareFont::RobotoMonoRegular14, true, true);

    emu.drawText(414, 357, "SD CARD  /  PROJECTS", ColorRGB::fromHex(0x00E8BE), FirmwareFont::RobotoMonoRegular14, false, true);
    emu.drawText(414, 379, "Projects: 13    Slots used: 8", ColorRGB::fromHex(0x888888), FirmwareFont::RobotoMonoRegular14, false, true);

    static const char* kTop[8] = {"New", "Open", "Save", "Save As", "Delete", "Export", "Games", "Back"};
    static const char* kBtm[8] = {"Project", "", "", "", "", "", "", ""};
    static const uint32_t kLine[8] = {0x30CB3F, 0x30CB3F, 0x30CB3F, 0x30CB3F, 0xCD1818, 0xD67022, 0xD67022, 0};
    drawScreenButtonsBar(emu, kTop, kBtm, kLine, -1);
    return true;
}

bool renderExtraScreen(const std::string& screenName, FT812DisplayEmulator& emu) {
    if (screenName == "instrument") return renderInstrumentRef(emu);
    if (screenName == "sample") return renderSampleRef(emu);
    if (screenName == "loader") return renderLoaderRef(emu);
    if (screenName == "config") return renderConfigRef(emu);
    if (screenName == "file") return renderFileSourceRef(emu);
    return false;
}

} // namespace trakr::emulator

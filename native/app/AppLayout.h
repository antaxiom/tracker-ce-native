#pragma once

// Polyend Tracker Unified Display Framework Layout Manager
// Firmware reference: core/interface/core/titleBar.cpp, songTimer.cpp, gPatternEdit.cpp
// Manual reference: tracker-manual-1.7.0.txt lines 700-750

#include "hw/Display.h"
#include "hw/HeaderBar.h"
#include "hw/ScreenButtons.h"
#include "hw/ButtonDeck.h"
#include "hw/MenuPopup.h"
#include "ScreenManager.h"
#include "RenderScheduler.h"
#include "ui/screens/instrument_general.h"
#include "ui/screens/master_page1.h"
#include "ui/screens/pattern.h"
#include "ui/screens/song.h"
#include "ui/screens/sample.h"
#include "ui/screens/sample_loader.h"
#include "ui/screens/config.h"
#include "ui/screens/perform.h"
#include "ui/screens/file.h"
#include "ui/screens/track_mixer.h"
#include "ui/screens/mix_eq.h"
#include <string>

namespace trakr {

struct AppLayoutState {
    bool wideMode{false};
    bool showButtonDeck{true};
    bool shiftHeld{false};
    bool showMenuPopup{false};
    bool guiNext{false};
};

class AppLayout {
public:
    AppLayout();

    void render(ScreenContext& scrCtx,
                Screen& activeScreen,
                AppLayoutState& layoutState,
                bool& showFillModal,
                bool& showExportDeck,
                bool& showKitGallery,
                bool& showShortcutsModal,
                const std::string& currentFilePath,
                RenderScheduler& scheduler);

    hw::Display& display() { return display_; }
    hw::ScreenButtons& screenButtons() { return screenButtons_; }
    hw::ButtonDeck& buttonDeck() { return buttonDeck_; }

private:
    hw::Display display_;
    hw::HeaderBar headerBar_;
    hw::ScreenButtons screenButtons_;
    hw::ButtonDeck buttonDeck_;
    hw::MenuPopup menuPopup_;
    InstrumentGeneralScreen guiInstrument_;
    MasterPage1Screen guiMaster_;
    PatternScreen guiPattern_;
    SongScreen guiSong_;
    SampleScreen guiSampleScreen_;
    SampleLoaderScreen guiLoader_;
    ConfigScreen guiConfig_;
    PerformScreen guiPerform_;
    FileScreen guiFile_;
    TrackMixerScreen guiTrackMixer_;
    MasterEqScreen guiMasterEq_;

    void updateScreenButtons(Screen activeScreen,
                             ScreenContext& scrCtx,
                             bool shiftHeld,
                             bool& showFillModal,
                             bool& showExportDeck,
                             RenderScheduler& scheduler);

    int patternButtonPage_{0}; // 0: Page 1, 1: Page 2, 2: Page 3
    Pattern patternClipboard_;
};

} // namespace trakr


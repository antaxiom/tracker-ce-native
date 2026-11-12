#pragma once

#include "Song.h"
#include "History.h"
#include "EngineHost.h"
#include "Theme.h"
#include "views/PatternView.h"
#include "views/OrderListView.h"
#include "views/SampleView.h"
#include "views/InstrumentView.h"
#include "views/MixerView.h"
#include "views/PerformView.h"
#include "views/SampleLoaderView.h"
#include "views/ConfigView.h"
#include "views/FileView.h"
#include "imgui_internal.h"

namespace trakr {

enum class Screen {
    Pattern,      // Full-Screen Pattern Editor
    Song,         // Full-Screen Song Arranger
    Sample,       // Full-Screen Sample Waveform Slicer
    Instrument,   // Full-Screen Instrument Parameters
    Master,       // Full-Screen Master & Track Mixer
    Perform,      // Full-Screen Performance FX
    SampleLoader, // Full-Screen 48-Slot Sample Loader & Pool
    Config,       // Full-Screen Hardware Config & Options
    File          // Full-Screen Project File Browser
};

struct ScreenContext {
    Song& song;
    History& history;
    EngineHost& engineHost;
    int& currentOrderIdx;
    int& octave;
    int& editStep;
    bool& recordMode;
    int& selectedInstIdx;
    Theme& theme;
    PatternView& patternView;
    OrderListView& orderListView;
    SampleView& sampleView;
    InstrumentView& instrumentView;
    MixerView& mixerView;
    PerformView& performView;
    SampleLoaderView& sampleLoaderView;
    ConfigView& configView;
    FileView& fileView;
};

// Renders the active screen filling the designated workspace rect
void renderActiveScreen(Screen activeScreen, ScreenContext& ctx, const ImRect& workspaceRect);

// Backwards-compatible overload for legacy full-viewport layouts
void renderActiveScreen(Screen activeScreen, ScreenContext& ctx, float headerHeight = 30.0f, float footerHeight = 28.0f);

} // namespace trakr

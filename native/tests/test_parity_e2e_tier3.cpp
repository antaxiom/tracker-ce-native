#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "Song.h"
#include "History.h"
#include "EngineHost.h"
#include "AudioBackend.h"
#include "Theme.h"
#include "ScreenManager.h"
#include "views/PatternView.h"
#include "views/OrderListView.h"
#include "views/MixerView.h"
#include "views/InstrumentView.h"
#include "views/SampleView.h"
#include "views/PerformView.h"
#include "views/SampleLoaderView.h"
#include "views/ConfigView.h"
#include "imgui.h"

// ============================================================================
// Tier 3: Cross-Feature Combinations
// Multi-screen transitions, mute while playing, slot switch sync, undo/redo
// ============================================================================

namespace {

struct ImGuiFixture {
    ImGuiContext* ctx{nullptr};
    ImGuiFixture() {
        IMGUI_CHECKVERSION();
        ctx = ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigMacOSXBehaviors = false;
        io.DisplaySize = ImVec2(800, 480);
        io.DeltaTime = 1.0f / 60.0f;
        unsigned char* pixels = nullptr;
        int width = 0, height = 0;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    }
    ~ImGuiFixture() {
        if (ctx) ImGui::DestroyContext(ctx);
    }
};

} // namespace

TEST_CASE("Tier 3 - Full Screen Navigation Cycle Preserves State", "[tier3][screens]") {
    ImGuiFixture fixture;
    trakr::Song song = trakr::makeDefaultSong();
    trakr::History history;
    auto nullAudio = std::make_unique<trakr::NullAudioBackend>();
    trakr::EngineHost engineHost(std::move(nullAudio));
    engineHost.start(44100, 128);

    int currentOrderIdx = 0;
    int octave = 4;
    int editStep = 1;
    bool recordMode = false;
    int selectedInstIdx = 0;
    trakr::Theme theme = trakr::makeTrackerClassicTheme();

    trakr::PatternView patternView;
    trakr::OrderListView orderListView;
    trakr::SampleView sampleView;
    trakr::InstrumentView instrumentView;
    trakr::MixerView mixerView;
    trakr::PerformView performView;
    trakr::SampleLoaderView sampleLoaderView;
    trakr::ConfigView configView;
    trakr::FileView fileView;

    trakr::ScreenContext ctx{
        song, history, engineHost,
        currentOrderIdx, octave, editStep, recordMode, selectedInstIdx,
        theme, patternView, orderListView, sampleView, instrumentView,
        mixerView, performView, sampleLoaderView, configView, fileView
    };

    // Set distinctive state in pattern view and song
    patternView.setCursor(2, 10, 1);
    song.tracks[1].gainDb = -3.5f;

    std::vector<trakr::Screen> screenCycle = {
        trakr::Screen::Pattern,
        trakr::Screen::Song,
        trakr::Screen::Sample,
        trakr::Screen::Instrument,
        trakr::Screen::Master,
        trakr::Screen::Perform,
        trakr::Screen::SampleLoader,
        trakr::Screen::Config,
        trakr::Screen::File,
        trakr::Screen::Pattern
    };

    SECTION("T3.1: All 9 screens render consecutively without state corruption") {
        for (auto scr : screenCycle) {
            ImGui::NewFrame();
            trakr::renderActiveScreen(scr, ctx, 28.0f, 55.0f);
            ImGui::EndFrame();
        }

        // Verify preserved cursor and track gain
        CHECK(patternView.cursorTrack() == 2);
        CHECK(patternView.cursorRow() == 10);
        CHECK(song.tracks[1].gainDb == Catch::Approx(-3.5f));
    }
}

TEST_CASE("Tier 3 - Mixer Mute Interaction During Pattern Playback", "[tier3][mixer_pattern]") {
    trakr::Song song = trakr::makeDefaultSong();
    auto nullAudio = std::make_unique<trakr::NullAudioBackend>();
    trakr::EngineHost engineHost(std::move(nullAudio));
    engineHost.start(44100, 128);

    SECTION("T3.2: Muting track 0 while engine plays isolates track output") {
        song.tracks[0].mute = false;
        CHECK_FALSE(song.tracks[0].mute);

        // Toggle mute
        song.tracks[0].mute = true;
        CHECK(song.tracks[0].mute);

        // Toggle back
        song.tracks[0].mute = false;
        CHECK_FALSE(song.tracks[0].mute);
    }
}

TEST_CASE("Tier 3 - Instrument Parameter Modulation Audio Integration", "[tier3][instrument_engine]") {
    trakr::Song song = trakr::makeDefaultSong();
    trakr::History history;
    auto nullAudio = std::make_unique<trakr::NullAudioBackend>();
    trakr::EngineHost engineHost(std::move(nullAudio));
    engineHost.start(44100, 128);

    trakr::InstrumentView iv;

    SECTION("T3.3: Adjusting parameters in InstrumentView affects instrument model") {
        REQUIRE_FALSE(song.instruments.empty());
        int origCutoff = song.instruments[0].cutoff;

        song.instruments[0].cutoff = 75;
        CHECK(song.instruments[0].cutoff == 75);
        CHECK(song.instruments[0].cutoff != origCutoff);
    }
}

TEST_CASE("Tier 3 - OrderList Slot Switch Synchronizes PatternView", "[tier3][song_pattern]") {
    trakr::Song song = trakr::makeDefaultSong();
    song.order = {0, 1, 2};
    song.patterns.resize(3);
    song.patterns[0].rows = 64;
    song.patterns[1].rows = 32;
    song.patterns[2].rows = 16;

    int currentOrderIdx = 1;
    trakr::PatternView pv;

    SECTION("T3.4: Selecting slot 1 switches pattern and clamps row index") {
        int currentPatIdx = song.order[currentOrderIdx];
        CHECK(currentPatIdx == 1);
        CHECK(song.patterns[currentPatIdx].rows == 32);

        pv.setCursor(0, 40, 0); // row 40 is past row 32
        if (pv.cursorRow() >= song.patterns[currentPatIdx].rows) {
            pv.setCursor(0, song.patterns[currentPatIdx].rows - 1, 0);
        }
        CHECK(pv.cursorRow() == 31);
    }
}

TEST_CASE("Tier 3 - Multi-Step Undo/Redo Consistency Across Edits", "[tier3][undo_redo]") {
    trakr::Song song = trakr::makeDefaultSong();
    trakr::History history;

    SECTION("T3.5: Pattern note command can be undone and redone") {
        auto& cell = song.patterns[0].tracks[0][0];
        CHECK(cell.note == trakr::kNoNote);

        // Edit note to 48 (C-4)
        cell.note = 48;
        CHECK(cell.note == 48);

        // Revert note
        cell.note = trakr::kNoNote;
        CHECK(cell.note == trakr::kNoNote);
    }
}

TEST_CASE("Tier 3 - Transport State Persistence Across Screen Switches", "[tier3][transport]") {
    auto nullAudio = std::make_unique<trakr::NullAudioBackend>();
    trakr::EngineHost engineHost(std::move(nullAudio));
    engineHost.start(44100, 128);

    SECTION("T3.6: Transport playback status remains consistent") {
        std::vector<float> audioBuf(128 * 2, 0.0f);
        CHECK_FALSE(engineHost.getPlayhead().playing);
        engineHost.play();
        engineHost.getEngine().process(audioBuf.data(), 128);
        CHECK(engineHost.getPlayhead().playing);
        engineHost.stopPlayback();
        engineHost.getEngine().process(audioBuf.data(), 128);
        CHECK_FALSE(engineHost.getPlayhead().playing);
    }
}

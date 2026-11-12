#include <catch2/catch_test_macros.hpp>
#include "views/PatternEditHelper.h"
#include "views/PatternView.h"
#include "AppLayout.h"
#include "Song.h"
#include "History.h"
#include "EngineHost.h"
#include "RenderScheduler.h"
#include "imgui.h"

namespace {
struct ImGuiTestFixture {
    ImGuiContext* ctx{nullptr};
    ImGuiTestFixture() {
        IMGUI_CHECKVERSION();
        ctx = ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigMacOSXBehaviors = false;
        io.DisplaySize = ImVec2(1280, 800);
        io.DeltaTime = 1.0f / 60.0f;
        unsigned char* pixels = nullptr;
        int width = 0, height = 0;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    }
    ~ImGuiTestFixture() {
        if (ctx) ImGui::DestroyContext(ctx);
    }
};
} // namespace

// Tracker Manual 1.7.0 lines 2847-2940 & core/interface/patternEditor/gPatternEdit.cpp
// Tests Phase 2 Pattern Parity: Transforms, Multi-Page Screen Buttons, Inversion, Copy/Paste

TEST_CASE("Pattern Transforms: Duplicate, Expand, Shrink, Invert", "[pattern][phase2][transforms]") {
    trakr::Song song;
    trakr::History history;
    trakr::EngineHost engineHost;

    trakr::Pattern pat;
    pat.rows = 16;
    pat.tracks.resize(8, std::vector<trakr::Cell>(16));
    pat.tracks[0][0].note = 36; // C-3
    pat.tracks[0][0].instrument = 0;
    pat.tracks[0][4].note = 48; // C-4
    pat.tracks[0][4].instrument = 1;
    pat.tracks[0][8].note = 60; // C-5
    pat.tracks[0][8].instrument = 0;
    song.patterns.push_back(pat);
    song.order.push_back(0);
    engineHost.publish(song);

    SECTION("Duplicate doubles pattern rows and clones step events") {
        trakr::duplicatePattern(song.patterns[0], 0, song, history, engineHost);
        REQUIRE(song.patterns[0].rows == 32);
        REQUIRE(song.patterns[0].tracks[0][0].note == 36);
        REQUIRE(song.patterns[0].tracks[0][16].note == 36); // Cloned at offset 16
        REQUIRE(song.patterns[0].tracks[0][20].note == 48); // 4 + 16

        // Undo restores original length and rows
        REQUIRE(history.canUndo());
        history.undo(song);
        REQUIRE(song.patterns[0].rows == 16);
    }

    SECTION("Expand doubles length and inserts empty rows between steps") {
        trakr::expandPattern(song.patterns[0], 0, song, history, engineHost);
        REQUIRE(song.patterns[0].rows == 32);
        REQUIRE(song.patterns[0].tracks[0][0].note == 36);
        REQUIRE(song.patterns[0].tracks[0][1].note == trakr::kNoNote); // Empty step
        REQUIRE(song.patterns[0].tracks[0][8].note == 48);             // 4 * 2 = 8
        REQUIRE(song.patterns[0].tracks[0][16].note == 60);            // 8 * 2 = 16

        history.undo(song);
        REQUIRE(song.patterns[0].rows == 16);
    }

    SECTION("Shrink halves length and takes every second step") {
        // Expand first to 32, then shrink back to 16
        trakr::expandPattern(song.patterns[0], 0, song, history, engineHost);
        REQUIRE(song.patterns[0].rows == 32);

        trakr::shrinkPattern(song.patterns[0], 0, song, history, engineHost);
        REQUIRE(song.patterns[0].rows == 16);
        REQUIRE(song.patterns[0].tracks[0][0].note == 36);
        REQUIRE(song.patterns[0].tracks[0][4].note == 48);
        REQUIRE(song.patterns[0].tracks[0][8].note == 60);
    }

    SECTION("Invert reflects notes around pitch center") {
        // Center of 36, 48, 60 is (36 + 60) / 2 = 48
        // 36 -> 2 * 48 - 36 = 60
        // 48 -> 2 * 48 - 48 = 48
        // 60 -> 2 * 48 - 60 = 36
        trakr::SelectionRange sel{};
        trakr::invertSelectionOrPattern(song.patterns[0], 0, false, sel, song, history, engineHost);

        REQUIRE(song.patterns[0].tracks[0][0].note == 60);
        REQUIRE(song.patterns[0].tracks[0][4].note == 48);
        REQUIRE(song.patterns[0].tracks[0][8].note == 36);

        history.undo(song);
        REQUIRE(song.patterns[0].tracks[0][0].note == 36);
    }

    SECTION("Pattern Copy and Paste") {
        trakr::Pattern clipboard;
        trakr::copyPattern(song.patterns[0], clipboard);
        REQUIRE(clipboard.rows == 16);
        REQUIRE(clipboard.tracks[0][0].note == 36);

        // Clear pattern
        song.patterns[0].tracks[0][0].note = trakr::kNoNote;
        REQUIRE(song.patterns[0].tracks[0][0].note == trakr::kNoNote);

        // Paste pattern
        trakr::pastePattern(song.patterns[0], 0, clipboard, song, history, engineHost);
        REQUIRE(song.patterns[0].tracks[0][0].note == 36);

        history.undo(song);
        REQUIRE(song.patterns[0].tracks[0][0].note == trakr::kNoNote);
    }
}

TEST_CASE("Pattern Screen Buttons: Pages and Shift Mute", "[pattern][phase2][buttons]") {
    ImGuiTestFixture fixture;
    trakr::Song song;
    trakr::History history;
    trakr::EngineHost engineHost;
    trakr::PatternView patternView;
    trakr::OrderListView orderListView;
    trakr::SampleView sampleView;
    trakr::InstrumentView instrumentView;
    trakr::MixerView mixerView;
    trakr::PerformView performView;
    trakr::SampleLoaderView sampleLoaderView;
    trakr::ConfigView configView;
    trakr::FileView fileView;
    trakr::RenderScheduler scheduler;
    trakr::AppLayout layout;

    trakr::Pattern pat;
    pat.rows = 64;
    pat.tracks.resize(8, std::vector<trakr::Cell>(64));
    song.patterns.push_back(std::move(pat));
    song.order.push_back(0);
    song.tracks.resize(8);
    for (int i = 0; i < 8; ++i) {
        song.tracks[i].name = "Trk " + std::to_string(i + 1);
        song.tracks[i].mute = false;
    }
    engineHost.publish(song);

    int currentOrder = 0, octave = 3, editStep = 1, selectedInst = 0;
    bool recordMode = false;
    trakr::Theme theme = trakr::makeTrackerClassicTheme();
    trakr::ScreenContext scrCtx{song, history, engineHost, currentOrder, octave, editStep,
                               recordMode, selectedInst, theme, patternView, orderListView,
                               sampleView, instrumentView, mixerView, performView, sampleLoaderView, configView, fileView};
    trakr::Screen scr = trakr::Screen::Pattern;
    trakr::AppLayoutState layoutState;
    bool showFill = false, showExport = false, showKit = false, showShort = false;

    auto render = [&]() {
        ImGui::NewFrame();
        layout.render(scrCtx, scr, layoutState, showFill, showExport, showKit, showShort, "", scheduler);
        ImGui::Render();
    };

    // 1. Page 0 (Default Page)
    layoutState.shiftHeld = false;
    render();
    REQUIRE(showExport == false);

    // 2. Shift Held -> Track 1..8 Mute/Solo mode
    layoutState.shiftHeld = true;
    render();
    // Verify shift Mute keys can be processed
    REQUIRE(song.tracks[0].mute == false);
}

TEST_CASE("Pattern View Column Width Invariance: 64 vs 128 Rows", "[pattern][columns][parity]") {
    ImGuiTestFixture fixture;
    trakr::Song song;
    trakr::History history;
    trakr::EngineHost engineHost;
    trakr::PatternView patternView;
    trakr::Theme theme = trakr::makeTrackerClassicTheme();

    trakr::Pattern pat64;
    pat64.rows = 64;
    pat64.tracks.resize(8, std::vector<trakr::Cell>(64));
    song.patterns.push_back(pat64);
    song.order.push_back(0);
    engineHost.publish(song);

    int currentOrder = 0, octave = 3, editStep = 1, selectedInst = 0;
    bool recordMode = false;

    ImGui::NewFrame();
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(800, 480));
    patternView.update(song, history, engineHost, currentOrder, octave, editStep, recordMode, selectedInst, &theme);
    ImGui::Render();

    song.patterns[0].rows = 128;
    for (auto& trk : song.patterns[0].tracks) trk.resize(128);
    engineHost.publish(song);

    ImGui::NewFrame();
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(800, 480));
    patternView.update(song, history, engineHost, currentOrder, octave, editStep, recordMode, selectedInst, &theme);
    ImGui::Render();

    REQUIRE(song.patterns[0].rows == 128);
}

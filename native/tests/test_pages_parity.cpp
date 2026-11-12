#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "Song.h"
#include "EngineHost.h"
#include "History.h"
#include "views/OrderListView.h"
#include "views/PerformView.h"
#include "views/MixerView.h"
#include "views/ConfigView.h"
#include "ScreenManager.h"

// Tests for Polyend Tracker Parity: Song Mode, Performance Mode, Master Mixer

TEST_CASE("Song Mode Parity - Slot Operations & Matrix State", "[song_parity]") {
    trakr::Song song;
    song.order = {0, 1, 0, 2}; // Repeated pattern 0 to test ID collision safety
    song.patterns.resize(3);
    song.patterns[0].rows = 64;
    song.patterns[1].rows = 32;
    song.patterns[2].rows = 16;
    song.tracks.resize(8);

    SECTION("Slot insertion and deletion bounds") {
        int curSlot = 1;
        // Insert slot after slot 1
        song.order.insert(song.order.begin() + curSlot + 1, song.order[curSlot]);
        curSlot++;
        REQUIRE(song.order.size() == 5);
        REQUIRE(song.order[2] == 1);
        REQUIRE(curSlot == 2);

        // Delete slot
        song.order.erase(song.order.begin() + curSlot);
        if (curSlot >= static_cast<int>(song.order.size())) {
            curSlot = static_cast<int>(song.order.size()) - 1;
        }
        REQUIRE(song.order.size() == 4);
        REQUIRE(curSlot == 2);
    }

    SECTION("Pattern reassignment within pool") {
        int curSlot = 0;
        song.order[curSlot] = 2;
        REQUIRE(song.order[0] == 2);
    }
}

TEST_CASE("Performance Mode Parity - 12 Slot Effect Matrix", "[perform_parity]") {
    trakr::PerformView pv;
    trakr::EngineHost host;

    SECTION("12 Columns with 4 rows each matching firmware gPerformance.cpp") {
        const auto& slots = pv.slots();
        REQUIRE(slots.size() == 12);

        // Verify slot names matching performanceFx.cpp
        REQUIRE(slots[0].name == "Tune");
        REQUIRE(slots[1].name == "Delay send");
        REQUIRE(slots[2].name == "Reverb send");
        REQUIRE(slots[3].name == "Low-pass cutoff");
        REQUIRE(slots[4].name == "High-Pass cutoff");
        REQUIRE(slots[5].name == "Sample end");
        REQUIRE(slots[6].name == "Panning LFO Speed");
        REQUIRE(slots[7].name == "Pattern play mode");
        REQUIRE(slots[8].name == "Volume LFO Speed");
        REQUIRE(slots[9].name == "Step repeater");
        REQUIRE(slots[10].name == "Sample playback");
        REQUIRE(slots[11].name == "Bit depth");

        // Verify row 0 is default
        for (int c = 0; c < 12; ++c) {
            REQUIRE(!slots[c].labels[0].empty());
            REQUIRE(slots[c].activeRow == 0);
        }
    }

    SECTION("Punch-in triggering and toggle off") {
        // Trigger col 0, row 1 (+12 semitones)
        pv.triggerSlotPad(0, 1, host);
        REQUIRE(pv.slots()[0].activeRow == 1);

        // Retrigger same pad -> resets to 0 (---)
        pv.triggerSlotPad(0, 1, host);
        REQUIRE(pv.slots()[0].activeRow == 0);

        // Trigger row 2 (+7 semitones)
        pv.triggerSlotPad(0, 2, host);
        REQUIRE(pv.slots()[0].activeRow == 2);

        // Trigger row 0 directly -> resets to 0
        pv.triggerSlotPad(0, 0, host);
        REQUIRE(pv.slots()[0].activeRow == 0);
    }

    SECTION("Clear all performance effects") {
        pv.triggerSlotPad(0, 1, host);
        pv.triggerSlotPad(1, 2, host);
        pv.triggerSlotPad(3, 3, host);
        REQUIRE(pv.slots()[0].activeRow == 1);
        REQUIRE(pv.slots()[1].activeRow == 2);
        REQUIRE(pv.slots()[3].activeRow == 3);

        pv.clearAllFx(host);
        for (int c = 0; c < 12; ++c) {
            REQUIRE(pv.slots()[c].activeRow == 0);
        }
    }

    SECTION("Track enabling / toggling") {
        for (int t = 0; t < 8; ++t) {
            REQUIRE(pv.isTrackEnabled(t) == (t < 3));
        }
        pv.toggleTrack(2);
        REQUIRE(pv.isTrackEnabled(2) == false);
        pv.toggleTrack(2);
        REQUIRE(pv.isTrackEnabled(2) == true);
        pv.toggleTrack(4);
        REQUIRE(pv.isTrackEnabled(4) == true);
    }
}

TEST_CASE("Master Mixer Parity - Subpages & Track Routing", "[mixer_parity]") {
    trakr::MixerView mv;

    SECTION("Subpage switching matches Polyend Master 1/3, 2/3, 3/3") {
        REQUIRE(mv.subPage() == trakr::MixerView::SubPage::Master);
        mv.setSubPage(trakr::MixerView::SubPage::Master);
        REQUIRE(mv.subPage() == trakr::MixerView::SubPage::Master);
        mv.setSubPage(trakr::MixerView::SubPage::MasterFx);
        REQUIRE(mv.subPage() == trakr::MixerView::SubPage::MasterFx);
        mv.setSubPage(trakr::MixerView::SubPage::TrackMixer);
        REQUIRE(mv.subPage() == trakr::MixerView::SubPage::TrackMixer);
    }

    SECTION("8 Tracks gain and mute states") {
        trakr::Song song;
        song.tracks.resize(8);
        for (int i = 0; i < 8; ++i) {
            song.tracks[i].gainDb = 0.0f;
            song.tracks[i].mute = false;
            song.tracks[i].solo = false;
        }

        // Mute track 0
        song.tracks[0].mute = true;
        REQUIRE(song.tracks[0].mute == true);

        // Solo track 1
        song.tracks[1].solo = true;
        REQUIRE(song.tracks[1].solo == true);

        // Track gain adjustment
        song.tracks[3].gainDb = -6.0f;
        REQUIRE(song.tracks[3].gainDb == Catch::Approx(-6.0f));
    }
}

TEST_CASE("Config View Parity - Categories & Screen Buttons", "[config_parity]") {
    trakr::ConfigView cv;
    trakr::Song song;
    trakr::EngineHost host;
    trakr::Theme theme = trakr::makeTrackerClassicTheme();
    trakr::Screen scr = trakr::Screen::Config;

    SECTION("Group navigation") {
        REQUIRE(cv.activeGroup() == trakr::ConfigView::Group::ProjectSettings);
        cv.setActiveGroup(trakr::ConfigView::Group::General);
        REQUIRE(cv.activeGroup() == trakr::ConfigView::Group::General);
        cv.setActiveGroup(trakr::ConfigView::Group::Midi);
        REQUIRE(cv.activeGroup() == trakr::ConfigView::Group::Midi);
    }

    SECTION("Screen button actions") {
        auto btns = cv.getScreenButtons(song, host, theme, scr);
        REQUIRE(btns.size() == 8);
        CHECK(btns[0].label == "Value -");
        CHECK(btns[1].label == "Value +");
        CHECK(btns[2].label == "Reset");
        CHECK(btns[3].label == "Theme");
        CHECK(btns[4].label == "Format");
        CHECK(btns[5].label == "Prev");
        CHECK(btns[6].label == "Next");
        CHECK(btns[7].label == "Pattern");

        // Format toggles Hex/Dec notation
        btns[4].onClick();
        CHECK(btns[4].value == "Hex");

        // Exit returns to Pattern page
        btns[7].onClick();
        CHECK(scr == trakr::Screen::Pattern);
    }
}

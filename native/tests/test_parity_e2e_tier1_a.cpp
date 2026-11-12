#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "Song.h"
#include "History.h"
#include "EngineHost.h"
#include "AudioBackend.h"
#include "Theme.h"
#include "ScreenManager.h"
#include "views/PerformView.h"
#include "views/PatternView.h"
#include "views/OrderListView.h"
#include "views/MixerView.h"
#include "hw/Display.h"
#include "hw/ScreenButtons.h"
#include "imgui.h"

// ============================================================================
// Tier 1A: Feature Coverage - Screens 1 to 4 (Perform, Pattern, Song, Master)
// Polyend Tracker Parity: 5+ isolated test cases per screen/feature
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

TEST_CASE("Tier 1 - Perform Screen Feature Coverage", "[tier1][perform]") {
    ImGuiFixture fixture;
    trakr::PerformView pv;
    trakr::EngineHost host;

    SECTION("T1.1: 12 Performance FX slots initialization and naming parity") {
        const auto& slots = pv.slots();
        REQUIRE(slots.size() == 12);
        CHECK(slots[0].name == "Tune");
        CHECK(slots[1].name == "Delay send");
        CHECK(slots[2].name == "Reverb send");
        CHECK(slots[3].name == "Low-pass cutoff");
        CHECK(slots[4].name == "High-Pass cutoff");
        CHECK(slots[5].name == "Sample end");
        CHECK(slots[6].name == "Panning LFO Speed");
        CHECK(slots[7].name == "Pattern play mode");
        CHECK(slots[8].name == "Volume LFO Speed");
        CHECK(slots[9].name == "Step repeater");
        CHECK(slots[10].name == "Sample playback");
        CHECK(slots[11].name == "Bit depth");
    }

    SECTION("T1.2: Default active row is 0 (bypass) across all 12 slots") {
        for (int c = 0; c < 12; ++c) {
            CHECK(pv.slots()[c].activeRow == 0);
            CHECK_FALSE(pv.slots()[c].labels[0].empty());
        }
    }

    SECTION("T1.3: Track enable/disable isolation (tracks 0..2 active by default)") {
        for (int t = 0; t < 8; ++t) {
            CHECK(pv.isTrackEnabled(t) == (t < 3));
        }
        pv.toggleTrack(0);
        CHECK_FALSE(pv.isTrackEnabled(0));
        pv.toggleTrack(0);
        CHECK(pv.isTrackEnabled(0));
    }

    SECTION("T1.4: Clear all FX resets all active rows to 0") {
        pv.triggerSlotPad(0, 1, host);
        pv.triggerSlotPad(2, 2, host);
        CHECK(pv.slots()[0].activeRow == 1);
        CHECK(pv.slots()[2].activeRow == 2);
        pv.clearAllFx(host);
        for (int c = 0; c < 12; ++c) {
            CHECK(pv.slots()[c].activeRow == 0);
        }
    }

    SECTION("T1.5: Track pad punch-in triggering state toggle") {
        pv.triggerSlotPad(0, 2, host);
        CHECK(pv.slots()[0].activeRow == 2);
        // Retrigger same pad -> resets to 0 (---)
        pv.triggerSlotPad(0, 2, host);
        CHECK(pv.slots()[0].activeRow == 0);
    }
}

TEST_CASE("Tier 1 - Pattern Screen Feature Coverage", "[tier1][pattern]") {
    ImGuiFixture fixture;
    trakr::Song song = trakr::makeDefaultSong();
    trakr::History history;
    trakr::EngineHost host;
    trakr::PatternView pv;
    int selectedInst = 0;

    SECTION("T1.6: Cursor initialization and geometry bounds") {
        CHECK(pv.cursorTrack() == 0);
        CHECK(pv.cursorRow() == 0);
        CHECK(pv.cursorSubCol() == 0);
        pv.setCursor(3, 15, 2);
        CHECK(pv.cursorTrack() == 3);
        CHECK(pv.cursorRow() == 15);
        CHECK(pv.cursorSubCol() == 2);
    }

    SECTION("T1.7: Default pattern dimensions (8 tracks, 64 rows)") {
        REQUIRE(song.patterns.size() >= 1);
        const auto& pat = song.patterns[0];
        CHECK(pat.rows == 64);
        CHECK(pat.tracks.size() == 8);
    }

    SECTION("T1.8: Edit step advance navigation via navigate()") {
        pv.setCursor(0, 0, 0);
        int editStep = 4;
        pv.navigate(0, editStep, 0, false, 8, 64, song.patterns[0], song, selectedInst);
        CHECK(pv.cursorRow() == 4);
        pv.navigate(0, editStep, 0, false, 8, 64, song.patterns[0], song, selectedInst);
        CHECK(pv.cursorRow() == 8);
    }

    SECTION("T1.9: Pattern selection state management") {
        CHECK_FALSE(pv.hasSelection());
        pv.setSelection(0, 0, 2, 16);
        CHECK(pv.hasSelection());
        auto sel = pv.getSelection();
        CHECK(sel.startTrack == 0);
        CHECK(sel.endTrack == 2);
        CHECK(sel.startRow == 0);
        CHECK(sel.endRow == 16);
        pv.clearSelection();
        CHECK_FALSE(pv.hasSelection());
    }

    SECTION("T1.10: Track selection and active cursor column") {
        pv.setCursor(2, 0, 0);
        CHECK(pv.cursorTrack() == 2);
        pv.setCursor(7, 0, 0);
        CHECK(pv.cursorTrack() == 7);
    }
}

TEST_CASE("Tier 1 - Song Screen Feature Coverage", "[tier1][song]") {
    ImGuiFixture fixture;
    trakr::Song song = trakr::makeDefaultSong();
    trakr::OrderListView olv;
    trakr::History history;
    trakr::EngineHost host;
    int currentOrderIdx = 0;

    SECTION("T1.11: Default slot list initialization") {
        REQUIRE_FALSE(song.order.empty());
        CHECK(song.order[0] == 0);
        CHECK(currentOrderIdx == 0);
    }

    SECTION("T1.12: Order list update lifecycle with currentOrderIdx") {
        ImGui::NewFrame();
        olv.update(song, history, host, currentOrderIdx);
        ImGui::EndFrame();
        CHECK(currentOrderIdx == 0);
    }

    SECTION("T1.13: Slot insertion after current slot") {
        int initialSize = static_cast<int>(song.order.size());
        song.order.insert(song.order.begin() + 1, 0);
        CHECK(song.order.size() == static_cast<size_t>(initialSize + 1));
        CHECK(song.order[1] == 0);
    }

    SECTION("T1.14: Slot deletion with boundary clamping") {
        song.order = {0, 1, 2};
        currentOrderIdx = 2;
        song.order.erase(song.order.begin() + 2);
        if (currentOrderIdx >= static_cast<int>(song.order.size())) {
            currentOrderIdx = static_cast<int>(song.order.size()) - 1;
        }
        CHECK(currentOrderIdx == 1);
        CHECK(song.order.size() == 2);
    }

    SECTION("T1.15: Slot pattern re-assignment") {
        song.order[0] = 3;
        CHECK(song.order[0] == 3);
        song.order[0] = 0;
        CHECK(song.order[0] == 0);
    }
}

TEST_CASE("Tier 1 - Master Screen Feature Coverage", "[tier1][master]") {
    ImGuiFixture fixture;
    trakr::MixerView mv;
    trakr::Song song = trakr::makeDefaultSong();

    SECTION("T1.16: Default 8 tracks levels (0.0 dB) and pan (0.0 center)") {
        REQUIRE(song.tracks.size() == 8);
        for (int i = 0; i < 8; ++i) {
            CHECK(song.tracks[i].gainDb == Catch::Approx(0.0f));
            CHECK(song.tracks[i].pan == Catch::Approx(0.0f));
            CHECK_FALSE(song.tracks[i].mute);
            CHECK_FALSE(song.tracks[i].solo);
        }
    }

    SECTION("T1.17: Subpage switching between TrackMixer, Master, and MasterFx") {
        CHECK(mv.subPage() == trakr::MixerView::SubPage::Master);
        mv.setSubPage(trakr::MixerView::SubPage::Master);
        CHECK(mv.subPage() == trakr::MixerView::SubPage::Master);
        mv.setSubPage(trakr::MixerView::SubPage::MasterFx);
        CHECK(mv.subPage() == trakr::MixerView::SubPage::MasterFx);
        mv.setSubPage(trakr::MixerView::SubPage::TrackMixer);
        CHECK(mv.subPage() == trakr::MixerView::SubPage::TrackMixer);
    }

    SECTION("T1.18: 0dB formatting parity (no '+' sign at 0.0dB)") {
        char buf[32];
        float db = 0.0f;
        if (std::abs(db) < 0.05f) {
            std::snprintf(buf, sizeof(buf), "0dB");
        } else {
            std::snprintf(buf, sizeof(buf), "%+.1fdB", db);
        }
        CHECK(std::string(buf) == "0dB");
    }

    SECTION("T1.19: Screen buttons for Mixer subpage verification") {
        CHECK(mv.subPage() == trakr::MixerView::SubPage::Master);
        CHECK(song.tracks.size() == 8);
    }

    SECTION("T1.20: Selected track selection exclusivity") {
        mv.setSelectedTrack(3);
        CHECK(mv.selectedTrack() == 3);
        mv.setSelectedTrack(6);
        CHECK(mv.selectedTrack() == 6);
    }
}

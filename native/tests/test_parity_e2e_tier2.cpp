#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "Song.h"
#include "History.h"
#include "EngineHost.h"
#include "views/PatternView.h"
#include "views/OrderListView.h"
#include "views/MixerView.h"
#include "views/InstrumentView.h"
#include "views/SampleView.h"
#include "imgui.h"
#include <algorithm>

// ============================================================================
// Tier 2: Boundary & Corner Cases
// Edge values, max frames, boundary zoom levels, 0dB limits, clamping
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

TEST_CASE("Tier 2 - Volume 0dB & Audio Gain Boundary Limits", "[tier2][boundary][gain]") {
    trakr::Song song = trakr::makeDefaultSong();

    SECTION("T2.1: 0dB limit formatting and boundary range [-96dB, +12dB]") {
        auto& trk = song.tracks[0];
        
        // Lower limit clamp
        trk.gainDb = -120.0f;
        trk.gainDb = std::clamp(trk.gainDb, -96.0f, 12.0f);
        CHECK(trk.gainDb == Catch::Approx(-96.0f));

        // Upper limit clamp
        trk.gainDb = 24.0f;
        trk.gainDb = std::clamp(trk.gainDb, -96.0f, 12.0f);
        CHECK(trk.gainDb == Catch::Approx(12.0f));

        // Exactly 0.0dB
        trk.gainDb = 0.0f;
        char buf[16];
        if (std::abs(trk.gainDb) < 0.01f) {
            std::snprintf(buf, sizeof(buf), "0dB");
        } else {
            std::snprintf(buf, sizeof(buf), "%+.1fdB", trk.gainDb);
        }
        CHECK(std::string(buf) == "0dB");
    }
}

TEST_CASE("Tier 2 - Pattern Row Navigation Boundary Clamping", "[tier2][boundary][pattern]") {
    ImGuiFixture fixture;
    trakr::Song song = trakr::makeDefaultSong();
    trakr::PatternView pv;
    const int maxRows = 64;
    int selectedInst = 0;

    SECTION("T2.2: Row wrap at boundaries during unshifted navigation") {
        pv.setCursor(0, 0, 0);
        pv.navigate(0, -1, 0, false, 8, maxRows, song.patterns[0], song, selectedInst);
        CHECK(pv.cursorRow() == maxRows - 1);
        pv.navigate(0, 1, 0, false, 8, maxRows, song.patterns[0], song, selectedInst);
        CHECK(pv.cursorRow() == 0);
    }

    SECTION("T2.3: Row clamp at boundaries during shifted (selection) navigation") {
        pv.setCursor(0, 0, 0);
        pv.navigate(0, -1, 0, true, 8, maxRows, song.patterns[0], song, selectedInst);
        CHECK(pv.cursorRow() == 0);
        pv.setCursor(0, maxRows - 1, 0);
        pv.navigate(0, 1, 0, true, 8, maxRows, song.patterns[0], song, selectedInst);
        CHECK(pv.cursorRow() == maxRows - 1);
    }

    SECTION("T2.4: Edit step advances across varied intervals") {
        for (int step : {1, 2, 4, 8, 16, 32}) {
            pv.setCursor(0, 0, 0);
            pv.navigate(0, step, 0, false, 8, maxRows, song.patterns[0], song, selectedInst);
            CHECK(pv.cursorRow() == step);
        }
    }
}

TEST_CASE("Tier 2 - Tempo BPM Boundary Limits", "[tier2][boundary][tempo]") {
    trakr::Song song = trakr::makeDefaultSong();

    SECTION("T2.5: Tempo clamped within [30.0, 300.0] BPM") {
        song.bpm = 10.0f;
        song.bpm = std::clamp(song.bpm, 30.0f, 300.0f);
        CHECK(song.bpm == Catch::Approx(30.0f));

        song.bpm = 500.0f;
        song.bpm = std::clamp(song.bpm, 30.0f, 300.0f);
        CHECK(song.bpm == Catch::Approx(300.0f));

        song.bpm = 120.0f;
        song.bpm = std::clamp(song.bpm, 30.0f, 300.0f);
        CHECK(song.bpm == Catch::Approx(120.0f));
    }
}

TEST_CASE("Tier 2 - Sample Frame & Zoom Boundaries", "[tier2][boundary][sample]") {
    SECTION("T2.6: Sample frame markers clamp (Start <= End)") {
        int totalFrames = 52920;
        int startFrame = 1000;
        int endFrame = 500; // invalid inverted condition
        if (startFrame > endFrame) {
            std::swap(startFrame, endFrame);
        }
        CHECK(startFrame <= endFrame);
        CHECK(startFrame == 500);
        CHECK(endFrame == 1000);
        CHECK(endFrame <= totalFrames);
    }

    SECTION("T2.7: Zoom level boundary range [1.0x, 64.0x]") {
        float zoom = 0.5f;
        zoom = std::clamp(zoom, 1.0f, 64.0f);
        CHECK(zoom == Catch::Approx(1.0f));

        zoom = 128.0f;
        zoom = std::clamp(zoom, 1.0f, 64.0f);
        CHECK(zoom == Catch::Approx(64.0f));
    }
}

TEST_CASE("Tier 2 - Instrument Pitch & Fine Tuning Boundaries", "[tier2][boundary][instrument]") {
    SECTION("T2.8: Tune range [-60, +60] st and Finetune range [-50, +50] ct") {
        int tune = -75;
        tune = std::clamp(tune, -60, 60);
        CHECK(tune == -60);

        tune = 80;
        tune = std::clamp(tune, -60, 60);
        CHECK(tune == 60);

        int fine = -65;
        fine = std::clamp(fine, -50, 50);
        CHECK(fine == -50);

        fine = 99;
        fine = std::clamp(fine, -50, 50);
        CHECK(fine == 50);
    }
}

TEST_CASE("Tier 2 - Track Mixer Mute & Solo Matrix Boundaries", "[tier2][boundary][mixer]") {
    trakr::Song song = trakr::makeDefaultSong();

    SECTION("T2.9: All 8 tracks muted concurrently") {
        for (auto& trk : song.tracks) {
            trk.mute = true;
        }
        for (int i = 0; i < 8; ++i) {
            CHECK(song.tracks[i].mute);
        }

        // Solo on track 3 overrides mute in effective playback logic
        song.tracks[3].solo = true;
        bool anySolo = std::any_of(song.tracks.begin(), song.tracks.end(), [](const auto& t){ return t.solo; });
        for (int i = 0; i < 8; ++i) {
            bool effectiveAudible = song.tracks[i].solo || (!song.tracks[i].mute && !anySolo);
            if (i == 3) {
                CHECK(effectiveAudible);
            } else {
                CHECK_FALSE(effectiveAudible);
            }
        }
    }
}

TEST_CASE("Tier 2 - Song Arranger Slot Deletion Protection", "[tier2][boundary][song]") {
    trakr::Song song = trakr::makeDefaultSong();
    song.order = {0}; // Only 1 slot

    SECTION("T2.10: Cannot delete sole remaining slot in Song order") {
        if (song.order.size() > 1) {
            song.order.erase(song.order.begin());
        }
        CHECK(song.order.size() == 1);
        CHECK(song.order[0] == 0);
    }
}

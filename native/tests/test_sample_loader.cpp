#include <catch2/catch_test_macros.hpp>
#include "views/SampleLoaderView.h"
#include "ScreenManager.h"

using namespace trakr;

TEST_CASE("SampleLoaderView 48-Slot Pool Parity", "[sample_loader]") {
    Song song;
    History history;
    EngineHost engineHost;
    SampleLoaderView loader;
    int selectedInst = 0;

    song.instruments.resize(4);
    for (int i = 0; i < 4; ++i) {
        song.instruments[i].name = "Inst " + std::to_string(i + 1);
        auto samp = std::make_shared<SampleData>();
        samp->channels = 1;
        samp->sampleRate = 44100;
        samp->ch[0].resize(44100, 0.2f);
        song.instruments[i].sample = samp;
    }

    Pattern pat;
    pat.rows = 16;
    pat.tracks.resize(8, std::vector<Cell>(16));
    pat.tracks[0][0].instrument = 0; // Inst 1 used
    pat.tracks[1][0].instrument = 2; // Inst 3 used
    // Inst 1 and Inst 3 are unused (indices 1 and 3)
    song.patterns.push_back(std::move(pat));

    SECTION("Delete instrument slot") {
        loader.deleteInstrumentSlot(song, history, engineHost, 0);
        REQUIRE(song.instruments[0].name == "Empty slot");
        REQUIRE(song.instruments[0].sample == nullptr);
    }

    SECTION("Delete unused instruments clears only unused slots") {
        loader.deleteUnusedInstruments(song, history, engineHost);
        // Inst 0 is used in pat.tracks[0][0]
        CHECK(song.instruments[0].name == "Inst 1");
        CHECK(song.instruments[0].sample != nullptr);

        // Inst 1 is unused
        CHECK(song.instruments[1].name == "Empty slot");
        CHECK(song.instruments[1].sample == nullptr);

        // Inst 2 is used in pat.tracks[1][0]
        CHECK(song.instruments[2].name == "Inst 3");
        CHECK(song.instruments[2].sample != nullptr);

        // Inst 3 is unused
        CHECK(song.instruments[3].name == "Empty slot");
        CHECK(song.instruments[3].sample == nullptr);
    }

    SECTION("Screen buttons provide hardware parity actions") {
        Screen scr = Screen::SampleLoader;
        auto btns = loader.getScreenButtons(song, history, engineHost, selectedInst, scr);
        REQUIRE(btns.size() == 8);
        CHECK(btns[0].label == "Preview");
        CHECK(btns[1].label == "Delete");
        CHECK(btns[2].label == "Delete");
        CHECK(btns[3].label == "Add Next");
        CHECK(btns[4].label == "Import");
        CHECK(btns[5].label == "Parent");
        CHECK(btns[6].label == "Enter");
        CHECK(btns[7].label == "Pattern");

        // Clicking Exit returns to Pattern page
        btns[7].onClick();
        CHECK(scr == Screen::Pattern);
    }
}

namespace {
struct LoaderImGuiFixture {
    ImGuiContext* ctx{nullptr};
    LoaderImGuiFixture() {
        IMGUI_CHECKVERSION();
        ctx = ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2(800, 480);
        io.DeltaTime = 1.0f / 60.0f;
        unsigned char* pixels = nullptr;
        int width = 0, height = 0;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    }
    ~LoaderImGuiFixture() {
        if (ctx) ImGui::DestroyContext(ctx);
    }
};
} // namespace

TEST_CASE("SampleLoaderView Edge Conditions and Memory Stress", "[sample_loader]") {
    Song song;
    History history;
    EngineHost engineHost;
    SampleLoaderView loader;
    int selectedInst = 0;

    SECTION("Out-of-bound delete operations are safe") {
        loader.deleteInstrumentSlot(song, history, engineHost, -1);
        loader.deleteInstrumentSlot(song, history, engineHost, 48);
        loader.deleteInstrumentSlot(song, history, engineHost, 100);
        CHECK(song.instruments.empty());
    }

    SECTION("addNextAvailableSlot fills gaps and respects 48-slot pool limit") {
        song.instruments.resize(48);
        for (int i = 0; i < 48; ++i) {
            auto samp = std::make_shared<SampleData>();
            samp->ch[0].resize(100, 0.1f);
            song.instruments[i].sample = samp;
            song.instruments[i].name = "Inst " + std::to_string(i);
        }
        int origSelected = selectedInst;
        loader.addNextAvailableSlot(song, history, engineHost, selectedInst);
        CHECK(selectedInst == origSelected);

        // Make gap at slot 7
        song.instruments[7].sample = nullptr;
        song.instruments[7].name = "Empty slot";
        loader.addNextAvailableSlot(song, history, engineHost, selectedInst);
        CHECK(selectedInst == 7);
    }

    SECTION("deleteUnusedInstruments with malformed pattern references") {
        song.instruments.resize(2);
        song.instruments[0].name = "Slot 0";
        song.instruments[1].name = "Slot 1";
        Pattern pat;
        pat.rows = 4;
        pat.tracks.resize(1, std::vector<Cell>(4));
        pat.tracks[0][0].instrument = -1;
        pat.tracks[0][1].instrument = 999;
        song.patterns.push_back(std::move(pat));

        loader.deleteUnusedInstruments(song, history, engineHost);
        CHECK(song.instruments[0].name == "Empty slot");
        CHECK(song.instruments[1].name == "Empty slot");
    }

    SECTION("Firmware parity bottom bar matches gSampleImport labels") {
        Screen scr = Screen::SampleLoader;
        auto btns = loader.getScreenButtons(song, history, engineHost, selectedInst, scr, true);
        REQUIRE(btns.size() == 8);
        CHECK(btns[0].label == "Add");
        CHECK(btns[1].label == "Add next");
        CHECK(btns[2].label == "Preview");
        CHECK(btns[3].label == "Add");
        CHECK(btns[4].label == "Import");
        CHECK(btns[5].label == "Delete");
        CHECK(btns[5].value == "unused");
        CHECK(btns[6].label == "Preview Vol");
        CHECK(btns[7].label == "Memory");
    }
}

TEST_CASE("SampleLoaderView ImGui Render Stress", "[sample_loader]") {
    LoaderImGuiFixture fixture;
    Song song;
    History history;
    EngineHost engineHost;
    SampleLoaderView loader;
    int selectedInst = 0;

    SECTION("Render empty song without crash") {
        ImGui::NewFrame();
        loader.update(song, history, engineHost, selectedInst);
        ImGui::Render();
    }

    SECTION("Render with 48 full instruments and overflow memory > 133s") {
        song.instruments.resize(48);
        for (int i = 0; i < 48; ++i) {
            auto samp = std::make_shared<SampleData>();
            samp->channels = 1;
            samp->sampleRate = 44100;
            samp->ch[0].resize(44100 * 4, 0.1f);
            song.instruments[i].sample = samp;
            song.instruments[i].name = "LongInstrumentNameWithSpecialChars_%s_%d_#!" + std::to_string(i);
        }
        ImGui::NewFrame();
        loader.update(song, history, engineHost, selectedInst);
        ImGui::Render();
    }

    SECTION("Render in parity state") {
        loader.setParityState();
        song.instruments.resize(1);
        song.instruments[0].name = "909 Kick";
        ImGui::NewFrame();
        loader.update(song, history, engineHost, selectedInst);
        ImGui::Render();
    }

    SECTION("Render with tiny viewport early-exits safely") {
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2(50, 50);
        ImGui::NewFrame();
        loader.update(song, history, engineHost, selectedInst);
        ImGui::Render();
    }
}

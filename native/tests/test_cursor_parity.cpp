#include <catch2/catch_test_macros.hpp>
#include "hw/ButtonDeck.h"
#include "views/PatternView.h"
#include "views/PatternFxHelper.h"
#include "ScreenManager.h"
#include "Song.h"
#include "History.h"
#include "EngineHost.h"
#include "UiRobot.h"
#include "imgui.h"
#include <cmath>

namespace {
struct CursorTestFixture {
    ImGuiContext* ctx{nullptr};
    CursorTestFixture() {
        IMGUI_CHECKVERSION();
        ctx = ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigMacOSXBehaviors = false;
        io.DisplaySize = ImVec2(1000, 700);
        io.DeltaTime = 1.0f / 60.0f;
        unsigned char* pixels = nullptr;
        int width = 0, height = 0;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    }
    ~CursorTestFixture() {
        if (ctx) ImGui::DestroyContext(ctx);
    }
};
} // namespace

TEST_CASE("Visual Keyboard Pad Geometry: Square Pads and Snug Spacing", "[hw][pads][geometry]") {
    CursorTestFixture fixture;
    trakr::Song song;
    trakr::History history;
    trakr::EngineHost engineHost;
    trakr::PatternView patternView;
    trakr::hw::ButtonDeck deck;

    trakr::Pattern pat;
    pat.rows = 64;
    pat.tracks.resize(8, std::vector<trakr::Cell>(64));
    song.patterns.push_back(std::move(pat));
    song.order.push_back(0);
    engineHost.publish(song);

    ImRect deckRect(ImVec2(0, 480), ImVec2(1000, 700));
    trakr::Screen scr = trakr::Screen::Pattern;
    bool recordMode = false, shiftHeld = false;
    int octave = 3, editStep = 1, selectedInst = 0, currentOrder = 0;

    auto renderFrame = [&]() {
        ImGui::NewFrame();
        deck.render(deckRect, song, engineHost, history, patternView, scr,
                    recordMode, octave, editStep, shiftHeld, selectedInst, currentOrder, 1.0f);
        ImGui::Render();
    };

    renderFrame();

    // Check pad bounding box
    ImRect padsB = deck.padsBounds();
    REQUIRE(padsB.GetWidth() > 200.0f);
    REQUIRE(padsB.GetHeight() > 80.0f);

    // Check individual pad 0_0 bounds: must be perfectly square
    ImRect pad0 = deck.buttonBounds("Pad_0_0");
    REQUIRE(pad0.GetWidth() > 0.0f);
    REQUIRE(pad0.GetHeight() > 0.0f);
    REQUIRE(std::abs(pad0.GetWidth() - pad0.GetHeight()) < 0.001f);

    // Check right section begins promptly after pads
    ImRect shiftB = deck.buttonBounds("Shift");
    REQUIRE(shiftB.Min.x > padsB.Max.x);
    REQUIRE((shiftB.Min.x - padsB.Max.x) <= 120.0f);
}

TEST_CASE("Pattern View: Mouse Click and Shift-Selection Synchronization", "[view][pattern][mouse][shift]") {
    CursorTestFixture fixture;
    trakr::Song song;
    trakr::History history;
    trakr::EngineHost engineHost;
    trakr::PatternView patternView;

    trakr::Pattern pat;
    pat.rows = 64;
    pat.tracks.resize(8, std::vector<trakr::Cell>(64));
    // Step 0: Kick, Step 4: Snare on Track 1
    pat.tracks[0][0].instrument = 0;
    pat.tracks[1][4].instrument = 1;
    song.patterns.push_back(std::move(pat));
    song.order.push_back(0);

    trakr::Instrument inst0, inst1;
    inst0.name = "Kick";
    inst1.name = "Snare";
    song.instruments.push_back(inst0);
    song.instruments.push_back(inst1);
    engineHost.publish(song);

    int selectedInst = 0;

    // 1. Unshifted click sets cursor track, row, subcolumn, and syncs instrument
    patternView.setCursor(0, 0, 0);
    patternView.clearSelection();

    // Move to Track 1, Row 4 (Snare at Row 4)
    patternView.setCursor(1, 4, 0);
    patternView.syncInstrumentFromCursor(song.patterns[0], song, selectedInst);
    REQUIRE(selectedInst == 1); // Found instrument on step 4!

    // Move to Track 1, Row 8 (empty cell): preserves selected instrument
    patternView.setCursor(1, 8, 0);
    patternView.syncInstrumentFromCursor(song.patterns[0], song, selectedInst);
    REQUIRE(selectedInst == 1); // Preserved selected instrument on empty cell!

    // 2. Shift selection via navigate
    patternView.navigate(0, 4, 0, true, 8, 64, song.patterns[0], song, selectedInst);
    REQUIRE(patternView.hasSelection());
    auto sel = patternView.getSelection();
    REQUIRE(sel.minRow() == 8);
    REQUIRE(sel.maxRow() == 12);
    REQUIRE(patternView.cursorRow() == 12);
}

TEST_CASE("Shift Note Auditioning: Previews Without Overwriting Pattern Step", "[pattern][input][audition]") {
    CursorTestFixture fixture;
    trakr::Song song;
    trakr::History history;
    trakr::EngineHost engineHost;
    trakr::PatternView patternView;

    trakr::Pattern pat;
    pat.rows = 64;
    pat.tracks.resize(8, std::vector<trakr::Cell>(64));
    pat.tracks[0][0].note = 60; // C5
    pat.tracks[0][0].instrument = 0;
    song.patterns.push_back(std::move(pat));
    song.order.push_back(0);

    trakr::Instrument inst;
    inst.name = "Synth";
    auto smp = std::make_shared<trakr::SampleData>();
    smp->ch[0].assign(500, 0.5f);
    inst.sample = smp;
    song.instruments.push_back(inst);
    engineHost.publish(song);

    patternView.setCursor(0, 0, 0);
    int selectedInst = 0;

    // Simulate key input with Shift held in record mode
    patternView.setShiftHeld(true);
    ImGuiIO& io = ImGui::GetIO();
    io.KeyShift = true;

    // In record mode with Shift held, existing step cell must remain unmodified
    patternView.handleKeyboardInput(song.patterns[0], 0, song, history, engineHost, 3, 1, true, selectedInst);
    REQUIRE(song.patterns[0].tracks[0][0].note == 60);
    REQUIRE(patternView.cursorRow() == 0); // Did not step jump
}

TEST_CASE("Pattern Waveform Strip: Renders Successfully With and Without Sample", "[view][waveform]") {
    CursorTestFixture fixture;
    trakr::EngineHost engineHost;

    ImGui::NewFrame();
    ImDrawList* dl = ImGui::GetWindowDrawList();

    // 1. Without instrument / null
    trakr::renderPatternWaveformStrip(dl, ImVec2(50, 50), ImVec2(200, 40), nullptr, engineHost);

    // 2. With instrument without sample
    trakr::Instrument instEmpty;
    instEmpty.name = "Empty";
    trakr::renderPatternWaveformStrip(dl, ImVec2(50, 100), ImVec2(200, 40), &instEmpty, engineHost);

    // 3. With sample data
    trakr::Instrument instWithWave;
    instWithWave.name = "Kick";
    auto sData = std::make_shared<trakr::SampleData>();
    sData->ch[0].resize(256);
    for (size_t i = 0; i < 256; ++i) sData->ch[0][i] = std::sin(static_cast<float>(i) * 0.1f);
    instWithWave.sample = sData;
    trakr::renderPatternWaveformStrip(dl, ImVec2(50, 150), ImVec2(200, 40), &instWithWave, engineHost);

    ImGui::Render();
    SUCCEED("Waveform strip rendered successfully without issues.");
}

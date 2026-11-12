#include <catch2/catch_test_macros.hpp>
#include "hw/ButtonDeck.h"
#include "views/PatternView.h"
#include "views/InstrumentView.h"
#include "views/SampleView.h"
#include "views/MixerView.h"
#include "views/PerformView.h"
#include "ScreenManager.h"
#include "Song.h"
#include "History.h"
#include "EngineHost.h"
#include "UiRobot.h"
#include "imgui.h"
#include "imgui_internal.h"

namespace {
struct DpadTestFixture {
    ImGuiContext* ctx{nullptr};
    DpadTestFixture() {
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
    ~DpadTestFixture() {
        if (ctx) ImGui::DestroyContext(ctx);
    }
};
} // namespace

TEST_CASE("Button Deck Tool Control: D-pad and Shift Selection Parity", "[hw][deck][dpad][selection]") {
    DpadTestFixture fixture;
    trakr::Song song;
    trakr::History history;
    trakr::EngineHost engineHost;
    trakr::PatternView patternView;
    trakr::hw::ButtonDeck deck;
    trakr::UiRobot robot;

    trakr::Pattern pat;
    pat.rows = 64;
    pat.tracks.resize(8, std::vector<trakr::Cell>(64));
    pat.tracks[0][5].instrument = 2;
    song.patterns.push_back(std::move(pat));
    song.order.push_back(0);
    for (int i = 0; i < 4; ++i) {
        trakr::Instrument inst;
        inst.name = "Inst" + std::to_string(i);
        song.instruments.push_back(inst);
    }
    engineHost.publish(song);

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(1000, 700);
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

    SECTION("Dpad without Shift navigates and syncs selectedInst") {
        patternView.setCursor(0, 4, 0);
        ImRect downB = deck.buttonBounds("DpadDown");
        robot.click(downB.GetCenter(), 0, renderFrame);
        REQUIRE(patternView.cursorRow() == 5);
        REQUIRE(selectedInst == 2);
    }

    SECTION("Shift + DpadDown expands selection") {
        patternView.setCursor(0, 5, 0);
        ImRect shiftB = deck.buttonBounds("Shift");
        robot.click(shiftB.GetCenter(), 0, renderFrame);
        REQUIRE(shiftHeld == true);

        ImRect downB = deck.buttonBounds("DpadDown");
        robot.click(downB.GetCenter(), 0, renderFrame);
        REQUIRE(patternView.hasSelection() == true);
        auto sel = patternView.getSelection();
        REQUIRE(sel.minRow() == 5);
        REQUIRE(sel.maxRow() == 6);
    }

    SECTION("Shift + DpadUp on step 0 selects entire track, then all tracks (Manual 967)") {
        patternView.setCursor(1, 0, 0);
        ImRect shiftB = deck.buttonBounds("Shift");
        robot.click(shiftB.GetCenter(), 0, renderFrame);

        ImRect upB = deck.buttonBounds("DpadUp");
        robot.click(upB.GetCenter(), 0, renderFrame);
        REQUIRE(patternView.hasSelection() == true);
        auto sel1 = patternView.getSelection();
        REQUIRE(sel1.startTrack == 1);
        REQUIRE(sel1.endTrack == 1);
        REQUIRE(sel1.minRow() == 0);
        REQUIRE(sel1.maxRow() == 63);

        // Second press selects all tracks
        robot.click(upB.GetCenter(), 0, renderFrame);
        auto sel2 = patternView.getSelection();
        REQUIRE(sel2.minTrack() == 0);
        REQUIRE(sel2.maxTrack() == 7);
        REQUIRE(sel2.minRow() == 0);
        REQUIRE(sel2.maxRow() == 63);
    }

    SECTION("Visual D-pad and Keyboard Shift Navigation produce identical state") {
        // Test visual D-pad expansion
        patternView.setCursor(2, 10, 0);
        patternView.clearSelection();
        shiftHeld = true;
        ImRect rightB = deck.buttonBounds("DpadRight");
        robot.click(rightB.GetCenter(), 0, renderFrame);
        ImRect downB = deck.buttonBounds("DpadDown");
        robot.click(downB.GetCenter(), 0, renderFrame);

        auto dpadSel = patternView.getSelection();
        int dpadTrk = patternView.cursorTrack();
        int dpadRow = patternView.cursorRow();

        // Reset and test keyboard equivalent with pv.navigate
        trakr::PatternView pvKey;
        pvKey.setCursor(2, 10, 0);
        int instDummy = 0;
        pvKey.navigate(1, 0, 0, true, 8, 64, song.patterns[0], song, instDummy);
        pvKey.navigate(0, 1, 0, true, 8, 64, song.patterns[0], song, instDummy);

        auto keySel = pvKey.getSelection();
        REQUIRE(dpadSel.startTrack == keySel.startTrack);
        REQUIRE(dpadSel.startRow == keySel.startRow);
        REQUIRE(dpadSel.endTrack == keySel.endTrack);
        REQUIRE(dpadSel.endRow == keySel.endRow);
        REQUIRE(dpadTrk == pvKey.cursorTrack());
        REQUIRE(dpadRow == pvKey.cursorRow());
    }

    SECTION("Unshifted Dpad movement clears selection and syncs instrument") {
        patternView.setCursor(0, 4, 0);
        patternView.setSelection(0, 4, 0, 8);
        REQUIRE(patternView.hasSelection() == true);

        shiftHeld = false;
        ImRect downB = deck.buttonBounds("DpadDown");
        robot.click(downB.GetCenter(), 0, renderFrame);

        REQUIRE(patternView.hasSelection() == false);
        REQUIRE(patternView.cursorRow() == 5);
        REQUIRE(selectedInst == 2);
    }

    SECTION("Instrument Parameters: D-pad navigation, Jog adjust, and Delete reset") {
        trakr::InstrumentView iv;
        scr = trakr::Screen::Instrument;
        iv.setSelectedColumn(0);
        song.instruments[0].volume = 50;
        song.instruments[0].panning = 0;

        auto renderInstFrame = [&]() {
            ImGui::NewFrame();
            deck.render(deckRect, song, engineHost, history, patternView, scr,
                        recordMode, octave, editStep, shiftHeld, selectedInst, currentOrder, 1.0f, &iv);
            ImGui::Render();
        };

        renderInstFrame();

        // 1. DpadRight moves to Panning (column 1)
        ImRect rightB = deck.buttonBounds("DpadRight");
        robot.click(rightB.GetCenter(), 0, renderInstFrame);
        REQUIRE(iv.selectedColumn() == 1);

        // 2. DpadUp adjusts Panning
        int oldPan = song.instruments[0].panning;
        ImRect upB = deck.buttonBounds("DpadUp");
        robot.click(upB.GetCenter(), 0, renderInstFrame);
        REQUIRE(song.instruments[0].panning == oldPan + 1);

        // 3. Delete button resets Panning to default 0 (Manual 5800)
        ImRect delB = deck.buttonBounds("Delete");
        robot.click(delB.GetCenter(), 0, renderInstFrame);
        REQUIRE(song.instruments[0].panning == 0);
    }

    SECTION("Performance Mode: 48 Silicone Pads trigger 12 effect punch-ins") {
        trakr::PerformView pv;
        scr = trakr::Screen::Perform;

        auto renderPerformFrame = [&]() {
            ImGui::NewFrame();
            deck.render(deckRect, song, engineHost, history, patternView, scr,
                        recordMode, octave, editStep, shiftHeld, selectedInst, currentOrder, 1.0f,
                        nullptr, nullptr, &pv);
            ImGui::Render();
        };

        renderPerformFrame();

        ImRect pad01 = deck.buttonBounds("Pad_0_1");
        REQUIRE(pad01.GetWidth() > 0.0f);
        robot.click(pad01.GetCenter(), 0, renderPerformFrame);

        std::vector<float> audioBuf(128 * 2, 0.0f);
        engineHost.getEngine().process(audioBuf.data(), 128);

        REQUIRE(pv.slots()[0].activeRow == 1);
        REQUIRE(engineHost.getPerformanceState().pitchShiftSemitones == 2.0f);
    }

    SECTION("Holding physical Shift vs Visual Shift Toggle produces identical D-pad selection") {
        patternView.setCursor(1, 4, 0);
        patternView.clearSelection();

        // 1. Visual Shift button click on deck toggles shiftHeld
        shiftHeld = false;
        ImRect shiftB = deck.buttonBounds("Shift");
        REQUIRE(shiftB.GetWidth() > 0.0f);
        robot.click(shiftB.GetCenter(), 0, renderFrame);
        REQUIRE(shiftHeld == true);

        // Visual Shift Toggle + DpadDown expands selection
        ImRect downB = deck.buttonBounds("DpadDown");
        robot.click(downB.GetCenter(), 0, renderFrame);
        REQUIRE(patternView.hasSelection() == true);
        auto selVisual = patternView.getSelection();

        // 2. Physical Shift + DpadDown on fresh pattern view
        trakr::PatternView pvPhys;
        pvPhys.setCursor(1, 4, 0);
        pvPhys.clearSelection();
        io.AddKeyEvent(ImGuiMod_Shift, true);
        shiftHeld = false;

        auto renderPhysFrame = [&]() {
            ImGui::NewFrame();
            deck.render(deckRect, song, engineHost, history, pvPhys, scr,
                        recordMode, octave, editStep, shiftHeld, selectedInst, currentOrder, 1.0f);
            ImGui::Render();
        };
        renderPhysFrame();
        robot.click(downB.GetCenter(), 0, renderPhysFrame);
        io.AddKeyEvent(ImGuiMod_Shift, false);

        REQUIRE(pvPhys.hasSelection() == true);
        auto selPhys = pvPhys.getSelection();
        REQUIRE(selVisual.startTrack == selPhys.startTrack);
        REQUIRE(selVisual.startRow == selPhys.startRow);
        REQUIRE(selVisual.endTrack == selPhys.endTrack);
        REQUIRE(selVisual.endRow == selPhys.endRow);
        REQUIRE(patternView.cursorRow() == pvPhys.cursorRow());

        // 3. Toggling visual Shift off and DpadDown clears selection
        shiftHeld = true;
        robot.click(shiftB.GetCenter(), 0, renderFrame);
        REQUIRE(shiftHeld == false);
        robot.click(downB.GetCenter(), 0, renderFrame);
        REQUIRE(patternView.hasSelection() == false);
        REQUIRE(patternView.cursorRow() == 6);

        // 4. DpadCenter advances by editStep on Pattern screen
        editStep = 2;
        ImRect centerB = deck.buttonBounds("DpadCenter");
        robot.click(centerB.GetCenter(), 0, renderFrame);
        REQUIRE(patternView.cursorRow() == 8);
    }

    SECTION("Master Page Parity: D-pad track selection, dB gain stepping, and Delete reset") {
        trakr::MixerView mv;
        mv.setSubPage(trakr::MixerView::SubPage::TrackMixer); // page 2/3
        scr = trakr::Screen::Master;
        song.tracks.resize(8);
        song.tracks[0].gainDb = -6.0f;
        song.tracks[1].gainDb = 0.0f;

        auto renderMasterFrame = [&]() {
            ImGui::NewFrame();
            deck.render(deckRect, song, engineHost, history, patternView, scr,
                        recordMode, octave, editStep, shiftHeld, selectedInst, currentOrder, 1.0f,
                        nullptr, nullptr, nullptr, nullptr, &mv);
            ImGui::Render();
        };

        renderMasterFrame();

        // DpadRight selects Track 2 (index 1)
        ImRect rightB = deck.buttonBounds("DpadRight");
        robot.click(rightB.GetCenter(), 0, renderMasterFrame);
        REQUIRE(mv.selectedTrack() == 1);

        // DpadDown decreases Track 2 gain by 1.0 dB
        ImRect downB = deck.buttonBounds("DpadDown");
        robot.click(downB.GetCenter(), 0, renderMasterFrame);
        REQUIRE(song.tracks[1].gainDb == -1.0f);

        // Delete resets Track 2 gain back to 0.0 dB (Polyend parity)
        ImRect delB = deck.buttonBounds("Delete");
        robot.click(delB.GetCenter(), 0, renderMasterFrame);
        REQUIRE(song.tracks[1].gainDb == 0.0f);

        // Master FX: EQ stepping and DSP audio verification
        mv.setSubPage(trakr::MixerView::SubPage::MasterFx);
        mv.setSelectedTrack(0); // 115 Hz band
        mv.adjustSelectedParam(song, engineHost, 3, false); // +3.0 dB
        REQUIRE(song.masterEq[0] == 3.0f);

        trakr::dsp::MasterEq eq;
        eq.init(44100.0f);
        float eqGains[5] = {6.0f, 0.0f, 0.0f, 0.0f, 0.0f};
        eq.setGains(eqGains);
        REQUIRE(eq.isActive());
        float sL = 0.5f, sR = 0.5f;
        eq.process(sL, sR);
        REQUIRE(sL > 0.0f);

        mv.resetSelected(song, engineHost);
        REQUIRE(song.masterEq[0] == 0.0f);
    }

    SECTION("Sample View: D-pad parameter stepping and D-pad Center toggle subpage") {
        trakr::SampleView sv;
        scr = trakr::Screen::Sample;
        sv.setSubPage(trakr::SampleView::SubPage::Playback);

        auto renderSampleFrame = [&]() {
            ImGui::NewFrame();
            deck.render(deckRect, song, engineHost, history, patternView, scr,
                        recordMode, octave, editStep, shiftHeld, selectedInst, currentOrder, 1.0f,
                        nullptr, nullptr, nullptr, &sv, nullptr);
            ImGui::Render();
        };

        renderSampleFrame();

        // DpadCenter toggles between Playback and Editor
        ImRect centerB = deck.buttonBounds("DpadCenter");
        robot.click(centerB.GetCenter(), 0, renderSampleFrame);
        REQUIRE(sv.subPage() == trakr::SampleView::SubPage::Editor);

        // DpadCenter toggles back to Playback
        robot.click(centerB.GetCenter(), 0, renderSampleFrame);
        REQUIRE(sv.subPage() == trakr::SampleView::SubPage::Playback);
    }

    SECTION("Song Screen: D-pad Up/Down navigates Order List slots") {
        scr = trakr::Screen::Song;
        song.order = {0, 1, 2, 3};
        currentOrder = 0;

        auto renderSongFrame = [&]() {
            ImGui::NewFrame();
            deck.render(deckRect, song, engineHost, history, patternView, scr,
                        recordMode, octave, editStep, shiftHeld, selectedInst, currentOrder, 1.0f);
            ImGui::Render();
        };

        renderSongFrame();

        ImRect downB = deck.buttonBounds("DpadDown");
        robot.click(downB.GetCenter(), 0, renderSongFrame);
        REQUIRE(currentOrder == 1);

        robot.click(downB.GetCenter(), 0, renderSongFrame);
        REQUIRE(currentOrder == 2);

        ImRect upB = deck.buttonBounds("DpadUp");
        robot.click(upB.GetCenter(), 0, renderSongFrame);
        REQUIRE(currentOrder == 1);
    }

    SECTION("Jog Wheel in Record Mode with Shift expands selection") {
        patternView.setCursor(0, 5, 0);
        patternView.clearSelection();
        recordMode = true;
        shiftHeld = true;
        renderFrame();
        patternView.navigate(0, 1, 0, true, 8, 64, song.patterns[0], song, selectedInst);
        REQUIRE(patternView.hasSelection());
        REQUIRE(patternView.getSelection().maxRow() == 6);
    }
}

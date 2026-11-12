#include <catch2/catch_test_macros.hpp>
#include "hw/ButtonDeck.h"
#include "views/PatternView.h"
#include "ScreenManager.h"
#include "Song.h"
#include "History.h"
#include "EngineHost.h"
#include "UiRobot.h"
#include "imgui.h"
#include "imgui_internal.h"

namespace {
struct DeckTestFixture {
    ImGuiContext* ctx{nullptr};
    DeckTestFixture() {
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
    ~DeckTestFixture() {
        if (ctx) ImGui::DestroyContext(ctx);
    }
};
} // namespace

// Automated Tool Control & Machine Vision Verification for Button Deck
// Reference: tracker-manual-1.7.0.txt lines 206-350, 859-1100

TEST_CASE("Button Deck Layout Machine Vision & Parity", "[hw][deck][vision]") {
    DeckTestFixture fixture;
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
    trakr::Instrument inst;
    inst.name = "Kick";
    song.instruments.push_back(inst);
    engineHost.publish(song);

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(1000, 700);

    ImRect deckRect(ImVec2(0, 480), ImVec2(1000, 700));
    trakr::Screen scr = trakr::Screen::Pattern;
    bool recordMode = false;
    int octave = 3, editStep = 1;
    bool shiftHeld = false;
    int selectedInst = 0, currentOrder = 0;

    auto renderFrame = [&]() {
        ImGui::NewFrame();
        deck.render(deckRect, song, engineHost, history, patternView, scr,
                    recordMode, octave, editStep, shiftHeld, selectedInst, currentOrder, 1.0f);
        ImGui::Render();
    };

    renderFrame();

    SECTION("Machine Vision: Pads are positioned on Left/Center, Jog Wheel on Right") {
        ImRect pads = deck.padsBounds();
        ImVec2 jog = deck.jogCenter();
        float radius = deck.jogRadius();

        // Machine vision assertions
        REQUIRE(pads.GetWidth() >= 340.0f);
        REQUIRE(pads.Min.x >= deckRect.Min.x);
        REQUIRE(pads.Max.x < jog.x - radius); // Pads must be to the left of the jog wheel
        REQUIRE(jog.x > deckRect.Min.x + deckRect.GetWidth() * 0.5f); // Jog is in the right half
        REQUIRE(radius >= 30.0f);
    }
}

TEST_CASE("Button Deck Tool Control: Octave Buttons 1..4", "[hw][deck][octave]") {
    DeckTestFixture fixture;
    trakr::Song song;
    trakr::History history;
    trakr::EngineHost engineHost;
    trakr::PatternView patternView;
    trakr::hw::ButtonDeck deck;
    trakr::UiRobot robot;

    trakr::Pattern pat;
    pat.rows = 64;
    pat.tracks.resize(8, std::vector<trakr::Cell>(64));
    song.patterns.push_back(std::move(pat));
    song.order.push_back(0);

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(1000, 700);

    ImRect deckRect(ImVec2(0, 480), ImVec2(1000, 700));
    trakr::Screen scr = trakr::Screen::Pattern;
    bool recordMode = false;
    int octave = 1, editStep = 1;
    bool shiftHeld = false;
    int selectedInst = 0, currentOrder = 0;

    auto renderFrame = [&]() {
        ImGui::NewFrame();
        deck.render(deckRect, song, engineHost, history, patternView, scr,
                    recordMode, octave, editStep, shiftHeld, selectedInst, currentOrder, 1.0f);
        ImGui::Render();
    };

    renderFrame();

    SECTION("Tool control: Click 1, 2, 3, 4 sets octave accurately") {
        for (int n = 1; n <= 4; ++n) {
            ImRect b = deck.buttonBounds(std::to_string(n));
            REQUIRE(b.GetWidth() > 0.0f);
            robot.click(b.GetCenter(), 0, renderFrame);
            REQUIRE(octave == n);
        }
    }

    SECTION("Tool control: Shift + Octave sets octaves 5..8") {
        ImRect shiftB = deck.buttonBounds("Shift");
        robot.click(shiftB.GetCenter(), 0, renderFrame);
        REQUIRE(shiftHeld == true);

        ImRect b3 = deck.buttonBounds("3");
        robot.click(b3.GetCenter(), 0, renderFrame);
        REQUIRE(octave == 7); // 3 + 4
    }
}

TEST_CASE("Button Deck Tool Control: Jog Wheel", "[hw][deck][jog]") {
    DeckTestFixture fixture;
    trakr::Song song;
    trakr::History history;
    trakr::EngineHost engineHost;
    trakr::PatternView patternView;
    trakr::hw::ButtonDeck deck;
    trakr::UiRobot robot;

    trakr::Pattern pat;
    pat.rows = 64;
    pat.tracks.resize(8, std::vector<trakr::Cell>(64));
    song.patterns.push_back(std::move(pat));
    song.order.push_back(0);
    engineHost.publish(song);

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(1000, 700);

    ImRect deckRect(ImVec2(0, 480), ImVec2(1000, 700));
    trakr::Screen scr = trakr::Screen::Pattern;
    bool recordMode = false;
    int octave = 3, editStep = 1;
    bool shiftHeld = false;
    int selectedInst = 0, currentOrder = 0;

    auto renderFrame = [&]() {
        ImGui::NewFrame();
        deck.render(deckRect, song, engineHost, history, patternView, scr,
                    recordMode, octave, editStep, shiftHeld, selectedInst, currentOrder, 1.0f);
        ImGui::Render();
    };

    renderFrame();

    SECTION("Tool control: Mouse wheel on Jog Wheel steps rows in navigate mode") {
        patternView.setCursor(0, 10, 0);
        robot.setMousePos(deck.jogCenter());
        robot.mouseWheel(1.0f, renderFrame);
        REQUIRE(patternView.cursorRow() == 11);
        robot.mouseWheel(-1.0f, renderFrame);
        REQUIRE(patternView.cursorRow() == 10);
    }

    SECTION("Tool control: Circular mouse drag on Jog Wheel rotates jogAngle") {
        float initialAngle = deck.jogAngle();
        ImVec2 center = deck.jogCenter();
        float r = deck.jogRadius() * 0.7f;
        robot.drag(ImVec2(center.x, center.y - r), ImVec2(center.x + r, center.y), 6, renderFrame);
        REQUIRE(deck.jogAngle() != initialAngle);
    }
}

TEST_CASE("Button Deck Tool Control: Insert, Delete, Copy, Paste", "[hw][deck][edit]") {
    DeckTestFixture fixture;
    trakr::Song song;
    trakr::History history;
    trakr::EngineHost engineHost;
    trakr::PatternView patternView;
    trakr::hw::ButtonDeck deck;
    trakr::UiRobot robot;

    trakr::Pattern pat;
    pat.rows = 64;
    pat.tracks.resize(8, std::vector<trakr::Cell>(64));
    pat.tracks[0][5].note = 48; // C-4
    pat.tracks[0][5].instrument = 1;
    song.patterns.push_back(std::move(pat));
    song.order.push_back(0);
    engineHost.publish(song);

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(1000, 700);

    ImRect deckRect(ImVec2(0, 480), ImVec2(1000, 700));
    trakr::Screen scr = trakr::Screen::Pattern;
    bool recordMode = true;
    int octave = 3, editStep = 1;
    bool shiftHeld = false;
    int selectedInst = 0, currentOrder = 0;

    auto renderFrame = [&]() {
        ImGui::NewFrame();
        deck.render(deckRect, song, engineHost, history, patternView, scr,
                    recordMode, octave, editStep, shiftHeld, selectedInst, currentOrder, 1.0f);
        ImGui::Render();
    };

    renderFrame();

    SECTION("Insert pushes cells down") {
        patternView.setCursor(0, 5, 0);
        ImRect insB = deck.buttonBounds("Insert");
        robot.click(insB.GetCenter(), 0, renderFrame);
        REQUIRE(song.patterns[0].tracks[0][5].note == trakr::kNoNote);
        REQUIRE(song.patterns[0].tracks[0][6].note == 48);
    }

    SECTION("Delete clears cell at cursor") {
        patternView.setCursor(0, 6, 0);
        ImRect delB = deck.buttonBounds("Delete");
        robot.click(delB.GetCenter(), 0, renderFrame);
        REQUIRE(song.patterns[0].tracks[0][6].note == trakr::kNoNote);
    }

    SECTION("Copy and Paste duplicates cell") {
        song.patterns[0].tracks[0][2].note = 52;
        patternView.setCursor(0, 2, 0);
        ImRect cpyB = deck.buttonBounds("Copy");
        robot.click(cpyB.GetCenter(), 0, renderFrame);

        patternView.setCursor(0, 15, 0);
        ImRect shiftB = deck.buttonBounds("Shift");
        robot.click(shiftB.GetCenter(), 0, renderFrame); // Activate shift -> Copy becomes Paste
        ImRect pasteB = deck.buttonBounds("Paste");
        robot.click(pasteB.GetCenter(), 0, renderFrame);

        REQUIRE(song.patterns[0].tracks[0][15].note == 52);
    }

    SECTION("Shift + Insert jumps Home (row 0)") {
        patternView.setCursor(0, 25, 0);
        ImRect shiftB = deck.buttonBounds("Shift");
        robot.click(shiftB.GetCenter(), 0, renderFrame);
        ImRect homeB = deck.buttonBounds("Home");
        robot.click(homeB.GetCenter(), 0, renderFrame);
        REQUIRE(patternView.cursorRow() == 0);
    }
}

TEST_CASE("Button Deck Tool Control: 4x12 Silicone Pads", "[hw][deck][pads]") {
    DeckTestFixture fixture;
    trakr::Song song;
    trakr::History history;
    trakr::EngineHost engineHost;
    trakr::PatternView patternView;
    trakr::hw::ButtonDeck deck;
    trakr::UiRobot robot;

    trakr::Pattern pat;
    pat.rows = 64;
    pat.tracks.resize(8, std::vector<trakr::Cell>(64));
    song.patterns.push_back(std::move(pat));
    song.order.push_back(0);
    trakr::Instrument inst;
    inst.name = "Synth";
    inst.isSynth = true;
    inst.sustain = 1.0f;
    song.instruments.push_back(inst);
    engineHost.publish(song);

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(1000, 700);

    ImRect deckRect(ImVec2(0, 480), ImVec2(1000, 700));
    trakr::Screen scr = trakr::Screen::Pattern;
    bool recordMode = false;
    int octave = 3, editStep = 2;
    bool shiftHeld = false;
    int selectedInst = 0, currentOrder = 0;

    auto renderFrame = [&]() {
        ImGui::NewFrame();
        deck.render(deckRect, song, engineHost, history, patternView, scr,
                    recordMode, octave, editStep, shiftHeld, selectedInst, currentOrder, 1.0f);
        ImGui::Render();
    };

    renderFrame();

    SECTION("Pad mouse down triggers preview voice; mouse up stops voice") {
        ImRect pads = deck.padsBounds();
        ImVec2 pad0(pads.Min.x + 15.0f, pads.Max.y - 15.0f);
        std::vector<float> audioBuf(256 * 2, 0.0f);

        robot.setMousePos(pad0);
        robot.mouseDown(0);
        renderFrame();
        engineHost.getEngine().process(audioBuf.data(), 256);
        REQUIRE(engineHost.isPreviewVoiceActive() == true);

        robot.mouseUp(0);
        renderFrame();
        engineHost.getEngine().process(audioBuf.data(), 256);
        REQUIRE(engineHost.isPreviewVoiceActive() == false);
    }

    SECTION("Pad click in recordMode writes note and advances cursor") {
        recordMode = true;
        patternView.setCursor(0, 4, 0);

        ImRect pads = deck.padsBounds();
        // Click first pad of bottom row (C3)
        ImVec2 padC3(pads.Min.x + 15.0f, pads.Max.y - 15.0f);
        robot.click(padC3, 0, renderFrame);

        REQUIRE(song.patterns[0].tracks[0][4].note == 36); // C3 = 3 * 12 = 36
        REQUIRE(song.patterns[0].tracks[0][4].instrument == selectedInst);
        REQUIRE(patternView.cursorRow() == 4 + editStep); // Advanced by editStep
    }

    SECTION("Menu button click opens MenuPopup overlay and toggles accurately") {
        bool showMenu = false;
        auto renderWithMenu = [&]() {
            ImGui::NewFrame();
            deck.render(deckRect, song, engineHost, history, patternView, scr,
                        recordMode, octave, editStep, shiftHeld, selectedInst, currentOrder, 1.0f,
                        nullptr, &showMenu);
            ImGui::Render();
        };

        renderWithMenu();
        ImRect menuB = deck.buttonBounds("Menu");
        REQUIRE(menuB.GetWidth() > 0.0f);
        robot.click(menuB.GetCenter(), 0, renderWithMenu);
        REQUIRE(showMenu == true);
        renderWithMenu();
        REQUIRE(showMenu == true); // Remains open on subsequent frame
        robot.click(menuB.GetCenter(), 0, renderWithMenu);
        REQUIRE(showMenu == false); // Toggled closed
    }

    SECTION("Dedicated Polyend screen navigation buttons switch active screen") {
        auto renderNav = [&]() {
            ImGui::NewFrame();
            deck.render(deckRect, song, engineHost, history, patternView, scr,
                        recordMode, octave, editStep, shiftHeld, selectedInst, currentOrder, 1.0f);
            ImGui::Render();
        };

        renderNav();
        ImRect songB = deck.buttonBounds("Song");
        REQUIRE(songB.GetWidth() > 0.0f);
        robot.click(songB.GetCenter(), 0, renderNav);
        REQUIRE(scr == trakr::Screen::Song);

        ImRect perfB = deck.buttonBounds("Perform");
        REQUIRE(perfB.GetWidth() > 0.0f);
        robot.click(perfB.GetCenter(), 0, renderNav);
        REQUIRE(scr == trakr::Screen::Perform);

        ImRect patB = deck.buttonBounds("Pattern");
        REQUIRE(patB.GetWidth() > 0.0f);
        robot.click(patB.GetCenter(), 0, renderNav);
        REQUIRE(scr == trakr::Screen::Pattern);
    }
}

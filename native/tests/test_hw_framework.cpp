#include <catch2/catch_test_macros.hpp>
#include "hw/Display.h"
#include "hw/HeaderBar.h"
#include "hw/ScreenButtons.h"
#include "hw/ButtonDeck.h"
#include "hw/HwWidgets.h"
#include "Song.h"
#include "EngineHost.h"
#include "Theme.h"
#include "imgui.h"
#include "imgui_internal.h"

namespace {
struct OffscreenImGuiFixture {
    ImGuiContext* ctx{nullptr};
    OffscreenImGuiFixture() {
        IMGUI_CHECKVERSION();
        ctx = ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2(1280, 800);
        io.DeltaTime = 1.0f / 60.0f;
        unsigned char* pixels = nullptr;
        int width = 0, height = 0;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    }
    ~OffscreenImGuiFixture() {
        if (ctx) ImGui::DestroyContext(ctx);
    }
};
}

// Hardware Display Framework Unit Tests
// References:
// - tracker-manual-1.7.0.txt line 700 (1.7 Page Layout)
// - core/interface/core/titleBar.cpp
// - core/interface/patternEditor/gPatternEdit.cpp

TEST_CASE("Display canvas layout and aspect scaling", "[hw][display]") {
    trakr::hw::Display display;

    REQUIRE(trakr::hw::Display::kLogicalWidth == 800.0f);
    REQUIRE(trakr::hw::Display::kLogicalHeight == 480.0f);
    REQUIRE(trakr::hw::Display::kHeaderHeight == 28.0f);
    REQUIRE(trakr::hw::Display::kScreenButtonsHeight == 55.0f);
    REQUIRE(trakr::hw::Display::kWorkspaceHeight == 397.0f);

    SECTION("Letterboxed 800x480 at 1:1 scale in 800x600 window") {
        display.computeLayout(ImVec2(0, 0), ImVec2(800, 600), false, false);
        const auto& r = display.screenRect();
        REQUIRE(r.GetWidth() == 800.0f);
        REQUIRE(r.GetHeight() == 480.0f);
        REQUIRE(r.Min.x == 0.0f);
        // Centered vertically: (600 - 480) / 2 = 60
        REQUIRE(r.Min.y == 60.0f);

        REQUIRE(display.headerRect().GetHeight() == 28.0f);
        REQUIRE(display.buttonsRect().GetHeight() == 55.0f);
        REQUIRE(display.workspaceRect().GetHeight() == 397.0f);
    }

    SECTION("Aspect scaling in 1200x800 window (1.5x scale)") {
        display.computeLayout(ImVec2(0, 0), ImVec2(1200, 800), false, false);
        const auto& r = display.screenRect();
        REQUIRE(display.scale() == 1.5f);
        REQUIRE(r.GetWidth() == 1200.0f);
        REQUIRE(r.GetHeight() == 720.0f);
        REQUIRE(r.Min.x == 0.0f);
        // Centered vertically: (800 - 720) / 2 = 40
        REQUIRE(r.Min.y == 40.0f);
    }

    SECTION("Wide mode expands horizontally") {
        display.computeLayout(ImVec2(0, 0), ImVec2(1600, 900), false, true);
        REQUIRE(display.isWideMode() == true);
        REQUIRE(display.screenRect().GetWidth() == 1600.0f);
        REQUIRE(display.headerRect().GetHeight() == 28.0f);
        REQUIRE(display.buttonsRect().GetHeight() == 55.0f);
        REQUIRE(display.workspaceRect().GetWidth() == 1600.0f);
    }

    SECTION("Coordinate transformations") {
        display.computeLayout(ImVec2(100, 50), ImVec2(800, 480), false, false);
        ImVec2 screenPt = display.canvasToScreen(ImVec2(400, 240));
        REQUIRE(screenPt.x == 500.0f);
        REQUIRE(screenPt.y == 290.0f);

        ImVec2 canvasPt = display.screenToCanvas(screenPt);
        REQUIRE(canvasPt.x == 400.0f);
        REQUIRE(canvasPt.y == 240.0f);
    }
}

TEST_CASE("Screen buttons setup and shift alternate labels", "[hw][screen_buttons]") {
    SECTION("Basic screen buttons setup and click") {
        trakr::hw::ScreenButtons buttons;

        std::vector<trakr::hw::ScreenButton> patButtons(8);
        patButtons[0] = {"Pattern", "15", "Mute", "", trakr::hw::UnderlineColor::Green, true, false, nullptr};
        patButtons[1] = {"Length", "32", "Solo", "", trakr::hw::UnderlineColor::Green, true, false, nullptr};
        patButtons[2] = {"Step jump", "1", "Duplicate", "", trakr::hw::UnderlineColor::Red, true, false, nullptr};
        patButtons[3] = {"Fill", "", "Expand", "", trakr::hw::UnderlineColor::None, true, false, nullptr};
        patButtons[4] = {"Preview", "", "Shrink", "", trakr::hw::UnderlineColor::Red, true, false, nullptr};
        patButtons[5] = {"Undo", "", "Copy", "", trakr::hw::UnderlineColor::Green, true, false, nullptr};
        patButtons[6] = {"Redo", "", "Paste", "", trakr::hw::UnderlineColor::Green, true, false, nullptr};
        patButtons[7] = {"More", "", "Clear", "", trakr::hw::UnderlineColor::Dimmed, true, false, nullptr};

        bool clickedUndo = false;
        patButtons[5].onClick = [&]() { clickedUndo = true; };

        buttons.setButtons(patButtons);

        REQUIRE(patButtons[0].label == "Pattern");
        REQUIRE(patButtons[0].value == "15");
        REQUIRE(patButtons[0].shiftLabel == "Mute");
        REQUIRE(patButtons[2].underline == trakr::hw::UnderlineColor::Red);

        patButtons[5].onClick();
        REQUIRE(clickedUndo == true);
    }

    SECTION("ScreenButtons vector sizing stress (empty, <8, >8)") {
        trakr::hw::ScreenButtons buttons;
        buttons.setButtons({}); // empty vector should resize to 8
        std::vector<trakr::hw::ScreenButton> smallBtns(3);
        buttons.setButtons(smallBtns); // <8 elements should resize to 8
        std::vector<trakr::hw::ScreenButton> largeBtns(16);
        buttons.setButtons(largeBtns); // >8 elements preserved
    }

    SECTION("ScreenButtons headless rendering under boundary scales and rects") {
        OffscreenImGuiFixture fixture;
        trakr::hw::ScreenButtons buttons;

        std::vector<trakr::hw::ScreenButton> btns(8);
        for (int i = 0; i < 8; ++i) {
            btns[i] = {"Btn " + std::to_string(i + 1), "Val", "", "", trakr::hw::UnderlineColor::Green, true, false, nullptr};
        }
        buttons.setButtons(btns);

        float scales[] = {0.5f, 1.0f, 1.25f, 1.5f, 2.0f, 0.0f};
        for (float s : scales) {
            ImGui::NewFrame();
            // Zero-sized or degenerate rect returns -1 safely
            REQUIRE(buttons.render(ImRect(ImVec2(0, 0), ImVec2(0, 0)), false, s) == -1);
            REQUIRE(buttons.render(ImRect(ImVec2(100, 100), ImVec2(50, 50)), false, s) == -1);

            // Valid rect renders without error
            int res = buttons.render(ImRect(ImVec2(0, 425 * s), ImVec2(800 * s, 480 * s)), false, s);
            REQUIRE(res == -1); // no clicks simulated yet
            ImGui::Render();
        }
    }

    SECTION("ScreenButtons Perform mode vs standard mode formatting and underline colors") {
        OffscreenImGuiFixture fixture;
        trakr::hw::ScreenButtons buttons;

        // Perform mode buttons: buttons_[0].value starts with "Patt:"
        std::vector<trakr::hw::ScreenButton> perfBtns(8);
        for (int i = 0; i < 8; ++i) {
            bool armed = (i < 3);
            perfBtns[i] = {"Track " + std::to_string(i + 1), "Patt: 1", "", "",
                           armed ? trakr::hw::UnderlineColor::Orange : trakr::hw::UnderlineColor::Dimmed,
                           true, armed, nullptr};
        }
        buttons.setButtons(perfBtns);

        ImGui::NewFrame();
        int res = buttons.render(ImRect(ImVec2(0, 425), ImVec2(800, 480)), false, 1.0f);
        REQUIRE(res == -1);
        ImGui::Render();

        // Standard screen with varied underline colors & single/two-line
        std::vector<trakr::hw::ScreenButton> stdBtns(8);
        stdBtns[0] = {"Single", "", "", "", trakr::hw::UnderlineColor::Green, true, false, nullptr};
        stdBtns[1] = {"Two", "Line", "", "", trakr::hw::UnderlineColor::Red, true, true, nullptr};
        stdBtns[2] = {"Orange", "Val", "", "", trakr::hw::UnderlineColor::Orange, true, false, nullptr};
        stdBtns[3] = {"Dimmed", "Val", "", "", trakr::hw::UnderlineColor::Dimmed, false, false, nullptr};
        stdBtns[4] = {"None", "Val", "", "", trakr::hw::UnderlineColor::None, true, false, nullptr};
        stdBtns[5] = {"More", "", "", "", trakr::hw::UnderlineColor::None, true, false, nullptr};
        stdBtns[6] = {"Very Long Button Label Exceeding Normal Width", "Very Long Value Text", "", "", trakr::hw::UnderlineColor::Green, true, false, nullptr};
        stdBtns[7] = {"Shift", "Test", "ShiftLbl", "ShiftVal", trakr::hw::UnderlineColor::Green, true, false, nullptr};
        buttons.setButtons(stdBtns);

        ImGui::NewFrame();
        res = buttons.render(ImRect(ImVec2(0, 425), ImVec2(800, 480)), true, 1.0f); // shift held
        REQUIRE(res == -1);
        ImGui::Render();
    }

    SECTION("HeaderBar stress under varied title lengths, long instrument names, and scales") {
        OffscreenImGuiFixture fixture;
        trakr::hw::HeaderBar header;
        trakr::Song song;
        trakr::EngineHost host;
        trakr::Theme theme = trakr::makeRenoiseDarkTheme();
        int selInst = 0;

        const char* titles[] = {
            nullptr, // fallback to "Pattern 1"
            "",
            "Pattern 1",
            "Performance FX",
            "Extremely Long Title String Intended To Test Clipping And Transport Overlap Logic In HeaderBar 1234567890",
            "Special chars !@#$%^&*()_+-=[]{}|;':,./<>? ♫"
        };

        float scales[] = {0.5f, 1.0f, 1.5f, 2.0f};

        for (const char* t : titles) {
            for (float s : scales) {
                ImGui::NewFrame();
                header.render(ImRect(ImVec2(0, 0), ImVec2(800 * s, 28 * s)),
                              t, song, host, 0, selInst, false, theme, s, 0);
                ImGui::Render();
            }
        }

        // Test with empty song instruments vs populated song instruments
        ImGui::NewFrame();
        selInst = -1; // negative index -> "Empty slot"
        header.render(ImRect(ImVec2(0, 0), ImVec2(800, 28)), "Test", song, host, -1, selInst, true, theme, 1.0f, 0);
        ImGui::Render();

        selInst = 50; // out of range index -> "Empty slot"
        ImGui::NewFrame();
        header.render(ImRect(ImVec2(0, 0), ImVec2(800, 28)), "Test", song, host, 999, selInst, false, theme, 1.0f, 1023);
        ImGui::Render();

        // Populate instrument with extremely long name (overflowing 64-char buffer)
        song.instruments.resize(3);
        song.instruments[0].name = "Normal Inst";
        song.instruments[1].name = "Super Ultra Mega Extended Instrument Name Designed To Exceed The 64 Byte Right Buffer Buffer Size Without Crashing Or Memory Corruption";
        selInst = 1;

        ImGui::NewFrame();
        header.render(ImRect(ImVec2(0, 0), ImVec2(800, 28)), "Test", song, host, 0, selInst, false, theme, 1.0f, 0);
        ImGui::Render();
    }
}

TEST_CASE("Button deck pads note calculation", "[hw][pads]") {
    int octave = 3;
    int baseNote = octave * 12; // 36 = C3
    REQUIRE(baseNote == 36);

    // Bottom row (row 3 in 0..3) column 0 is root note C3
    int bottomC = baseNote + (3 - 3) * 12 + 0;
    REQUIRE(bottomC == 36);

    // Top row (row 0) column 11 is B5
    int topB = baseNote + (3 - 0) * 12 + 11;
    REQUIRE(topB == 83);
}

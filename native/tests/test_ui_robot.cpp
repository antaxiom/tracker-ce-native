#include <catch2/catch_test_macros.hpp>
#include "UiRobot.h"
#include "views/TransportView.h"
#include "views/PatternView.h"
#include "views/InstrumentView.h"
#include "views/SampleView.h"
#include "views/MixerView.h"
#include "views/OrderListView.h"
#include "DockLayout.h"
#include "Theme.h"
#include "Song.h"
#include "History.h"
#include "EngineHost.h"
#include "AudioBackend.h"
#include "imgui.h"
#include "imgui_internal.h"

using namespace trakr;

namespace {

struct OffscreenImGuiFixture {
    ImGuiContext* ctx{nullptr};

    OffscreenImGuiFixture() {
        IMGUI_CHECKVERSION();
        ctx = ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigMacOSXBehaviors = false;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.DisplaySize = ImVec2(1280, 800);
        io.DeltaTime = 1.0f / 60.0f;
        io.ConfigDebugHighlightIdConflicts = true;

        unsigned char* pixels = nullptr;
        int width = 0, height = 0;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    }

    ~OffscreenImGuiFixture() {
        if (ctx) ImGui::DestroyContext(ctx);
    }
};

} // namespace

TEST_CASE("UiRobot Automated Testing and Duplicate ID Detection", "[ui_robot]") {
    OffscreenImGuiFixture fixture;

    SECTION("UiRobot correctly flags intentionally colliding IDs") {
        UiRobot robot;

        auto renderCollidingWindow = []() {
            ImGui::NewFrame();
            ImGui::SetNextWindowPos(ImVec2(50, 50));
            ImGui::SetNextWindowSize(ImVec2(300, 200));
            if (ImGui::Begin("CollisionTestWindow")) {
                // Two visible items with the exact same identifier in the same scope!
                ImGui::Button("DuplicateButton");
                ImGui::Button("DuplicateButton");
            }
            ImGui::End();
            ImGui::Render();
        };

        // Sweep over the collision test window
        robot.sweepWindow("CollisionTestWindow", 10.0f, renderCollidingWindow);
        const auto& rep = robot.getReport();

        // The robot MUST catch the conflicting identifier!
        REQUIRE(rep.idConflictsDetected > 0);
        REQUIRE(!rep.errors.empty());
    }

    SECTION("UiRobot exercises all tracker views and detects zero conflicts") {
        Song song = makeDefaultSong();
        History history;
        auto nullAudio = std::make_unique<NullAudioBackend>();
        EngineHost engineHost(std::move(nullAudio));
        engineHost.start(44100, 128);

        TransportView transportView;
        PatternView patternView;
        InstrumentView instrumentView;
        SampleView sampleView;
        MixerView mixerView;
        OrderListView orderListView;

        int octave = 4;
        int editStep = 1;
        bool recordMode = true;
        int currentOrderIdx = 0;
        int selectedInstIdx = 0;
        Theme theme = makeRenoiseDarkTheme();

        bool showPattern = true, showInstruments = true, showSample = true, showMixer = true, showOrder = true;

        auto renderAllViews = [&]() {
            ImGui::NewFrame();

            ImGuiID dockspace_id = ImGui::GetID("TestDockSpace");
            ImGui::DockSpaceOverViewport(dockspace_id, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

            static bool dockInitialized = false;
            if (!dockInitialized) {
                dockInitialized = true;
                buildDefaultDockLayout(dockspace_id, ImVec2(1280, 800));
            }

            transportView.update(song, history, engineHost, octave, editStep, recordMode,
                                 &showPattern, &showInstruments, &showSample, &showMixer, &showOrder);
            ImGui::SetNextWindowSize(ImVec2(800, 500), ImGuiCond_FirstUseEver);
            orderListView.update(song, history, engineHost, currentOrderIdx);
            ImGui::SetNextWindowSize(ImVec2(1000, 600), ImGuiCond_FirstUseEver);
            patternView.update(song, history, engineHost, currentOrderIdx, octave, editStep, recordMode, selectedInstIdx, &theme);
            ImGui::SetNextWindowSize(ImVec2(800, 500), ImGuiCond_FirstUseEver);
            instrumentView.update(song, history, engineHost, selectedInstIdx);
            ImGui::SetNextWindowSize(ImVec2(800, 500), ImGuiCond_FirstUseEver);
            sampleView.update(song, history, engineHost, selectedInstIdx);
            ImGui::SetNextWindowSize(ImVec2(800, 500), ImGuiCond_FirstUseEver);
            mixerView.update(song, history, engineHost);

            ImGui::Render();
        };

        UiRobot robot;
        UiConflictReport report = robot.runAutonomousSuite(song, history, engineHost,
                                                          octave, editStep, recordMode,
                                                          currentOrderIdx, selectedInstIdx,
                                                          &theme, renderAllViews);

        // Verification: The automated robot ran hundreds of frames and tested widgets
        REQUIRE(report.framesSimulated > 50);
        REQUIRE(report.widgetsTested > 200);

        // Crucial invariant: zero UI ID conflicts across all tracker views!
        if (!report.errors.empty()) {
            for (const auto& err : report.errors) {
                UNSCOPED_INFO(err);
            }
        }
        REQUIRE(report.idConflictsDetected == 0);
        REQUIRE(report.stackImbalancesDetected == 0);
        REQUIRE(report.numericalErrorsDetected == 0);
        REQUIRE(report.passed());
    }
}


#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <fcntl.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cmath>
#include <memory>

extern char** environ;

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlgpu3.h"

#include "Song.h"
#include "History.h"
#include "Engine.h"
#include "EngineHost.h"
#include "AudioBackend.h"
#include "MiniaudioBackend.h"
#include "SongJson.h"
#include "Render.h"
#include "PolyendImport.h"
#include "PolyendExport.h"
#include "FileDialogs.h"
#include "Theme.h"
#include "FontLoader.h"
#include "RenderScheduler.h"
#include "ShortcutMap.h"
#include "views/TransportView.h"
#include "views/PatternView.h"
#include "views/InstrumentView.h"
#include "views/SampleView.h"
#include "views/MixerView.h"
#include "views/OrderListView.h"
#include "views/FillModal.h"
#include "views/ExportDeck.h"
#include "DockLayout.h"
#include "MenuBar.h"
#include "UiRobot.h"
#include "MidiManager.h"
#include "DemoSong.h"
#include "ScreenManager.h"
#include "AppLayout.h"
#include "kit/KitGallery.h"
#include <cstdio>
#include <cstring>
#include <string>
#include <filesystem>

static ImGuiKey parseScriptKey(const std::string& name) {
    if (name == "Up") return ImGuiKey_UpArrow;
    if (name == "Down") return ImGuiKey_DownArrow;
    if (name == "Left") return ImGuiKey_LeftArrow;
    if (name == "Right") return ImGuiKey_RightArrow;
    if (name == "Enter") return ImGuiKey_Enter;
    if (name == "Escape" || name == "Esc") return ImGuiKey_Escape;
    if (name == "Space") return ImGuiKey_Space;
    if (name == "Tab") return ImGuiKey_Tab;
    if (name == "Backspace") return ImGuiKey_Backspace;
    if (name == "Delete" || name == "Del") return ImGuiKey_Delete;
    if (name == "Insert" || name == "Ins") return ImGuiKey_Insert;
    if (name == "Home") return ImGuiKey_Home;
    if (name == "End") return ImGuiKey_End;
    if (name == "PageUp") return ImGuiKey_PageUp;
    if (name == "PageDown") return ImGuiKey_PageDown;
    if (name == "Minus" || name == "-") return ImGuiKey_Minus;
    if (name == "Equal" || name == "=") return ImGuiKey_Equal;
    if (name == "Plus" || name == "+") return ImGuiKey_Equal; // shift not modeled here
    if (name.size() >= 2 && (name[0] == 'F' || name[0] == 'f')) {
        int n = std::atoi(name.c_str() + 1);
        if (n >= 1 && n <= 12) return static_cast<ImGuiKey>(ImGuiKey_F1 + (n - 1));
    }
    if (name.size() == 1) {
        char c = name[0];
        if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');
        if (c >= 'A' && c <= 'Z') return static_cast<ImGuiKey>(ImGuiKey_A + (c - 'A'));
        if (c >= '0' && c <= '9') return static_cast<ImGuiKey>(ImGuiKey_0 + (c - '0'));
    }
    return ImGuiKey_None;
}

// "Ctrl+Z"/"Shift+Up" style chords for scripted and remote input.
static unsigned int splitScriptMods(const std::string& name, std::string& base) {
    unsigned int mods = 0;
    base = name;
    for (;;) {
        if (base.rfind("Ctrl+", 0) == 0) { mods |= 1u; base = base.substr(5); }
        else if (base.rfind("Shift+", 0) == 0) { mods |= 2u; base = base.substr(6); }
        else if (base.rfind("Alt+", 0) == 0) { mods |= 4u; base = base.substr(4); }
        else break;
    }
    return mods;
}
static ImGuiKey scriptModKey(unsigned int bit) {
    // ImGui derives io.KeyCtrl/Shift/Alt from ImGuiMod_* key events (not the
    // physical keys). On macOS Cmd/Ctrl are swapped, so Super gives io.KeyCtrl.
    const bool mac = ImGui::GetIO().ConfigMacOSXBehaviors;
    if (bit == 1u) return static_cast<ImGuiKey>(mac ? ImGuiMod_Super : ImGuiMod_Ctrl);
    if (bit == 2u) return static_cast<ImGuiKey>(ImGuiMod_Shift);
    return static_cast<ImGuiKey>(ImGuiMod_Alt);
}

int main(int argc, char** argv) {
    bool smoke_test_mode = false, auto_test_mode = false, select_demo = false, start_wide = false, use_polyend_font = false, lcd_shot_mode = false, gui_next = true;
    std::string screenshot_path, init_screen, script_arg, remote_fifo, loader_dir, instrument_page, master_page, config_edit;
    for (int i = 1; i < argc; ++i) {
        if (!std::strcmp(argv[i], "--smoke-test")) smoke_test_mode = true;
        else if (!std::strcmp(argv[i], "--auto-test")) auto_test_mode = smoke_test_mode = true;
        else if (!std::strcmp(argv[i], "--wide")) start_wide = true;
        else if (!std::strcmp(argv[i], "--select-demo")) select_demo = true;
        else if (!std::strcmp(argv[i], "--export")) init_screen = "export";
        else if (!std::strcmp(argv[i], "--polyend-font")) use_polyend_font = true;
        else if (!std::strcmp(argv[i], "--lcd-shot")) lcd_shot_mode = true;
        else if (!std::strcmp(argv[i], "--gui-next")) gui_next = true;
        else if (!std::strcmp(argv[i], "--loader-dir") && i + 1 < argc) loader_dir = argv[++i];
        else if (!std::strcmp(argv[i], "--instrument-page") && i + 1 < argc) instrument_page = argv[++i];
        else if (!std::strcmp(argv[i], "--master-page") && i + 1 < argc) master_page = argv[++i];
        else if (!std::strcmp(argv[i], "--config-edit") && i + 1 < argc) config_edit = argv[++i];
        else if (!std::strcmp(argv[i], "--screenshot") && i + 1 < argc) screenshot_path = argv[++i];
        else if (!std::strcmp(argv[i], "--screen") && i + 1 < argc) init_screen = argv[++i];
        else if (!std::strcmp(argv[i], "--script") && i + 1 < argc) script_arg = argv[++i];
        else if (!std::strcmp(argv[i], "--remote") && i + 1 < argc) remote_fifo = argv[++i];
    }

    if (!SDL_Init(SDL_INIT_VIDEO)) { std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError()); return 1; }
    float scale = lcd_shot_mode ? 1.0f : SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
    if (scale <= 0.25f || scale > 4.0f) scale = 1.0f;
    // --lcd-shot: exact 800x480 framebuffer with no DPI scaling, so the frame IS the LCD.
    int winW = lcd_shot_mode ? 800 : static_cast<int>(1280 * scale);
    int winH = lcd_shot_mode ? 480 : static_cast<int>(800 * scale);
    // Automated captures (parity shots, scripts, remote operator, smoke tests)
    // must never steal focus from the user's other work. Screenshot/script runs
    // stay fully hidden; remote-operator runs stay visible but non-focusable.
    const bool hidden_run = lcd_shot_mode || !screenshot_path.empty() || !script_arg.empty() ||
                            smoke_test_mode || auto_test_mode;
    const bool automated_run = hidden_run || !remote_fifo.empty();
    // Interactive windows are also created hidden and shown after two frames:
    // ImGui only knows the real menu-bar work area on frame 2, so showing frame 1
    // would visibly rescale the whole UI once.
    bool defer_show = !automated_run;
    if (automated_run) {
        SDL_SetHint(SDL_HINT_WINDOW_ACTIVATE_WHEN_SHOWN, "0");
        SDL_SetHint(SDL_HINT_WINDOW_ACTIVATE_WHEN_RAISED, "0");
    }
    SDL_WindowFlags winFlags = SDL_WINDOW_RESIZABLE | (lcd_shot_mode ? 0 : SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (hidden_run || defer_show) winFlags |= SDL_WINDOW_HIDDEN;
    if (automated_run) winFlags |= SDL_WINDOW_NOT_FOCUSABLE;
    SDL_Window* window = SDL_CreateWindow("Trakr", winW, winH, winFlags);
    if (!window) { std::fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError()); return 1; }
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    if (!automated_run && !defer_show) { SDL_ShowWindow(window); SDL_RaiseWindow(window); }
    else if (automated_run && !hidden_run) { SDL_ShowWindow(window); }

    SDL_GPUDevice* gpu_device = SDL_CreateGPUDevice(
        SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_MSL | SDL_GPU_SHADERFORMAT_METALLIB, true, nullptr);
    if (!gpu_device) { std::fprintf(stderr, "SDL_CreateGPUDevice failed: %s\n", SDL_GetError()); SDL_DestroyWindow(window); SDL_Quit(); return 1; }
    if (!SDL_ClaimWindowForGPUDevice(gpu_device, window)) { std::fprintf(stderr, "SDL_ClaimWindowForGPUDevice failed: %s\n", SDL_GetError()); SDL_DestroyGPUDevice(gpu_device); SDL_DestroyWindow(window); SDL_Quit(); return 1; }
    SDL_SetGPUSwapchainParameters(gpu_device, window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_VSYNC);

    // Dear ImGui setup
    IMGUI_CHECKVERSION(); ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable;

    trakr::Theme theme = trakr::makeTrackerClassicTheme();
    trakr::applyTheme(theme, ImGui::GetStyle());
    ImGui::GetStyle().ScaleAllSizes(scale);

    ImGui_ImplSDL3_InitForSDLGPU(window);
    ImGui_ImplSDLGPU3_InitInfo init_info = {.Device = gpu_device, .ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(gpu_device, window), .SwapchainComposition = SDL_GPU_SWAPCHAINCOMPOSITION_SDR, .PresentMode = SDL_GPU_PRESENTMODE_VSYNC};
    ImGui_ImplSDLGPU3_Init(&init_info);

    trakr::AppFonts fonts = trakr::loadFonts(scale, use_polyend_font, lcd_shot_mode);
    trakr::Song song = trakr::makeDefaultSong();
    trakr::populateDemoSong(song);
    trakr::History history;

    // Headless parity/screenshot runs do not need a real audio device; skipping
    // CoreAudio makes automated launches faster and side-effect free.
    const bool use_null_audio = smoke_test_mode || hidden_run || !remote_fifo.empty();
    auto audioBackend = use_null_audio
                            ? std::unique_ptr<trakr::AudioBackend>(std::make_unique<trakr::NullAudioBackend>())
                            : std::unique_ptr<trakr::AudioBackend>(std::make_unique<trakr::MiniaudioBackend>());
    trakr::EngineHost engineHost(std::move(audioBackend));
    engineHost.start(44100, 256);
    engineHost.publish(song);

    // Application state
    trakr::RenderScheduler scheduler;
    trakr::ShortcutMap shortcuts;

    trakr::TransportView transportView; trakr::PatternView patternView;
    trakr::InstrumentView instrumentView; trakr::SampleView sampleView;
    trakr::MixerView mixerView; trakr::OrderListView orderListView;
    trakr::FillModal fillModal; trakr::ExportDeck exportDeck; trakr::PerformView performView; trakr::SampleLoaderView sampleLoaderView; trakr::ConfigView configView; trakr::FileView fileView;
    int currentOrderIdx = 0, octave = 3, editStep = 1, selectedInstIdx = 0;
    bool recordMode = false;
    bool showFillModal = false, showKitGallery = false, resetLayoutRequested = false, showShortcutsModal = false, showExportDeck = false;
    trakr::Screen activeScreen = (init_screen == "song") ? trakr::Screen::Song :
        (init_screen == "sample") ? trakr::Screen::Sample : (init_screen == "instrument") ? trakr::Screen::Instrument :
        (init_screen == "master" || init_screen == "fill_master") ? trakr::Screen::Master :
        (init_screen == "perform") ? trakr::Screen::Perform : (init_screen == "loader") ? trakr::Screen::SampleLoader :
        (init_screen == "config") ? trakr::Screen::Config : (init_screen == "file") ? trakr::Screen::File : trakr::Screen::Pattern;
    if (init_screen == "master" || init_screen == "fill_master") {
        for (size_t i = 0; i < song.tracks.size(); ++i) {
            auto& t = song.tracks[i];
            t.gainDb = 6.0f; // fwref scenario trackVolume = 100%
            t.pan = 0.0f;
            t.name = "Track " + std::to_string(i + 1); // scenario trackNames
        }
    } else if (init_screen == "config") {
        configView.setActiveGroup(trakr::ConfigView::Group::ProjectSettings);
    } else if (init_screen == "pattern") {
        for (auto& trk : song.patterns[0].tracks) for (auto& stp : trk) { stp.note = -1; stp.instrument = -1; stp.fx[0] = {}; stp.fx[1] = {}; }
        // The firmware reference uses the default track names (mtStructs.cpp
        // setDefaultTrackName); the demo song carries sample-oriented names.
        for (size_t i = 0; i < song.tracks.size(); ++i) {
            song.tracks[i].name = (i < 8) ? ("Track " + std::to_string(i + 1))
                                         : ("Midi " + std::to_string(i + 1));
        }
        // Parity reference frame starts with the cursor on row 1 (kept at the
        // centre row; rows above are blank).
        patternView.setCursor(0, 0, 0);
    } else if (init_screen == "loader") {
        sampleLoaderView.setParityState(); song.instruments.resize(48);
        const char* kDemo[8] = {"909 Kick", "909 Snare", "909 Clap", "909 ClosedHat",
                                "909 OpenHat", "909 LowTom", "909 MidTom", "909 HiTom"};
        for (int i = 0; i < 8; ++i) song.instruments[i].name = kDemo[i];
        for (int i = 8; i < 48; ++i) song.instruments[i].name = "---";
    } else if (init_screen == "file") {
        fileView.setParityState();
    } else if (init_screen == "song") {
        // Mirror the canonical fwref song scenario: 16 track columns (8 pattern
        // + 8 MIDI), the canonical 14-slot playlist, tempo 130, and per-pattern
        // track occupancy matching normalPatternMask [15,19,60,1] so
        // cSongPlayer::fillBlocks draws the same blocks the reference does.
        song.bpm = 130.0f;
        song.tracks.resize(16);
        for (int i = 0; i < 16; ++i) {
            song.tracks[i].name = (i < 8) ? ("Track " + std::to_string(i + 1))
                                          : ("Midi " + std::to_string(i + 1));
            song.tracks[i].mute = (i == 2 || i == 9); // scenario normalMuted[2], midiMuted[1]
        }
        while (static_cast<int>(song.patterns.size()) < 4) {
            trakr::Pattern p;
            p.rows = 64;
            song.patterns.push_back(std::move(p));
        }
        static const unsigned kMask[4] = {15u, 19u, 60u, 1u};
        for (int pi = 0; pi < 4; ++pi) {
            trakr::Pattern& pat = song.patterns[pi];
            pat.rows = 64;
            pat.tracks.assign(16, std::vector<trakr::Cell>(64));
            for (auto& trk : pat.tracks)
                for (auto& cl : trk) { cl = trakr::Cell{}; cl.note = -1; cl.instrument = -1; }
            for (int t = 0; t < 8; ++t)
                if (kMask[pi] & (1u << t)) { pat.tracks[t][0].note = 48; pat.tracks[t][0].instrument = 0; }
            // midiPatternMask [2]: pattern 0, midi track index 1 (overall track 9,
            // which is muted per midiMuted[1]) -> renders as a muted grey_44 block.
            if (pi == 0)
                for (int m = 0; m < 8; ++m)
                    if (2u & (1u << m)) { pat.tracks[8 + m][0].note = 48; pat.tracks[8 + m][0].instrument = 0; }
        }
        song.order = {0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3};
    } else if (init_screen == "sample") {
        // Synthetic parity sample mirroring the fwref generator
        // (scenario_sample.cpp buildSampleWaveform): amp = round(100*|sin(0.11*i)|*exp(-0.004*i))
        // over 300 points. Encode it as a bipolar signal so the spectrum's
        // per-column min/max reproduces the decaying envelope, and place the
        // start/end/loop/slice markers at the same 600-unit positions.
        auto sd = std::make_shared<trakr::SampleData>();
        sd->channels = 1;
        sd->sampleRate = 2652; // 240 frames ~= 0.089s, 3480 ~= 1.312s
        const int n = 3600;    // 600 px * 6 samples/column
        sd->ch[0].resize(static_cast<size_t>(n));
        for (int j = 0; j < n; ++j) {
            const int k = std::min(299, j / 12);
            const double env = std::exp(-0.004 * k);
            const double wv = std::sin(0.11 * k);
            // Firmware sample units: int8 peak = int16 / 300
            // (graphicProcessing.cpp:478), so the normalized demo sample uses
            // 300/32767 per peak unit to keep the reference envelope identical.
            const double a = 100.0 * std::fabs(wv) * env * 300.0 / 32767.0;
            sd->ch[0][static_cast<size_t>(j)] = static_cast<float>((j % 2) ? -a : a);
        }
        auto& inst = song.instruments[0];
        inst.sample = sd;
        // Frames chosen so each second value rounds to the reference string
        // (0.089s / 0.336s / 1.008s / 1.312s at 2652 Hz).
        inst.startFrame = 236;
        inst.endFrame = 3480;
        inst.loopStart = 891;
        inst.loopEnd = 2673;
        inst.loop = trakr::Instrument::Loop::Forward;
        inst.slices = {600, 1500, 2400, 3300};
        inst.wavetableWindow = 32;
        inst.wavetablePos = 0.0f;
    }
    if (!instrument_page.empty()) {
        if (instrument_page == "effects")
            instrumentView.setSubPage(trakr::InstrumentView::SubPage::Effects);
        else if (instrument_page == "automation")
            instrumentView.setSubPage(trakr::InstrumentView::SubPage::Automation);
        else if (instrument_page == "general")
            instrumentView.setSubPage(trakr::InstrumentView::SubPage::General);
    }
    if (!master_page.empty()) {
        if (master_page == "eq")
            mixerView.setSubPage(trakr::MixerView::SubPage::MasterFx);
        else if (master_page == "global")
            mixerView.setSubPage(trakr::MixerView::SubPage::GlobalMixer);
        else if (master_page == "mixer")
            mixerView.setSubPage(trakr::MixerView::SubPage::TrackMixer);
        else if (master_page == "master")
            mixerView.setSubPage(trakr::MixerView::SubPage::Master);
    }
    if (!config_edit.empty()) configView.requestEditor(config_edit);
    if (!loader_dir.empty()) sampleLoaderView.setDirectory(loader_dir);
    if (init_screen == "fill" || init_screen == "fill_master") showFillModal = true;
    if (init_screen == "export") showExportDeck = true;
    if (select_demo) patternView.setSelection(3, 0, 3, 12);
    trakr::AppLayout appLayout;
    trakr::AppLayoutState layoutState{start_wide, true, false};
    layoutState.guiNext = gui_next;
    if (lcd_shot_mode) {
        // 1:1 LCD capture: no button deck, no wide mode, no menu bar.
        layoutState.showButtonDeck = false;
        layoutState.wideMode = false;
    }
    std::string currentFilePath, dlgOpen, dlgSave, dlgWav, dlgStems, dlgPolyExport, dlgPatternWav, dlgPatternStems;

    // File page commands reuse the in-app SD-card style dialogs.
    fileView.setActions(
        [&]() { trakr::dialogs::showOpenSong(window, &dlgOpen, &scheduler); },
        [&]() {
            if (!currentFilePath.empty()) trakr::saveSongJson(song, currentFilePath);
            else trakr::dialogs::showSaveSong(window, &dlgSave, &scheduler);
        },
        [&]() { trakr::dialogs::showSaveSong(window, &dlgSave, &scheduler); },
        // New Project: drop the open path so the next Save asks for a file.
        [&]() { currentFilePath.clear(); patternView.setCursor(0, 0, 0); engineHost.setPosition(0, 0); });

    auto handleImport = [&](const std::string& p) {
        std::string ext = std::filesystem::path(p).extension().string();
        trakr::Instrument inst; trakr::Pattern pat; trakr::Song poly;
        if (ext == ".pti" && trakr::io::loadPtiInstrument(p, inst)) { song.instruments.push_back(std::move(inst)); engineHost.publish(song); }
        else if (ext == ".mtp" && trakr::io::loadMtpPattern(p, pat)) { song.patterns.push_back(std::move(pat)); song.order.push_back(static_cast<int>(song.patterns.size() - 1)); engineHost.publish(song); }
        else if (trakr::io::loadPolyendProject(p, poly)) { song = std::move(poly); engineHost.publish(song); }
    };

    if (auto_test_mode) {
        std::printf("[UI ROBOT] Autonomous UI testing and conflict audit started...\n");
        trakr::UiRobot robot;
        auto onFrame = [&]() {
            ImGui_ImplSDLGPU3_NewFrame(); ImGui_ImplSDL3_NewFrame(); ImGui::NewFrame();
            ImGuiID dock_id = ImGui::GetID("TrakrRootDockSpace");
            ImGui::DockSpaceOverViewport(dock_id, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
            static bool dInit = false;
            if (!dInit) { dInit = true; trakr::buildDefaultDockLayout(dock_id, ImVec2(1280, 800)); }
            transportView.update(song, history, engineHost, octave, editStep, recordMode);
            orderListView.update(song, history, engineHost, currentOrderIdx);
            patternView.update(song, history, engineHost, currentOrderIdx, octave, editStep, recordMode, selectedInstIdx, &theme);
            instrumentView.update(song, history, engineHost, selectedInstIdx);
            sampleView.update(song, history, engineHost, selectedInstIdx);
            mixerView.update(song, history, engineHost); performView.update(song, history, engineHost);
            ImGui::Render();
        };
        auto rep = robot.runAutonomousSuite(song, history, engineHost, octave, editStep, recordMode,
                                           currentOrderIdx, selectedInstIdx, &theme, onFrame);
        trakr::hw::ButtonDeck testDeck;
        bool testShift = false;
        ImRect testDeckRect(ImVec2(0, 500), ImVec2(1280, 800));
        auto onDeckFrame = [&]() {
            ImGui_ImplSDLGPU3_NewFrame(); ImGui_ImplSDL3_NewFrame(); ImGui::NewFrame();
            ImGuiIO& io = ImGui::GetIO();
            io.MousePos = robot.getMousePos();
            io.MouseDown[0] = robot.isMouseDown(0);
            if (robot.isMouseDown(0)) io.MouseClicked[0] = true;
            if (robot.getMouseWheel() != 0.0f) io.MouseWheel = robot.getMouseWheel();
            testDeck.render(testDeckRect, song, engineHost, history, patternView, activeScreen,
                            recordMode, octave, editStep, testShift, selectedInstIdx, currentOrderIdx, 1.0f);
            ImGui::Render();
        };
        robot.runHardwareDeckVerification(song, history, engineHost, testDeck, patternView, octave, editStep,
                                          recordMode, testShift, selectedInstIdx, currentOrderIdx,
                                          testDeckRect, onDeckFrame);
        rep = robot.getReport();
        std::printf("[UI ROBOT] Completed %d simulated frames and %d widget checks across 6 views.\n", rep.framesSimulated, rep.widgetsTested);
        if (rep.passed()) std::printf("[UI ROBOT AUDIT PASSED] 0 ID conflicts, 0 stack errors, 0 numerical errors.\n");
        else {
            std::printf("[UI ROBOT AUDIT FAILED] %d conflicts, %d stack errors, %zu failures:\n",
                        rep.idConflictsDetected, rep.stackImbalancesDetected, rep.errors.size());
            for (const auto& err : rep.errors) std::printf("  -> %s\n", err.c_str());
        }
    }

    std::vector<ImGuiKey> script_keys;
    std::vector<unsigned int> script_mods;
    if (!script_arg.empty()) {
        size_t start = 0;
        while (start <= script_arg.size()) {
            size_t comma = script_arg.find(',', start);
            std::string tok = script_arg.substr(start, comma == std::string::npos ? std::string::npos : comma - start);
            if (!tok.empty()) {
                std::string base;
                unsigned int mods = splitScriptMods(tok, base);
                ImGuiKey k = parseScriptKey(base);
                if (k != ImGuiKey_None) { script_keys.push_back(k); script_mods.push_back(mods); }
            }
            if (comma == std::string::npos) break;
            start = comma + 1;
        }
    }
    const int kScriptBase = 20, kScriptStep = 8;
    int script_frame = 0, script_capture_step = -1, script_shots_done = 0;
    bool script_done = false;

    int remote_fd = -1;
    std::string remote_buf, remote_shot_path;
    ImGuiKey remote_pending_key = ImGuiKey_None;
    unsigned int remote_pending_mods = 0;
    float remote_click_x = 400.0f, remote_click_y = 240.0f;
    std::string remote_type;
    int remote_click_phase = 0;
    int remote_key_phase = 0; // 1 = press queued, 2 = release due
    if (!remote_fifo.empty()) {
        remote_fd = ::open(remote_fifo.c_str(), O_RDONLY | O_NONBLOCK);
        if (remote_fd < 0) std::fprintf(stderr, "[remote] cannot open fifo %s\n", remote_fifo.c_str());
    }

    bool running = !auto_test_mode;
    while (running) {
        SDL_Event event;
        bool hasEvent = scheduler.waitEvent(&event);
        if (hasEvent) {
            do {
                ImGui_ImplSDL3_ProcessEvent(&event);
                if (event.type == SDL_EVENT_QUIT || (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window))) running = false;
                if (event.type == SDL_EVENT_DROP_FILE && event.drop.data) handleImport(event.drop.data);
                if (event.type == SDL_EVENT_KEY_DOWN && (event.key.key == SDLK_ESCAPE || ((event.key.mod & SDL_KMOD_GUI) && event.key.key == SDLK_Q))) running = false;
                scheduler.requestRedraw(6);
            } while (SDL_PollEvent(&event));
        }

        if (!dlgOpen.empty()) { trakr::Song s; if (trakr::loadSongJson(s, dlgOpen)) { song = std::move(s); history.clear(); engineHost.publish(song); currentFilePath = dlgOpen; } dlgOpen.clear(); }
        if (!dlgSave.empty()) { trakr::saveSongJson(song, dlgSave); currentFilePath = dlgSave; dlgSave.clear(); }
        if (!dlgWav.empty()) { trakr::renderToWav(song, dlgWav); dlgWav.clear(); }
        if (!dlgStems.empty()) { trakr::renderStemsToWav(song, dlgStems + "/stem"); dlgStems.clear(); }
        if (!dlgPolyExport.empty()) { trakr::io::exportPolyendProject(dlgPolyExport, song); dlgPolyExport.clear(); }
        if (!dlgPatternWav.empty() || !dlgPatternStems.empty()) {
            if (currentOrderIdx >= 0 && currentOrderIdx < static_cast<int>(song.order.size())) {
                int pi = song.order[currentOrderIdx];
                if (pi >= 0 && pi < static_cast<int>(song.patterns.size())) {
                    trakr::Song single = song;
                    single.patterns = {song.patterns[pi]};
                    single.order = {0};
                    if (!dlgPatternWav.empty()) trakr::renderToWav(single, dlgPatternWav);
                    if (!dlgPatternStems.empty()) trakr::renderStemsToWav(single, dlgPatternStems + "/stem");
                }
            }
            dlgPatternWav.clear();
            dlgPatternStems.clear();
        }

        bool isPlaying = engineHost.getPlayhead().playing;
        scheduler.setPlaying(isPlaying);
        if (!scheduler.shouldRedraw() && !smoke_test_mode && screenshot_path.empty() && remote_fd < 0) continue;
        if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED) { SDL_Delay(10); continue; }

        // Global shortcuts. Skip them while an InputText has focus (the in-app
        // save/open dialogs) so typing never toggles transport or undoes.
        const bool textInputFocused = ImGui::GetIO().WantTextInput;
        if (!textInputFocused && shortcuts.isTriggered(trakr::ActionId::PlayStop)) {
            if (activeScreen == trakr::Screen::Sample) {
                if (isPlaying) engineHost.stopPlayback();
                else if (selectedInstIdx >= 0 && selectedInstIdx < static_cast<int>(song.instruments.size())) {
                    engineHost.previewNoteOn(-1, selectedInstIdx, song.instruments[selectedInstIdx].rootNote, 0.9f);
                    sampleView.triggerAudition();
                }
                scheduler.requestRedraw(60);
            } else {
                if (isPlaying) {
                    engineHost.stopPlayback();
                    engineHost.setPosition(currentOrderIdx, 0);
                } else {
                    engineHost.play(currentOrderIdx, 0);
                }
                scheduler.requestRedraw();
            }
        }
        if (!textInputFocused && shortcuts.isTriggered(trakr::ActionId::Stop)) {
            engineHost.allNotesOff();
            sampleView.stopAudition(engineHost);
            scheduler.requestRedraw();
        }
        if (sampleView.isAuditioning()) { scheduler.requestRedraw(2); }
        if (!textInputFocused && shortcuts.isTriggered(trakr::ActionId::RecordToggle)) { recordMode = !recordMode; scheduler.requestRedraw(); }
        if (!textInputFocused && shortcuts.isTriggered(trakr::ActionId::NewProject)) {
            song = trakr::makeDefaultSong(); history.clear(); currentFilePath.clear(); patternView.setCursor(0, 0, 0); engineHost.publish(song); scheduler.requestRedraw();
        }
        if (!textInputFocused && shortcuts.isTriggered(trakr::ActionId::OpenProject)) { trakr::dialogs::showOpenSong(window, &dlgOpen, &scheduler); }
        if (!textInputFocused && shortcuts.isTriggered(trakr::ActionId::Fill)) { showFillModal = true; scheduler.requestRedraw(); }
        if (!textInputFocused && shortcuts.isTriggered(trakr::ActionId::ExportDeck)) { showExportDeck = !showExportDeck; scheduler.requestRedraw(); }
        if (!textInputFocused && shortcuts.isTriggered(trakr::ActionId::Undo) && history.canUndo()) { history.undo(song); engineHost.publish(song); scheduler.requestRedraw(); }
        if (!textInputFocused && shortcuts.isTriggered(trakr::ActionId::Redo) && history.canRedo()) { history.redo(song); engineHost.publish(song); scheduler.requestRedraw(); }
        if (!textInputFocused && shortcuts.isTriggered(trakr::ActionId::Save)) { if (!currentFilePath.empty()) trakr::saveSongJson(song, currentFilePath); else trakr::dialogs::showSaveSong(window, &dlgSave, &scheduler); }
        if (!textInputFocused && shortcuts.isTriggered(trakr::ActionId::SaveAs)) { trakr::dialogs::showSaveSong(window, &dlgSave, &scheduler); }

        // MIDI Event Processing
        trakr::MidiEvent mEv;
        while (trakr::MidiManager::instance().pollEvent(mEv)) {
            if (mEv.type == trakr::MidiEvent::Type::NoteOn && mEv.velocity > 0) {
                engineHost.previewNoteOn(patternView.cursorTrack(), selectedInstIdx, mEv.note, mEv.velocity / 127.0f);
                if (recordMode) {
                    int curPat = (currentOrderIdx >= 0 && currentOrderIdx < static_cast<int>(song.order.size())) ? song.order[currentOrderIdx] : 0;
                    if (curPat >= 0 && curPat < static_cast<int>(song.patterns.size())) {
                        auto& p = song.patterns[curPat];
                        int trk = patternView.cursorTrack(), row = patternView.cursorRow();
                        if (trk < static_cast<int>(p.tracks.size()) && row < static_cast<int>(p.tracks[trk].size())) {
                            trakr::Cell oldCell = p.tracks[trk][row], newCell = oldCell;
                            newCell.note = mEv.note;
                            newCell.instrument = static_cast<int16_t>(selectedInstIdx);
                            newCell.volume = static_cast<int16_t>(mEv.velocity / 2);
                            history.execute(std::make_unique<trakr::SetCellCommand>(curPat, trk, row, newCell, oldCell), song);
                            engineHost.publish(song);
                            patternView.setCursor(trk, std::min(p.rows - 1, row + editStep), patternView.cursorSubCol());
                            scheduler.requestRedraw(6);
                        }
                    }
                }
            } else if (mEv.type == trakr::MidiEvent::Type::NoteOff || (mEv.type == trakr::MidiEvent::Type::NoteOn && mEv.velocity == 0)) {
                engineHost.previewNoteOff(patternView.cursorTrack());
            }
        }
        if (!textInputFocused && shortcuts.isTriggered(trakr::ActionId::OctaveUp) && octave < 8) octave++;
        if (!textInputFocused && shortcuts.isTriggered(trakr::ActionId::OctaveDown) && octave > 0) octave--;
        if (!textInputFocused && shortcuts.isTriggered(trakr::ActionId::ToggleKitGallery)) { showKitGallery = !showKitGallery; scheduler.requestRedraw(); }
        if (!textInputFocused && shortcuts.isTriggered(trakr::ActionId::ResetLayout)) { resetLayoutRequested = true; scheduler.requestRedraw(); }
        script_frame++;
        if (remote_fd >= 0) {
            char rbuf[512];
            ssize_t rn;
            while ((rn = ::read(remote_fd, rbuf, sizeof(rbuf))) > 0) remote_buf.append(rbuf, static_cast<size_t>(rn));
            size_t nl;
            while ((nl = remote_buf.find('\n')) != std::string::npos) {
                std::string line = remote_buf.substr(0, nl);
                remote_buf.erase(0, nl + 1);
                if (line.rfind("key ", 0) == 0) {
                    std::string base;
                    unsigned int mods = splitScriptMods(line.substr(4), base);
                    ImGuiKey k = parseScriptKey(base);
                    if (k != ImGuiKey_None) { remote_pending_key = k; remote_pending_mods = mods; remote_key_phase = 1; }
                } else if (line.rfind("shot ", 0) == 0) {
                    remote_shot_path = line.substr(5);
                } else if (line.rfind("click ", 0) == 0) {
                    std::sscanf(line.c_str() + 6, "%f %f", &remote_click_x, &remote_click_y);
                    remote_click_phase = 1;
                } else if (line.rfind("type ", 0) == 0) {
                    remote_type = line.substr(5);
                } else if (line == "quit") {
                    running = false;
                }
            }
            ImGuiIO& rio = ImGui::GetIO();
            if (!remote_type.empty()) {
                rio.AddInputCharactersUTF8(remote_type.c_str());
                remote_type.clear();
            }
            // Hold the last remote pointer position so menus/submenus stay hovered.
            rio.AddMousePosEvent(remote_click_x, remote_click_y);
            if (remote_click_phase == 1) {
                rio.AddMousePosEvent(remote_click_x, remote_click_y);
                remote_click_phase = 2;
            } else if (remote_click_phase == 2) {
                rio.AddMousePosEvent(remote_click_x, remote_click_y);
                rio.AddMouseButtonEvent(0, true);
                remote_click_phase = 3;
            } else if (remote_click_phase == 3) {
                rio.AddMousePosEvent(remote_click_x, remote_click_y);
                rio.AddMouseButtonEvent(0, false);
                remote_click_phase = 0;
            }
            if (remote_pending_key != ImGuiKey_None) {
                if (remote_key_phase == 1) {
                    for (unsigned int b = 1; b <= 4; b <<= 1)
                        if (remote_pending_mods & b) rio.AddKeyEvent(scriptModKey(b), true);
                    rio.AddKeyEvent(remote_pending_key, true);
                    remote_key_phase = 2;
                } else {
                    rio.AddKeyEvent(remote_pending_key, false);
                    for (unsigned int b = 1; b <= 4; b <<= 1)
                        if (remote_pending_mods & b) rio.AddKeyEvent(scriptModKey(b), false);
                    remote_pending_key = ImGuiKey_None;
                    remote_pending_mods = 0;
                    remote_key_phase = 0;
                }
            }
        }
        if (!script_keys.empty()) {
            ImGuiIO& script_io = ImGui::GetIO();
            script_io.AddMousePosEvent(400.0f, 240.0f); // keep hover over the screen each frame
            if (script_frame == kScriptBase - 8) script_capture_step = 0; // initial frame
            for (size_t i = 0; i < script_keys.size(); ++i) {
                int press = kScriptBase + static_cast<int>(i) * kScriptStep;
                if (script_frame == press) {
                    for (unsigned int b = 1; b <= 4; b <<= 1)
                        if (script_mods[i] & b) script_io.AddKeyEvent(scriptModKey(b), true);
                    script_io.AddKeyEvent(script_keys[i], true);
                }
                if (script_frame == press + 1) {
                    script_io.AddKeyEvent(script_keys[i], false);
                    for (unsigned int b = 1; b <= 4; b <<= 1)
                        if (script_mods[i] & b) script_io.AddKeyEvent(scriptModKey(b), false);
                }
                if (script_frame == press + 4) script_capture_step = static_cast<int>(i) + 1;
            }
            if (script_frame > kScriptBase + static_cast<int>(script_keys.size()) * kScriptStep + 8) script_done = true;
        }
        ImGui_ImplSDLGPU3_NewFrame(); ImGui_ImplSDL3_NewFrame(); ImGui::NewFrame();

        // Menu Bar
        auto activePattern = [&]() -> trakr::Pattern* {
            if (currentOrderIdx < 0 || currentOrderIdx >= static_cast<int>(song.order.size())) return nullptr;
            int pi = song.order[currentOrderIdx];
            if (pi < 0 || pi >= static_cast<int>(song.patterns.size())) return nullptr;
            return &song.patterns[pi];
        };
        int menuPatIdx = (currentOrderIdx >= 0 && currentOrderIdx < static_cast<int>(song.order.size())) ? song.order[currentOrderIdx] : -1;
        trakr::MenuBarContext menuCtx{window, song, history, engineHost, shortcuts, scheduler, theme, running,
                                      showKitGallery, showFillModal, showShortcutsModal, showExportDeck, resetLayoutRequested,
                                      layoutState.showButtonDeck, layoutState.wideMode, recordMode,
                                      !patternView.getClipboard().empty(),
                                      currentFilePath, dlgOpen, dlgSave, dlgWav, dlgStems, dlgPolyExport,
                                      dlgPatternWav, dlgPatternStems};
        menuCtx.editCopy = [&]() { if (auto* pat = activePattern()) patternView.copySelectionOrCell(*pat); };
        menuCtx.editCut = [&]() {
            if (auto* pat = activePattern()) {
                patternView.copySelectionOrCell(*pat);
                patternView.deleteRowOrCellAtCursor(*pat, menuPatIdx, song, history, engineHost, false);
            }
        };
        menuCtx.editPaste = [&]() {
            if (auto* pat = activePattern()) patternView.pasteAtCursor(*pat, menuPatIdx, song, history, engineHost);
        };
        menuCtx.resetCursor = [&]() { patternView.setCursor(0, 0, 0); };
        if (!lcd_shot_mode) trakr::renderMainMenuBar(menuCtx);

        // Screen switching helper with clean note silencing
        auto switchScreen = [&](trakr::Screen s) {
            if (activeScreen != s) {
                engineHost.allNotesOff(); sampleView.stopAudition(engineHost);
                activeScreen = s; scheduler.requestRedraw();
            }
        };

        if (ImGui::GetIO().KeyAlt || ImGui::GetIO().KeyCtrl) {
            for (int k = 0; k < 6; ++k)
                if (ImGui::IsKeyPressed(static_cast<ImGuiKey>(ImGuiKey_1 + k), false)) switchScreen(static_cast<trakr::Screen>(k));
        }
        if (ImGui::IsKeyPressed(ImGuiKey_F9, false)) { layoutState.showButtonDeck = !layoutState.showButtonDeck; scheduler.requestRedraw(); }
        if (ImGui::IsKeyPressed(ImGuiKey_F10, false)) { layoutState.wideMode = !layoutState.wideMode; scheduler.requestRedraw(); }

        // Unified Display Framework (800x480 Canvas, HeaderBar, ScreenButtons, ButtonDeck)
        if (activeScreen == trakr::Screen::File && !currentFilePath.empty()) {
            fileView.setProjectName(std::filesystem::path(currentFilePath).stem().string());
        }
        // Capture runs freeze the semantic animation clock so lcd shots,
        // screenshots and scripted captures stay deterministic.
        const bool freezeAnimation =
            lcd_shot_mode || !screenshot_path.empty() || smoke_test_mode || auto_test_mode;
        appLayout.setAnimationClock(freezeAnimation ? 0.0 : ImGui::GetTime() * 1000.0,
                                    freezeAnimation);

        trakr::ScreenContext scrCtx{song, history, engineHost, currentOrderIdx, octave, editStep,
                                   recordMode, selectedInstIdx, theme, patternView, orderListView,
                                   sampleView, instrumentView, mixerView, performView, sampleLoaderView, configView, fileView};
        appLayout.render(scrCtx, activeScreen, layoutState, showFillModal, showExportDeck,
                         showKitGallery, showShortcutsModal, currentFilePath, scheduler);

        // Modals (Overlay cleanly above screen when open)
        int curPat = (currentOrderIdx >= 0 && currentOrderIdx < static_cast<int>(song.order.size())) ? song.order[currentOrderIdx] : 0;
        if (curPat < 0 || curPat >= static_cast<int>(song.patterns.size())) curPat = 0;
        float modalFooterH = layoutState.showButtonDeck ? 260.0f : 56.0f;
        if (showFillModal) fillModal.draw(&showFillModal, song.patterns[curPat], curPat, patternView.cursorTrack(),
                                         selectedInstIdx, history, song, modalFooterH, patternView.hasSelection(), patternView.getSelection());
        if (showExportDeck) exportDeck.draw(&showExportDeck, song, modalFooterH);
        if (showKitGallery) trakr::kit::renderKitGallery(&showKitGallery);

        if (showShortcutsModal) {
            ImGui::OpenPopup("Shortcuts");
            if (ImGui::BeginPopupModal("Shortcuts", &showShortcutsModal, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::TextUnformatted("Screen Buttons: F1..F8 (Shift+F1..F8)\nSwitch Pages: Alt+1..6 | F9: Button Deck | F10: Wide Mode\nSpace: Play/Stop | Rec / Ctrl+R: Toggle Rec Mode\nZ..M: Piano Keys | A / 1: Note Off");
                if (ImGui::Button("Close", ImVec2(100, 25))) { ImGui::CloseCurrentPopup(); showShortcutsModal = false; }
                ImGui::EndPopup();
            }
        }

        ImGui::Render();
        ImDrawData* draw_data = ImGui::GetDrawData();
        const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);

        SDL_GPUCommandBuffer* cmd_buffer = SDL_AcquireGPUCommandBuffer(gpu_device);
        if (smoke_test_mode && screenshot_path.empty()) {
            SDL_CancelGPUCommandBuffer(cmd_buffer);
            static int test_frames = 15;
            if (--test_frames <= 0) {
                std::printf("[SMOKE TEST SUCCESS] Verified ImGui UI loops and engine integration.\n");
                running = false;
            }
            continue;
        }

        SDL_GPUTexture* swapchain_texture = nullptr;
        Uint32 swapchain_w = 0, swapchain_h = 0;
        bool has_tex = SDL_AcquireGPUSwapchainTexture(cmd_buffer, window, &swapchain_texture, &swapchain_w, &swapchain_h);

        if (!has_tex || !swapchain_texture || is_minimized) {
            SDL_CancelGPUCommandBuffer(cmd_buffer);
            SDL_Delay(16);
            if (smoke_test_mode) running = false;
            continue;
        }

        ImGui_ImplSDLGPU3_PrepareDrawData(draw_data, cmd_buffer);

        static int frame_count = 0; frame_count++;
        const bool remote_capture = !remote_shot_path.empty();
        bool capture_this_frame = remote_capture || (script_keys.empty()
            ? (!screenshot_path.empty() && frame_count >= 6)
            : (script_capture_step >= 0));
        std::string out_png = remote_capture ? remote_shot_path : screenshot_path;
        if (capture_this_frame && !script_keys.empty()) {
            std::string base = screenshot_path;
            size_t dot = base.rfind(".png");
            if (dot != std::string::npos) base = base.substr(0, dot);
            char buf[32]; std::snprintf(buf, sizeof(buf), "_step%02d.png", script_capture_step + 1);
            out_png = base + buf;
        }
        SDL_GPUTransferBuffer* screenshot_tb = nullptr;

        SDL_GPUColorTargetInfo target_info = {.texture = swapchain_texture, .clear_color = SDL_FColor{0.075f, 0.082f, 0.094f, 1.0f},
                                              .load_op = SDL_GPU_LOADOP_CLEAR, .store_op = SDL_GPU_STOREOP_STORE};
        SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(cmd_buffer, &target_info, 1, nullptr);
        ImGui_ImplSDLGPU3_RenderDrawData(draw_data, cmd_buffer, render_pass);
        SDL_EndGPURenderPass(render_pass);

        if (capture_this_frame && swapchain_w > 0 && swapchain_h > 0) {
            SDL_GPUTransferBufferCreateInfo tb_info = {SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD, swapchain_w * swapchain_h * 4};
            if ((screenshot_tb = SDL_CreateGPUTransferBuffer(gpu_device, &tb_info))) {
                if (SDL_GPUCopyPass* cp = SDL_BeginGPUCopyPass(cmd_buffer)) {
                    SDL_GPUTextureRegion src = {swapchain_texture, 0, 0, 0, 0, 0, swapchain_w, swapchain_h, 1};
                    SDL_GPUTextureTransferInfo dst = {screenshot_tb, 0, 0, 0};
                    SDL_DownloadFromGPUTexture(cp, &src, &dst);
                    SDL_EndGPUCopyPass(cp);
                }
            }
        }

        if (screenshot_tb) {
            if (SDL_GPUFence* f = SDL_SubmitGPUCommandBufferAndAcquireFence(cmd_buffer)) {
                SDL_WaitForGPUFences(gpu_device, true, &f, 1); SDL_ReleaseGPUFence(gpu_device, f);
            }
            if (void* px = SDL_MapGPUTransferBuffer(gpu_device, screenshot_tb, false)) {
                if (SDL_Surface* s = SDL_CreateSurfaceFrom(swapchain_w, swapchain_h, SDL_PIXELFORMAT_BGRA32, px, swapchain_w * 4)) {
                    std::string bmp = out_png + ".bmp";
                    if (SDL_SaveBMP(s, bmp.c_str())) {
                        // Convert without a shell: the path may come from the
                        // remote FIFO and must not be interpolated into a command.
                        char* argv[] = {const_cast<char*>("sips"), const_cast<char*>("-s"),
                                        const_cast<char*>("format"), const_cast<char*>("png"),
                                        bmp.data(), const_cast<char*>("--out"),
                                        out_png.data(), nullptr};
                        posix_spawn_file_actions_t actions;
                        posix_spawn_file_actions_init(&actions);
                        posix_spawn_file_actions_addopen(&actions, 1, "/dev/null", O_WRONLY, 0);
                        posix_spawn_file_actions_addopen(&actions, 2, "/dev/null", O_WRONLY, 0);
                        pid_t pid = -1;
                        if (posix_spawn(&pid, "/usr/bin/sips", &actions, nullptr, argv, environ) == 0)
                            waitpid(pid, nullptr, 0);
                        posix_spawn_file_actions_destroy(&actions);
                        ::unlink(bmp.c_str());
                    }
                    SDL_DestroySurface(s);
                }
                SDL_UnmapGPUTransferBuffer(gpu_device, screenshot_tb);
            }
            SDL_ReleaseGPUTransferBuffer(gpu_device, screenshot_tb);
        } else {
            SDL_SubmitGPUCommandBuffer(cmd_buffer);
        }

        if (defer_show && frame_count >= 2) {
            SDL_ShowWindow(window);
            SDL_RaiseWindow(window);
            defer_show = false;
        }

        scheduler.onFrameRendered();
        SDL_Delay(16);
        if (capture_this_frame) {
            if (remote_capture) remote_shot_path.clear();
            if (remote_fd < 0) {
                if (script_keys.empty()) running = false;
                else {
                    script_capture_step = -1;
                    script_shots_done++;
                    if (script_shots_done > static_cast<int>(script_keys.size())) running = false;
                }
            }
        }
    }

    engineHost.stop(); SDL_WaitForGPUIdle(gpu_device);
    ImGui_ImplSDLGPU3_Shutdown(); ImGui_ImplSDL3_Shutdown(); ImGui::DestroyContext();
    SDL_ReleaseWindowFromGPUDevice(gpu_device, window); SDL_DestroyGPUDevice(gpu_device); SDL_DestroyWindow(window); SDL_Quit();
    return 0;
}

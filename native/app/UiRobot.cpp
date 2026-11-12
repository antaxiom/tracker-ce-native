#include "UiRobot.h"
#include "imgui_internal.h"
#include "Song.h"
#include "History.h"
#include "EngineHost.h"
#include "Theme.h"
#include "views/PatternView.h"
#include "hw/ButtonDeck.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace trakr {

UiRobot::UiRobot() {
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigDebugHighlightIdConflicts = true;
    io.ConfigMacOSXBehaviors = false;
}

void UiRobot::setMousePos(const ImVec2& pos) {
    currentMousePos_ = pos;
    ImGuiIO& io = ImGui::GetIO();
    io.AddMousePosEvent(pos.x, pos.y);
}

void UiRobot::mouseMove(const ImVec2& targetPos, int steps, const std::function<void()>& onFrame) {
    ImVec2 start = currentMousePos_;
    steps = std::max(1, steps);
    for (int i = 1; i <= steps; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(steps);
        setMousePos(ImVec2(start.x + (targetPos.x - start.x) * t, start.y + (targetPos.y - start.y) * t));
        if (onFrame) onFrame();
        checkFrameHealth("mouseMove");
    }
}

void UiRobot::mouseDown(int button) {
    if (button >= 0 && button < 5) {
        mouseDown_[button] = true;
        ImGui::GetIO().AddMouseButtonEvent(button, true);
    }
}

void UiRobot::mouseUp(int button) {
    if (button >= 0 && button < 5) {
        mouseDown_[button] = false;
        ImGui::GetIO().AddMouseButtonEvent(button, false);
    }
}

void UiRobot::click(const ImVec2& pos, int button, const std::function<void()>& onFrame) {
    setMousePos(pos);
    if (onFrame) onFrame();
    if (onFrame) onFrame();
    checkFrameHealth("click:hover");
    mouseDown(button);
    if (onFrame) onFrame();
    checkFrameHealth("click:down");
    mouseUp(button);
    if (onFrame) onFrame();
    checkFrameHealth("click:up");
}

void UiRobot::drag(const ImVec2& from, const ImVec2& to, int steps, const std::function<void()>& onFrame) {
    setMousePos(from);
    if (onFrame) onFrame();
    mouseDown(0);
    mouseMove(to, steps, onFrame);
    mouseUp(0);
    if (onFrame) onFrame();
}

void UiRobot::mouseWheel(float delta, const std::function<void()>& onFrame) {
    mouseWheel_ = delta;
    ImGuiIO& io = ImGui::GetIO();
    io.AddMouseWheelEvent(0.0f, delta);
    io.MouseWheel = delta;
    if (onFrame) onFrame();
    checkFrameHealth("mouseWheel");
    mouseWheel_ = 0.0f;
    io.AddMouseWheelEvent(0.0f, 0.0f);
    io.MouseWheel = 0.0f;
}

void UiRobot::pressKey(ImGuiKey key, bool ctrl, bool shift, bool alt, const std::function<void()>& onFrame) {
    ImGuiIO& io = ImGui::GetIO();
    io.KeyCtrl = ctrl;
    io.KeySuper = ctrl;
    io.KeyShift = shift;
    io.KeyAlt = alt;
    io.AddKeyEvent(key, true);
    if (onFrame) onFrame();
    checkFrameHealth("pressKey:down");
    io.AddKeyEvent(key, false);
    io.KeyCtrl = false; io.KeySuper = false; io.KeyShift = false; io.KeyAlt = false;
    if (onFrame) onFrame();
    checkFrameHealth("pressKey:up");
}

void UiRobot::checkFrameHealth(const char* contextTag) {
    report_.framesSimulated++;
    ImGuiContext& g = *GImGui;

    // 1. Detect Dear ImGui ID conflicts
    if (g.IO.ConfigDebugHighlightIdConflicts && (g.DebugDrawIdConflictsId != 0 || g.HoveredIdPreviousFrameItemCount > 1)) {
        report_.idConflictsDetected++;
        char buf[256];
        snprintf(buf, sizeof(buf), "[ID CONFLICT] Duplicate identifier detected (ID: 0x%08X, Count: %d) at context: %s",
                 g.DebugDrawIdConflictsId ? g.DebugDrawIdConflictsId : g.HoveredIdPreviousFrame,
                 g.HoveredIdPreviousFrameItemCount, contextTag ? contextTag : "unknown");
        report_.errors.push_back(buf);
    }

    // 2. Validate stack balances
    if (g.ColorStack.Size != 0) {
        report_.stackImbalancesDetected++;
        report_.errors.push_back(std::string("[STACK ERROR] Unbalanced ColorStack size: ") + std::to_string(g.ColorStack.Size));
    }
    if (g.StyleVarStack.Size != 0) {
        report_.stackImbalancesDetected++;
        report_.errors.push_back(std::string("[STACK ERROR] Unbalanced StyleVarStack size: ") + std::to_string(g.StyleVarStack.Size));
    }
    if (g.FontStack.Size != 0) {
        report_.stackImbalancesDetected++;
        report_.errors.push_back(std::string("[STACK ERROR] Unbalanced FontStack size: ") + std::to_string(g.FontStack.Size));
    }
}

void UiRobot::sweepWindow(const char* windowName, float stepSize, const std::function<void()>& onFrame) {
    if (onFrame) onFrame(); // Prime frame to ensure window exists
    ImGuiWindow* win = ImGui::FindWindowByName(windowName);
    if (!win || win->Hidden || win->Collapsed) return;

    ImVec2 min = win->Pos;
    ImVec2 max = ImVec2(win->Pos.x + win->Size.x, win->Pos.y + win->Size.y);
    stepSize = std::max(8.0f, stepSize);

    for (float y = min.y + 12.0f; y < max.y - 4.0f; y += stepSize) {
        for (float x = min.x + 12.0f; x < max.x - 4.0f; x += stepSize) {
            setMousePos(ImVec2(x, y));
            report_.widgetsTested++;
            if (onFrame) onFrame(); // Frame 1: hover item
            if (onFrame) onFrame(); // Frame 2: verify duplicates
            checkFrameHealth(windowName);
        }
    }
}

void UiRobot::sweepAllActiveWindows(float stepSize, const std::function<void()>& onFrame) {
    ImGuiContext& g = *GImGui;
    for (ImGuiWindow* w : g.Windows) {
        if (w && w->Active && !w->Hidden && !w->Collapsed && w->Size.x > 30.0f && w->Size.y > 30.0f) {
            sweepWindow(w->Name, stepSize, onFrame);
        }
    }
}

bool UiRobot::clickWindowItem(const char* windowName, const ImVec2& relativePos, const std::function<void()>& onFrame) {
    if (onFrame) onFrame();
    ImGuiWindow* win = ImGui::FindWindowByName(windowName);
    if (!win || win->Hidden || win->Collapsed) return false;
    click(ImVec2(win->Pos.x + relativePos.x, win->Pos.y + relativePos.y), 0, onFrame);
    return true;
}

UiConflictReport UiRobot::runAutonomousSuite(Song& song, History& history, EngineHost& engineHost,
                                            int& octave, int& editStep, bool& recordMode,
                                            int& currentOrderIdx, int& selectedInstIdx,
                                            Theme* theme, const std::function<void()>& onFrame) {
    resetReport();

    // Phase 1: Comprehensive hover sweep across each view window to trigger ID collision detection
    const char* targetWindows[] = {"Transport", "Order List", "Instruments", "Sample View", "Mixer", "Pattern Editor"};
    for (const char* wName : targetWindows) {
        sweepWindow(wName, 18.0f, onFrame);
    }

    // Phase 2: Systematic button clicking and control manipulation
    // 2.1 Transport controls: Play, Stop, Rec, Loop
    clickWindowItem("Transport", ImVec2(25, 20), onFrame); // Play/Stop
    clickWindowItem("Transport", ImVec2(25, 20), onFrame); // Stop
    clickWindowItem("Transport", ImVec2(80, 20), onFrame); // Rec
    clickWindowItem("Transport", ImVec2(125, 20), onFrame); // Loop

    // 2.2 Order List: Add, Dup, Del
    clickWindowItem("Order List", ImVec2(30, 42), onFrame); // + Add
    clickWindowItem("Order List", ImVec2(75, 42), onFrame); // Dup
    clickWindowItem("Order List", ImVec2(120, 42), onFrame); // Del

    // 2.3 Instruments: New, Dup, Audition, Del
    clickWindowItem("Instruments", ImVec2(30, 42), onFrame); // + New
    clickWindowItem("Instruments", ImVec2(80, 42), onFrame); // Dup
    clickWindowItem("Instruments", ImVec2(130, 42), onFrame); // Audition
    clickWindowItem("Instruments", ImVec2(180, 42), onFrame); // Del

    // 2.4 Sample View: Slicing (Grid 4, Grid 8, Grid 16, Detect, Clear), Normalize, Reverse, Loop/Filter/LFO
    clickWindowItem("Sample View", ImVec2(40, 45), onFrame); // Grid 4
    clickWindowItem("Sample View", ImVec2(95, 45), onFrame); // Grid 8
    clickWindowItem("Sample View", ImVec2(150, 45), onFrame); // Grid 16
    clickWindowItem("Sample View", ImVec2(205, 45), onFrame); // Detect
    clickWindowItem("Sample View", ImVec2(260, 45), onFrame); // Clear
    clickWindowItem("Sample View", ImVec2(180, 210), onFrame); // Loop mode toggle
    clickWindowItem("Sample View", ImVec2(250, 210), onFrame); // Normalize
    clickWindowItem("Sample View", ImVec2(325, 210), onFrame); // Reverse
    clickWindowItem("Sample View", ImVec2(40, 280), onFrame);  // Filter type toggle
    clickWindowItem("Sample View", ImVec2(40, 340), onFrame);  // LFO dest toggle
    clickWindowItem("Sample View", ImVec2(120, 340), onFrame); // LFO wave toggle

    // 2.5 Mixer View: Mute, Solo, Pan, Gain
    for (int t = 0; t < std::min(4, static_cast<int>(song.tracks.size())); ++t) {
        float xOff = 20.0f + t * 45.0f;
        clickWindowItem("Mixer", ImVec2(xOff, 65), onFrame);     // Mute
        clickWindowItem("Mixer", ImVec2(xOff + 18, 65), onFrame); // Solo
    }

    // 2.6 Pattern Editor: Note entry via keys
    pressKey(ImGuiKey_Z, false, false, false, onFrame); // C-4
    pressKey(ImGuiKey_S, false, false, false, onFrame); // C#4
    pressKey(ImGuiKey_Q, false, false, false, onFrame); // C-5
    pressKey(ImGuiKey_A, false, false, false, onFrame); // Note Off
    pressKey(ImGuiKey_UpArrow, true, false, false, onFrame); // Transpose +1
    pressKey(ImGuiKey_Z, true, false, false, onFrame);  // Undo
    pressKey(ImGuiKey_Y, true, false, false, onFrame);  // Redo

    // 2.7 Theme switching
    if (theme) {
        *theme = makeTrackerClassicTheme();
        applyTheme(*theme, ImGui::GetStyle());
        if (onFrame) onFrame();
        checkFrameHealth("Theme:TrackerClassic");

        *theme = makeColorblindTheme();
        applyTheme(*theme, ImGui::GetStyle());
        if (onFrame) onFrame();
        checkFrameHealth("Theme:Colorblind");

        *theme = makeRenoiseDarkTheme();
        applyTheme(*theme, ImGui::GetStyle());
        if (onFrame) onFrame();
        checkFrameHealth("Theme:Renoise");
    }

    // Phase 3: Invariant Validation
    for (const auto& inst : song.instruments) {
        if (inst.sample) {
            for (float s : inst.sample->ch[0]) {
                if (!std::isfinite(s)) {
                    report_.numericalErrorsDetected++;
                    report_.errors.push_back("[NUMERICAL] Non-finite sample float detected");
                    break;
                }
            }
        }
    }

    return report_;
}

void UiRobot::runHardwareDeckVerification(Song& song, History& history, EngineHost& engineHost,
                                         hw::ButtonDeck& deck,
                                         PatternView& patternView,
                                         int& octave, int& editStep, bool& recordMode,
                                         bool& shiftHeld, int selectedInstIdx, int currentOrderIdx,
                                         const ImRect& deckRect,
                                         const std::function<void()>& onFrame) {
    if (onFrame) onFrame();

    // 1. Layout & Machine Vision checks
    ImRect pads = deck.padsBounds();
    ImVec2 jog = deck.jogCenter();
    float jogR = deck.jogRadius();

    if (pads.GetWidth() < 200.0f || pads.Max.x >= jog.x - jogR) {
        report_.errors.push_back("[MACHINE VISION] Pad matrix overlaps or placed to the right of jog wheel");
    }

    // 2. Octave buttons tool control (1..4)
    for (int n = 1; n <= 4; ++n) {
        ImRect b = deck.buttonBounds(std::to_string(n));
        if (b.GetWidth() > 0.0f) {
            click(b.GetCenter(), 0, onFrame);
            if (octave != n) report_.errors.push_back("[TOOL CONTROL] Octave button " + std::to_string(n) + " failed");
        }
    }

    // 3. Shift toggle tool control
    ImRect shiftB = deck.buttonBounds("Shift");
    if (shiftB.GetWidth() > 0.0f) {
        click(shiftB.GetCenter(), 0, onFrame);
        if (!shiftHeld) report_.errors.push_back("[TOOL CONTROL] Shift button failed to toggle shiftHeld");
        click(shiftB.GetCenter(), 0, onFrame);
        if (shiftHeld) report_.errors.push_back("[TOOL CONTROL] Shift button failed to clear shiftHeld");
    }

    // 4. Transport: Play/Stop, Rec
    std::vector<float> audioBuf(256 * 2, 0.0f);
    ImRect playB = deck.buttonBounds("Play");
    if (playB.GetWidth() > 0.0f) {
        click(playB.GetCenter(), 0, onFrame);
        engineHost.getEngine().process(audioBuf.data(), 128);
        if (!engineHost.getPlayhead().playing) report_.errors.push_back("[TOOL CONTROL] Play button failed to start playback");
        if (onFrame) onFrame();
        ImRect stopB = deck.buttonBounds("STOP");
        if (stopB.GetWidth() > 0.0f) click(stopB.GetCenter(), 0, onFrame);
        engineHost.getEngine().process(audioBuf.data(), 128);
        if (engineHost.getPlayhead().playing) report_.errors.push_back("[TOOL CONTROL] Stop button failed to stop playback");
    }

    ImRect recB = deck.buttonBounds("Rec");
    if (recB.GetWidth() > 0.0f) {
        click(recB.GetCenter(), 0, onFrame);
        if (!recordMode) report_.errors.push_back("[TOOL CONTROL] Rec button failed to enable record mode");
        click(recB.GetCenter(), 0, onFrame);
        if (recordMode) report_.errors.push_back("[TOOL CONTROL] Rec button failed to disable record mode");
    }

    // 5. Jog Wheel Drag Tool Control
    patternView.setCursor(patternView.cursorTrack(), 10, patternView.cursorSubCol());
    int startRow = patternView.cursorRow();
    setMousePos(jog);
    mouseWheel(1.0f, onFrame);
    if (patternView.cursorRow() == startRow && !song.patterns.empty() && song.patterns[0].rows > 4) {
        report_.errors.push_back("[TOOL CONTROL] Jog wheel failed to navigate rows");
    }

    // 6. Pad Note Audition Tool Control
    if (!song.instruments.empty()) {
        song.instruments[0].isSynth = true;
        song.instruments[0].sustain = 1.0f;
        engineHost.publish(song);
    }
    ImVec2 pad0Pos(pads.Min.x + 15.0f, pads.Max.y - 15.0f);
    setMousePos(pad0Pos);
    mouseDown(0);
    if (onFrame) onFrame();
    engineHost.getEngine().process(audioBuf.data(), 256);
    if (!engineHost.isPreviewVoiceActive()) {
        report_.errors.push_back("[TOOL CONTROL] Pad click failed to audition note on preview voice");
    }
    mouseUp(0);
    if (onFrame) onFrame();
    for (int b = 0; b < 25; ++b) engineHost.getEngine().process(audioBuf.data(), 256);
    if (engineHost.isPreviewVoiceActive()) {
        report_.errors.push_back("[TOOL CONTROL] Pad release failed to turn off preview voice");
    }

    // 7. Subcolumn Jump Keys Tool Control (Note, Instr, FX1, FX2)
    ImRect instrB = deck.buttonBounds("Instr");
    ImRect fx1B = deck.buttonBounds("FX1");
    ImRect fx2B = deck.buttonBounds("FX2");
    ImRect noteB = deck.buttonBounds("Note");
    if (instrB.GetWidth() > 0.0f) {
        click(instrB.GetCenter(), 0, onFrame);
        if (patternView.cursorSubCol() != 1) report_.errors.push_back("[TOOL CONTROL] Instr button failed to focus instrument subcolumn");
    }
    if (fx1B.GetWidth() > 0.0f) {
        click(fx1B.GetCenter(), 0, onFrame);
        if (patternView.cursorSubCol() != 2) report_.errors.push_back("[TOOL CONTROL] FX1 button failed to focus FX1 subcolumn");
    }
    if (fx2B.GetWidth() > 0.0f) {
        click(fx2B.GetCenter(), 0, onFrame);
        if (patternView.cursorSubCol() != 3) report_.errors.push_back("[TOOL CONTROL] FX2 button failed to focus FX2 subcolumn");
    }
    if (noteB.GetWidth() > 0.0f) {
        click(noteB.GetCenter(), 0, onFrame);
        if (patternView.cursorSubCol() != 0) report_.errors.push_back("[TOOL CONTROL] Note button failed to focus note subcolumn");
    }

    // 8. Authentic Polyend Hardware Page & Menu Keys Tool Control
    ImRect menuB = deck.buttonBounds("Menu");
    if (menuB.GetWidth() > 0.0f) click(menuB.GetCenter(), 0, onFrame);
    ImRect patB = deck.buttonBounds("Pattern");
    if (patB.GetWidth() > 0.0f) click(patB.GetCenter(), 0, onFrame);
}

} // namespace trakr

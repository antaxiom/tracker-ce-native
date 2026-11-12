#pragma once

#include "imgui.h"
#include "imgui_internal.h"
#include <string>
#include <vector>
#include <functional>

namespace trakr {

class Song;
class History;
class EngineHost;
struct Theme;
namespace hw { class ButtonDeck; }

// Result of automated UI testing & conflict audit
struct UiConflictReport {
    int framesSimulated{0};
    int widgetsTested{0};
    int idConflictsDetected{0};
    int stackImbalancesDetected{0};
    int numericalErrorsDetected{0};
    std::vector<std::string> errors;

    bool passed() const {
        return idConflictsDetected == 0 && stackImbalancesDetected == 0 && numericalErrorsDetected == 0 && errors.empty();
    }
};

// Automated UI Robot that controls the mouse, keyboard, and clicks widgets
// while monitoring Dear ImGui internal state for duplicate IDs and conflicts.
class UiRobot {
public:
    UiRobot();
    ~UiRobot() = default;

    // Simulation Input controls
    void setMousePos(const ImVec2& pos);
    void mouseMove(const ImVec2& targetPos, int steps = 4, const std::function<void()>& onFrame = nullptr);
    void mouseDown(int button = 0);
    void mouseUp(int button = 0);
    void click(const ImVec2& pos, int button = 0, const std::function<void()>& onFrame = nullptr);
    void drag(const ImVec2& from, const ImVec2& to, int steps = 8, const std::function<void()>& onFrame = nullptr);
    void mouseWheel(float delta, const std::function<void()>& onFrame = nullptr);
    void pressKey(ImGuiKey key, bool ctrl = false, bool shift = false, bool alt = false, const std::function<void()>& onFrame = nullptr);

    // Frame inspection & conflict detection
    void checkFrameHealth(const char* contextTag = nullptr);

    // High-level automated sweeps
    void sweepWindow(const char* windowName, float stepSize, const std::function<void()>& onFrame);
    void sweepAllActiveWindows(float stepSize, const std::function<void()>& onFrame);

    // Widget search & automated clicking
    bool clickWindowItem(const char* windowName, const ImVec2& relativePos, const std::function<void()>& onFrame);

    // Hardware Deck Tool Control & Machine Vision Verification
    void runHardwareDeckVerification(Song& song, History& history, EngineHost& engineHost,
                                     hw::ButtonDeck& deck,
                                     class PatternView& patternView,
                                     int& octave, int& editStep, bool& recordMode,
                                     bool& shiftHeld, int selectedInstIdx, int currentOrderIdx,
                                     const ImRect& deckRect,
                                     const std::function<void()>& onFrame);

    // Full Autonomous Audit Suite
    UiConflictReport runAutonomousSuite(Song& song, History& history, EngineHost& engineHost,
                                       int& octave, int& editStep, bool& recordMode,
                                       int& currentOrderIdx, int& selectedInstIdx,
                                       Theme* theme, const std::function<void()>& renderFrameCallback);

    const UiConflictReport& getReport() const { return report_; }
    void resetReport() { report_ = UiConflictReport(); }
    const ImVec2& getMousePos() const { return currentMousePos_; }
    bool isMouseDown(int button = 0) const { return (button >= 0 && button < 5) ? mouseDown_[button] : false; }
    float getMouseWheel() const { return mouseWheel_; }

private:
    UiConflictReport report_;
    ImVec2 currentMousePos_{100.0f, 100.0f};
    bool mouseDown_[5]{false};
    float mouseWheel_{0.0f};
};

} // namespace trakr


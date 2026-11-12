#pragma once

// Polyend Tracker Hardware Button & Pad Deck Parity
// Reference: tracker-manual-1.7.0.txt lines 206-350, 859-1100 & Tracker / Mini physical layout
// Provides on-screen D-pad, Note/Instr/FX1/FX2 jump keys, Play/Rec transport,
// Jog wheel rotary dial emulation, and 4x12 responsive silicone pad matrix.

#include "imgui.h"
#include "imgui_internal.h"
#include <string>
#include <vector>

namespace trakr {
    class Song;
    class EngineHost;
    class History;
    enum class Screen;
    class PatternView;
    class InstrumentView;
    class PerformView;
    class SampleView;
    class MixerView;
}

namespace trakr::hw {

class ButtonDeck {
public:
    ButtonDeck();

    // Renders the on-screen hardware deck below the 800x480 screen
    void render(const ImRect& rect,
                trakr::Song& song,
                trakr::EngineHost& engineHost,
                trakr::History& history,
                trakr::PatternView& patternView,
                trakr::Screen& activeScreen,
                bool& recordMode,
                int& octave,
                int& editStep,
                bool& shiftHeld,
                int& selectedInstIdx,
                int& currentOrderIdx,
                float scale = 1.0f,
                trakr::InstrumentView* instrumentView = nullptr,
                bool* pShowMenuPopup = nullptr,
                trakr::PerformView* performView = nullptr,
                trakr::SampleView* sampleView = nullptr,
                trakr::MixerView* mixerView = nullptr,
                bool* pShowExportDeck = nullptr);

    float jogAngle() const { return jogAngle_; }
    void setJogAngle(float a) { jogAngle_ = a; }

    // Machine vision / tool control inspection bounds
    ImRect padsBounds() const { return padsBounds_; }
    ImVec2 jogCenter() const { return jogCenter_; }
    float jogRadius() const { return jogRadius_; }
    ImRect buttonBounds(const std::string& name) const;

private:
    float jogAngle_{0.0f};
    int activePadNote_{-1};
    int activePerformCol_{-1};
    int activePerformRow_{-1};
    bool jogDragging_{false};
    float lastJogDragAngle_{0.0f};

    ImRect padsBounds_{};
    ImVec2 jogCenter_{};
    float jogRadius_{0.0f};
    struct BtnBounds { std::string name; ImRect rect; };
    std::vector<BtnBounds> buttonBoundsList_;

    void renderDpad(ImDrawList* dl, const ImVec2& center, float size,
                    trakr::PatternView& patternView, trakr::Song& song,
                    trakr::EngineHost& engineHost, int patIdx, bool isShift,
                    int& selectedInstIdx, trakr::Screen activeScreen, int editStep,
                    trakr::InstrumentView* instrumentView,
                    trakr::SampleView* sampleView,
                    trakr::MixerView* mixerView,
                    int& currentOrderIdx);
    void renderJogWheel(ImDrawList* dl, const ImVec2& center, float radius,
                        trakr::PatternView& patternView, trakr::Song& song,
                        trakr::History& history, trakr::EngineHost& engineHost,
                        int patIdx, bool recordMode, bool isShift,
                        trakr::InstrumentView* iv, trakr::Screen activeScreen, int& instIdx,
                        trakr::SampleView* sv, trakr::MixerView* mv, int& currentOrderIdx);
    void renderPads(ImDrawList* dl, const ImRect& padArea,
                    trakr::Song& song, trakr::EngineHost& engineHost,
                    trakr::History& history, trakr::PatternView& patternView,
                    int patIdx, int selectedInstIdx, bool recordMode, int octave, int editStep,
                    bool isShift, trakr::Screen activeScreen,
                    trakr::PerformView* performView);
};

} // namespace trakr::hw

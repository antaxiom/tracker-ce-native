#pragma once

// Trakr Native Performance Mode parity
// Firmware reference: core/interface/performanceMode/gPerformance.cpp, fPerformance.cpp
// Manual reference: Section 12.1 Performance Mode Overview (lines 16180-16300)

#include "View.h"
#include "Song.h"
#include "History.h"
#include "EngineHost.h"
#include <array>
#include <string>

namespace trakr {

struct PerformSlot12 {
    std::string name;
    std::array<std::string, 4> labels;
    std::array<float, 4> values;
    int activeRow{0}; // 0 = default top row, 1..3 = punch-in value
};

class PerformView : public View {
public:
    PerformView();
    const char* name() const override { return "Performance FX"; }
    void draw(bool* p_open = nullptr) override;

    void update(Song& song, History& history, EngineHost& engineHost);

    bool isTrackEnabled(int t) const { return (t >= 0 && t < 8) ? trackEnabled_[t] : false; }
    void toggleTrack(int t) { if (t >= 0 && t < 8) trackEnabled_[t] = !trackEnabled_[t]; }
    void clearAllFx(EngineHost& engineHost);

    const std::array<PerformSlot12, 12>& slots() const { return slots_; }
    void triggerSlotPad(int col, int row, EngineHost& engineHost);

private:
    void applyToEngine(EngineHost& engineHost);

    std::array<PerformSlot12, 12> slots_;
    std::array<bool, 8> trackEnabled_{true, true, true, false, false, false, false, false};
    bool latchMode_{true};
    int performanceEditPlace_{-1}; // gPerformance.cpp performanceEditPlace; -1 hides the edit frame
};

} // namespace trakr

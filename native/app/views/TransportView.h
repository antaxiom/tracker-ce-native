#pragma once

#include "View.h"
#include "Song.h"
#include "History.h"
#include "EngineHost.h"

namespace trakr {

class TransportView : public View {
public:
    const char* name() const override { return "Transport"; }
    void draw(bool* p_open = nullptr) override;

    void update(Song& song, History& history, EngineHost& engineHost,
                int& octave, int& editStep, bool& recordMode,
                bool* pShowPattern = nullptr, bool* pShowInstruments = nullptr,
                bool* pShowSample = nullptr, bool* pShowMixer = nullptr,
                bool* pShowOrder = nullptr,
                const std::string& currentFilePath = "");

    bool isPlaying() const { return playing_; }

private:
    bool playing_{false};
    bool loopPattern_{true};
    float masterPeakL_{0.0f};
    float masterPeakR_{0.0f};
};

} // namespace trakr

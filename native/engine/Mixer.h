#pragma once

#include "Song.h"
#include "dsp/Delay.h"
#include "dsp/Reverb.h"
#include "dsp/MasterEq.h"
#include "dsp/Overdrive.h"
#include <vector>
#include <atomic>
#include <cmath>

namespace trakr {

struct MeterLevels {
    float peakL{0.0f};
    float peakR{0.0f};
};

class Mixer {
public:
    explicit Mixer(size_t trackCount = 8, float sampleRate = 44100.0f);

    void init(float sampleRate);
    void resizeTracks(size_t count);

    // Process a block of track audio into master output buffer
    void process(const std::vector<TrackInfo>& trackInfos,
                 const float* const* trackAudioL,
                 const float* const* trackAudioR,
                 float* masterOutStereo,
                 int frames,
                 float masterGainDb = 0.0f,
                 float masterPan = 0.0f,
                 float delaySendBoost = 0.0f,
                 float reverbSendBoost = 0.0f,
                 const float* masterEq = nullptr,
                 float masterSaturation = 0.0f,
                 float masterBassBoost = 0.0f,
                 float masterSpace = 0.0f);

    void setBitCrush(int bits) { bitCrushBits_ = bits; }
    int getBitCrush() const { return bitCrushBits_; }

    MeterLevels getTrackMeters(size_t trackIndex) const;
    MeterLevels getMasterMeters() const;

private:
    float softLimit(float x) const;

    struct TrackMeter {
        std::atomic<float> peakL{0.0f};
        std::atomic<float> peakR{0.0f};
    };

    std::vector<TrackMeter> trackMeters_;
    std::atomic<float> masterPeakL_{0.0f};
    std::atomic<float> masterPeakR_{0.0f};

    dsp::Delay delay_;
    dsp::Reverb reverb_;
    dsp::MasterEq masterEq_;
    dsp::Overdrive masterSaturation_;
    int bitCrushBits_{16};
    std::vector<float> sendDelayBufL_;
    std::vector<float> sendDelayBufR_;
    std::vector<float> sendReverbBufL_;
    std::vector<float> sendReverbBufR_;
};

} // namespace trakr

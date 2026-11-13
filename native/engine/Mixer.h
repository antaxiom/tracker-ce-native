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

    // Global Mixer (Master 3/3): return/dry gains, already converted from the
    // aux fader positions. Order: Delay, Reverb, Dry Mix, Line In.
    void setAuxGains(float delay, float reverb, float dry, float line) {
        auxDelayGain_ = delay;
        auxReverbGain_ = reverb;
        auxDryGain_ = dry;
        auxLineGain_ = line;
    }

    MeterLevels getTrackMeters(size_t trackIndex) const;
    MeterLevels getMasterMeters() const;
    // Aux bus meters (0 Delay, 1 Reverb, 2 Dry Mix, 3 Line In).
    MeterLevels getAuxMeters(int aux) const;

private:
    float softLimit(float x) const;

    struct TrackMeter {
        std::atomic<float> peakL{0.0f};
        std::atomic<float> peakR{0.0f};
    };

    std::vector<TrackMeter> trackMeters_;
    std::atomic<float> masterPeakL_{0.0f};
    std::atomic<float> masterPeakR_{0.0f};
    std::atomic<float> auxPeakL_[4]{};
    std::atomic<float> auxPeakR_[4]{};
    float auxDelayGain_{1.0f};
    float auxReverbGain_{1.0f};
    float auxDryGain_{1.0f};
    float auxLineGain_{1.0f};

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

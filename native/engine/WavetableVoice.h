#pragma once

#include "Voice.h"
#include "Song.h"
#include "dsp/Adsr.h"
#include "dsp/Svf.h"
#include "dsp/Lfo.h"
#include <memory>
#include <vector>

namespace trakr {

class WavetableVoice : public IVoice {
public:
    explicit WavetableVoice(int sampleRate = 44100);

    void setSampleRate(int sampleRate);

    void noteOn(int note, float velocity, const Instrument* inst) override;
    void noteOff() override;
    void stop() override;
    bool isActive() const override;
    void process(float* outL, float* outR, int frames) override;

    void setVolume(float v) override;
    void setPanning(float p) override;
    void setPitchRatio(float ratio) override;
    void setPosition(float posNormalized) override;
    void setCutCountdown(int frames) override;
    void setRetrigger(int intervalFrames, int count = 0) override;
    void setGlide(float targetRatio, float rate = 0.05f) override;
    void setReverse(bool reverse) override;
    void setCutoff(float cutoffHz) override;
    void setResonance(float res) override;
    void setFilterType(int type) override;

private:
    float interpolateSample(const float* data, size_t length, double pos) const;

    int sampleRate_{44100};
    std::shared_ptr<const SampleData> sample_{nullptr};

    bool active_{false};
    bool isGranular_{false};
    int windowSize_{256};
    double windowOffset_{0.0};
    double phaseInWindow_{0.0};
    double step_{1.0};

    // Granular grain state
    double grainPos_{0.0};
    int grainLengthSamples_{2205}; // ~50ms
    int grainCounter_{0};

    float volume_{1.0f};
    float pan_{0.0f};
    float basePitchRatio_{1.0f};
    float currentPitchRatio_{1.0f};
    int cutCountdown_{-1};
    int retriggerCountdown_{0};
    float glideTargetRatio_{-1.0f};
    float glideRate_{0.05f};

    dsp::Adsr adsr_;
    dsp::Svf filterL_;
    dsp::Svf filterR_;
    dsp::Lfo lfo_;
    bool useFilter_{false};
    bool useLfo_{false};
    Instrument::LfoTarget lfoTarget_{Instrument::LfoTarget::None};
    float baseCutoff_{20000.0f};
    float baseResonance_{0.707f};
    dsp::FilterType svfType_{dsp::FilterType::Lowpass};
};

} // namespace trakr

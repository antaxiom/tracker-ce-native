#pragma once

#include "Voice.h"
#include "Song.h"
#include "dsp/Adsr.h"
#include "dsp/Svf.h"
#include "dsp/Overdrive.h"
#include "dsp/Bitcrush.h"
#include <cstdint>

namespace trakr {

enum class ChipModel : int {
    Pulse2A03 = 0,
    SidSawTri = 1,
    NoiseLfsr = 2,
    FmBell = 3,
    SuperSquare = 4,
    ZapLaser = 5,
    ArpChord = 6,
    ChipDrum = 7
};

class ChipSynthVoice : public IVoice {
public:
    explicit ChipSynthVoice(int sampleRate = 44100);

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

    void setModel(ChipModel model) { model_ = model; }
    void setShape(float shape) { shape_ = shape; }
    void setPwm(float rateHz, float depth) { pwmRateHz_ = rateHz; pwmDepth_ = depth; }

private:
    float polyBlep(float t, float dt) const;
    uint16_t stepLfsr(uint16_t lfsr, bool shortPeriod) const;

    int sampleRate_{44100};
    bool active_{false};
    ChipModel model_{ChipModel::Pulse2A03};

    float phase_{0.0f};
    float osc2Phase_{0.0f};
    float pwmPhase_{0.0f};
    float vibPhase_{0.0f};
    float lfsrPhase_{0.0f};
    uint16_t lfsrState_{0x7FFFu};

    int noteAgeSamples_{0};
    int arpSampleCounter_{0};
    uint32_t arpStepCounter_{0};

    float shape_{0.5f};
    float pwmRateHz_{2.0f};
    float pwmDepth_{0.3f};
    float vibRateHz_{5.0f};
    float vibDepth_{0.0f};

    float baseFreq_{440.0f};
    float currentPitchRatio_{1.0f};
    float glideTargetRatio_{-1.0f};
    float glideRate_{0.05f};
    int cutCountdown_{-1};

    float volume_{1.0f};
    float pan_{0.0f};

    dsp::Adsr adsr_;
    dsp::Svf filterL_;
    dsp::Svf filterR_;
    dsp::Overdrive overdrive_;
    dsp::Bitcrush bitcrush_;

    bool useFilter_{false};
    float baseCutoff_{20000.0f};
    float baseResonance_{0.707f};
    dsp::FilterType svfType_{dsp::FilterType::Lowpass};
};

} // namespace trakr

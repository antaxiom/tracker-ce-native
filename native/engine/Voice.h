#pragma once

#include "Song.h"
#include "dsp/Adsr.h"
#include "dsp/Svf.h"
#include "dsp/Lfo.h"
#include <memory>
#include <cmath>

namespace trakr {

class IVoice {
public:
    virtual ~IVoice() = default;

    virtual void noteOn(int note, float velocity, const Instrument* inst) = 0;
    virtual void noteOff() = 0;
    virtual void stop() = 0;
    virtual bool isActive() const = 0;
    virtual void process(float* outL, float* outR, int frames) = 0;

    virtual void setVolume(float v) = 0;
    virtual void setPanning(float p) = 0;
    virtual void setPitchRatio(float ratio) = 0;
    virtual void setPosition(float posNormalized) = 0;
    virtual void setCutCountdown(int frames) { (void)frames; }
    virtual void setRetrigger(int intervalFrames, int count = 0) { (void)intervalFrames; (void)count; }
    virtual void setGlide(float targetRatio, float rate = 0.05f) { (void)targetRatio; (void)rate; }
    virtual void setReverse(bool reverse) { (void)reverse; }
    virtual void setCutoff(float cutoffHz) { (void)cutoffHz; }
    virtual void setResonance(float res) { (void)res; }
    virtual void setFilterType(int type) { (void)type; }
    virtual void setOverdrive(float od) { (void)od; }
    virtual void setBitDepth(int bits) { (void)bits; }
    virtual void setDelaySend(float send) { (void)send; }
    virtual void setReverbSend(float send) { (void)send; }
    virtual float getDelaySend() const { return 0.0f; }
    virtual float getReverbSend() const { return 0.0f; }
    virtual void setNudgeDelay(int frames) { (void)frames; }
    virtual void setSlice(int sliceIndex) { (void)sliceIndex; }
    virtual void resetFx() {}
};

class SampleVoice : public IVoice {
public:
    explicit SampleVoice(int sampleRate = 44100);

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
    void setOverdrive(float od) override { overdrive_ = std::clamp(od, 0.0f, 100.0f); }
    void setBitDepth(int bits) override { bitDepth_ = std::clamp(bits, 4, 16); }
    void setDelaySend(float send) override { delaySend_ = std::clamp(send, 0.0f, 1.0f); }
    void setReverbSend(float send) override { reverbSend_ = std::clamp(send, 0.0f, 1.0f); }
    float getDelaySend() const override { return delaySend_; }
    float getReverbSend() const override { return reverbSend_; }
    void setSlice(int) override {}
    void resetFx() override {
        glideTargetRatio_ = -1.0f;
        retriggerInterval_ = 0;
        retriggerCountdown_ = 0;
        retriggerRemaining_ = 0;
        cutCountdown_ = -1;
    }

private:
    float interpolateCubic(const float* data, size_t length, double pos) const;

    int sampleRate_{44100};
    std::shared_ptr<const SampleData> sample_{nullptr};
    const Instrument* inst_{nullptr};

    double pos_{0.0};
    double step_{1.0};
    bool active_{false};

    int rootNote_{60};
    int finetune_{0};
    float basePitchRatio_{1.0f};
    float currentPitchRatio_{1.0f};

    float volume_{1.0f};
    float pan_{0.0f}; // -1.0 (L) to +1.0 (R)
    Instrument::Loop loopMode_{Instrument::Loop::None};
    int loopStart_{0};
    int loopEnd_{0};
    int endFrame_{0};
    bool pingPongForward_{true};
    bool reverse_{false};

    // Step FX state
    int cutCountdown_{-1};
    int retriggerInterval_{0};
    int retriggerCountdown_{0};
    int retriggerRemaining_{0};
    float glideTargetRatio_{-1.0f};
    float glideRate_{0.05f};

    // DSP components
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
    float overdrive_{0.0f};
    int bitDepth_{16};
    float delaySend_{0.0f};
    float reverbSend_{0.0f};
};

} // namespace trakr

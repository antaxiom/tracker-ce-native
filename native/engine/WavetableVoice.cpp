#include "WavetableVoice.h"
#include <algorithm>
#include <cmath>

namespace trakr {

WavetableVoice::WavetableVoice(int sampleRate)
    : sampleRate_(sampleRate) {
    adsr_.init(static_cast<float>(sampleRate_));
    lfo_.init(static_cast<float>(sampleRate_));
    filterL_.init(static_cast<float>(sampleRate_));
    filterR_.init(static_cast<float>(sampleRate_));
}

void WavetableVoice::setSampleRate(int sampleRate) {
    sampleRate_ = sampleRate;
    adsr_.init(static_cast<float>(sampleRate_));
    lfo_.init(static_cast<float>(sampleRate_));
    filterL_.init(static_cast<float>(sampleRate_));
    filterR_.init(static_cast<float>(sampleRate_));
}

void WavetableVoice::noteOn(int note, float velocity, const Instrument* inst) {
    if (!inst) { stop(); return; }

    sample_ = inst->sample;
    isGranular_ = (inst->loop == Instrument::Loop::Granular);
    windowSize_ = std::clamp(inst->wavetableWindow, 32, 2048);

    float semitones = static_cast<float>(note - inst->rootNote) + inst->finetune * 0.01f;
    basePitchRatio_ = std::pow(2.0f, semitones / 12.0f);
    currentPitchRatio_ = basePitchRatio_;

    float freq = 440.0f * std::pow(2.0f, (note - 69) / 12.0f);
    step_ = (freq * windowSize_) / sampleRate_;

    volume_ = velocity;
    phaseInWindow_ = 0.0;
    cutCountdown_ = -1;
    retriggerCountdown_ = 0;
    glideTargetRatio_ = -1.0f;

    if (sample_ && !sample_->ch[0].empty()) {
        size_t totalLen = sample_->ch[0].size();
        size_t maxOffset = (totalLen > static_cast<size_t>(windowSize_)) ? (totalLen - windowSize_) : 0;
        windowOffset_ = inst->wavetablePos * maxOffset;
        grainPos_ = windowOffset_;
    } else {
        windowOffset_ = 0.0;
        grainPos_ = 0.0;
    }

    grainLengthSamples_ = std::max(64, static_cast<int>(inst->granularLengthMs * 0.001f * sampleRate_));
    grainCounter_ = 0;

    adsr_.init(static_cast<float>(sampleRate_));
    adsr_.setParameters(inst->attackMs * 0.001f, inst->decayMs * 0.001f, inst->sustain, inst->releaseMs * 0.001f);
    adsr_.noteOn();

    useFilter_ = (inst->filterType != Instrument::FilterType::None);
    baseCutoff_ = inst->filterCutoffHz;
    baseResonance_ = inst->filterResonance;
    svfType_ = dsp::FilterType::Lowpass;
    if (inst->filterType == Instrument::FilterType::Highpass) svfType_ = dsp::FilterType::Highpass;
    else if (inst->filterType == Instrument::FilterType::Bandpass) svfType_ = dsp::FilterType::Bandpass;
    filterL_.init(static_cast<float>(sampleRate_));
    filterR_.init(static_cast<float>(sampleRate_));
    filterL_.setParameters(svfType_, baseCutoff_, baseResonance_);
    filterR_.setParameters(svfType_, baseCutoff_, baseResonance_);

    useLfo_ = (inst->lfoTarget != Instrument::LfoTarget::None && inst->lfoDepth > 0.001f);
    lfoTarget_ = inst->lfoTarget;
    dsp::LfoWaveform wave = dsp::LfoWaveform::Sine;
    lfo_.init(static_cast<float>(sampleRate_));
    lfo_.setParameters(wave, inst->lfoRateHz, inst->lfoDepth, static_cast<dsp::LfoTarget>(inst->lfoTarget));
    lfo_.reset();

    active_ = true;
}

void WavetableVoice::noteOff() {
    adsr_.noteOff();
}

void WavetableVoice::stop() {
    active_ = false;
    sample_ = nullptr;
    cutCountdown_ = -1;
}

bool WavetableVoice::isActive() const {
    return active_;
}

float WavetableVoice::interpolateSample(const float* data, size_t length, double pos) const {
    if (!data || length == 0) return 0.0f;
    size_t i0 = static_cast<size_t>(pos) % length;
    size_t i1 = (i0 + 1) % length;
    float frac = static_cast<float>(pos - static_cast<size_t>(pos));
    return data[i0] * (1.0f - frac) + data[i1] * frac;
}

void WavetableVoice::process(float* outL, float* outR, int frames) {
    if (!active_ || !sample_ || sample_->ch[0].empty()) {
        active_ = false;
        return;
    }

    const float* dataL = sample_->ch[0].data();
    const float* dataR = (sample_->channels > 1 && !sample_->ch[1].empty()) ? sample_->ch[1].data() : dataL;
    size_t length = sample_->ch[0].size();

    float gainL = volume_ * std::min(1.0f, 1.0f - pan_);
    float gainR = volume_ * std::min(1.0f, 1.0f + pan_);

    for (int i = 0; i < frames; ++i) {
        if (cutCountdown_ >= 0 && --cutCountdown_ <= 0) { stop(); break; }

        if (glideTargetRatio_ > 0.0f) {
            currentPitchRatio_ += (glideTargetRatio_ - currentPitchRatio_) * glideRate_;
            if (std::abs(glideTargetRatio_ - currentPitchRatio_) < 0.001f) {
                currentPitchRatio_ = glideTargetRatio_;
                glideTargetRatio_ = -1.0f;
            }
        }

        float env = adsr_.process();
        if (!adsr_.isActive()) { stop(); break; }

        float lfoVal = useLfo_ ? lfo_.process() : 0.0f;
        float finalCutoff = baseCutoff_;
        if (useLfo_ && lfoTarget_ == Instrument::LfoTarget::Cutoff) {
            finalCutoff = std::clamp(baseCutoff_ * std::pow(2.0f, lfoVal * 2.0f), 20.0f, 20000.0f);
        }

        float sL = 0.0f, sR = 0.0f;
        if (!isGranular_) {
            double readPos = windowOffset_ + phaseInWindow_;
            sL = interpolateSample(dataL, length, readPos);
            sR = interpolateSample(dataR, length, readPos);

            phaseInWindow_ += step_ * currentPitchRatio_;
            while (phaseInWindow_ >= windowSize_) phaseInWindow_ -= windowSize_;
        } else {
            float grainPhase = static_cast<float>(grainCounter_) / grainLengthSamples_;
            float grainWindow = std::sin(3.14159265f * grainPhase);

            double readPos = grainPos_ + grainCounter_ * currentPitchRatio_;
            sL = interpolateSample(dataL, length, readPos) * grainWindow;
            sR = interpolateSample(dataR, length, readPos) * grainWindow;

            if (++grainCounter_ >= grainLengthSamples_) {
                grainCounter_ = 0;
            }
        }

        sL *= env * gainL;
        sR *= env * gainR;

        if (useFilter_) {
            filterL_.setParameters(svfType_, finalCutoff, baseResonance_);
            filterR_.setParameters(svfType_, finalCutoff, baseResonance_);
            sL = filterL_.process(sL);
            sR = filterR_.process(sR);
        }

        outL[i] += sL;
        outR[i] += sR;
    }
}

void WavetableVoice::setVolume(float v) { volume_ = v; }
void WavetableVoice::setPanning(float p) { pan_ = p; }
void WavetableVoice::setPitchRatio(float ratio) { currentPitchRatio_ = ratio; }
void WavetableVoice::setPosition(float posNormalized) {
    if (sample_ && !sample_->ch[0].empty()) {
        size_t totalLen = sample_->ch[0].size();
        size_t maxOffset = (totalLen > static_cast<size_t>(windowSize_)) ? (totalLen - windowSize_) : 0;
        windowOffset_ = posNormalized * maxOffset;
        grainPos_ = windowOffset_;
    }
}
void WavetableVoice::setCutCountdown(int frames) { cutCountdown_ = frames; }
void WavetableVoice::setRetrigger(int intervalFrames, int count) { (void)count; retriggerCountdown_ = intervalFrames; }
void WavetableVoice::setGlide(float targetRatio, float rate) { glideTargetRatio_ = targetRatio; glideRate_ = rate; }
void WavetableVoice::setReverse(bool reverse) { (void)reverse; }
void WavetableVoice::setCutoff(float cutoffHz) { baseCutoff_ = std::clamp(cutoffHz, 20.0f, 20000.0f); useFilter_ = true; }
void WavetableVoice::setResonance(float res) { baseResonance_ = std::clamp(res, 0.1f, 10.0f); }
void WavetableVoice::setFilterType(int type) {
    if (type == 1) svfType_ = dsp::FilterType::Highpass;
    else if (type == 2) svfType_ = dsp::FilterType::Bandpass;
    else svfType_ = dsp::FilterType::Lowpass;
    useFilter_ = true;
}

} // namespace trakr

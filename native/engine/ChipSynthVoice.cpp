#include "ChipSynthVoice.h"
#include <algorithm>
#include <cmath>

namespace trakr {

namespace {
constexpr int8_t kArpChords[8][4] = {
    {0, 0, 0, 0}, {0, 3, 7, 12}, {0, 4, 7, 12}, {0, 7, 12, 19},
    {0, 5, 7, 12}, {0, 4, 7, 11}, {0, 3, 7, 10}, {0, 12, 0, 12}
};
}

ChipSynthVoice::ChipSynthVoice(int sampleRate)
    : sampleRate_(sampleRate) {
    adsr_.init(static_cast<float>(sampleRate_));
    filterL_.init(static_cast<float>(sampleRate_));
    filterR_.init(static_cast<float>(sampleRate_));
}

void ChipSynthVoice::setSampleRate(int sampleRate) {
    sampleRate_ = sampleRate;
    adsr_.init(static_cast<float>(sampleRate_));
    filterL_.init(static_cast<float>(sampleRate_));
    filterR_.init(static_cast<float>(sampleRate_));
}

float ChipSynthVoice::polyBlep(float t, float dt) const {
    if (dt <= 0.00001f) return 0.0f;
    if (t < dt) {
        float n = t / dt;
        return n + n - n * n - 1.0f;
    }
    if (t > 1.0f - dt) {
        float n = (t - 1.0f) / dt;
        return n * n + n + n + 1.0f;
    }
    return 0.0f;
}

uint16_t ChipSynthVoice::stepLfsr(uint16_t lfsr, bool shortPeriod) const {
    uint16_t bit = (lfsr ^ (lfsr >> (shortPeriod ? 6 : 1))) & 1u;
    return static_cast<uint16_t>((lfsr >> 1) | (bit << 14));
}

void ChipSynthVoice::noteOn(int note, float velocity, const Instrument* inst) {
    baseFreq_ = 440.0f * std::pow(2.0f, (note - 69) / 12.0f);
    currentPitchRatio_ = 1.0f;
    glideTargetRatio_ = -1.0f;
    volume_ = velocity;
    phase_ = 0.0f;
    osc2Phase_ = 0.0f;
    lfsrState_ = 0x7FFFu;
    noteAgeSamples_ = 0;
    arpSampleCounter_ = 0;
    arpStepCounter_ = 0;
    cutCountdown_ = -1;

    adsr_.init(static_cast<float>(sampleRate_));
    if (inst) {
        model_ = static_cast<ChipModel>(std::clamp(inst->chipModel, 0, 7));
        adsr_.setParameters(inst->attackMs * 0.001f, inst->decayMs * 0.001f, inst->sustain, inst->releaseMs * 0.001f);

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
    } else {
        adsr_.setParameters(0.002f, 0.1f, 0.7f, 0.01f);
        useFilter_ = false;
    }
    adsr_.noteOn();
    active_ = true;
}

void ChipSynthVoice::noteOff() {
    adsr_.noteOff();
}

void ChipSynthVoice::stop() {
    active_ = false;
    cutCountdown_ = -1;
}

bool ChipSynthVoice::isActive() const {
    return active_;
}

void ChipSynthVoice::process(float* outL, float* outR, int frames) {
    if (!active_) return;

    float invSr = 1.0f / sampleRate_;
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

        noteAgeSamples_++;

        int8_t arpSemi = 0;
        if (model_ == ChipModel::ArpChord) {
            if (++arpSampleCounter_ >= static_cast<int>(sampleRate_ * 0.06f)) {
                arpSampleCounter_ = 0;
                arpStepCounter_ = (arpStepCounter_ + 1) & 3;
            }
            arpSemi = kArpChords[2][arpStepCounter_];
        }

        float activeFreq = baseFreq_ * currentPitchRatio_ * std::pow(2.0f, arpSemi / 12.0f);
        float dt = std::clamp(activeFreq * invSr, 0.0001f, 0.45f);

        pwmPhase_ += pwmRateHz_ * invSr;
        if (pwmPhase_ >= 1.0f) pwmPhase_ -= 1.0f;
        float pwmLfo = (pwmPhase_ < 0.5f) ? (pwmPhase_ * 4.0f - 1.0f) : (3.0f - pwmPhase_ * 4.0f);
        float duty = std::clamp(0.125f + 0.75f * shape_ + 0.35f * pwmDepth_ * pwmLfo, 0.05f, 0.95f);

        phase_ += dt;
        if (phase_ >= 1.0f) phase_ -= 1.0f;

        float sig = 0.0f;
        switch (model_) {
            case ChipModel::Pulse2A03:
            case ChipModel::ArpChord: {
                sig = (phase_ < duty) ? 0.75f : -0.75f;
                sig += polyBlep(phase_, dt);
                sig -= polyBlep(std::fmod(phase_ - duty + 1.0f, 1.0f), dt);
                break;
            }
            case ChipModel::SidSawTri: {
                float saw = 2.0f * phase_ - 1.0f - polyBlep(phase_, dt);
                float tri = (phase_ < 0.5f) ? (phase_ * 4.0f - 1.0f) : (3.0f - phase_ * 4.0f);
                sig = (1.0f - duty) * tri + duty * saw * 0.8f;
                break;
            }
            case ChipModel::NoiseLfsr: {
                lfsrPhase_ += dt * 8.0f;
                while (lfsrPhase_ >= 1.0f) {
                    lfsrPhase_ -= 1.0f;
                    lfsrState_ = stepLfsr(lfsrState_, shape_ >= 0.5f);
                }
                sig = ((lfsrState_ & 1u) != 0u) ? 0.75f : -0.75f;
                break;
            }
            case ChipModel::FmBell: {
                osc2Phase_ += dt * 3.5f;
                if (osc2Phase_ >= 1.0f) osc2Phase_ -= std::floor(osc2Phase_);
                float mod = std::sin(6.2831853f * osc2Phase_) * (2.5f * env);
                sig = std::sin(6.2831853f * (phase_ + mod * 0.25f)) * 0.8f;
                break;
            }
            case ChipModel::SuperSquare: {
                osc2Phase_ += dt * 1.012f;
                if (osc2Phase_ >= 1.0f) osc2Phase_ -= 1.0f;
                float p1 = (phase_ < 0.5f) ? 0.5f : -0.5f;
                float p2 = (osc2Phase_ < 0.5f) ? 0.5f : -0.5f;
                sig = (p1 + p2) * 0.7f;
                break;
            }
            case ChipModel::ZapLaser: {
                float zapEnv = std::max(0.0f, 1.0f - noteAgeSamples_ * invSr * 12.0f);
                phase_ += dt * (1.0f + 8.0f * zapEnv * zapEnv);
                if (phase_ >= 1.0f) phase_ -= 1.0f;
                sig = ((phase_ < 0.5f) ? 0.8f : -0.8f) * zapEnv;
                break;
            }
            case ChipModel::ChipDrum: {
                float drumEnv = std::max(0.0f, 1.0f - noteAgeSamples_ * invSr * 8.0f);
                phase_ += dt * (1.0f + 16.0f * drumEnv);
                if (phase_ >= 1.0f) phase_ -= 1.0f;
                lfsrState_ = stepLfsr(lfsrState_, false);
                float noise = ((lfsrState_ & 1u) != 0u) ? 0.4f : -0.4f;
                sig = std::sin(6.2831853f * phase_) * drumEnv + noise * drumEnv * 0.5f;
                break;
            }
        }

        float sL = sig * env * gainL;
        float sR = sig * env * gainR;

        if (useFilter_) {
            filterL_.setParameters(svfType_, baseCutoff_, baseResonance_);
            filterR_.setParameters(svfType_, baseCutoff_, baseResonance_);
            sL = filterL_.process(sL);
            sR = filterR_.process(sR);
        }

        outL[i] += sL;
        outR[i] += sR;
    }
}

void ChipSynthVoice::setVolume(float v) { volume_ = v; }
void ChipSynthVoice::setPanning(float p) { pan_ = p; }
void ChipSynthVoice::setPitchRatio(float ratio) { currentPitchRatio_ = ratio; }
void ChipSynthVoice::setPosition(float posNormalized) { shape_ = posNormalized; }
void ChipSynthVoice::setCutCountdown(int frames) { cutCountdown_ = frames; }
void ChipSynthVoice::setRetrigger(int intervalFrames, int count) { (void)intervalFrames; (void)count; }
void ChipSynthVoice::setGlide(float targetRatio, float rate) { glideTargetRatio_ = targetRatio; glideRate_ = rate; }
void ChipSynthVoice::setReverse(bool reverse) { (void)reverse; }
void ChipSynthVoice::setCutoff(float cutoffHz) { baseCutoff_ = std::clamp(cutoffHz, 20.0f, 20000.0f); useFilter_ = true; }
void ChipSynthVoice::setResonance(float res) { baseResonance_ = std::clamp(res, 0.1f, 10.0f); }
void ChipSynthVoice::setFilterType(int type) {
    if (type == 1) svfType_ = dsp::FilterType::Highpass;
    else if (type == 2) svfType_ = dsp::FilterType::Bandpass;
    else svfType_ = dsp::FilterType::Lowpass;
    useFilter_ = true;
}

} // namespace trakr

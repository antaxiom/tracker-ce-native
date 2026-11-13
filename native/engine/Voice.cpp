#include "Voice.h"
#include <algorithm>

namespace trakr {

SampleVoice::SampleVoice(int sampleRate)
    : sampleRate_(sampleRate) {
    setSampleRate(sampleRate);
}

void SampleVoice::setSampleRate(int sampleRate) {
    sampleRate_ = std::max(8000, sampleRate);
}

void SampleVoice::noteOn(int note, float velocity, const Instrument* inst) {
    inst_ = inst;
    if (!inst) {
        stop();
        return;
    }

    static std::shared_ptr<const SampleData> sFallbackSynth = []() {
        auto sd = std::make_shared<SampleData>();
        sd->sampleRate = 44100;
        sd->channels = 1;
        sd->ch[0].resize(2048);
        for (size_t i = 0; i < sd->ch[0].size(); ++i) {
            float phase = static_cast<float>(i) / 2048.0f;
            float sum = 0.0f;
            for (int h = 1; h <= 8; ++h) sum += (std::sin(2.0f * 3.14159265f * h * phase) / (h * h));
            sd->ch[0][i] = sum * 0.30f;
        }
        return sd;
    }();

    sample_ = (inst->sample && !inst->sample->ch[0].empty()) ? inst->sample : sFallbackSynth;
    rootNote_ = inst->rootNote;
    finetune_ = inst->finetune;
    loopMode_ = (inst->sample && !inst->sample->ch[0].empty()) ? inst->loop : Instrument::Loop::None;

    const size_t totalFrames = sample_->ch[0].size();
    loopStart_ = std::clamp(inst->loopStart, 0, static_cast<int>(totalFrames - 1));
    loopEnd_ = inst->loopEnd > inst->loopStart ? std::min(inst->loopEnd, static_cast<int>(totalFrames)) : static_cast<int>(totalFrames);
    endFrame_ = inst->endFrame > 0 ? std::min(inst->endFrame, static_cast<int>(totalFrames)) : static_cast<int>(totalFrames);

    pos_ = static_cast<double>(std::clamp(inst->startFrame, 0, static_cast<int>(totalFrames - 1)));
    if (loopMode_ == Instrument::Loop::Slice && inst->sample && !inst->slices.empty()) {
        int sIdx = std::clamp(inst->selectedSlice, 0, static_cast<int>(inst->slices.size()) - 1);
        pos_ = static_cast<double>(inst->slices[sIdx]);
        int nextSlice = (sIdx + 1 < static_cast<int>(inst->slices.size())) ? inst->slices[sIdx + 1] : endFrame_;
        endFrame_ = nextSlice;
    }
    pingPongForward_ = true;

    // Pitch calculation (Manual 6.3 Tune -24..+24 st, Finetune -100..+100 cents)
    const float semitones = static_cast<float>(note - rootNote_ + inst->tune) + static_cast<float>(finetune_) / 100.0f;
    const float rateFactor = static_cast<float>(sample_->sampleRate) / static_cast<float>(sampleRate_);
    basePitchRatio_ = std::pow(2.0f, semitones / 12.0f) * rateFactor;
    currentPitchRatio_ = 1.0f;
    step_ = basePitchRatio_;

    // Panning & FX (Manual 6.3 Panning -50..+50, 6.4 Overdrive & Bit depth)
    pan_ = std::clamp(inst->panning / 50.0f, -1.0f, 1.0f);
    panForTrig_ = -2.0f;
    overdrive_ = static_cast<float>(inst->overdrive);
    bitDepth_ = inst->bitDepth;

    // Volume & velocity (Manual 6.3 Volume 0..100: 50 is 0.0 dB, 0 is -inf, 100 is +24 dB)
    float volDb = inst->gainDb;
    if (inst->volume != 50) {
        volDb += (inst->volume == 0) ? -100.0f : ((inst->volume - 50) * 0.48f);
    }
    const float dbGain = (volDb <= -99.0f) ? 0.0f : std::pow(10.0f, volDb / 20.0f);
    volume_ = std::clamp(velocity, 0.0f, 1.0f) * dbGain;

    // ADSR Envelope
    adsr_.init(static_cast<float>(sampleRate_));
    adsr_.setParameters(inst->attackMs * 0.001f, inst->decayMs * 0.001f, inst->sustain, inst->releaseMs * 0.001f);
    adsr_.noteOn();

    // SVF Filter (Manual 6.3 Filter Type, Cutoff 0..100%, Resonance 0..100%)
    if (inst->filterType != Instrument::FilterType::None) {
        useFilter_ = true;
        float cutHz = (inst->cutoff < 100) ? (20.0f * std::pow(1000.0f, inst->cutoff / 100.0f)) : inst->filterCutoffHz;
        baseCutoff_ = std::clamp(cutHz, 20.0f, 20000.0f);
        float resQ = (inst->resonance > 0) ? (0.707f + (inst->resonance / 100.0f) * 6.0f) : inst->filterResonance;
        baseResonance_ = std::clamp(resQ, 0.1f, 10.0f);
        svfType_ = dsp::FilterType::Lowpass;
        if (inst->filterType == Instrument::FilterType::Highpass) svfType_ = dsp::FilterType::Highpass;
        else if (inst->filterType == Instrument::FilterType::Bandpass) svfType_ = dsp::FilterType::Bandpass;
        filterL_.init(static_cast<float>(sampleRate_));
        filterR_.init(static_cast<float>(sampleRate_));
        filterL_.setParameters(svfType_, baseCutoff_, baseResonance_);
        filterR_.setParameters(svfType_, baseCutoff_, baseResonance_);
    } else {
        useFilter_ = false;
    }

    // LFO
    if (inst->lfoTarget != Instrument::LfoTarget::None && inst->lfoDepth > 0.001f) {
        useLfo_ = true;
        lfoTarget_ = inst->lfoTarget;
        dsp::LfoWaveform wave = dsp::LfoWaveform::Sine;
        if (inst->lfoWave == Instrument::LfoWave::Triangle) wave = dsp::LfoWaveform::Triangle;
        else if (inst->lfoWave == Instrument::LfoWave::Saw) wave = dsp::LfoWaveform::Saw;
        else if (inst->lfoWave == Instrument::LfoWave::Square) wave = dsp::LfoWaveform::Square;
        lfo_.init(static_cast<float>(sampleRate_));
        lfo_.setParameters(wave, inst->lfoRateHz, inst->lfoDepth, static_cast<dsp::LfoTarget>(inst->lfoTarget));
    } else {
        useLfo_ = false;
    }

    // Step FX reset
    cutCountdown_ = -1;
    retriggerInterval_ = 0;
    retriggerCountdown_ = 0;
    retriggerRemaining_ = 0;
    glideTargetRatio_ = -1.0f;
    reverse_ = false;

    active_ = true;
}

void SampleVoice::noteOff() {
    adsr_.noteOff();
}

void SampleVoice::stop() {
    active_ = false;
    adsr_.reset();
    sample_.reset();
}

bool SampleVoice::isActive() const {
    return active_;
}

void SampleVoice::setVolume(float v) {
    volume_ = std::max(0.0f, v);
}

void SampleVoice::setPanning(float p) {
    pan_ = std::clamp(p, -1.0f, 1.0f);
    panForTrig_ = -2.0f;
}

void SampleVoice::setPitchRatio(float ratio) {
    currentPitchRatio_ = std::max(0.01f, ratio);
    step_ = basePitchRatio_ * currentPitchRatio_;
}

void SampleVoice::setPosition(float posNormalized) {
    if (!sample_ || sample_->ch[0].empty()) return;
    const size_t total = sample_->ch[0].size();
    pos_ = std::clamp(static_cast<double>(posNormalized) * total, 0.0, static_cast<double>(total - 1));
}

void SampleVoice::setCutCountdown(int frames) {
    cutCountdown_ = frames;
}

void SampleVoice::setRetrigger(int intervalFrames, int count) {
    retriggerInterval_ = intervalFrames;
    retriggerCountdown_ = intervalFrames;
    retriggerRemaining_ = count;
}

void SampleVoice::setGlide(float targetRatio, float rate) {
    glideTargetRatio_ = targetRatio;
    glideRate_ = rate;
}

void SampleVoice::setReverse(bool reverse) {
    reverse_ = reverse;
    if (reverse_ && sample_ && !sample_->ch[0].empty()) {
        pos_ = (endFrame_ > 0) ? static_cast<double>(endFrame_ - 1) : static_cast<double>(sample_->ch[0].size() - 1);
    }
}

float SampleVoice::interpolateCubic(const float* data, size_t length, double pos) const {
    const int idx = static_cast<int>(pos);
    const float frac = static_cast<float>(pos - idx);

    const int i0 = std::max(0, idx - 1);
    const int i1 = std::min(static_cast<int>(length - 1), idx);
    const int i2 = std::min(static_cast<int>(length - 1), idx + 1);
    const int i3 = std::min(static_cast<int>(length - 1), idx + 2);

    const float y0 = data[i0];
    const float y1 = data[i1];
    const float y2 = data[i2];
    const float y3 = data[i3];

    // Catmull-Rom spline
    const float a = -0.5f * y0 + 1.5f * y1 - 1.5f * y2 + 0.5f * y3;
    const float b = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
    const float c = -0.5f * y0 + 0.5f * y2;
    const float d = y1;

    return ((a * frac + b) * frac + c) * frac + d;
}

void SampleVoice::process(float* outL, float* outR, int frames) {
    if (!active_ || !sample_ || frames <= 0) return;

    const auto& leftCh = sample_->ch[0];
    const bool isStereo = (sample_->channels > 1 && !sample_->ch[1].empty());
    const float* dataL = leftCh.data();
    const float* dataR = isStereo ? sample_->ch[1].data() : leftCh.data();
    const size_t totalFrames = leftCh.size();
    const bool applyBitDepth = (bitDepth_ < 16 && bitDepth_ >= 4);
    const float bitDepthSteps =
        applyBitDepth ? std::pow(2.0f, static_cast<float>(bitDepth_ - 1)) : 0.0f;

    for (int i = 0; i < frames; ++i) {
        if (!active_) break;

        if (cutCountdown_ > 0) {
            if (--cutCountdown_ == 0) {
                stop();
                break;
            }
        }

        if (retriggerInterval_ > 0) {
            if (--retriggerCountdown_ <= 0) {
                retriggerCountdown_ = retriggerInterval_;
                pos_ = reverse_ ? ((endFrame_ > 0) ? endFrame_ - 1 : sample_->ch[0].size() - 1) : 0.0;
                adsr_.noteOn();
                if (retriggerRemaining_ > 0 && --retriggerRemaining_ == 0) {
                    retriggerInterval_ = 0;
                }
            }
        }

        if (glideTargetRatio_ > 0.0f) {
            currentPitchRatio_ += (glideTargetRatio_ - currentPitchRatio_) * glideRate_;
            step_ = basePitchRatio_ * currentPitchRatio_;
        }

        float envVal = adsr_.process();
        if (!adsr_.isActive()) {
            stop();
            break;
        }

        const float sampL = interpolateCubic(dataL, totalFrames, pos_) * headroomGain_;
        const float sampR = interpolateCubic(dataR, totalFrames, pos_) * headroomGain_;

        float gain = volume_ * envVal;
        float currentPan = pan_;
        if (useLfo_) {
            float lfoVal = lfo_.process();
            if (lfoTarget_ == Instrument::LfoTarget::Volume) {
                gain *= std::clamp(1.0f + lfoVal, 0.0f, 2.0f);
            } else if (lfoTarget_ == Instrument::LfoTarget::Panning) {
                currentPan = std::clamp(currentPan + lfoVal, -1.0f, 1.0f);
            } else if (lfoTarget_ == Instrument::LfoTarget::Cutoff && useFilter_) {
                float modCutoff = std::clamp(baseCutoff_ * (1.0f + lfoVal), 20.0f, static_cast<float>(sampleRate_) * 0.49f);
                filterL_.setParameters(svfType_, modCutoff, baseResonance_);
                filterR_.setParameters(svfType_, modCutoff, baseResonance_);
            }
        }

        float voiceL = sampL * gain;
        float voiceR = sampR * gain;

        if (useFilter_) {
            voiceL = filterL_.process(voiceL);
            voiceR = filterR_.process(voiceR);
        }

        if (overdrive_ > 0.0f) {
            const float drive = 1.0f + (overdrive_ / 100.0f) * 3.5f;
            voiceL = std::tanh(voiceL * drive);
            voiceR = std::tanh(voiceR * drive);
        }

        if (applyBitDepth) {
            voiceL = std::round(voiceL * bitDepthSteps) / bitDepthSteps;
            voiceR = std::round(voiceR * bitDepthSteps) / bitDepthSteps;
        }

        if (currentPan != panForTrig_) {
            panForTrig_ = currentPan;
            const float panAngle = (currentPan + 1.0f) * (static_cast<float>(M_PI) / 4.0f);
            cachedPanL_ = std::cos(panAngle);
            cachedPanR_ = std::sin(panAngle);
        }

        outL[i] += voiceL * cachedPanL_;
        outR[i] += voiceR * cachedPanR_;

        // Position advance & looping
        if (reverse_) {
            pos_ -= step_;
            if (pos_ <= 0.0) {
                stop();
                break;
            }
        } else if (loopMode_ == Instrument::Loop::None) {
            pos_ += step_;
            if (pos_ >= endFrame_) {
                stop();
                break;
            }
        } else if (loopMode_ == Instrument::Loop::Forward) {
            pos_ += step_;
            if (pos_ >= loopEnd_) {
                const double loopLen = loopEnd_ - loopStart_;
                if (loopLen > 0) {
                    pos_ = loopStart_ + std::fmod(pos_ - loopStart_, loopLen);
                } else {
                    stop();
                    break;
                }
            }
        } else if (loopMode_ == Instrument::Loop::Backward) {
            pos_ -= step_;
            if (pos_ < loopStart_) {
                const double loopLen = loopEnd_ - loopStart_;
                if (loopLen > 0) {
                    pos_ = loopEnd_ - std::fmod(loopStart_ - pos_, loopLen);
                } else {
                    stop();
                    break;
                }
            }
        } else if (loopMode_ == Instrument::Loop::PingPong) {
            if (pingPongForward_) {
                pos_ += step_;
                if (pos_ >= loopEnd_) {
                    pos_ = loopEnd_ - (pos_ - loopEnd_);
                    pingPongForward_ = false;
                }
            } else {
                pos_ -= step_;
                if (pos_ <= loopStart_) {
                    pos_ = loopStart_ + (loopStart_ - pos_);
                    pingPongForward_ = true;
                }
            }
        }
    }
}

void SampleVoice::setCutoff(float cutoffHz) {
    baseCutoff_ = std::clamp(cutoffHz, 20.0f, 20000.0f);
    useFilter_ = true;
}

void SampleVoice::setResonance(float res) {
    baseResonance_ = std::clamp(res, 0.1f, 10.0f);
}

void SampleVoice::setFilterType(int type) {
    if (type == 1) svfType_ = dsp::FilterType::Highpass;
    else if (type == 2) svfType_ = dsp::FilterType::Bandpass;
    else svfType_ = dsp::FilterType::Lowpass;
    useFilter_ = true;
}

} // namespace trakr

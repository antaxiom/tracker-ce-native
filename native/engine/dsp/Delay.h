#pragma once

#include <vector>
#include <cmath>
#include <algorithm>

namespace trakr::dsp {

class Delay {
public:
    void init(float sampleRate, float maxDelaySec = 2.0f) {
        sampleRate_ = std::max(1000.0f, sampleRate);
        size_t bufferSize = static_cast<size_t>(sampleRate_ * maxDelaySec) + 16;
        bufferL_.assign(bufferSize, 0.0f);
        bufferR_.assign(bufferSize, 0.0f);
        writeIndex_ = 0;
        setParameters(0.35f, 0.4f, 0.3f, 4000.0f);
    }

    void reset() {
        std::fill(bufferL_.begin(), bufferL_.end(), 0.0f);
        std::fill(bufferR_.begin(), bufferR_.end(), 0.0f);
        writeIndex_ = 0;
        filterStateL_ = 0.0f;
        filterStateR_ = 0.0f;
    }

    void setParameters(float delaySec, float feedback, float mix, float dampingCutoffHz = 4000.0f) {
        delaySamples_ = std::clamp(delaySec * sampleRate_, 1.0f, static_cast<float>(bufferL_.size() - 2));
        feedback_ = std::clamp(feedback, 0.0f, 0.95f); // Safe ceiling < 1.0
        mix_ = std::clamp(mix, 0.0f, 1.0f);

        // Simple 1-pole damping filter coefficient
        float x = 2.0f * static_cast<float>(M_PI) * dampingCutoffHz / sampleRate_;
        dampCoeff_ = std::clamp(x / (x + 1.0f), 0.01f, 0.99f);
    }

    void process(float inL, float inR, float& outL, float& outR) {
        if (bufferL_.empty()) {
            outL = inL;
            outR = inR;
            return;
        }

        const size_t bufSize = bufferL_.size();

        // Read with linear interpolation
        float readPos = static_cast<float>(writeIndex_) - delaySamples_;
        while (readPos < 0.0f) readPos += static_cast<float>(bufSize);

        size_t i0 = static_cast<size_t>(readPos) % bufSize;
        size_t i1 = (i0 + 1) % bufSize;
        float frac = readPos - static_cast<float>(static_cast<size_t>(readPos));

        float delayedL = bufferL_[i0] + frac * (bufferL_[i1] - bufferL_[i0]);
        float delayedR = bufferR_[i0] + frac * (bufferR_[i1] - bufferR_[i0]);

        // Damping lowpass
        filterStateL_ += dampCoeff_ * (delayedL - filterStateL_);
        filterStateR_ += dampCoeff_ * (delayedR - filterStateR_);

        // Soft clip feedback to eliminate blowups
        float fbL = std::tanh(inL + filterStateL_ * feedback_);
        float fbR = std::tanh(inR + filterStateR_ * feedback_);

        bufferL_[writeIndex_] = fbL;
        bufferR_[writeIndex_] = fbR;

        writeIndex_ = (writeIndex_ + 1) % bufSize;

        outL = (1.0f - mix_) * inL + mix_ * delayedL;
        outR = (1.0f - mix_) * inR + mix_ * delayedR;
    }

private:
    float sampleRate_{44100.0f};
    std::vector<float> bufferL_;
    std::vector<float> bufferR_;
    size_t writeIndex_{0};

    float delaySamples_{10000.0f};
    float feedback_{0.4f};
    float mix_{0.3f};
    float dampCoeff_{0.5f};
    float filterStateL_{0.0f};
    float filterStateR_{0.0f};
};

} // namespace trakr::dsp


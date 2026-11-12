#pragma once

#include <vector>
#include <cmath>
#include <algorithm>

namespace trakr::dsp {

class CombFilter {
public:
    void init(size_t delayLength) {
        buffer_.assign(std::max<size_t>(delayLength, 1), 0.0f);
        index_ = 0;
        filterState_ = 0.0f;
    }

    void reset() {
        std::fill(buffer_.begin(), buffer_.end(), 0.0f);
        index_ = 0;
        filterState_ = 0.0f;
    }

    float process(float input, float feedback, float damp) {
        float output = buffer_[index_];
        filterState_ = output * (1.0f - damp) + filterState_ * damp;
        // Soft saturate feedback to strictly prevent any numeric blow-up
        buffer_[index_] = std::tanh(input + filterState_ * feedback);
        index_ = (index_ + 1) % buffer_.size();
        return output;
    }

private:
    std::vector<float> buffer_;
    size_t index_{0};
    float filterState_{0.0f};
};

class AllpassFilter {
public:
    void init(size_t delayLength) {
        buffer_.assign(std::max<size_t>(delayLength, 1), 0.0f);
        index_ = 0;
    }

    void reset() {
        std::fill(buffer_.begin(), buffer_.end(), 0.0f);
        index_ = 0;
    }

    float process(float input, float feedback = 0.5f) {
        float bufOut = buffer_[index_];
        float output = -input + bufOut;
        buffer_[index_] = input + (bufOut * feedback);
        index_ = (index_ + 1) % buffer_.size();
        return output;
    }

private:
    std::vector<float> buffer_;
    size_t index_{0};
};

class Reverb {
public:
    void init(float sampleRate) {
        sampleRate_ = std::max(1000.0f, sampleRate);
        float scale = sampleRate_ / 44100.0f;

        const size_t combLengths[8] = {
            static_cast<size_t>(1116 * scale),
            static_cast<size_t>(1188 * scale),
            static_cast<size_t>(1277 * scale),
            static_cast<size_t>(1356 * scale),
            static_cast<size_t>(1422 * scale),
            static_cast<size_t>(1491 * scale),
            static_cast<size_t>(1557 * scale),
            static_cast<size_t>(1617 * scale)
        };

        const size_t allpassLengths[4] = {
            static_cast<size_t>(556 * scale),
            static_cast<size_t>(441 * scale),
            static_cast<size_t>(341 * scale),
            static_cast<size_t>(225 * scale)
        };

        for (int i = 0; i < 8; ++i) {
            combsL_[i].init(combLengths[i]);
            combsR_[i].init(combLengths[i] + 23); // Stereo spread offset
        }

        for (int i = 0; i < 4; ++i) {
            allpassL_[i].init(allpassLengths[i]);
            allpassR_[i].init(allpassLengths[i] + 23);
        }

        setParameters(0.5f, 0.2f, 0.25f);
    }

    void reset() {
        for (int i = 0; i < 8; ++i) {
            combsL_[i].reset();
            combsR_[i].reset();
        }
        for (int i = 0; i < 4; ++i) {
            allpassL_[i].reset();
            allpassR_[i].reset();
        }
    }

    void setParameters(float roomSize, float damping, float mix) {
        roomSize_ = std::clamp(roomSize, 0.0f, 0.98f);
        damping_ = std::clamp(damping, 0.0f, 1.0f);
        mix_ = std::clamp(mix, 0.0f, 1.0f);
    }

    void process(float inL, float inR, float& outL, float& outR) {
        float monoIn = (inL + inR) * 0.015f;

        float outCombL = 0.0f;
        float outCombR = 0.0f;

        for (int i = 0; i < 8; ++i) {
            outCombL += combsL_[i].process(monoIn, roomSize_, damping_);
            outCombR += combsR_[i].process(monoIn, roomSize_, damping_);
        }

        for (int i = 0; i < 4; ++i) {
            outCombL = allpassL_[i].process(outCombL);
            outCombR = allpassR_[i].process(outCombR);
        }

        outL = (1.0f - mix_) * inL + mix_ * outCombL;
        outR = (1.0f - mix_) * inR + mix_ * outCombR;
    }

private:
    float sampleRate_{44100.0f};
    float roomSize_{0.5f};
    float damping_{0.2f};
    float mix_{0.25f};

    CombFilter combsL_[8];
    CombFilter combsR_[8];
    AllpassFilter allpassL_[4];
    AllpassFilter allpassR_[4];
};

} // namespace trakr::dsp


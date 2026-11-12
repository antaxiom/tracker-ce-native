#pragma once

#include <cmath>
#include <algorithm>

namespace trakr::dsp {

enum class LfoWaveform {
    Sine,
    Triangle,
    Saw,
    Square
};

enum class LfoTarget {
    None,
    Pitch,
    Volume,
    Panning,
    Cutoff
};

class Lfo {
public:
    void init(float sampleRate) {
        sampleRate_ = std::max(1000.0f, sampleRate);
        phase_ = 0.0f;
    }

    void reset() {
        phase_ = 0.0f;
    }

    void setParameters(LfoWaveform wave, float freqHz, float depth, LfoTarget target = LfoTarget::None) {
        wave_ = wave;
        freqHz_ = std::max(0.01f, freqHz);
        depth_ = std::clamp(depth, 0.0f, 1.0f);
        target_ = target;
        phaseInc_ = freqHz_ / sampleRate_;
    }

    float process() {
        float val = 0.0f;
        switch (wave_) {
            case LfoWaveform::Sine:
                val = std::sin(phase_ * 2.0f * static_cast<float>(M_PI));
                break;
            case LfoWaveform::Triangle:
                val = 4.0f * std::fabs(phase_ - 0.5f) - 1.0f;
                break;
            case LfoWaveform::Saw:
                val = 2.0f * phase_ - 1.0f;
                break;
            case LfoWaveform::Square:
                val = (phase_ < 0.5f) ? 1.0f : -1.0f;
                break;
        }

        phase_ += phaseInc_;
        if (phase_ >= 1.0f) phase_ -= 1.0f;

        return val * depth_;
    }

    LfoTarget target() const { return target_; }
    float depth() const { return depth_; }

private:
    float sampleRate_{44100.0f};
    LfoWaveform wave_{LfoWaveform::Sine};
    LfoTarget target_{LfoTarget::None};
    float freqHz_{1.0f};
    float depth_{0.5f};
    float phase_{0.0f};
    float phaseInc_{0.0f};
};

} // namespace trakr::dsp


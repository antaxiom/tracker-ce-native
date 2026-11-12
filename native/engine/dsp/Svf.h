#pragma once

#include <cmath>
#include <algorithm>

namespace trakr::dsp {

enum class FilterType {
    Lowpass,
    Highpass,
    Bandpass,
    Notch
};

class Svf {
public:
    void init(float sampleRate) {
        sampleRate_ = std::max(1000.0f, sampleRate);
        reset();
    }

    void reset() {
        ic1eq_ = 0.0f;
        ic2eq_ = 0.0f;
    }

    void setParameters(FilterType type, float cutoffHz, float resonance) {
        type_ = type;
        cutoffHz = std::clamp(cutoffHz, 20.0f, sampleRate_ * 0.49f);
        resonance = std::clamp(resonance, 0.1f, 10.0f);

        // Bilinear transform mapping
        float g = std::tan(static_cast<float>(M_PI) * cutoffHz / sampleRate_);
        k_ = 1.0f / resonance;
        a1_ = 1.0f / (1.0f + g * (g + k_));
        a2_ = g * a1_;
        a3_ = g * a2_;
    }

    float process(float v0) {
        float v3 = v0 - ic2eq_;
        float v1 = a1_ * ic1eq_ + a2_ * v3;
        float v2 = ic2eq_ + a2_ * ic1eq_ + a3_ * v3;
        ic1eq_ = 2.0f * v1 - ic1eq_;
        ic2eq_ = 2.0f * v2 - ic2eq_;

        float low = v2;
        float band = v1;
        float high = v0 - k_ * v1 - v2;
        float notch = high + low;

        switch (type_) {
            case FilterType::Lowpass: return low;
            case FilterType::Highpass: return high;
            case FilterType::Bandpass: return band;
            case FilterType::Notch: return notch;
        }
        return low;
    }

private:
    float sampleRate_{44100.0f};
    FilterType type_{FilterType::Lowpass};
    float k_{1.0f};
    float a1_{0.0f};
    float a2_{0.0f};
    float a3_{0.0f};
    float ic1eq_{0.0f};
    float ic2eq_{0.0f};
};

} // namespace trakr::dsp


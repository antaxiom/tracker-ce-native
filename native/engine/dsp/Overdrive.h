#pragma once

#include <cmath>
#include <algorithm>

namespace trakr::dsp {

// Soft-clipping saturation overdrive matching Polyend audio processor
class Overdrive {
public:
    Overdrive() = default;

    void setDrive(float driveNormalized) {
        // driveNormalized: 0.0 (clean) to 1.0 (heavy saturation)
        drive_ = std::clamp(driveNormalized, 0.0f, 1.0f);
        gain_ = 1.0f + drive_ * 8.0f;
    }

    float process(float in) const {
        if (drive_ <= 0.001f) return in;
        float x = in * gain_;
        // Pade rational approximation of tanh for musical soft clipping
        float x2 = x * x;
        float saturated = (x * (27.0f + x2)) / (27.0f + 9.0f * x2);
        // Normalize output level
        return saturated * (1.0f / (1.0f + drive_ * 0.5f));
    }

private:
    float drive_{0.0f};
    float gain_{1.0f};
};

} // namespace trakr::dsp


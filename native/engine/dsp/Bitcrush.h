#pragma once

#include <cmath>
#include <algorithm>

namespace trakr::dsp {

// Bit depth reduction and sample rate decimation matching Polyend effect_bitDepth
class Bitcrush {
public:
    Bitcrush() = default;

    void setBits(float bits) {
        // bits: 1.0 to 16.0
        bits_ = std::clamp(bits, 1.0f, 16.0f);
        levels_ = std::pow(2.0f, bits_);
        invLevels_ = 1.0f / levels_;
    }

    void setDecimation(int factor) {
        decimate_ = std::clamp(factor, 1, 32);
    }

    float process(float in) {
        if (bits_ >= 15.9f && decimate_ <= 1) return in;

        if (++sampleCounter_ >= decimate_) {
            sampleCounter_ = 0;
            // Quantize amplitude to discrete bit levels
            float scaled = std::clamp(in, -1.0f, 1.0f);
            heldSample_ = std::round(scaled * levels_ * 0.5f) * invLevels_ * 2.0f;
        }
        return heldSample_;
    }

    void reset() {
        sampleCounter_ = 0;
        heldSample_ = 0.0f;
    }

private:
    float bits_{16.0f};
    float levels_{65536.0f};
    float invLevels_{1.0f / 65536.0f};
    int decimate_{1};
    int sampleCounter_{0};
    float heldSample_{0.0f};
};

} // namespace trakr::dsp


#pragma once

#include <cmath>
#include <algorithm>
#include <array>

namespace trakr::dsp {

// 5-Band Master Graphic Equalizer
// Frequencies matching Polyend Tracker hardware: 115 Hz, 330 Hz, 990 Hz, 3000 Hz, 9900 Hz
// Range: -12.0 dB to +12.0 dB
class MasterEq {
public:
    void init(float sampleRate) {
        sampleRate_ = std::max(1000.0f, sampleRate);
        reset();
    }

    void reset() {
        for (auto& b : bandsL_) b.reset();
        for (auto& b : bandsR_) b.reset();
    }

    void setGains(const float gainsDb[5]) {
        static constexpr float kFreqs[5] = {115.0f, 330.0f, 990.0f, 3000.0f, 9900.0f};
        static constexpr float kQ[5] = {1.0f, 1.2f, 1.2f, 1.2f, 1.0f};

        bool anyActive = false;
        for (int i = 0; i < 5; ++i) {
            float gDb = std::clamp(gainsDb[i], -12.0f, 12.0f);
            if (std::abs(gDb) > 0.05f) anyActive = true;
            bandsL_[i].setPeaking(kFreqs[i], kQ[i], gDb, sampleRate_);
            bandsR_[i].setPeaking(kFreqs[i], kQ[i], gDb, sampleRate_);
        }
        active_ = anyActive;
    }

    bool isActive() const { return active_; }

    void process(float& l, float& r) {
        if (!active_) return;
        for (int i = 0; i < 5; ++i) {
            l = bandsL_[i].process(l);
            r = bandsR_[i].process(r);
        }
    }

private:
    struct Biquad {
        float b0{1.0f}, b1{0.0f}, b2{0.0f}, a1{0.0f}, a2{0.0f};
        float x1{0.0f}, x2{0.0f}, y1{0.0f}, y2{0.0f};

        void reset() { x1 = x2 = y1 = y2 = 0.0f; }

        void setPeaking(float f0, float q, float gainDb, float sr) {
            if (std::abs(gainDb) < 0.05f) {
                b0 = 1.0f; b1 = b2 = a1 = a2 = 0.0f;
                return;
            }
            float A = std::pow(10.0f, gainDb / 40.0f);
            float w0 = 2.0f * static_cast<float>(M_PI) * std::clamp(f0, 20.0f, sr * 0.45f) / sr;
            float alpha = std::sin(w0) / (2.0f * q);
            float cosw0 = std::cos(w0);

            float a0 = 1.0f + alpha / A;
            b0 = (1.0f + alpha * A) / a0;
            b1 = (-2.0f * cosw0) / a0;
            b2 = (1.0f - alpha * A) / a0;
            a1 = (-2.0f * cosw0) / a0;
            a2 = (1.0f - alpha / A) / a0;
        }

        float process(float in) {
            float out = b0 * in + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
            x2 = x1; x1 = in;
            y2 = y1; y1 = out;
            return out;
        }
    };

    float sampleRate_{44100.0f};
    bool active_{false};
    std::array<Biquad, 5> bandsL_;
    std::array<Biquad, 5> bandsR_;
};

} // namespace trakr::dsp

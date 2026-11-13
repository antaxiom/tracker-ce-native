#pragma once

// Firmware track fader curve.
// Source: core/AudioEngine/ConstantValues/ampLogarythmicValues.h
//   const float mixAmpLogValues[AMP_LOG_VALUES]   (101 entries)
// Converted to decibels (20*log10) so the app can map a gain to the same
// fader position the firmware uses: index 0 is silence, 86 is 0 dB and 100 is
// +6 dB. Do not re-tune: this is the hardware curve.

namespace trakr {

inline constexpr int kFaderPositions = 101;
inline constexpr float kFaderSilenceDb = -200.0f;

inline constexpr float kFaderDb[kFaderPositions] = {
    -200.0000f, -36.0000f, -35.1600f, -34.7400f, -34.3200f,
    -33.9000f, -33.4800f, -33.0600f, -32.6400f, -32.2200f,
    -31.8000f, -31.3800f, -30.9600f, -30.5400f, -30.1200f,
    -29.7000f, -29.2800f, -28.8600f, -28.4400f, -28.0200f,
    -27.6000f, -27.1800f, -26.7600f, -26.3400f, -25.9200f,
    -25.5000f, -25.0800f, -24.6600f, -24.2400f, -23.8200f,
    -23.4000f, -22.9800f, -22.5600f, -22.1400f, -21.7200f,
    -21.3000f, -20.8800f, -20.4600f, -20.0400f, -19.6200f,
    -19.2000f, -18.7800f, -18.3600f, -17.9400f, -17.5200f,
    -17.1000f, -16.6800f, -16.2600f, -15.8400f, -15.4200f,
    -15.0000f, -14.5800f, -14.1600f, -13.7400f, -13.3200f,
    -12.9000f, -12.4800f, -12.0600f, -11.6400f, -11.2200f,
    -10.8000f, -10.3800f, -9.9600f, -9.5400f, -9.1200f,
    -8.7000f, -8.2800f, -7.8600f, -7.4400f, -7.0200f,
    -6.6000f, -6.1800f, -5.7600f, -5.3400f, -4.9200f,
    -4.5000f, -4.0800f, -3.6600f, -3.2400f, -2.8200f,
    -2.4000f, -1.9800f, -1.5600f, -1.1400f, -0.7200f,
    -0.3000f, 0.0000f, 0.5400f, 0.9600f, 1.3800f,
    1.8000f, 2.2200f, 2.6400f, 3.0600f, 3.4800f,
    3.9000f, 4.3200f, 4.7400f, 5.1600f, 5.5800f,
    6.0000f,
};

// Fader position (0..100) that produces `gainDb`, matching mixAmpLogValues.
inline int faderValueForGainDb(float gainDb) {
    if (gainDb <= kFaderSilenceDb) return 0;
    int best = 1;
    float bestDelta = 1e9f;
    for (int i = 1; i < kFaderPositions; ++i) {
        const float d = kFaderDb[i] > gainDb ? kFaderDb[i] - gainDb : gainDb - kFaderDb[i];
        if (d < bestDelta) {
            bestDelta = d;
            best = i;
        }
    }
    return best;
}

inline float gainDbForFaderValue(int value) {
    if (value <= 0) return kFaderSilenceDb;
    if (value >= kFaderPositions) value = kFaderPositions - 1;
    return kFaderDb[value];
}

} // namespace trakr

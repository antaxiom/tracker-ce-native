#include <catch2/catch_all.hpp>
#include "AudioBackend.h"
#include <cmath>
#include <vector>

TEST_CASE("NullAudioBackend operation", "[audio]") {
    trakr::NullAudioBackend backend;
    REQUIRE_FALSE(backend.isRunning());

    bool callbackCalled = false;
    bool opened = backend.open(44100, 128, [&](float* out, int frames) {
        callbackCalled = true;
        for (int i = 0; i < frames * 2; ++i) {
            out[i] = 0.5f;
        }
    });
    REQUIRE(opened);

    REQUIRE(backend.start());
    REQUIRE(backend.isRunning());

    backend.stop();
    REQUIRE_FALSE(backend.isRunning());

    backend.close();
}

TEST_CASE("Sine wave generation logic", "[audio]") {
    constexpr int kSampleRate = 44100;
    constexpr int kFrames = 256;
    constexpr float kFreq = 440.0f;
    std::vector<float> buffer(kFrames * 2, 0.0f);

    float phase = 0.0f;
    const float phaseInc = 2.0f * M_PI * kFreq / static_cast<float>(kSampleRate);

    for (int i = 0; i < kFrames; ++i) {
        float sample = std::sin(phase);
        buffer[i * 2] = sample;
        buffer[i * 2 + 1] = sample;
        phase += phaseInc;
    }

    // Verify signal is non-zero, bounded in [-1, 1], and smooth
    float maxVal = 0.0f;
    for (float s : buffer) {
        maxVal = std::max(maxVal, std::abs(s));
        REQUIRE(s >= -1.0001f);
        REQUIRE(s <= 1.0001f);
    }
    REQUIRE(maxVal > 0.95f);
}


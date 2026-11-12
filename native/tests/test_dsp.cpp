#include <catch2/catch_test_macros.hpp>
#include "dsp/Adsr.h"
#include "dsp/Svf.h"
#include "dsp/Lfo.h"
#include "dsp/Delay.h"
#include "dsp/Reverb.h"

TEST_CASE("Adsr envelope stages", "[dsp][adsr]") {
    trakr::dsp::Adsr env;
    env.init(1000.0f); // 1 kHz sample rate for fast testing
    env.setParameters(0.01f, 0.02f, 0.5f, 0.02f);

    REQUIRE(env.currentLevel() == 0.0f);
    REQUIRE(!env.isActive());

    env.noteOn();
    REQUIRE(env.isActive());

    // Process attack (10 samples)
    for (int i = 0; i < 10; ++i) env.process();
    REQUIRE(env.currentLevel() == 1.0f);

    // Process decay (20 samples) down to sustain 0.5
    for (int i = 0; i < 20; ++i) env.process();
    REQUIRE(std::abs(env.currentLevel() - 0.5f) < 0.001f);

    // Process sustain
    for (int i = 0; i < 10; ++i) env.process();
    REQUIRE(std::abs(env.currentLevel() - 0.5f) < 0.001f);

    // Note off -> release (20 samples) down to 0
    env.noteOff();
    for (int i = 0; i < 25; ++i) env.process();
    REQUIRE(env.currentLevel() == 0.0f);
    REQUIRE(!env.isActive());
}

TEST_CASE("Svf filter stability and frequency response", "[dsp][svf]") {
    trakr::dsp::Svf filter;
    filter.init(44100.0f);
    filter.setParameters(trakr::dsp::FilterType::Lowpass, 1000.0f, 1.0f);

    // Feed DC step and check output is stable and finite
    float out = 0.0f;
    for (int i = 0; i < 500; ++i) {
        out = filter.process(1.0f);
        REQUIRE(!std::isnan(out));
        REQUIRE(!std::isinf(out));
    }
    // Lowpass of 1.0 DC approaches 1.0
    REQUIRE(std::abs(out - 1.0f) < 0.05f);
}

TEST_CASE("Lfo output range", "[dsp][lfo]") {
    trakr::dsp::Lfo lfo;
    lfo.init(44100.0f);
    lfo.setParameters(trakr::dsp::LfoWaveform::Sine, 5.0f, 0.8f, trakr::dsp::LfoTarget::Cutoff);

    for (int i = 0; i < 1000; ++i) {
        float val = lfo.process();
        REQUIRE(val >= -0.81f);
        REQUIRE(val <= 0.81f);
    }
}

TEST_CASE("Delay feedback and stability", "[dsp][delay]") {
    trakr::dsp::Delay delay;
    delay.init(44100.0f, 1.0f);
    delay.setParameters(0.01f, 0.9f, 0.5f, 5000.0f);

    float outL = 0.0f, outR = 0.0f;
    // Impulse
    delay.process(1.0f, 1.0f, outL, outR);

    // Run 10,000 samples of silence
    for (int i = 0; i < 10000; ++i) {
        delay.process(0.0f, 0.0f, outL, outR);
        REQUIRE(!std::isnan(outL));
        REQUIRE(!std::isinf(outL));
        REQUIRE(!std::isnan(outR));
        REQUIRE(!std::isinf(outR));
        REQUIRE(std::abs(outL) < 2.0f);
        REQUIRE(std::abs(outR) < 2.0f);
    }
}

TEST_CASE("Reverb max feedback stability test", "[dsp][reverb]") {
    trakr::dsp::Reverb reverb;
    reverb.init(44100.0f);
    reverb.setParameters(0.98f, 0.0f, 1.0f); // Maximum room size, no damping

    float outL = 0.0f, outR = 0.0f;
    // Impulse
    reverb.process(1.0f, 1.0f, outL, outR);

    // 44,100 samples of decay
    for (int i = 0; i < 44100; ++i) {
        reverb.process(0.0f, 0.0f, outL, outR);
        REQUIRE(!std::isnan(outL));
        REQUIRE(!std::isinf(outL));
        REQUIRE(!std::isnan(outR));
        REQUIRE(!std::isinf(outR));
        REQUIRE(std::abs(outL) < 2.0f);
        REQUIRE(std::abs(outR) < 2.0f);
    }
}

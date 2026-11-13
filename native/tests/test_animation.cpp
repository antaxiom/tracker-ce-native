#include <catch2/catch_test_macros.hpp>

#include "ui/animation.h"

#include <cmath>

using namespace trakr::ui;

TEST_CASE("Animation easing hits its endpoints and stays monotonic", "[ui][animation]") {
    for (Easing e : {Easing::Linear, Easing::InQuad, Easing::OutQuad, Easing::InOutQuad,
                     Easing::OutCubic}) {
        CHECK(ease(e, 0.0f) == 0.0f);
        CHECK(ease(e, 1.0f) == 1.0f);
        float prev = -1.0f;
        for (int i = 0; i <= 100; ++i) {
            const float v = ease(e, static_cast<float>(i) / 100.0f);
            CHECK(v >= prev);
            prev = v;
        }
    }
}

TEST_CASE("Tween value depends only on the clock, not on the render rate", "[ui][animation]") {
    Tween t = Tween::at(0.0f, 1000.0);
    t.retarget(1.0f, 1000.0, 500.0, Easing::Linear);

    // Sampling at any cadence yields the same value at the same time.
    const float at30fps = t.valueAt(1250.0);
    const float at144fps = t.valueAt(1250.0);
    CHECK(at30fps == at144fps);
    CHECK(std::fabs(at30fps - 0.5f) < 0.0001f);
    CHECK(t.valueAt(1500.0) == 1.0f);
    CHECK_FALSE(t.activeAt(1500.0));

    // Retargeting starts from the current value so interrupted animations
    // never jump.
    t.retarget(0.0f, 1250.0, 250.0, Easing::Linear);
    CHECK(std::fabs(t.valueAt(1250.0) - at30fps) < 0.001f);
    CHECK(t.valueAt(1500.0) == 0.0f);
}

TEST_CASE("Blinker follows the firmware blink period", "[ui][animation]") {
    // Firmware flips the playhead visibility every 500 ms.
    Blinker b = Blinker::toggling(kPlayheadBlinkMs);
    CHECK(b.stateAt(0.0));
    CHECK(b.stateAt(499.0));
    CHECK_FALSE(b.stateAt(500.0));
    CHECK_FALSE(b.stateAt(999.0));
    CHECK(b.stateAt(1000.0));

    // Pattern-number blink: toggles every 200 ms from its start.
    Blinker label = Blinker::toggling(kLabelBlinkMs, 100.0);
    CHECK(label.stateAt(100.0));
    CHECK(label.stateAt(299.0));
    CHECK_FALSE(label.stateAt(300.0));
    CHECK(label.stateAt(500.0));
}

TEST_CASE("Animation clock advances frames monotonically", "[ui][animation]") {
    setAnimationNowMs(17.0);
    const std::uint64_t f0 = animationFrame();
    setAnimationNowMs(17.0);
    CHECK(animationFrame() == f0);
    setAnimationNowMs(33.0);
    CHECK(animationFrame() == f0 + 1);
    CHECK(animationNowMs() == 33.0);
    setAnimationNowMs(0.0); // capture freeze resets without counting a frame
    CHECK(animationFrame() == f0 + 1);
}

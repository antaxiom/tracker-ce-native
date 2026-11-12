#include <catch2/catch_test_macros.hpp>
#include "Slicer.h"
#include <cmath>

using namespace trakr;

TEST_CASE("Slicer grid slicing", "[engine][slicer]") {
    size_t total = 1000;
    auto s4 = Slicer::grid(total, 4);
    REQUIRE(s4.size() == 4);
    REQUIRE(s4[0] == 0);
    REQUIRE(s4[1] == 250);
    REQUIRE(s4[2] == 500);
    REQUIRE(s4[3] == 750);

    auto s16 = Slicer::grid(total, 16);
    REQUIRE(s16.size() == 16);
    REQUIRE(s16[0] == 0);

    // Edge cases
    REQUIRE(Slicer::grid(0, 4).size() == 1);
    REQUIRE(Slicer::grid(100, 0).size() == 1);
}

TEST_CASE("Slicer transient detection and bounds", "[engine][slicer]") {
    // Create a synthetic buffer with sharp bursts at 0, 4000, 8000 frames
    const size_t total = 12000;
    std::vector<float> data(total, 0.0f);

    auto addBurst = [&](size_t start) {
        for (size_t i = 0; i < 300 && (start + i) < total; ++i) {
            float env = std::exp(-static_cast<float>(i) * 0.02f);
            data[start + i] = std::sin(static_cast<float>(i) * 0.3f) * env;
        }
    };

    addBurst(100);
    addBurst(4500);
    addBurst(9000);

    auto detected = Slicer::detectTransients(data.data(), total, 44100, 0.3f);
    REQUIRE(detected.size() >= 3);
    REQUIRE(detected[0] == 0);

    // Check bounds
    Instrument inst;
    auto s = std::make_shared<SampleData>();
    s->channels = 1;
    s->ch[0] = data;
    inst.sample = s;
    inst.slices = {0, 4500, 9000};

    auto b0 = Slicer::getSliceBounds(inst, 0);
    REQUIRE(b0.first == 0);
    REQUIRE(b0.second == 4500);

    auto b1 = Slicer::getSliceBounds(inst, 1);
    REQUIRE(b1.first == 4500);
    REQUIRE(b1.second == 9000);

    auto b2 = Slicer::getSliceBounds(inst, 2);
    REQUIRE(b2.first == 9000);
    REQUIRE(b2.second == static_cast<int>(total));
}


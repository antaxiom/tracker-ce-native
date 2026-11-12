#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "SampleOps.h"

using namespace trakr;

TEST_CASE("SampleOps Destructive Editing Parity", "[sample_ops]") {
    SampleData sample;
    sample.channels = 1;
    sample.sampleRate = 44100;
    sample.ch[0] = {0.1f, 0.2f, 0.5f, 0.8f, 0.4f, 0.2f, 0.1f, 0.0f};

    SECTION("Crop operation") {
        auto cropped = SampleOps::crop(sample, 2, 6);
        REQUIRE(cropped != nullptr);
        REQUIRE(cropped->ch[0].size() == 4);
        CHECK(cropped->ch[0][0] == Catch::Approx(0.5f));
        CHECK(cropped->ch[0][1] == Catch::Approx(0.8f));
        CHECK(cropped->ch[0][2] == Catch::Approx(0.4f));
        CHECK(cropped->ch[0][3] == Catch::Approx(0.2f));
    }

    SECTION("Reverse operation") {
        auto rev = SampleOps::reverse(sample, 1, 4);
        REQUIRE(rev != nullptr);
        REQUIRE(rev->ch[0].size() == 8);
        CHECK(rev->ch[0][0] == Catch::Approx(0.1f));
        CHECK(rev->ch[0][1] == Catch::Approx(0.8f));
        CHECK(rev->ch[0][2] == Catch::Approx(0.5f));
        CHECK(rev->ch[0][3] == Catch::Approx(0.2f));
        CHECK(rev->ch[0][4] == Catch::Approx(0.4f));
    }

    SECTION("Normalize operation") {
        auto norm = SampleOps::normalize(sample, 0, 8);
        REQUIRE(norm != nullptr);
        // Max peak was 0.8f, so gain factor is 1.0 / 0.8 = 1.25
        CHECK(norm->ch[0][3] == Catch::Approx(1.0f));
        CHECK(norm->ch[0][2] == Catch::Approx(0.5f * 1.25f));
    }

    SECTION("Amplify operation") {
        auto amp = SampleOps::amplify(sample, 0, 4, 2.0f);
        REQUIRE(amp != nullptr);
        CHECK(amp->ch[0][0] == Catch::Approx(0.2f));
        CHECK(amp->ch[0][1] == Catch::Approx(0.4f));
        CHECK(amp->ch[0][2] == Catch::Approx(1.0f)); // clamped
        CHECK(amp->ch[0][4] == Catch::Approx(0.4f)); // untouched
    }

    SECTION("FadeIn and FadeOut") {
        auto fIn = SampleOps::fadeIn(sample, 0, 4);
        REQUIRE(fIn != nullptr);
        CHECK(fIn->ch[0][0] == Catch::Approx(0.0f));
        CHECK(fIn->ch[0][1] == Catch::Approx(0.2f * 0.25f));

        auto fOut = SampleOps::fadeOut(sample, 4, 8);
        REQUIRE(fOut != nullptr);
        CHECK(fOut->ch[0][4] == Catch::Approx(0.4f * 1.0f));
        CHECK(fOut->ch[0][7] == Catch::Approx(0.0f));
    }

    SECTION("Silence operation") {
        auto sil = SampleOps::silence(sample, 2, 5);
        REQUIRE(sil != nullptr);
        CHECK(sil->ch[0][1] == Catch::Approx(0.2f));
        CHECK(sil->ch[0][2] == Catch::Approx(0.0f));
        CHECK(sil->ch[0][3] == Catch::Approx(0.0f));
        CHECK(sil->ch[0][4] == Catch::Approx(0.0f));
        CHECK(sil->ch[0][5] == Catch::Approx(0.2f));
    }
}

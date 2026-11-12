#include <catch2/catch_test_macros.hpp>
#include "hw/TextFit.h"
#include <cstring>

using namespace trakr::ui;

TEST_CASE("TextFit keeps normal labels at firmware advance", "[textfit]") {
    // 9 chars at 10px = 90px, fits a 96px screen button with no scaling.
    FittedText fit = fitText("Resonance", 96.0f, 10.0f, 22.0f);
    CHECK_FALSE(fit.scaled);
    CHECK_FALSE(fit.clipped);
    CHECK(fit.advance == 10.0f);
    CHECK(fit.chars == 9);
    CHECK(fit.width == 90.0f);
}

TEST_CASE("TextFit shrinks long option labels instead of overfilling", "[textfit]") {
    // The exact offenders from the second-page audits.
    for (const char* label : {"Destination", "Reverb send", "Filter Type", "Delay send"}) {
        FittedText fit = fitText(label, 96.0f, 10.0f, 22.0f);
        CHECK(fit.scaled);
        CHECK_FALSE(fit.clipped);
        CHECK(fit.width <= 96.0f);
        CHECK(fit.advance >= 7.0f);
        CHECK(fit.fontSize < 22.0f);
        CHECK(fit.chars == static_cast<int>(std::strlen(label)));
    }
}

TEST_CASE("TextFit clips text that cannot be shrunk to the minimum", "[textfit]") {
    FittedText fit = fitText("This label is far too long for a button", 96.0f, 10.0f, 22.0f);
    CHECK(fit.clipped);
    CHECK(fit.advance == 7.0f);
    CHECK(fit.width <= 96.0f);
    CHECK(fit.chars * fit.advance <= 96.0f);
}

TEST_CASE("TextFit pattern track headers fit the 78px cell", "[textfit]") {
    for (const char* label : {"Track 1", "Track 4", "Track 8"}) {
        FittedText fit = fitText(label, 78.0f, 8.0f, 18.0f);
        CHECK(fit.width <= 78.0f);
    }
}

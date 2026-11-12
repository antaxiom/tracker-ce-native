#pragma once

#include "Song.h"
#include "Scale.h"
#include <vector>

namespace trakr {

enum class FillMode {
    Euclidean = 0,
    RandomInScale,
    LinearRamp,
    Arpeggio,
    Count
};

struct FillConfig {
    FillMode mode{FillMode::Euclidean};
    int startRow{0};
    int endRow{63};
    int instrument{0};
    int rootNote{0}; // 0 = C
    ScaleType scale{ScaleType::Minor};
    int baseNote{48}; // C-3
    int octaveRange{2};
    int hits{4};       // For Euclidean
    int rotation{0};   // For Euclidean
    int volumeStart{48};
    int volumeEnd{48};
    bool fillNotes{true};
    bool fillVolumes{true};
};

std::vector<bool> generateEuclideanRhythm(int hits, int steps, int rotation = 0);
void applyFill(Pattern& pat, int track, const FillConfig& config);

} // namespace trakr


#pragma once

#include "Song.h"
#include <vector>
#include <utility>
#include <cstddef>

namespace trakr {

class Slicer {
public:
    // Slices sample into N equal slices (e.g. 4, 8, 16, 32, 48)
    static std::vector<int> grid(size_t totalFrames, int numSlices);

    // Detects transient attack points using short-time energy derivative
    static std::vector<int> detectTransients(const float* data, size_t totalFrames, int sampleRate = 44100, float threshold = 0.35f);

    // Returns {startFrame, endFrame} for a given 0-based slice index
    static std::pair<int, int> getSliceBounds(const Instrument& inst, int sliceIndex);
};

} // namespace trakr


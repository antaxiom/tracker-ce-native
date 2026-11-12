#pragma once

// Polyend Tracker Sample Editor Destructive DSP Operations
// Reference: Manual 1.7.0 Section 8.9 (lines 11798-11855)

#include "Song.h"
#include <memory>

namespace trakr {

class SampleOps {
public:
    // Crops the sample to the frame range [startFrame, endFrame)
    static std::shared_ptr<SampleData> crop(const SampleData& src, int startFrame, int endFrame);

    // Reverses audio samples within the frame range [startFrame, endFrame)
    static std::shared_ptr<SampleData> reverse(const SampleData& src, int startFrame, int endFrame);

    // Normalizes peak amplitude within [startFrame, endFrame) to 1.0 (0 dBFS)
    static std::shared_ptr<SampleData> normalize(const SampleData& src, int startFrame, int endFrame);

    // Applies linear gain scaling to the frame range [startFrame, endFrame)
    static std::shared_ptr<SampleData> amplify(const SampleData& src, int startFrame, int endFrame, float gainLinear);

    // Applies linear fade-in from 0.0 to 1.0 across [startFrame, endFrame)
    static std::shared_ptr<SampleData> fadeIn(const SampleData& src, int startFrame, int endFrame);

    // Applies linear fade-out from 1.0 to 0.0 across [startFrame, endFrame)
    static std::shared_ptr<SampleData> fadeOut(const SampleData& src, int startFrame, int endFrame);

    // Zeroes audio within [startFrame, endFrame)
    static std::shared_ptr<SampleData> silence(const SampleData& src, int startFrame, int endFrame);
};

} // namespace trakr

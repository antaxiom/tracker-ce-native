#include "Slicer.h"
#include <cmath>
#include <algorithm>

namespace trakr {

std::vector<int> Slicer::grid(size_t totalFrames, int numSlices) {
    if (totalFrames == 0 || numSlices <= 0) return {0};
    numSlices = std::clamp(numSlices, 1, 64);
    std::vector<int> slices;
    slices.reserve(numSlices);
    for (int i = 0; i < numSlices; ++i) {
        int frame = static_cast<int>((static_cast<uint64_t>(i) * totalFrames) / numSlices);
        slices.push_back(frame);
    }
    return slices;
}

std::vector<int> Slicer::detectTransients(const float* data, size_t totalFrames, int sampleRate, float threshold) {
    std::vector<int> slices = {0};
    if (!data || totalFrames < 512 || sampleRate <= 0) return slices;

    constexpr size_t kWin = 256;
    constexpr size_t kHop = 128;
    const size_t numHops = (totalFrames >= kWin) ? (totalFrames - kWin) / kHop : 0;
    if (numHops < 2) return slices;

    std::vector<float> energy(numHops, 0.0f);
    for (size_t h = 0; h < numHops; ++h) {
        float sum = 0.0f;
        const size_t offset = h * kHop;
        for (size_t i = 0; i < kWin; ++i) {
            float s = data[offset + i];
            sum += s * s;
        }
        energy[h] = std::sqrt(sum / kWin);
    }

    // Energy flux
    std::vector<float> flux(numHops, 0.0f);
    float maxFlux = 0.0001f;
    for (size_t h = 1; h < numHops; ++h) {
        float diff = std::max(0.0f, energy[h] - energy[h - 1]);
        flux[h] = diff;
        if (diff > maxFlux) maxFlux = diff;
    }

    const float fluxThresh = maxFlux * std::clamp(threshold, 0.05f, 0.95f);
    const int minDistanceFrames = sampleRate / 20; // At least 50ms between slice points
    int lastSliceFrame = 0;

    for (size_t h = 1; h < numHops - 1; ++h) {
        if (flux[h] > fluxThresh && flux[h] > flux[h - 1] && flux[h] >= flux[h + 1]) {
            int frame = static_cast<int>(h * kHop);
            if (frame - lastSliceFrame >= minDistanceFrames) {
                slices.push_back(frame);
                lastSliceFrame = frame;
            }
        }
    }
    return slices;
}

std::pair<int, int> Slicer::getSliceBounds(const Instrument& inst, int sliceIndex) {
    if (!inst.sample || inst.sample->ch[0].empty()) {
        return {inst.startFrame, inst.endFrame};
    }
    const int total = static_cast<int>(inst.sample->ch[0].size());
    if (inst.slices.empty()) {
        int end = (inst.endFrame > 0) ? inst.endFrame : total;
        return {inst.startFrame, end};
    }

    int idx = std::clamp(sliceIndex, 0, static_cast<int>(inst.slices.size()) - 1);
    int start = inst.slices[idx];
    int end = (idx + 1 < static_cast<int>(inst.slices.size())) ? inst.slices[idx + 1] : total;
    return {start, end};
}

} // namespace trakr


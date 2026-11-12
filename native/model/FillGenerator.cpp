#include "FillGenerator.h"
#include <cmath>
#include <random>
#include <algorithm>

namespace trakr {

std::vector<bool> generateEuclideanRhythm(int hits, int steps, int rotation) {
    if (steps <= 0) return {};
    std::vector<bool> pattern(steps, false);
    if (hits <= 0) return pattern;
    if (hits >= steps) {
        std::fill(pattern.begin(), pattern.end(), true);
        return pattern;
    }

    // Bjorklund / Bresenham Euclidean distribution
    for (int i = 0; i < steps; ++i) {
        int cur = (i + 1) * hits / steps;
        int prev = i * hits / steps;
        if (cur > prev) {
            int rotatedIdx = ((i + rotation) % steps + steps) % steps;
            pattern[rotatedIdx] = true;
        }
    }
    return pattern;
}

void applyFill(Pattern& pat, int track, const FillConfig& config) {
    if (track < 0 || track >= static_cast<int>(pat.tracks.size())) return;
    auto& trk = pat.tracks[track];
    int numRows = static_cast<int>(trk.size());

    int start = std::clamp(config.startRow, 0, numRows - 1);
    int end = std::clamp(config.endRow, 0, numRows - 1);
    if (start > end) std::swap(start, end);
    int stepCount = end - start + 1;

    // Collect scale notes within octave range
    const auto& scaleInfo = getScaleInfo(config.scale);
    std::vector<int> scaleNotes;
    for (int oct = 0; oct < config.octaveRange; ++oct) {
        for (int interval : scaleInfo.intervals) {
            int note = config.baseNote + oct * 12 + interval;
            if (note >= 0 && note <= 119) {
                scaleNotes.push_back(note);
            }
        }
    }
    if (scaleNotes.empty()) scaleNotes.push_back(config.baseNote);

    std::mt19937 rng(42 + config.startRow * 31);

    if (config.mode == FillMode::Euclidean) {
        auto rhythm = generateEuclideanRhythm(config.hits, stepCount, config.rotation);
        for (int i = 0; i < stepCount; ++i) {
            int r = start + i;
            if (r >= numRows) break;
            if (rhythm[i]) {
                if (config.fillNotes) {
                    trk[r].note = static_cast<int16_t>(config.baseNote);
                    trk[r].instrument = static_cast<int16_t>(config.instrument);
                }
                if (config.fillVolumes) {
                    trk[r].volume = static_cast<int16_t>(config.volumeStart);
                }
            }
        }
    } else if (config.mode == FillMode::RandomInScale) {
        std::uniform_int_distribution<size_t> dist(0, scaleNotes.size() - 1);
        for (int i = 0; i < stepCount; ++i) {
            int r = start + i;
            if (r >= numRows) break;
            if (config.fillNotes) {
                trk[r].note = static_cast<int16_t>(scaleNotes[dist(rng)]);
                trk[r].instrument = static_cast<int16_t>(config.instrument);
            }
            if (config.fillVolumes) {
                float t = (stepCount > 1) ? static_cast<float>(i) / (stepCount - 1) : 0.0f;
                int vol = static_cast<int>(config.volumeStart + t * (config.volumeEnd - config.volumeStart));
                trk[r].volume = static_cast<int16_t>(std::clamp(vol, 0, 64));
            }
        }
    } else if (config.mode == FillMode::LinearRamp) {
        for (int i = 0; i < stepCount; ++i) {
            int r = start + i;
            if (r >= numRows) break;
            float t = (stepCount > 1) ? static_cast<float>(i) / (stepCount - 1) : 0.0f;
            if (config.fillVolumes) {
                int vol = static_cast<int>(config.volumeStart + t * (config.volumeEnd - config.volumeStart));
                trk[r].volume = static_cast<int16_t>(std::clamp(vol, 0, 64));
            }
        }
    } else if (config.mode == FillMode::Arpeggio) {
        for (int i = 0; i < stepCount; ++i) {
            int r = start + i;
            if (r >= numRows) break;
            if (config.fillNotes) {
                trk[r].note = static_cast<int16_t>(scaleNotes[i % scaleNotes.size()]);
                trk[r].instrument = static_cast<int16_t>(config.instrument);
            }
            if (config.fillVolumes) {
                trk[r].volume = static_cast<int16_t>(config.volumeStart);
            }
        }
    }
}

} // namespace trakr


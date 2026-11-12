#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace trakr {

enum class ScaleType {
    Chromatic = 0,
    Major,
    Minor,
    Dorian,
    Phrygian,
    Lydian,
    Mixolydian,
    Locrian,
    MajorPentatonic,
    MinorPentatonic,
    Blues,
    HarmonicMinor,
    MelodicMinor,
    Count
};

struct ScaleInfo {
    ScaleType type;
    const char* name;
    std::vector<int> intervals; // Semitones from root (0..11)
};

const std::vector<ScaleInfo>& getAvailableScales();
const ScaleInfo& getScaleInfo(ScaleType type);
const char* getNoteName(int noteIndex); // 0 = C, 1 = C#, ... 11 = B

bool isNoteInScale(int midiNote, int rootNote, ScaleType scale);
int snapToScale(int midiNote, int rootNote, ScaleType scale, bool snapUp = true);

} // namespace trakr


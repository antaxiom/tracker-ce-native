#include "Scale.h"
#include <algorithm>

namespace trakr {

namespace {

static const char* const kNoteNames[] = {
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
};

static const std::vector<ScaleInfo> kScales = {
    { ScaleType::Chromatic, "Chromatic", {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11} },
    { ScaleType::Major, "Major", {0, 2, 4, 5, 7, 9, 11} },
    { ScaleType::Minor, "Natural Minor", {0, 2, 3, 5, 7, 8, 10} },
    { ScaleType::Dorian, "Dorian", {0, 2, 3, 5, 7, 9, 10} },
    { ScaleType::Phrygian, "Phrygian", {0, 1, 3, 5, 7, 8, 10} },
    { ScaleType::Lydian, "Lydian", {0, 2, 4, 6, 7, 9, 11} },
    { ScaleType::Mixolydian, "Mixolydian", {0, 2, 4, 5, 7, 9, 10} },
    { ScaleType::Locrian, "Locrian", {0, 1, 3, 5, 6, 8, 10} },
    { ScaleType::MajorPentatonic, "Major Pentatonic", {0, 2, 4, 7, 9} },
    { ScaleType::MinorPentatonic, "Minor Pentatonic", {0, 3, 5, 7, 10} },
    { ScaleType::Blues, "Blues", {0, 3, 5, 6, 7, 10} },
    { ScaleType::HarmonicMinor, "Harmonic Minor", {0, 2, 3, 5, 7, 8, 11} },
    { ScaleType::MelodicMinor, "Melodic Minor", {0, 2, 3, 5, 7, 9, 11} }
};

} // namespace

const std::vector<ScaleInfo>& getAvailableScales() {
    return kScales;
}

const ScaleInfo& getScaleInfo(ScaleType type) {
    size_t idx = static_cast<size_t>(type);
    if (idx < kScales.size()) return kScales[idx];
    return kScales[0];
}

const char* getNoteName(int noteIndex) {
    int idx = ((noteIndex % 12) + 12) % 12;
    return kNoteNames[idx];
}

bool isNoteInScale(int midiNote, int rootNote, ScaleType scale) {
    if (scale == ScaleType::Chromatic || midiNote < 0) return true;
    const auto& info = getScaleInfo(scale);
    int semitoneFromRoot = ((midiNote - rootNote) % 12 + 12) % 12;
    return std::find(info.intervals.begin(), info.intervals.end(), semitoneFromRoot) != info.intervals.end();
}

int snapToScale(int midiNote, int rootNote, ScaleType scale, bool snapUp) {
    if (scale == ScaleType::Chromatic || midiNote < 0) return midiNote;
    if (isNoteInScale(midiNote, rootNote, scale)) return midiNote;

    const auto& info = getScaleInfo(scale);
    int semitone = ((midiNote - rootNote) % 12 + 12) % 12;
    int baseOctaveMidi = midiNote - semitone;

    if (snapUp) {
        for (int offset = 1; offset <= 11; ++offset) {
            int candidate = (semitone + offset) % 12;
            if (std::find(info.intervals.begin(), info.intervals.end(), candidate) != info.intervals.end()) {
                int result = baseOctaveMidi + candidate;
                if (candidate < semitone) result += 12;
                return std::clamp(result, 0, 119);
            }
        }
    } else {
        for (int offset = 1; offset <= 11; ++offset) {
            int candidate = ((semitone - offset) % 12 + 12) % 12;
            if (std::find(info.intervals.begin(), info.intervals.end(), candidate) != info.intervals.end()) {
                int result = baseOctaveMidi + candidate;
                if (candidate > semitone) result -= 12;
                return std::clamp(result, 0, 119);
            }
        }
    }
    return midiNote;
}

} // namespace trakr


#pragma once

#include <string>
#include <vector>
#include <memory>
#include <array>
#include <cstdint>

namespace trakr {

constexpr int kNoNote = -1;
constexpr int kNoteOff = 120;

struct FxCmd {
    uint8_t type{0};
    uint8_t value{0};
};

struct Cell {
    int16_t note{kNoNote};
    int16_t instrument{-1};
    int16_t volume{-1};
    std::array<FxCmd, 4> fx{};
};

struct Pattern {
    int rows{64};
    std::vector<std::vector<Cell>> tracks; // [track][row]
};

struct TrackInfo {
    std::string name{"Track"};
    float gainDb{0.0f};
    float pan{0.0f};
    bool mute{false};
    bool solo{false};
    int fxColumns{2};
    float delaySend{0.0f};
    float reverbSend{0.0f};
};

struct SampleData {
    std::vector<float> ch[2];
    int channels{1};
    int sampleRate{44100};
};

struct Instrument {
    std::string name{"Default"};
    std::shared_ptr<const SampleData> sample{nullptr};
    int rootNote{60};
    int tune{0};      // -24 to +24 semitones (Manual 6.3)
    int finetune{0};  // -100 to +100 cents (Manual 6.3)
    int panning{0};   // -50 Left to +50 Right, 0 = Center (Manual 6.3)
    int volume{50};   // 0 to 100, 50 = 0.0 dB, 0 = -inf, 100 = +24 dB (Manual 6.3)
    float gainDb{0.0f};
    enum class Loop { None, Forward, Backward, PingPong, Slice, BeatSlice, Wavetable, Granular } loop{Loop::None};
    int startFrame{0};
    int loopStart{0};
    int loopEnd{0};
    int endFrame{-1};
    std::vector<int> slices;
    int selectedSlice{0};
    int wavetableWindow{256};
    float wavetablePos{0.0f};
    float granularLengthMs{50.0f};
    int chipModel{0};
    bool isSynth{false};

    // Filter parameters (Manual 6.3)
    enum class FilterType { None, Lowpass, Highpass, Bandpass } filterType{FilterType::None};
    int cutoff{100};   // 0..100%
    int resonance{0};  // 0..100%
    float filterCutoffHz{20000.0f};
    float filterResonance{0.707f};

    // Instrument Effects (Manual 6.4)
    int overdrive{0};   // 0..100%
    int bitDepth{16};   // 4..16 bits, default 16
    int delaySend{0};   // 0..100% send (-inf to 0 dB)
    int reverbSend{0};  // 0..100% send (-inf to 0 dB)

    // Legacy / simple synth envelope & LFO
    float attackMs{1.0f};
    float decayMs{100.0f};
    float sustain{1.0f};
    float releaseMs{5.0f};
    enum class LfoWave { Sine, Triangle, Saw, Square } lfoWave{LfoWave::Sine};
    enum class LfoTarget { None, Pitch, Volume, Panning, Cutoff } lfoTarget{LfoTarget::None};
    float lfoRateHz{1.0f};
    float lfoDepth{0.0f};

    // Automation Parameters (Manual 6.5 / Firmware gInstrEdit.cpp)
    // 6 destinations: 0: Volume, 1: Panning, 2: Cutoff, 3: Wavetable Pos, 4: Granular Pos, 5: Finetune
    enum class AutoType { Off, Envelope, Lfo };
    enum class AutoShape { RevSaw, Saw, Triangle, Square, Random };
    struct AutoSlot {
        AutoType type{AutoType::Off};
        // Envelope settings
        float attackSec{0.01f};
        float decaySec{0.20f};
        float sustain{0.80f};
        float releaseSec{0.10f};
        // LFO settings
        AutoShape shape{AutoShape::Triangle};
        int speedStepIdx{6}; // default 16 steps (index into Tracker speed table)
        float amount{1.0f};  // 0.0f .. 1.0f (0..100%)
    };
    std::array<AutoSlot, 6> automation{};
};

struct Song {
    float bpm{120.0f};
    int lpb{4};
    std::vector<TrackInfo> tracks;
    std::vector<Pattern> patterns;
    std::vector<int> order;
    std::vector<Instrument> instruments;
    float masterGainDb{0.0f};
    float masterPan{0.0f};
    float delayTimeMs{250.0f};
    float delayFeedback{0.4f};
    float reverbSize{0.5f};
    float reverbDamping{0.3f};
    float masterBassBoost{0.0f};
    float masterSaturationDrive{0.0f};
    int masterSaturationType{0};
    float masterSpace{0.0f};
    float masterEq[5]{0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    // Global Mixer (Master 3/3) aux fader positions and mutes, in firmware
    // order: Delay, Reverb, Dry Mix, Line In.
    // Firmware defaults (mtStructs.h DEFAULT_*_VOLUME = 86 = 0 dB); Line In
    // starts muted.
    int auxVolume[4]{86, 86, 86, 86};
    bool auxMute[4]{false, false, false, true};
    // Project "Extra headroom": firmware lowers sample playback by 3 dB
    // (play_memory.cpp lowerVolume3dB) without touching the master fader.
    bool headroom{false};
};

struct SongSnapshot {
    float bpm{120.0f};
    int lpb{4};
    std::vector<TrackInfo> tracks;
    std::vector<Pattern> patterns;
    std::vector<int> order;
    std::vector<Instrument> instruments;
    float masterGainDb{0.0f};
    float masterPan{0.0f};
    float delayTimeMs{250.0f};
    float delayFeedback{0.4f};
    float reverbSize{0.5f};
    float reverbDamping{0.3f};
    float masterBassBoost{0.0f};
    float masterSaturationDrive{0.0f};
    int masterSaturationType{0};
    float masterSpace{0.0f};
    float masterEq[5]{0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    bool headroom{false};
    int auxVolume[4]{86, 86, 86, 86};
    bool auxMute[4]{false, false, false, true};
};

Song makeDefaultSong();
std::shared_ptr<const SongSnapshot> makeSnapshot(const Song& song);

} // namespace trakr


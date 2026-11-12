#pragma once

#include "Song.h"
#include <unordered_map>
#include <memory>
#include <functional>

namespace trakr {

class IVoice;

// Step FX Type IDs matching Polyend Tracker CE firmware enFxType (core/source/mtSequencer.h)
namespace FxType {
    constexpr uint8_t None = 0;
    constexpr uint8_t Off = 1;
    constexpr uint8_t MicroMove = 2;
    constexpr uint8_t Roll = 3;
    constexpr uint8_t Chance = 4;
    constexpr uint8_t RandomNote = 5;
    constexpr uint8_t RandomInstrument = 6;
    constexpr uint8_t RandomVolume = 7;
    constexpr uint8_t SendCcA = 8;
    constexpr uint8_t SendCcB = 9;
    constexpr uint8_t SendCcC = 10;
    constexpr uint8_t SendCcD = 11;
    constexpr uint8_t SendCcE = 12;
    constexpr uint8_t BreakPattern = 13;
    constexpr uint8_t MidiChord = 14;
    constexpr uint8_t Tempo = 15;
    constexpr uint8_t RandomValue = 16;
    constexpr uint8_t Swing = 17;
    constexpr uint8_t Volume = 18;
    constexpr uint8_t Glide = 19;
    constexpr uint8_t GateLength = 20;
    constexpr uint8_t Arp = 21;
    constexpr uint8_t Position = 22;
    constexpr uint8_t VolumeLfo = 23;
    constexpr uint8_t PanningLfo = 24;
    constexpr uint8_t SampleSlice = 25;
    constexpr uint8_t Reverse = 26;
    constexpr uint8_t FilterLowpass = 27;
    constexpr uint8_t FilterHighpass = 28;
    constexpr uint8_t FilterBandpass = 29;
    constexpr uint8_t DelaySend = 30;
    constexpr uint8_t Panning = 31;
    constexpr uint8_t ReverbSend = 32;
    constexpr uint8_t FinetuneLfo = 33;
    constexpr uint8_t Microtuning = 34;
    constexpr uint8_t FilterLfo = 35;
    constexpr uint8_t PositionLfo = 36;
    constexpr uint8_t SendCcF = 37;
    constexpr uint8_t Overdrive = 38;
    constexpr uint8_t BitDepth = 39;
    constexpr uint8_t Tune = 40;
    constexpr uint8_t SlideUp = 41;
    constexpr uint8_t SlideDown = 42;
    constexpr uint8_t AmpEnv = 43;
    constexpr uint8_t FilterEnv = 44;
    constexpr uint8_t GranularLength = 45;

    // Aliases
    constexpr uint8_t NoteCut = 20;
    constexpr uint8_t Legacy_Glide = 4;
    constexpr uint8_t Legacy_Microtuning = 5;
    constexpr uint8_t Legacy_Panning = 6;
    constexpr uint8_t Legacy_Tune = 11;
    constexpr uint8_t Legacy_Position = 18;
    constexpr uint8_t Legacy_Volume = 19;
    constexpr uint8_t Legacy_SampleSlice = 21;
    constexpr uint8_t Legacy_NoteCut = 30;
    constexpr uint8_t Legacy_Retrigger = 31;
}

class IStepFx {
public:
    virtual ~IStepFx() = default;
    virtual void apply(uint8_t value, IVoice* voice, int rowLengthFrames) = 0;
};

class StepFxRegistry {
public:
    static StepFxRegistry& instance();

    void registerFx(uint8_t type, std::shared_ptr<IStepFx> handler);
    void applyFx(uint8_t type, uint8_t value, IVoice* voice, int rowLengthFrames);

private:
    StepFxRegistry();
    std::unordered_map<uint8_t, std::shared_ptr<IStepFx>> handlers_;
};

} // namespace trakr

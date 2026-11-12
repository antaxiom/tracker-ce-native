#include "StepFx.h"
#include "Voice.h"
#include <cmath>

namespace trakr {

namespace {

class VolumeFx : public IStepFx {
public:
    void apply(uint8_t value, IVoice* voice, int /*rowLengthFrames*/) override {
        if (!voice) return;
        float vol = static_cast<float>(value) / 100.0f;
        voice->setVolume(vol);
    }
};

class PanningFx : public IStepFx {
public:
    void apply(uint8_t value, IVoice* voice, int /*rowLengthFrames*/) override {
        if (!voice) return;
        float pan = (static_cast<float>(value) - 50.0f) / 50.0f;
        voice->setPanning(pan);
    }
};

class PositionFx : public IStepFx {
public:
    void apply(uint8_t value, IVoice* voice, int /*rowLengthFrames*/) override {
        if (!voice) return;
        float pos = static_cast<float>(value) / 100.0f;
        voice->setPosition(pos);
    }
};

class MicrotuningFx : public IStepFx {
public:
    void apply(uint8_t value, IVoice* voice, int /*rowLengthFrames*/) override {
        if (!voice) return;
        float cents = static_cast<float>(value) - 50.0f;
        float ratio = std::pow(2.0f, cents / 1200.0f);
        voice->setPitchRatio(ratio);
    }
};

class TuneFx : public IStepFx {
public:
    void apply(uint8_t value, IVoice* voice, int /*rowLengthFrames*/) override {
        if (!voice) return;
        float semitones = static_cast<float>(value) - 50.0f;
        float ratio = std::pow(2.0f, semitones / 12.0f);
        voice->setPitchRatio(ratio);
    }
};

class ReverseFx : public IStepFx {
public:
    void apply(uint8_t /*value*/, IVoice* voice, int /*rowLengthFrames*/) override {
        if (!voice) return;
        voice->setReverse(true);
    }
};

class NoteCutFx : public IStepFx {
public:
    void apply(uint8_t value, IVoice* voice, int rowLengthFrames) override {
        if (!voice) return;
        if (value == 0) {
            voice->stop();
        } else {
            int cutFrames = std::max(1, (rowLengthFrames * value) / 16);
            voice->setCutCountdown(cutFrames);
        }
    }
};

class RollFx : public IStepFx {
public:
    void apply(uint8_t value, IVoice* voice, int rowLengthFrames) override {
        if (!voice || rowLengthFrames <= 0) return;
        int div = std::clamp<int>(value, 1, 16);
        int interval = std::max(1, rowLengthFrames / div);
        voice->setRetrigger(interval, div);
    }
};

class GlideFx : public IStepFx {
public:
    void apply(uint8_t value, IVoice* voice, int /*rowLengthFrames*/) override {
        if (!voice) return;
        float semitones = static_cast<float>(value) - 50.0f;
        float ratio = std::pow(2.0f, semitones / 12.0f);
        voice->setGlide(ratio, 0.05f);
    }
};

class FilterFx : public IStepFx {
public:
    explicit FilterFx(int filterType) : filterType_(filterType) {}
    void apply(uint8_t value, IVoice* voice, int /*rowLengthFrames*/) override {
        if (!voice) return;
        float norm = static_cast<float>(value) / 100.0f;
        float cutoff = 20.0f * std::pow(1000.0f, norm);
        voice->setCutoff(cutoff);
        voice->setFilterType(filterType_);
    }
private:
    int filterType_{0};
};

class SlideFx : public IStepFx {
public:
    explicit SlideFx(bool up) : up_(up) {}
    void apply(uint8_t value, IVoice* voice, int /*rowLengthFrames*/) override {
        if (!voice) return;
        float semi = (static_cast<float>(value) * 0.1f) * (up_ ? 1.0f : -1.0f);
        voice->setGlide(std::pow(2.0f, semi / 12.0f), 0.08f);
    }
private:
    bool up_{true};
};

class RandomNoteFx : public IStepFx {
public:
    void apply(uint8_t value, IVoice* voice, int /*rowLengthFrames*/) override {
        if (!voice || value == 0) return;
        int maxSemi = static_cast<int>(value) % 24;
        int offset = (rand() % (maxSemi * 2 + 1)) - maxSemi;
        voice->setPitchRatio(std::pow(2.0f, offset / 12.0f));
    }
};

class RandomVolumeFx : public IStepFx {
public:
    void apply(uint8_t value, IVoice* voice, int /*rowLengthFrames*/) override {
        if (!voice) return;
        float baseVol = static_cast<float>(value) / 100.0f;
        float rnd = (static_cast<float>(rand() % 40) - 20.0f) / 100.0f;
        voice->setVolume(std::clamp(baseVol + rnd, 0.0f, 1.0f));
    }
};

class OffFx : public IStepFx {
public:
    void apply(uint8_t /*value*/, IVoice* voice, int /*rowLengthFrames*/) override {
        if (voice) voice->resetFx();
    }
};

class OverdriveFx : public IStepFx {
public:
    void apply(uint8_t value, IVoice* voice, int /*rowLengthFrames*/) override {
        if (voice) voice->setOverdrive(static_cast<float>(value));
    }
};

class BitDepthFx : public IStepFx {
public:
    void apply(uint8_t value, IVoice* voice, int /*rowLengthFrames*/) override {
        if (voice) voice->setBitDepth(std::clamp<int>(value, 4, 16));
    }
};

class GateLengthFx : public IStepFx {
public:
    void apply(uint8_t value, IVoice* voice, int rowLengthFrames) override {
        if (!voice || rowLengthFrames <= 0) return;
        int cutFrames = std::max(1, (rowLengthFrames * std::clamp<int>(value, 0, 100)) / 100);
        voice->setCutCountdown(cutFrames);
    }
};

class SampleSliceFx : public IStepFx {
public:
    void apply(uint8_t value, IVoice* voice, int /*rowLengthFrames*/) override {
        if (voice) voice->setSlice(static_cast<int>(value));
    }
};

class ChanceFx : public IStepFx {
public:
    void apply(uint8_t value, IVoice* voice, int /*rowLengthFrames*/) override {
        if (voice && value < 100 && (rand() % 100) >= value) voice->stop();
    }
};

class DelaySendFx : public IStepFx {
public:
    void apply(uint8_t value, IVoice* voice, int /*rowLengthFrames*/) override {
        if (voice) voice->setDelaySend(static_cast<float>(value) / 100.0f);
    }
};

class ReverbSendFx : public IStepFx {
public:
    void apply(uint8_t value, IVoice* voice, int /*rowLengthFrames*/) override {
        if (voice) voice->setReverbSend(static_cast<float>(value) / 100.0f);
    }
};

class MicroMoveFx : public IStepFx {
public:
    void apply(uint8_t value, IVoice* voice, int rowLengthFrames) override {
        if (voice && rowLengthFrames > 0) {
            voice->setNudgeDelay((rowLengthFrames * std::clamp<int>(value, 0, 100)) / 100);
        }
    }
};

class ArpFx : public IStepFx {
public:
    void apply(uint8_t value, IVoice* voice, int rowLengthFrames) override {
        if (!voice || rowLengthFrames <= 0 || value == 0) return;
        int rate = std::clamp<int>(value % 16, 1, 16);
        voice->setRetrigger(std::max(1, rowLengthFrames / rate));
    }
};

class LfoRateFx : public IStepFx {
public:
    explicit LfoRateFx(int target) : target_(target) {}
    void apply(uint8_t value, IVoice* voice, int /*rowLengthFrames*/) override {
        (void)value; (void)voice; (void)target_;
    }
private:
    int target_{0};
};

class GranularLengthFx : public IStepFx {
public:
    void apply(uint8_t value, IVoice* voice, int /*rowLengthFrames*/) override {
        (void)value; (void)voice;
    }
};

class DummyFx : public IStepFx {
public:
    void apply(uint8_t /*value*/, IVoice* /*voice*/, int /*rowLengthFrames*/) override {}
};

} // namespace

StepFxRegistry& StepFxRegistry::instance() {
    static StepFxRegistry reg;
    return reg;
}

StepFxRegistry::StepFxRegistry() {
    registerFx(FxType::Volume, std::make_shared<VolumeFx>());
    registerFx(FxType::Panning, std::make_shared<PanningFx>());
    registerFx(FxType::Position, std::make_shared<PositionFx>());
    registerFx(FxType::Microtuning, std::make_shared<MicrotuningFx>());
    registerFx(FxType::Tune, std::make_shared<TuneFx>());
    registerFx(FxType::Reverse, std::make_shared<ReverseFx>());
    registerFx(FxType::Roll, std::make_shared<RollFx>());
    registerFx(FxType::Glide, std::make_shared<GlideFx>());
    registerFx(FxType::FilterLowpass, std::make_shared<FilterFx>(0));
    registerFx(FxType::FilterHighpass, std::make_shared<FilterFx>(1));
    registerFx(FxType::FilterBandpass, std::make_shared<FilterFx>(2));
    registerFx(FxType::SlideUp, std::make_shared<SlideFx>(true));
    registerFx(FxType::SlideDown, std::make_shared<SlideFx>(false));
    registerFx(FxType::RandomNote, std::make_shared<RandomNoteFx>());
    registerFx(FxType::RandomVolume, std::make_shared<RandomVolumeFx>());
    registerFx(FxType::Off, std::make_shared<OffFx>());
    registerFx(FxType::Overdrive, std::make_shared<OverdriveFx>());
    registerFx(FxType::BitDepth, std::make_shared<BitDepthFx>());
    registerFx(FxType::GateLength, std::make_shared<GateLengthFx>());
    registerFx(FxType::SampleSlice, std::make_shared<SampleSliceFx>());
    registerFx(FxType::Chance, std::make_shared<ChanceFx>());
    registerFx(FxType::DelaySend, std::make_shared<DelaySendFx>());
    registerFx(FxType::ReverbSend, std::make_shared<ReverbSendFx>());
    registerFx(FxType::MicroMove, std::make_shared<MicroMoveFx>());
    registerFx(FxType::Arp, std::make_shared<ArpFx>());
    registerFx(FxType::Tempo, std::make_shared<DummyFx>());
    registerFx(FxType::Swing, std::make_shared<DummyFx>());
    registerFx(FxType::RandomInstrument, std::make_shared<DummyFx>());
    registerFx(FxType::RandomValue, std::make_shared<DummyFx>());
    registerFx(FxType::VolumeLfo, std::make_shared<LfoRateFx>(0));
    registerFx(FxType::PanningLfo, std::make_shared<LfoRateFx>(1));
    registerFx(FxType::FilterLfo, std::make_shared<LfoRateFx>(2));
    registerFx(FxType::PositionLfo, std::make_shared<LfoRateFx>(3));
    registerFx(FxType::FinetuneLfo, std::make_shared<LfoRateFx>(4));
    registerFx(FxType::AmpEnv, std::make_shared<DummyFx>());
    registerFx(FxType::FilterEnv, std::make_shared<DummyFx>());
    registerFx(FxType::GranularLength, std::make_shared<GranularLengthFx>());
    registerFx(FxType::SendCcA, std::make_shared<DummyFx>());
    registerFx(FxType::SendCcB, std::make_shared<DummyFx>());
    registerFx(FxType::SendCcC, std::make_shared<DummyFx>());
    registerFx(FxType::SendCcD, std::make_shared<DummyFx>());
    registerFx(FxType::SendCcE, std::make_shared<DummyFx>());
    registerFx(FxType::SendCcF, std::make_shared<DummyFx>());
    registerFx(FxType::MidiChord, std::make_shared<DummyFx>());
    registerFx(FxType::BreakPattern, std::make_shared<DummyFx>());
}

void StepFxRegistry::registerFx(uint8_t type, std::shared_ptr<IStepFx> handler) {
    handlers_[type] = std::move(handler);
}

void StepFxRegistry::applyFx(uint8_t type, uint8_t value, IVoice* voice, int rowLengthFrames) {
    if (type == FxType::None || !voice) return;
    auto it = handlers_.find(type);
    if (it != handlers_.end() && it->second) {
        it->second->apply(value, voice, rowLengthFrames);
    }
}

} // namespace trakr

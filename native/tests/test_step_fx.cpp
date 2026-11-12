#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "StepFx.h"
#include "Voice.h"
#include "views/PatternFxHelper.h"

// Mock Voice to record parameter adjustments applied by Step FX
class MockVoice : public trakr::IVoice {
public:
    void noteOn(int note, float velocity, const trakr::Instrument* inst) override {
        (void)note; (void)velocity; (void)inst; active = true;
    }
    void noteOff() override { active = false; }
    void stop() override { active = false; stopped = true; }
    bool isActive() const override { return active; }
    void process(float* outL, float* outR, int frames) override {
        (void)outL; (void)outR; (void)frames;
    }

    void setVolume(float v) override { volume = v; }
    void setPanning(float p) override { pan = p; }
    void setPitchRatio(float ratio) override { pitchRatio = ratio; }
    void setPosition(float posNormalized) override { position = posNormalized; }
    void setCutCountdown(int frames) override { cutCountdown = frames; }
    void setRetrigger(int intervalFrames, int count = 0) override {
        retriggerInterval = intervalFrames; retriggerCount = count;
    }
    void setGlide(float targetRatio, float rate = 0.05f) override {
        glideRatio = targetRatio; glideRate = rate;
    }
    void setReverse(bool rev) override { reverse = rev; }
    void setCutoff(float cutoffHz) override { cutoff = cutoffHz; }
    void setResonance(float res) override { resonance = res; }
    void setFilterType(int type) override { filterType = type; }
    void setOverdrive(float od) override { overdrive = od; }
    void setBitDepth(int bits) override { bitDepth = bits; }
    void setSlice(int sliceIndex) override { slice = sliceIndex; }
    void resetFx() override { fxReset = true; }
    void setDelaySend(float send) override { delaySend = send; }
    void setReverbSend(float send) override { reverbSend = send; }
    void setNudgeDelay(int frames) override { nudgeDelay = frames; }

    bool active{true};
    bool stopped{false};
    float volume{1.0f};
    float pan{0.0f};
    float pitchRatio{1.0f};
    float position{0.0f};
    int cutCountdown{-1};
    int retriggerInterval{0};
    int retriggerCount{0};
    float glideRatio{1.0f};
    float glideRate{0.0f};
    bool reverse{false};
    float cutoff{20000.0f};
    float resonance{0.707f};
    int filterType{0};
    float overdrive{0.0f};
    int bitDepth{16};
    int slice{0};
    bool fxReset{false};
    float delaySend{0.0f};
    float reverbSend{0.0f};
    int nudgeDelay{0};
};

TEST_CASE("Step FX parity - handler dispatch and parameter mapping", "[stepfx]") {
    auto& reg = trakr::StepFxRegistry::instance();
    MockVoice voice;

    // 1. Volume FX (V)
    reg.applyFx(trakr::FxType::Volume, 50, &voice, 1000);
    CHECK(voice.volume == Catch::Approx(0.5f));

    // 2. Panning FX (P: 0=Left, 50=Center, 100=Right)
    reg.applyFx(trakr::FxType::Panning, 0, &voice, 1000);
    CHECK(voice.pan == Catch::Approx(-1.0f));
    reg.applyFx(trakr::FxType::Panning, 50, &voice, 1000);
    CHECK(voice.pan == Catch::Approx(0.0f));
    reg.applyFx(trakr::FxType::Panning, 100, &voice, 1000);
    CHECK(voice.pan == Catch::Approx(1.0f));

    // 3. Overdrive FX (D)
    reg.applyFx(trakr::FxType::Overdrive, 75, &voice, 1000);
    CHECK(voice.overdrive == Catch::Approx(75.0f));

    // 4. Bit Depth FX (E: 4..16)
    reg.applyFx(trakr::FxType::BitDepth, 8, &voice, 1000);
    CHECK(voice.bitDepth == 8);
    reg.applyFx(trakr::FxType::BitDepth, 2, &voice, 1000);
    CHECK(voice.bitDepth == 4); // Clamped minimum 4

    // 5. Gate Length FX (q: 0..100% of row frames)
    reg.applyFx(trakr::FxType::GateLength, 50, &voice, 1000);
    CHECK(voice.cutCountdown == 500);

    // 6. Sample Slice FX (S)
    reg.applyFx(trakr::FxType::SampleSlice, 5, &voice, 1000);
    CHECK(voice.slice == 5);

    // 7. Off FX (!)
    voice.fxReset = false;
    reg.applyFx(trakr::FxType::Off, 0, &voice, 1000);
    CHECK(voice.fxReset == true);

    // 8. Filters (L, H, B)
    reg.applyFx(trakr::FxType::FilterLowpass, 50, &voice, 1000);
    CHECK(voice.filterType == 0);
    reg.applyFx(trakr::FxType::FilterHighpass, 50, &voice, 1000);
    CHECK(voice.filterType == 1);
    reg.applyFx(trakr::FxType::FilterBandpass, 50, &voice, 1000);
    CHECK(voice.filterType == 2);

    // 9. Roll FX (R)
    reg.applyFx(trakr::FxType::Roll, 4, &voice, 1000);
    CHECK(voice.retriggerInterval == 250);

    // 10. Delay Send (s) and Reverb Send (t)
    reg.applyFx(trakr::FxType::DelaySend, 60, &voice, 1000);
    CHECK(voice.delaySend == Catch::Approx(0.6f));
    reg.applyFx(trakr::FxType::ReverbSend, 80, &voice, 1000);
    CHECK(voice.reverbSend == Catch::Approx(0.8f));

    // 11. MicroMove (m)
    reg.applyFx(trakr::FxType::MicroMove, 25, &voice, 1000);
    CHECK(voice.nudgeDelay == 250);

    // 12. Arp (A)
    reg.applyFx(trakr::FxType::Arp, 4, &voice, 1000);
    CHECK(voice.retriggerInterval == 250);
}

TEST_CASE("Step FX definitions - character and name lookup matching firmware", "[stepfx]") {
    // Characters matching core/interface/core/interfaceDefs.h
    CHECK(trakr::fxTypeToChar(trakr::FxType::None) == '-');
    CHECK(trakr::fxTypeToChar(trakr::FxType::Off) == '!');
    CHECK(trakr::fxTypeToChar(trakr::FxType::Volume) == 'V');
    CHECK(trakr::fxTypeToChar(trakr::FxType::Panning) == 'P');
    CHECK(trakr::fxTypeToChar(trakr::FxType::Overdrive) == 'D');
    CHECK(trakr::fxTypeToChar(trakr::FxType::BitDepth) == 'E');
    CHECK(trakr::fxTypeToChar(trakr::FxType::DelaySend) == 's');
    CHECK(trakr::fxTypeToChar(trakr::FxType::ReverbSend) == 't');
    CHECK(trakr::fxTypeToChar(trakr::FxType::GateLength) == 'q');
    CHECK(trakr::fxTypeToChar(trakr::FxType::SampleSlice) == 'S');
    CHECK(trakr::fxTypeToChar(trakr::FxType::Roll) == 'R');
    CHECK(trakr::fxTypeToChar(trakr::FxType::Chance) == 'C');

    // Inverse lookup
    CHECK(trakr::fxCharToType('V') == trakr::FxType::Volume);
    CHECK(trakr::fxCharToType('v') == trakr::FxType::RandomVolume);
    CHECK(trakr::fxCharToType('D') == trakr::FxType::Overdrive);
    CHECK(trakr::fxCharToType('E') == trakr::FxType::BitDepth);
    CHECK(trakr::fxCharToType('s') == trakr::FxType::DelaySend);
    CHECK(trakr::fxCharToType('t') == trakr::FxType::ReverbSend);
}

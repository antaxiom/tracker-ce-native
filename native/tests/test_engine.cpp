#include <catch2/catch_all.hpp>
#include "Engine.h"
#include "Voice.h"
#include "Mixer.h"
#include "StepFx.h"
#include "EngineHost.h"
#include "Render.h"

#include <dr_wav.h>

#include <cmath>
#include <vector>
#include <thread>
#include <atomic>
#include <filesystem>

namespace {

std::shared_ptr<trakr::SampleData> createTestSine(int sampleRate, float freq, float durationSec) {
    auto sample = std::make_shared<trakr::SampleData>();
    sample->sampleRate = sampleRate;
    sample->channels = 1;

    const size_t totalFrames = static_cast<size_t>(sampleRate * durationSec);
    sample->ch[0].resize(totalFrames);

    const float phaseInc = 2.0f * M_PI * freq / static_cast<float>(sampleRate);
    float phase = 0.0f;
    for (size_t i = 0; i < totalFrames; ++i) {
        sample->ch[0][i] = std::sin(phase);
        phase += phaseInc;
    }
    return sample;
}

} // namespace

TEST_CASE("Voice playback and pitch transposition", "[engine]") {
    constexpr int kSampleRate = 44100;
    auto sineSample = createTestSine(kSampleRate, 440.0f, 0.5f);

    trakr::Instrument inst;
    inst.name = "Sine Inst";
    inst.sample = sineSample;
    inst.rootNote = 60; // C-4
    inst.finetune = 0;
    inst.loop = trakr::Instrument::Loop::None;

    trakr::SampleVoice voice(kSampleRate);
    REQUIRE_FALSE(voice.isActive());

    // Trigger root note
    voice.noteOn(60, 1.0f, &inst);
    REQUIRE(voice.isActive());

    std::vector<float> outL(256, 0.0f);
    std::vector<float> outR(256, 0.0f);
    voice.process(outL.data(), outR.data(), 256);

    float maxVal = 0.0f;
    for (float s : outL) maxVal = std::max(maxVal, std::abs(s));
    REQUIRE(maxVal > 0.5f);

    // Note-off should fade out smoothly
    voice.noteOff();
    REQUIRE(voice.isActive()); // Still releasing

    // Process enough frames to complete 2ms release
    std::vector<float> releaseL(512, 0.0f);
    std::vector<float> releaseR(512, 0.0f);
    voice.process(releaseL.data(), releaseR.data(), 512);
    REQUIRE_FALSE(voice.isActive()); // Should be inactive after fade
}

TEST_CASE("Voice forward loop mode", "[engine]") {
    constexpr int kSampleRate = 44100;
    auto sineSample = createTestSine(kSampleRate, 220.0f, 0.1f);

    trakr::Instrument inst;
    inst.sample = sineSample;
    inst.rootNote = 60;
    inst.loop = trakr::Instrument::Loop::Forward;
    inst.loopStart = 100;
    inst.loopEnd = 500;

    trakr::SampleVoice voice(kSampleRate);
    voice.noteOn(60, 1.0f, &inst);

    // Process many frames beyond loopEnd; voice must stay active and loop smoothly
    std::vector<float> outL(4096, 0.0f);
    std::vector<float> outR(4096, 0.0f);
    voice.process(outL.data(), outR.data(), 4096);
    REQUIRE(voice.isActive());

    voice.stop();
    REQUIRE_FALSE(voice.isActive());
}

TEST_CASE("Mixer pan law and soft limiter", "[engine]") {
    trakr::Mixer mixer(2);

    std::vector<trakr::TrackInfo> tracks(2);
    tracks[0].gainDb = 0.0f;
    tracks[0].pan = -1.0f; // Hard Left
    tracks[1].gainDb = 0.0f;
    tracks[1].pan = 1.0f;  // Hard Right

    constexpr int kFrames = 64;
    std::vector<float> track0L(kFrames, 1.0f);
    std::vector<float> track0R(kFrames, 1.0f);
    std::vector<float> track1L(kFrames, 1.0f);
    std::vector<float> track1R(kFrames, 1.0f);

    const float* inL[2] = {track0L.data(), track1L.data()};
    const float* inR[2] = {track0R.data(), track1R.data()};

    std::vector<float> masterOut(kFrames * 2, 0.0f);
    mixer.process(tracks, inL, inR, masterOut.data(), kFrames);

    // Track 0 hard left: right channel must be near zero
    // Track 1 hard right: left channel must be near zero
    REQUIRE(masterOut[0] > 0.9f); // Left from track 0
    REQUIRE(masterOut[1] > 0.9f); // Right from track 1

    // Test soft limiter: feed massive signals (+10.0f) and verify output is smoothly saturated
    std::vector<float> loudL(kFrames, 10.0f);
    std::vector<float> loudR(kFrames, 10.0f);
    const float* loudPtrL[2] = {loudL.data(), loudL.data()};
    const float* loudPtrR[2] = {loudR.data(), loudR.data()};

    mixer.process(tracks, loudPtrL, loudPtrR, masterOut.data(), kFrames);
    for (float sample : masterOut) {
        REQUIRE(std::abs(sample) <= 1.05f); // Soft limited below 1.05
        REQUIRE_FALSE(std::isnan(sample));
    }
}

namespace {
// Records capture-device requests so the line-in wiring can be asserted
// without touching real audio hardware.
class FakeInputBackend : public trakr::AudioBackend {
public:
    bool open(int, int, Callback) override { return true; }
    void close() override {}
    bool start() override { return true; }
    void stop() override {}
    bool isRunning() const override { return true; }
    bool setInputEnabled(bool enabled) override {
        lastInputEnabled = enabled;
        ++inputEnableCalls;
        return true;
    }
    bool lastInputEnabled{false};
    int inputEnableCalls{0};
};
} // namespace

TEST_CASE("Line-in capture is requested only while the bus is unmuted", "[engine][aux]") {
    auto backend = std::make_unique<FakeInputBackend>();
    auto* fake = backend.get();
    trakr::EngineHost host(std::move(backend));

    trakr::Song muted;
    muted.tracks.resize(1);
    muted.auxMute[3] = true; // firmware default: line in muted
    host.publish(muted);
    REQUIRE_FALSE(fake->lastInputEnabled);

    trakr::Song unmuted = muted;
    unmuted.auxMute[3] = false;
    host.publish(unmuted);
    REQUIRE(fake->lastInputEnabled);

    host.publish(muted);
    REQUIRE_FALSE(fake->lastInputEnabled);
}

TEST_CASE("Mixer line-in bus monitoring", "[engine][aux]") {
    // Global Mixer "Line In" (gMasterParams.cpp showGlobalMixer): input frames
    // are summed into the master with the aux line gain and metered on aux 3.
    trakr::Mixer mixer(1);
    std::vector<trakr::TrackInfo> tracks(1);
    std::vector<float> inL(64, 0.0f), inR(64, 0.0f);
    const float* trackL[1] = {inL.data()};
    const float* trackR[1] = {inR.data()};
    std::vector<float> line(64 * 2, 0.0f);
    for (int i = 0; i < 64; ++i) {
        line[i * 2] = 0.25f;
        line[i * 2 + 1] = -0.25f;
    }
    std::vector<float> out(64 * 2, 0.0f);

    mixer.setAuxGains(0.0f, 0.0f, 1.0f, 1.0f);
    mixer.setLineIn(line.data(), 64);
    mixer.process(tracks, trackL, trackR, out.data(), 64, 0.0f);
    mixer.setLineIn(nullptr, 0);

    REQUIRE(out[0] == Catch::Approx(0.25f).margin(1e-3f));
    REQUIRE(out[1] == Catch::Approx(-0.25f).margin(1e-3f));
    const auto meters = mixer.getAuxMeters(3);
    REQUIRE(meters.peakL == Catch::Approx(0.25f).margin(1e-2f));
    REQUIRE(meters.peakR == Catch::Approx(0.25f).margin(1e-2f));

    // Muted bus (aux line gain 0) contributes nothing to the master.
    std::fill(out.begin(), out.end(), 0.0f);
    mixer.setAuxGains(0.0f, 0.0f, 1.0f, 0.0f);
    mixer.setLineIn(line.data(), 64);
    mixer.process(tracks, trackL, trackR, out.data(), 64, 0.0f);
    mixer.setLineIn(nullptr, 0);
    REQUIRE(out[0] == Catch::Approx(0.0f).margin(1e-6f));
}

TEST_CASE("Engine drains capture frames into the line-in bus", "[engine][aux]") {
    trakr::Engine engine(44100);
    trakr::Song song;
    song.tracks.resize(1);
    song.auxMute[3] = false;   // Line In unmuted
    song.auxVolume[3] = 86;    // 0 dB per the firmware default
    engine.setSongSnapshot(trakr::makeSnapshot(song));

    std::vector<float> line(64 * 2, 0.0f);
    for (int i = 0; i < 64; ++i) {
        line[i * 2] = 0.2f;
        line[i * 2 + 1] = 0.1f;
    }
    engine.pushInput(line.data(), 64);

    std::vector<float> out(64 * 2, 0.0f);
    engine.process(out.data(), 64);
    REQUIRE(out[0] == Catch::Approx(0.2f).margin(5e-3f));
    REQUIRE(out[1] == Catch::Approx(0.1f).margin(5e-3f));
    REQUIRE(engine.getAuxMeters(3).peakL == Catch::Approx(0.2f).margin(1e-2f));

    // No queued input: the bus is silent again.
    std::fill(out.begin(), out.end(), 0.0f);
    engine.process(out.data(), 64);
    REQUIRE(std::abs(out[0]) < 1e-6f);
}

TEST_CASE("Mixer mute and solo behavior", "[engine]") {
    trakr::Mixer mixer(2);

    std::vector<trakr::TrackInfo> tracks(2);
    tracks[0].gainDb = 0.0f;
    tracks[0].pan = 0.0f;
    tracks[1].gainDb = 0.0f;
    tracks[1].pan = 0.0f;

    constexpr int kFrames = 32;
    std::vector<float> sig(kFrames, 0.5f);
    const float* inL[2] = {sig.data(), sig.data()};
    const float* inR[2] = {sig.data(), sig.data()};
    std::vector<float> out(kFrames * 2, 0.0f);

    // Mute Track 0
    tracks[0].mute = true;
    tracks[1].mute = false;
    mixer.process(tracks, inL, inR, out.data(), kFrames);
    auto meters0 = mixer.getTrackMeters(0);
    auto meters1 = mixer.getTrackMeters(1);
    REQUIRE(meters0.peakL == 0.0f);
    REQUIRE(meters1.peakL > 0.0f);

    // Solo Track 0 -> Track 1 must now be muted
    trakr::Mixer soloMixer(2);
    tracks[0].mute = false;
    tracks[0].solo = true;
    tracks[1].solo = false;
    std::fill(out.begin(), out.end(), 0.0f);
    soloMixer.process(tracks, inL, inR, out.data(), kFrames);
    meters0 = soloMixer.getTrackMeters(0);
    meters1 = soloMixer.getTrackMeters(1);
    REQUIRE(meters0.peakL > 0.0f);
    REQUIRE(meters1.peakL == 0.0f);
}

TEST_CASE("StepFx registry dispatch", "[engine]") {
    auto sample = createTestSine(44100, 440.0f, 0.2f);
    trakr::Instrument inst;
    inst.sample = sample;
    inst.rootNote = 60;

    trakr::SampleVoice voice(44100);
    voice.noteOn(60, 1.0f, &inst);

    // Volume FX
    trakr::StepFxRegistry::instance().applyFx(trakr::FxType::Volume, 50, &voice, 1000);
    // Panning FX
    trakr::StepFxRegistry::instance().applyFx(trakr::FxType::Panning, 0, &voice, 1000); // Hard left
    // Microtuning FX
    trakr::StepFxRegistry::instance().applyFx(trakr::FxType::Microtuning, 75, &voice, 1000); // +25 cents

    std::vector<float> l(64, 0.0f), r(64, 0.0f);
    voice.process(l.data(), r.data(), 64);

    REQUIRE(voice.isActive());

    // Roll FX test
    trakr::StepFxRegistry::instance().applyFx(trakr::FxType::Roll, 4, &voice, 256);
    voice.process(l.data(), r.data(), 128);
    REQUIRE(voice.isActive());

    // Glide FX test
    trakr::StepFxRegistry::instance().applyFx(trakr::FxType::Glide, 62, &voice, 256);
    voice.process(l.data(), r.data(), 128);
    REQUIRE(voice.isActive());

    // NoteCut FX test (cut after 100 frames)
    trakr::StepFxRegistry::instance().applyFx(trakr::FxType::NoteCut, 8, &voice, 200);
    voice.process(l.data(), r.data(), 120);
    REQUIRE_FALSE(voice.isActive()); // Note was cleanly cut
}

TEST_CASE("Engine sample-accurate sequencing and transport", "[engine]") {
    constexpr int kSampleRate = 44100;
    trakr::Engine engine(kSampleRate);

    trakr::Song song = trakr::makeDefaultSong();
    song.bpm = 120.0f;
    song.lpb = 4; // 120 BPM * 4 LPB = 480 rows/min = 8 rows/sec -> 5512.5 frames/row

    auto sample = createTestSine(kSampleRate, 440.0f, 1.0f);
    song.instruments[0].sample = sample;

    // Put a note on row 0 and row 1
    song.patterns[0].tracks[0][0].note = 60;
    song.patterns[0].tracks[0][0].instrument = 0;
    song.patterns[0].tracks[0][1].note = 62;
    song.patterns[0].tracks[0][1].instrument = 0;

    engine.setSongSnapshot(trakr::makeSnapshot(song));
    engine.play(0, 0);
    REQUIRE(engine.isPlaying());

    // Process 256-frame blocks
    std::vector<float> block(256 * 2, 0.0f);
    engine.process(block.data(), 256);

    auto playhead = engine.getPlayhead();
    REQUIRE(playhead.playing);
    REQUIRE(playhead.row == 0);

    engine.stop();
    REQUIRE_FALSE(engine.isPlaying());
}

TEST_CASE("EngineHost threaded concurrency stress test", "[engine]") {
    trakr::EngineHost host(std::make_unique<trakr::NullAudioBackend>());
    REQUIRE(host.start(44100, 128));

    trakr::Song song = trakr::makeDefaultSong();
    host.publish(song);
    host.play(0, 0);

    std::atomic<bool> audioRunning{true};

    // Simulated audio thread
    std::thread audioThread([&]() {
        std::vector<float> buf(128 * 2, 0.0f);
        while (audioRunning.load(std::memory_order_relaxed)) {
            host.getEngine().process(buf.data(), 128);
        }
    });

    // UI thread rapidly publishing 500 song updates and note events
    for (int i = 0; i < 500; ++i) {
        song.bpm = 100.0f + static_cast<float>(i % 40);
        host.publish(song);
        host.previewNoteOn(0, 0, 60 + (i % 12), 0.8f);
        if (i % 50 == 0) {
            host.setPosition(0, i % 64);
        }
    }

    audioRunning.store(false, std::memory_order_relaxed);
    audioThread.join();

    host.stop();
    REQUIRE_FALSE(host.isRunning());
}

TEST_CASE("Offline render to WAV file", "[engine]") {
    trakr::Song song = trakr::makeDefaultSong();
    song.bpm = 120.0f;
    song.lpb = 4;
    song.patterns[0].rows = 16; // Short pattern for fast test

    auto sample = createTestSine(44100, 330.0f, 0.2f);
    song.instruments[0].sample = sample;
    song.patterns[0].tracks[0][0].note = 64;
    song.patterns[0].tracks[0][0].instrument = 0;

    const std::string outPath = "test_render_output.wav";
    if (std::filesystem::exists(outPath)) {
        std::filesystem::remove(outPath);
    }

    REQUIRE(trakr::renderToWav(song, outPath, 44100, 16));
    REQUIRE(std::filesystem::exists(outPath));

    // Verify written WAV format with dr_wav
    drwav wav;
    REQUIRE(drwav_init_file(&wav, outPath.c_str(), nullptr));
    REQUIRE(wav.channels == 2);
    REQUIRE(wav.sampleRate == 44100);
    REQUIRE(wav.bitsPerSample == 16);
    REQUIRE(wav.totalPCMFrameCount > 0);
    drwav_uninit(&wav);

    std::filesystem::remove(outPath);
}

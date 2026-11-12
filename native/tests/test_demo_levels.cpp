#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "DemoSong.h"
#include "Engine.h"
#include "EngineHost.h"
#include <cmath>
#include <vector>
#include <algorithm>

using namespace trakr;

namespace {

struct AudioStats {
    float peak{0.0f};
    float rms{0.0f};
    float peakDb{-100.0f};
    float rmsDb{-100.0f};
};

AudioStats analyzePatternAudio(Song song, int patIdx, int sampleRate = 44100) {
    song.order = {patIdx};
    Engine engine(sampleRate);
    engine.setSongSnapshot(makeSnapshot(song));
    engine.play(0, 0);

    int rows = song.patterns[patIdx].rows;
    double seconds = (static_cast<double>(rows) * 60.0) / (song.bpm * song.lpb);
    size_t totalFrames = static_cast<size_t>(std::ceil(seconds * sampleRate));

    constexpr size_t kBlock = 256;
    std::vector<float> buf(kBlock * 2, 0.0f);

    float maxAbs = 0.0f;
    double sumSq = 0.0;
    size_t totalSamples = 0;

    size_t rendered = 0;
    while (rendered < totalFrames) {
        size_t chunk = std::min(kBlock, totalFrames - rendered);
        engine.process(buf.data(), static_cast<int>(chunk));
        for (size_t i = 0; i < chunk * 2; ++i) {
            float val = std::abs(buf[i]);
            if (val > maxAbs) maxAbs = val;
            sumSq += buf[i] * buf[i];
        }
        totalSamples += chunk * 2;
        rendered += chunk;
    }

    AudioStats stats;
    stats.peak = maxAbs;
    stats.rms = (totalSamples > 0) ? static_cast<float>(std::sqrt(sumSq / totalSamples)) : 0.0f;
    stats.peakDb = (stats.peak > 1e-5f) ? (20.0f * std::log10(stats.peak)) : -100.0f;
    stats.rmsDb = (stats.rms > 1e-5f) ? (20.0f * std::log10(stats.rms)) : -100.0f;
    return stats;
}

} // namespace

TEST_CASE("Demo song level staging and pattern headroom", "[audio][demo][levels]") {
    Song song;
    populateDemoSong(song);

    SECTION("Pattern 0 (Intro / Groove) has comfortable headroom") {
        AudioStats s = analyzePatternAudio(song, 0);
        // Intro should peak below -3 dBFS and have healthy RMS
        REQUIRE(s.peakDb <= -3.0f);
        REQUIRE(s.peakDb >= -24.0f);
        REQUIRE(s.rmsDb <= -10.0f);
    }

    SECTION("Pattern 1 (Full 8-Track Arrangement) peaks <= -3.0 dBFS without blown bus") {
        AudioStats s = analyzePatternAudio(song, 1);
        // Pattern 1 has all 8 tracks firing simultaneously.
        // It must not clip or blow out the master bus!
        REQUIRE(s.peakDb <= -3.0f);
        REQUIRE(s.peakDb >= -18.0f);
        REQUIRE(s.rmsDb <= -10.0f);
        REQUIRE(s.rmsDb >= -26.0f);
    }

    SECTION("Pattern 2 (Breakdown) has clean headroom") {
        AudioStats s = analyzePatternAudio(song, 2);
        REQUIRE(s.peakDb <= -3.0f);
        REQUIRE(s.peakDb >= -24.0f);
    }
}

TEST_CASE("Dedicated preview voice isolation and note-off lifecycle", "[engine][preview][voice]") {
    Song song;
    populateDemoSong(song);

    EngineHost host(std::make_unique<NullAudioBackend>());
    host.start(44100, 128);
    host.publish(song);

    // Audition acid bass instrument (#3) on dedicated preview lane (-1)
    host.previewNoteOn(-1, 3, 60, 0.9f);

    std::vector<float> block(256 * 2, 0.0f);
    host.getEngine().process(block.data(), 256);

    // Should produce sound
    float maxAbs = 0.0f;
    for (float f : block) maxAbs = std::max(maxAbs, std::abs(f));
    REQUIRE(maxAbs > 0.01f);

    // Now send Note-Off
    host.previewNoteOff(-1);

    // Process 400ms (more than release time)
    for (int b = 0; b < 70; ++b) {
        host.getEngine().process(block.data(), 256);
    }

    // After release time has elapsed, preview voice must be completely silent
    host.getEngine().process(block.data(), 256);
    float postMaxAbs = 0.0f;
    for (float f : block) postMaxAbs = std::max(postMaxAbs, std::abs(f));
    REQUIRE(postMaxAbs < 1e-4f);

    // Verify allNotesOff silences voices
    host.previewNoteOn(-1, 4, 60, 0.9f);
    host.getEngine().process(block.data(), 256);
    host.allNotesOff();
    host.getEngine().process(block.data(), 256);
    float stopMaxAbs = 0.0f;
    for (float f : block) stopMaxAbs = std::max(stopMaxAbs, std::abs(f));
    REQUIRE(stopMaxAbs < 1e-4f);
}


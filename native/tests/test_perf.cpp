#include <catch2/catch_test_macros.hpp>
#include "Song.h"
#include "Engine.h"
#include <chrono>
#include <vector>
#include <iostream>

TEST_CASE("Engine benchmark: 32 active voices at 128 frames", "[perf][engine]") {
    trakr::Song song;
    song.bpm = 140.0f;
    song.lpb = 4;
    song.tracks.resize(32);
    for (size_t t = 0; t < 32; ++t) {
        song.tracks[t].name = "Track " + std::to_string(t + 1);
        song.tracks[t].gainDb = -6.0f;
    }

    trakr::Instrument inst;
    inst.name = "TestSample";
    auto sample = std::make_shared<trakr::SampleData>();
    sample->channels = 2;
    sample->sampleRate = 44100;
    sample->ch[0].assign(44100, 0.4f);
    sample->ch[1].assign(44100, 0.4f);
    inst.sample = sample;
    song.instruments.push_back(std::move(inst));

    trakr::Pattern pat;
    pat.rows = 64;
    pat.tracks.resize(32);
    for (size_t t = 0; t < 32; ++t) {
        pat.tracks[t].resize(64);
        pat.tracks[t][0].note = static_cast<int16_t>(48 + (t % 24));
        pat.tracks[t][0].instrument = 0;
        pat.tracks[t][0].volume = 40;
    }
    song.patterns.push_back(std::move(pat));
    song.order = {0};

    trakr::Engine engine(44100);
    engine.init(44100);
    engine.setSongSnapshot(trakr::makeSnapshot(song));

    // Start playing
    trakr::LiveEvent playEv{};
    playEv.type = trakr::LiveEvent::Type::Play;
    engine.pushLiveEvent(playEv);

    constexpr int kFrames = 128;
    std::vector<float> outputBuffer(kFrames * 2, 0.0f);

    // Warm-up
    for (int i = 0; i < 20; ++i) {
        engine.process(outputBuffer.data(), kFrames);
    }

    // Benchmark 1000 blocks (128,000 frames)
    const int kIterations = 1000;
    auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < kIterations; ++i) {
        engine.process(outputBuffer.data(), kFrames);
    }
    auto t1 = std::chrono::steady_clock::now();

    double totalMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    double avgMsPerBuffer = totalMs / static_cast<double>(kIterations);

    // 128 frames at 44.1kHz is 2.902 ms real-time. Target: < 20% CPU (< 0.58 ms per buffer)
    std::cout << "[PERF BENCHMARK] 32 Voices Engine: " << avgMsPerBuffer << " ms per 128-frame buffer ("
              << (avgMsPerBuffer / 2.902 * 100.0) << "% CPU load)\n";

    REQUIRE(avgMsPerBuffer < 0.60); // Real-time budget with headroom
}

TEST_CASE("Pattern view culling benchmark: 64 tracks x 512 rows", "[perf][culling]") {
    trakr::Song song;
    song.tracks.resize(64);
    trakr::Pattern pat;
    pat.rows = 512;
    pat.tracks.resize(64);
    for (int t = 0; t < 64; ++t) {
        pat.tracks[t].resize(512);
        for (int r = 0; r < 512; ++r) {
            pat.tracks[t][r].note = (r % 4 == 0) ? 60 : -1;
            pat.tracks[t][r].instrument = 0;
            pat.tracks[t][r].volume = 40;
        }
    }
    song.patterns.push_back(std::move(pat));

    // Simulate culling viewport of 32 visible rows and 12 visible tracks
    const int kVisibleRows = 32;
    const int kVisibleTracks = 12;
    const int kBenchmarkFrames = 1000;

    auto t0 = std::chrono::steady_clock::now();
    uint64_t dummyChecksum = 0;
    for (int f = 0; f < kBenchmarkFrames; ++f) {
        int scrollRow = (f * 2) % (512 - kVisibleRows);
        int scrollTrack = (f % (64 - kVisibleTracks));

        for (int r = 0; r < kVisibleRows; ++r) {
            int actualRow = scrollRow + r;
            for (int t = 0; t < kVisibleTracks; ++t) {
                int actualTrack = scrollTrack + t;
                const auto& cell = song.patterns[0].tracks[actualTrack][actualRow];
                dummyChecksum += (cell.note + cell.volume + cell.instrument);
            }
        }
    }
    auto t1 = std::chrono::steady_clock::now();

    double totalMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    double avgMsPerFrame = totalMs / static_cast<double>(kBenchmarkFrames);

    std::cout << "[PERF BENCHMARK] 64x512 Pattern View Culling: " << avgMsPerFrame << " ms per frame (target: < 2.0 ms)\n";
    (void)dummyChecksum;

    REQUIRE(avgMsPerFrame < 2.0); // Target < 2.0 ms
}


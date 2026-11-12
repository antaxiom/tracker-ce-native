#include <catch2/catch_test_macros.hpp>
#include "Song.h"
#include "Engine.h"
#include "Command.h"
#include "History.h"
#include <random>
#include <cmath>
#include <vector>

TEST_CASE("Fuzz testing audio engine with randomized song data", "[fuzz][engine]") {
    trakr::Song song = trakr::makeDefaultSong();

    // Generate random instruments with samples
    std::mt19937 rng(1337);
    for (int i = 0; i < 8; ++i) {
        trakr::Instrument inst;
        inst.name = "FuzzInst_" + std::to_string(i);
        auto sample = std::make_shared<trakr::SampleData>();
        sample->channels = 2;
        sample->sampleRate = 44100;
        sample->ch[0].resize(1000);
        sample->ch[1].resize(1000);
        std::uniform_real_distribution<float> sampDist(-1.0f, 1.0f);
        for (size_t s = 0; s < 1000; ++s) {
            sample->ch[0][s] = sampDist(rng);
            sample->ch[1][s] = sampDist(rng);
        }
        inst.sample = sample;
        inst.rootNote = 60;
        song.instruments.push_back(std::move(inst));
    }

    // Populate pattern with random notes, volumes, instruments, and FX
    std::uniform_int_distribution<int> noteDist(0, 119);
    std::uniform_int_distribution<int> instDist(0, 7);
    std::uniform_int_distribution<int> volDist(0, 64);
    std::uniform_int_distribution<int> fxTypeDist(0, 25);
    std::uniform_int_distribution<int> fxValDist(0, 255);

    auto& pat = song.patterns[0];
    for (size_t t = 0; t < pat.tracks.size(); ++t) {
        for (int r = 0; r < pat.rows; ++r) {
            auto& cell = pat.tracks[t][r];
            cell.note = static_cast<int16_t>(noteDist(rng));
            cell.instrument = static_cast<int16_t>(instDist(rng));
            cell.volume = static_cast<int16_t>(volDist(rng));
            cell.fx[0].type = static_cast<uint8_t>(fxTypeDist(rng));
            cell.fx[0].value = static_cast<uint8_t>(fxValDist(rng));
            cell.fx[1].type = static_cast<uint8_t>(fxTypeDist(rng));
            cell.fx[1].value = static_cast<uint8_t>(fxValDist(rng));
        }
    }

    trakr::Engine engine(44100);
    engine.init(44100);
    engine.setSongSnapshot(trakr::makeSnapshot(song));

    trakr::LiveEvent playEv{};
    playEv.type = trakr::LiveEvent::Type::Play;
    playEv.orderIndex = 0;
    playEv.row = 0;
    engine.pushLiveEvent(playEv);

    // Process 200 blocks (25,600 frames) of audio
    constexpr int kFrames = 128;
    std::vector<float> outputBuffer(kFrames * 2, 0.0f);

    for (int block = 0; block < 200; ++block) {
        engine.process(outputBuffer.data(), kFrames);

        for (int i = 0; i < kFrames * 2; ++i) {
            float s = outputBuffer[i];
            REQUIRE(std::isfinite(s));
            REQUIRE(!std::isnan(s));
            REQUIRE(!std::isinf(s));
            // Master limiter saturates cleanly
            REQUIRE(std::abs(s) <= 2.0f);
        }
    }
}

TEST_CASE("Fuzz testing history undo/redo stress", "[fuzz][history]") {
    trakr::Song song = trakr::makeDefaultSong();
    trakr::History history(200);

    std::mt19937 rng(999);
    std::uniform_int_distribution<int> actionDist(0, 4);
    std::uniform_int_distribution<int> trackDist(0, 7);
    std::uniform_int_distribution<int> rowDist(0, 60);

    for (int step = 0; step < 150; ++step) {
        int act = actionDist(rng);
        if (act == 0) {
            int trk = trackDist(rng);
            int row = rowDist(rng);
            trakr::Cell oldCell = song.patterns[0].tracks[trk][row];
            trakr::Cell newCell = oldCell;
            newCell.note = static_cast<int16_t>(step % 120);
            history.execute(std::make_unique<trakr::SetCellCommand>(0, trk, row, newCell, oldCell), song);
        } else if (act == 1) {
            int row = rowDist(rng);
            history.execute(std::make_unique<trakr::InsertRowsCommand>(0, row, 1), song);
        } else if (act == 2) {
            int row = rowDist(rng);
            history.execute(std::make_unique<trakr::DeleteRowsCommand>(0, row, 1), song);
        } else if (act == 3) {
            if (history.canUndo()) history.undo(song);
        } else {
            if (history.canRedo()) history.redo(song);
        }

        // Verify song invariants hold
        REQUIRE(song.patterns[0].tracks.size() == 8);
        REQUIRE(song.patterns[0].rows > 0);
    }
}


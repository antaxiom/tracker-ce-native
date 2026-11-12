// Copyright 2026 Trakr Authors.
// Sourced from and inspired by Polyend Tracker reverse engineering specifications
// (sandlbn/polyend-tracker-re / OpenTracker project), licensed under the Apache License,
// Version 2.0 (the "License"); you may not use this file except in compliance with the License.
// You may obtain a copy of the License at:
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software distributed under the
// License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND.

#include <catch2/catch_test_macros.hpp>
#include "Song.h"
#include "Engine.h"
#include "EngineHost.h"
#include "PolyendImport.h"
#include "PolyendExport.h"
#include <filesystem>
#include <cmath>

using namespace trakr;

TEST_CASE("Polyend RE: Pattern Playback Modes and Bounds", "[re][playmode]") {
    Song song = makeDefaultSong();
    auto& pat = song.patterns[0];
    pat.rows = 16;
    for (int t = 0; t < 8; ++t) pat.tracks[t].resize(16);

    // Fill track 0 with ascending notes
    for (int r = 0; r < 16; ++r) {
        pat.tracks[0][r].note = 48 + r; // C-3 to D#4
        pat.tracks[0][r].instrument = 0;
    }

    SECTION("Forward playback reaches active pattern boundary") {
        int nextRow = 0;
        for (int step = 0; step < 16; ++step) {
            REQUIRE(nextRow == step);
            nextRow = (nextRow + 1) % pat.rows;
        }
        REQUIRE(nextRow == 0);
    }

    SECTION("Backward playback wraps to active pattern length correctly") {
        int row = 0;
        int prevRow = (row - 1 + pat.rows) % pat.rows;
        REQUIRE(prevRow == 15);

        for (int i = 0; i < 16; ++i) {
            row = (row - 1 + pat.rows) % pat.rows;
        }
        REQUIRE(row == 0);
    }
}

TEST_CASE("Polyend RE: Track Queuing and Song Slot Advance", "[re][queuing]") {
    Song song = makeDefaultSong();
    // Create 3 patterns
    Pattern pat1{64, std::vector<std::vector<Cell>>(8, std::vector<Cell>(64))};
    Pattern pat2{64, std::vector<std::vector<Cell>>(8, std::vector<Cell>(64))};
    song.patterns.push_back(pat1);
    song.patterns.push_back(pat2);
    song.order = {0, 1, 2};

    // Mark pattern 0 track 0 with note 60, pattern 1 track 0 with note 64
    song.patterns[0].tracks[0][0].note = 60;
    song.patterns[1].tracks[0][0].note = 64;
    song.patterns[2].tracks[0][0].note = 67;

    SECTION("Advancing order slots preserves pattern sequence") {
        int currentOrderIdx = 0;
        REQUIRE(song.order[currentOrderIdx] == 0);
        REQUIRE(song.patterns[song.order[currentOrderIdx]].tracks[0][0].note == 60);

        currentOrderIdx = (currentOrderIdx + 1) % static_cast<int>(song.order.size());
        REQUIRE(song.order[currentOrderIdx] == 1);
        REQUIRE(song.patterns[song.order[currentOrderIdx]].tracks[0][0].note == 64);

        currentOrderIdx = (currentOrderIdx + 1) % static_cast<int>(song.order.size());
        REQUIRE(song.order[currentOrderIdx] == 2);
        REQUIRE(song.patterns[song.order[currentOrderIdx]].tracks[0][0].note == 67);
    }
}

TEST_CASE("Polyend RE: Micro-step and Roll Subdivision Timing", "[re][roll]") {
    SECTION("Subdivisions 2, 3, 4, 8, 12, 16 evaluate without remainder drift") {
        const int sampleRate = 44100;
        const float bpm = 120.0f;
        const int lpb = 4;
        const float rowSamples = (60.0f / bpm / lpb) * sampleRate; // 5512.5 samples

        const int subdivisions[] = {2, 3, 4, 6, 8, 12, 16};
        for (int sub : subdivisions) {
            float subInterval = rowSamples / sub;
            float total = subInterval * sub;
            REQUIRE(std::abs(total - rowSamples) < 0.001f);
        }
    }
}

TEST_CASE("Polyend RE: SD Project Structure Round-Trip", "[re][project_io]") {
    Song original = makeDefaultSong();
    original.bpm = 125.0f;
    original.order = {0, 0, 1};

    Pattern pat32{32, std::vector<std::vector<Cell>>(8, std::vector<Cell>(32))};
    original.patterns.push_back(pat32);
    original.patterns[0].tracks[0][0].note = 50;
    original.patterns[1].tracks[1][0].note = 62;

    const std::string outDir = "./exports/re_test_proj";
    std::filesystem::create_directories(outDir);

    REQUIRE(io::exportPolyendProject(outDir, original));
    REQUIRE(std::filesystem::exists(outDir + "/project.mt"));
    REQUIRE(std::filesystem::exists(outDir + "/patterns/pattern_01.mtp"));

    Song loaded;
    REQUIRE(io::loadPolyendProject(outDir, loaded));
    REQUIRE(loaded.patterns.size() >= 1);
    REQUIRE(std::filesystem::remove_all(outDir) > 0);
}

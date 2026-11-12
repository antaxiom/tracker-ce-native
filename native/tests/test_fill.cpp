#include <catch2/catch_test_macros.hpp>
#include "Scale.h"
#include "FillGenerator.h"
#include "Song.h"
#include "Command.h"
#include "History.h"

using namespace trakr;

TEST_CASE("Scale intervals and snapToScale", "[model][scale]") {
    // C Major: C D E F G A B
    REQUIRE(isNoteInScale(60, 0, ScaleType::Major)); // C-4
    REQUIRE(isNoteInScale(62, 0, ScaleType::Major)); // D-4
    REQUIRE(isNoteInScale(64, 0, ScaleType::Major)); // E-4
    REQUIRE_FALSE(isNoteInScale(61, 0, ScaleType::Major)); // C#4 not in C Maj

    // Snap C#4 (61) to D-4 (62) snapping up, or C-4 (60) snapping down
    REQUIRE(snapToScale(61, 0, ScaleType::Major, true) == 62);
    REQUIRE(snapToScale(61, 0, ScaleType::Major, false) == 60);

    // Pentatonic Minor: C, Eb, F, G, Bb (0, 3, 5, 7, 10)
    REQUIRE(isNoteInScale(60, 0, ScaleType::MinorPentatonic)); // C
    REQUIRE(isNoteInScale(63, 0, ScaleType::MinorPentatonic)); // Eb
    REQUIRE_FALSE(isNoteInScale(62, 0, ScaleType::MinorPentatonic)); // D

    REQUIRE(getNoteName(0) == std::string("C"));
    REQUIRE(getNoteName(1) == std::string("C#"));
    REQUIRE(getNoteName(11) == std::string("B"));
}

TEST_CASE("Euclidean rhythm distribution", "[model][fill]") {
    // E(3, 8) -> 3 hits in 8 steps
    auto e3_8 = generateEuclideanRhythm(3, 8);
    REQUIRE(e3_8.size() == 8);
    int hits = 0;
    for (bool h : e3_8) if (h) hits++;
    REQUIRE(hits == 3);

    // E(4, 16) -> 4 hits in 16 steps (every 4th step)
    auto e4_16 = generateEuclideanRhythm(4, 16);
    int count = 0;
    for (bool h : e4_16) if (h) count++;
    REQUIRE(count == 4);

    // Edge cases
    REQUIRE(generateEuclideanRhythm(0, 16).size() == 16);
    auto all = generateEuclideanRhythm(16, 16);
    for (bool h : all) REQUIRE(h);
}

TEST_CASE("Fill generator application and undo/redo", "[model][fill]") {
    Song song = makeDefaultSong();
    History history;

    FillConfig cfg;
    cfg.mode = FillMode::Euclidean;
    cfg.startRow = 0;
    cfg.endRow = 15;
    cfg.baseNote = 48; // C-3
    cfg.instrument = 2;
    cfg.hits = 4;
    cfg.volumeStart = 50;

    auto oldTrack = song.patterns[0].tracks[0];
    Pattern patCopy = song.patterns[0];
    applyFill(patCopy, 0, cfg);

    history.execute(std::make_unique<SetTrackCommand>(0, 0, patCopy.tracks[0], oldTrack), song);

    // Verify fill hits placed
    int filledNotes = 0;
    for (int r = 0; r <= 15; ++r) {
        if (song.patterns[0].tracks[0][r].note == 48) filledNotes++;
    }
    REQUIRE(filledNotes == 4);

    // Verify undo restores original track
    history.undo(song);
    REQUIRE(song.patterns[0].tracks[0][0].note == kNoNote);

    // Verify redo reapplies fill
    history.redo(song);
    int redoCount = 0;
    for (int r = 0; r <= 15; ++r) {
        if (song.patterns[0].tracks[0][r].note == 48) redoCount++;
    }
    REQUIRE(redoCount == 4);
}

TEST_CASE("Fill generator adapted to multi-track selection range", "[model][fill]") {
    Song song = makeDefaultSong();
    History history;

    // Simulate selection range: tracks 1..2, rows 8..15
    int minT = 1, maxT = 2;
    int minR = 8, maxR = 15;

    FillConfig cfg;
    cfg.mode = FillMode::Euclidean;
    cfg.startRow = minR;
    cfg.endRow = maxR;
    cfg.baseNote = 60; // C-4
    cfg.instrument = 1;
    cfg.hits = 4;
    cfg.volumeStart = 45;

    for (int t = minT; t <= maxT; ++t) {
        auto oldTrack = song.patterns[0].tracks[t];
        Pattern patCopy = song.patterns[0];
        applyFill(patCopy, t, cfg);
        history.execute(std::make_unique<SetTrackCommand>(0, t, patCopy.tracks[t], oldTrack), song);
    }

    // Verify both tracks 1 and 2 received the 4 hits in rows 8..15, and rows 0..7 remain empty
    for (int t = minT; t <= maxT; ++t) {
        int hitsInRange = 0;
        for (int r = 0; r < 8; ++r) REQUIRE(song.patterns[0].tracks[t][r].note == kNoNote);
        for (int r = minR; r <= maxR; ++r) {
            if (song.patterns[0].tracks[t][r].note == 60) hitsInRange++;
        }
        REQUIRE(hitsInRange == 4);
    }

    // Track 0 should be untouched
    for (int r = minR; r <= maxR; ++r) {
        REQUIRE(song.patterns[0].tracks[0][r].note != 60);
    }
}


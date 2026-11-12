#include <catch2/catch_test_macros.hpp>
#include "Song.h"
#include "Command.h"
#include "History.h"
#include <random>

static bool songsEqual(const trakr::Song& a, const trakr::Song& b) {
    if (a.bpm != b.bpm || a.lpb != b.lpb) return false;
    if (a.order != b.order) return false;
    if (a.tracks.size() != b.tracks.size()) return false;
    for (size_t t = 0; t < a.tracks.size(); ++t) {
        if (a.tracks[t].gainDb != b.tracks[t].gainDb) return false;
        if (a.tracks[t].pan != b.tracks[t].pan) return false;
        if (a.tracks[t].mute != b.tracks[t].mute) return false;
        if (a.tracks[t].solo != b.tracks[t].solo) return false;
    }
    if (a.patterns.size() != b.patterns.size()) return false;
    for (size_t p = 0; p < a.patterns.size(); ++p) {
        if (a.patterns[p].rows != b.patterns[p].rows) return false;
        if (a.patterns[p].tracks.size() != b.patterns[p].tracks.size()) return false;
        for (size_t t = 0; t < a.patterns[p].tracks.size(); ++t) {
            for (int r = 0; r < a.patterns[p].rows; ++r) {
                const auto& ca = a.patterns[p].tracks[t][r];
                const auto& cb = b.patterns[p].tracks[t][r];
                if (ca.note != cb.note || ca.instrument != cb.instrument || ca.volume != cb.volume) return false;
                for (size_t f = 0; f < 4; ++f) {
                    if (ca.fx[f].type != cb.fx[f].type || ca.fx[f].value != cb.fx[f].value) return false;
                }
            }
        }
    }
    return true;
}

TEST_CASE("History undo/redo property test", "[model][history]") {
    trakr::Song original = trakr::makeDefaultSong();
    trakr::Song current = original;
    trakr::History history(500);

    std::mt19937 rng(42);
    std::uniform_int_distribution<int> trackDist(0, 7);
    std::uniform_int_distribution<int> rowDist(0, 63);
    std::uniform_int_distribution<int> noteDist(0, 119);
    std::uniform_int_distribution<int> cmdTypeDist(0, 3);

    const int kNumCommands = 50;
    for (int i = 0; i < kNumCommands; ++i) {
        int cmdType = cmdTypeDist(rng);
        if (cmdType == 0) {
            // Set cell
            int trk = trackDist(rng);
            int row = rowDist(rng);
            trakr::Cell oldCell = current.patterns[0].tracks[trk][row];
            trakr::Cell newCell = oldCell;
            newCell.note = static_cast<int16_t>(noteDist(rng));
            newCell.instrument = 0;
            newCell.volume = 40;
            history.execute(std::make_unique<trakr::SetCellCommand>(0, trk, row, newCell, oldCell), current);
        } else if (cmdType == 1) {
            // Insert rows
            int row = rowDist(rng);
            history.execute(std::make_unique<trakr::InsertRowsCommand>(0, row, 2), current);
        } else if (cmdType == 2) {
            // Delete rows
            int row = rowDist(rng);
            history.execute(std::make_unique<trakr::DeleteRowsCommand>(0, row, 2), current);
        } else {
            // Change tempo
            float oldBpm = current.bpm;
            float newBpm = 100.0f + static_cast<float>(i);
            history.execute(std::make_unique<trakr::SetSongParamCommand>(newBpm, oldBpm, current.lpb, current.lpb), current);
        }
    }

    trakr::Song modified = current;
    REQUIRE(!songsEqual(original, modified));

    // Undo all
    while (history.canUndo()) {
        history.undo(current);
    }
    REQUIRE(songsEqual(original, current));

    // Redo all
    while (history.canRedo()) {
        history.redo(current);
    }
    REQUIRE(songsEqual(modified, current));
}


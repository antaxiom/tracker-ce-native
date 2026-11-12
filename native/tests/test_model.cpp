#include <catch2/catch_all.hpp>
#include "Song.h"
#include "Command.h"
#include "History.h"

TEST_CASE("Song default initialization", "[model]") {
    trakr::Song song = trakr::makeDefaultSong();

    REQUIRE(song.bpm == 120.0f);
    REQUIRE(song.lpb == 4);
    REQUIRE(song.tracks.size() == 8);
    REQUIRE(song.patterns.size() == 1);
    REQUIRE(song.patterns[0].rows == 64);
    REQUIRE(song.patterns[0].tracks.size() == 8);
    REQUIRE(song.order.size() == 1);
    REQUIRE(song.order[0] == 0);
    REQUIRE(song.instruments.size() == 1);
}

TEST_CASE("Cell default values", "[model]") {
    trakr::Cell cell;
    REQUIRE(cell.note == trakr::kNoNote);
    REQUIRE(cell.instrument == -1);
    REQUIRE(cell.volume == -1);
    REQUIRE(cell.fx[0].type == 0);
    REQUIRE(cell.fx[0].value == 0);
}

TEST_CASE("AddTrackCommand and RemoveTrackCommand", "[model][command]") {
    trakr::Song song = trakr::makeDefaultSong();
    trakr::History history;

    REQUIRE(song.tracks.size() == 8);
    REQUIRE(song.patterns[0].tracks.size() == 8);

    history.execute(std::make_unique<trakr::AddTrackCommand>(), song);
    REQUIRE(song.tracks.size() == 9);
    REQUIRE(song.patterns[0].tracks.size() == 9);
    REQUIRE(song.tracks[8].name == "Track 9");

    history.undo(song);
    REQUIRE(song.tracks.size() == 8);
    REQUIRE(song.patterns[0].tracks.size() == 8);

    history.redo(song);
    REQUIRE(song.tracks.size() == 9);

    history.execute(std::make_unique<trakr::RemoveTrackCommand>(), song);
    REQUIRE(song.tracks.size() == 8);
    REQUIRE(song.patterns[0].tracks.size() == 8);

    history.undo(song);
    REQUIRE(song.tracks.size() == 9);
    REQUIRE(song.tracks[8].name == "Track 9");
}


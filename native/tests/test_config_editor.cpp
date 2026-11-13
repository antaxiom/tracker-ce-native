#include <catch2/catch_test_macros.hpp>

#include "views/ConfigView.h"

#include "EngineHost.h"
#include "Song.h"
#include "Theme.h"

using trakr::ConfigView;

TEST_CASE("Config item editor - open and apply a text list", "[config_editor]") {
    ConfigView cv;
    trakr::Song song;
    trakr::EngineHost host;

    REQUIRE(cv.openEditorForName("Anti-aliasing", song, host));
    CHECK(cv.editing());
    CHECK(cv.selectedPlace() == 1); // frame stays on the submenu place
    CHECK(cv.editor().title == "Anti-aliasing");
    CHECK_FALSE(cv.editor().description.empty());
    REQUIRE(cv.editor().options.size() == 2);
    CHECK(cv.editor().options[0] == "Off");
    CHECK(cv.editor().options[1] == "On");
    CHECK(cv.editor().selected == 1); // firmware default: anti-aliasing on

    cv.moveEditorSelection(-1);
    CHECK(cv.editor().selected == 0);
    cv.applyEditor(song, host);
    CHECK_FALSE(cv.editing());
    CHECK(cv.currentRows(song)[5].value == "Off");

    // Apply persists only after Enter; Cancel drops the pending option.
    REQUIRE(cv.openEditorForName("Anti-aliasing", song, host));
    cv.moveEditorSelection(+1);
    cv.cancelEditor();
    CHECK_FALSE(cv.editing());
    CHECK(cv.currentRows(song)[5].value == "Off");
}

TEST_CASE("Config item editor - numeric list maps to the model", "[config_editor]") {
    ConfigView cv;
    trakr::Song song;
    trakr::EngineHost host;

    song.lpb = 4;
    REQUIRE(cv.openEditorForName("Pattern divider", song, host));
    REQUIRE(cv.editor().options.size() == 16);
    CHECK(cv.editor().options.front() == "1");
    CHECK(cv.editor().options.back() == "16");
    CHECK(cv.editor().selected == 3); // lpb 4 -> option index 3

    cv.moveEditorSelection(+1);
    cv.applyEditor(song, host);
    CHECK(song.lpb == 5);
    CHECK(cv.currentRows(song)[1].value == "5");
}

TEST_CASE("Config item editor - song-backed toggle", "[config_editor]") {
    ConfigView cv;
    trakr::Song song;
    trakr::EngineHost host;

    song.headroom = false;
    REQUIRE(cv.openEditorForName("Extra headroom", song, host));
    CHECK(cv.editor().selected == 0);
    cv.moveEditorSelection(+1);
    cv.applyEditor(song, host);
    CHECK(song.headroom);
}

TEST_CASE("Config item editor - label rows have no editor", "[config_editor]") {
    ConfigView cv;
    trakr::Song song;
    trakr::EngineHost host;

    // About rows mirror the firmware Info page label items.
    cv.setActiveGroup(ConfigView::Group::About);
    cv.selectItem(0);
    CHECK_FALSE(cv.openEditor(song, host));
    CHECK_FALSE(cv.editing());
}

TEST_CASE("Config item editor - option sets match the firmware tables", "[config_editor]") {
    ConfigView cv;
    trakr::Song song;
    trakr::EngineHost host;

    REQUIRE(cv.openEditorForName("Limiter mode", song, host));
    REQUIRE(cv.editor().options.size() == 2);
    CHECK(cv.editor().options[0] == "Subtle");
    CHECK(cv.editor().options[1] == "Extreme");
    cv.cancelEditor();

    REQUIRE(cv.openEditorForName("Notes in channel", song, host));
    REQUIRE(cv.editor().options.size() == 17);
    CHECK(cv.editor().options.front() == "ALL");
    CHECK(cv.editor().options.back() == "CH16");
    cv.cancelEditor();

    REQUIRE(cv.openEditorForName("Notes scale", song, host));
    CHECK(cv.editor().options.size() == 39);
    CHECK(cv.editor().options.front() == "Chromatic");
    cv.cancelEditor();
}

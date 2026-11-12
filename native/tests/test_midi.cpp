#include <catch2/catch_test_macros.hpp>
#include "MidiManager.h"
#include "views/ConfigView.h"
#include "ScreenManager.h"
#include "Song.h"
#include "EngineHost.h"
#include "Theme.h"

using namespace trakr;

TEST_CASE("MidiManager initialization and event queue", "[audio][midi]") {
    auto& midi = MidiManager::instance();
    REQUIRE(midi.init());

    // Port enumeration does not crash even if no device is connected
    auto ports = midi.getPortNames();
    (void)ports;

    // Test ring buffer
    MidiEvent ev1;
    ev1.type = MidiEvent::Type::NoteOn;
    ev1.channel = 0;
    ev1.note = 60; // Middle C
    ev1.velocity = 100;
    ev1.timestamp = 1.234;

    midi.pushEvent(ev1);

    MidiEvent popped;
    REQUIRE(midi.pollEvent(popped));
    REQUIRE(popped.type == MidiEvent::Type::NoteOn);
    REQUIRE(popped.note == 60);
    REQUIRE(popped.velocity == 100);

    // Queue is now empty
    REQUIRE_FALSE(midi.pollEvent(popped));

    // Multiple events
    for (int i = 0; i < 10; ++i) {
        MidiEvent ev;
        ev.type = (i % 2 == 0) ? MidiEvent::Type::NoteOn : MidiEvent::Type::NoteOff;
        ev.note = static_cast<uint8_t>(40 + i);
        ev.velocity = static_cast<uint8_t>(60 + i);
        midi.pushEvent(ev);
    }

    int received = 0;
    while (midi.pollEvent(popped)) {
        REQUIRE(popped.note == static_cast<uint8_t>(40 + received));
        received++;
    }
    REQUIRE(received == 10);
}

TEST_CASE("ConfigView MIDI In device selection", "[audio][midi][config]") {
    ConfigView cv;
    Song song;
    EngineHost engineHost;
    Theme theme = makeTrackerClassicTheme();

    cv.setActiveGroup(ConfigView::Group::Midi);
    cv.setSelectedItem(0); // MIDI In Device row

    // adjustSelectedValue triggers safe port selection cycle even with 0 or N ports
    cv.adjustSelectedValue(song, engineHost, theme, 1);
    Screen scr = Screen::Config;
    auto btns = cv.getScreenButtons(song, engineHost, theme, scr);
    REQUIRE(btns.size() == 8);
    REQUIRE(cv.activeGroup() == ConfigView::Group::Midi);
}


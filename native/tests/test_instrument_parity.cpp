#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "Song.h"
#include "Voice.h"
#include "views/InstrumentView.h"
#include "EngineHost.h"
#include "SongJson.h"
#include <filesystem>

using namespace trakr;

TEST_CASE("Instrument model defaults and bounds parity", "[instrument]") {
    Instrument inst;
    CHECK(inst.volume == 50);
    CHECK(inst.panning == 0);
    CHECK(inst.tune == 0);
    CHECK(inst.finetune == 0);
    CHECK(inst.filterType == Instrument::FilterType::None);
    CHECK(inst.cutoff == 100);
    CHECK(inst.resonance == 0);
    CHECK(inst.overdrive == 0);
    CHECK(inst.bitDepth == 16);
    CHECK(inst.delaySend == 0);
    CHECK(inst.reverbSend == 0);

    // Automation defaults
    REQUIRE(inst.automation.size() == 6);
    for (int d = 0; d < 6; ++d) {
        CHECK(inst.automation[d].type == Instrument::AutoType::Off);
        CHECK(inst.automation[d].amount == 1.0f);
    }
}

TEST_CASE("Instrument DSP parameter mapping in SampleVoice", "[instrument]") {
    SampleVoice voice(44100);
    Instrument inst;

    // Test tune & finetune pitch ratio
    inst.tune = 12; // +1 octave
    inst.finetune = 0;
    voice.noteOn(60, 1.0f, &inst);
    CHECK(voice.isActive());

    // Test overdrive and bit depth processing without crash
    inst.overdrive = 50;
    inst.bitDepth = 8;
    inst.filterType = Instrument::FilterType::Lowpass;
    inst.cutoff = 50;
    inst.resonance = 30;
    voice.noteOn(60, 1.0f, &inst);

    std::vector<float> bufL(128, 0.0f);
    std::vector<float> bufR(128, 0.0f);
    voice.process(bufL.data(), bufR.data(), 128);

    voice.stop();
    CHECK_FALSE(voice.isActive());
}

TEST_CASE("InstrumentView column selection and parameter adjustments", "[instrument]") {
    Song song = makeDefaultSong();
    EngineHost host;
    InstrumentView view;

    REQUIRE(!song.instruments.empty());
    int selInst = 0;

    // 1. Initial page is General
    CHECK(view.subPage() == InstrumentView::SubPage::General);
    CHECK(view.selectedColumn() == 0);

    // 2. Adjust Volume (col 0)
    view.setSelectedColumn(0);
    view.adjustSelectedParam(song, host, selInst, 5);
    CHECK(song.instruments[0].volume == 55);
    view.resetSelectedParam(song, host, selInst);
    CHECK(song.instruments[0].volume == 50);

    // 3. Adjust Panning (col 1)
    view.setSelectedColumn(1);
    view.adjustSelectedParam(song, host, selInst, 10);
    CHECK(song.instruments[0].panning == 10);
    view.resetSelectedParam(song, host, selInst);
    CHECK(song.instruments[0].panning == 0);

    // 4. Adjust Tune (col 2)
    view.setSelectedColumn(2);
    view.adjustSelectedParam(song, host, selInst, 3);
    CHECK(song.instruments[0].tune == 3);
    view.resetSelectedParam(song, host, selInst);
    CHECK(song.instruments[0].tune == 0);

    // 5. Adjust Finetune (col 3)
    view.setSelectedColumn(3);
    view.adjustSelectedParam(song, host, selInst, 5);
    CHECK(song.instruments[0].finetune == 5);
    view.resetSelectedParam(song, host, selInst);
    CHECK(song.instruments[0].finetune == 0);

    // 6. Filter Type list: Down moves to the next row, Up to the previous
    view.setSelectedColumn(4);
    view.adjustSelectedParam(song, host, selInst, -1);
    CHECK(song.instruments[0].filterType == Instrument::FilterType::Lowpass);
    view.adjustSelectedParam(song, host, selInst, 1);
    CHECK(song.instruments[0].filterType == Instrument::FilterType::None);
    view.adjustSelectedParam(song, host, selInst, 1);
    CHECK(song.instruments[0].filterType == Instrument::FilterType::Bandpass);
    view.resetSelectedParam(song, host, selInst);
    CHECK(song.instruments[0].filterType == Instrument::FilterType::None);

    // 7. Adjust Cutoff and Resonance (cols 5 and 6)
    view.setSelectedColumn(5);
    view.adjustSelectedParam(song, host, selInst, -10);
    CHECK(song.instruments[0].cutoff == 90);
    view.resetSelectedParam(song, host, selInst);
    CHECK(song.instruments[0].cutoff == 100);

    view.setSelectedColumn(6);
    view.adjustSelectedParam(song, host, selInst, 15);
    CHECK(song.instruments[0].resonance == 15);
    view.resetSelectedParam(song, host, selInst);
    CHECK(song.instruments[0].resonance == 0);

    // 8. Col 7 is "More" on the General page: a page button, not a value
    view.setSelectedColumn(7);
    view.adjustSelectedParam(song, host, selInst, 10);
    CHECK(song.instruments[0].overdrive == 0);

    // 9. Page key opens the Effects page
    view.togglePage();
    CHECK(view.subPage() == InstrumentView::SubPage::Effects);

    // 10. Effects page adjustments
    view.setSelectedColumn(0); // Overdrive
    view.adjustSelectedParam(song, host, selInst, 10);
    CHECK(song.instruments[0].overdrive == 10);

    view.setSelectedColumn(1); // Bit depth
    view.adjustSelectedParam(song, host, selInst, -4);
    CHECK(song.instruments[0].bitDepth == 12);

    view.setSelectedColumn(2); // Reverb send
    view.adjustSelectedParam(song, host, selInst, 15);
    CHECK(song.instruments[0].reverbSend == 15);

    view.setSelectedColumn(3); // Delay send
    view.adjustSelectedParam(song, host, selInst, 20);
    CHECK(song.instruments[0].delaySend == 20);

    // Reset Effects
    view.resetSelectedParam(song, host, selInst);
    CHECK(song.instruments[0].delaySend == 0);

    // Back to General page
    view.togglePage();
    CHECK(view.subPage() == InstrumentView::SubPage::General);

    // 11. Automation page (reached from the Effects page button)
    view.setSubPage(InstrumentView::SubPage::Automation);
    CHECK(view.subPage() == InstrumentView::SubPage::Automation);

    // Select Type col (2), Down moves to Envelope then LFO
    view.setSelectedColumn(2);
    view.adjustSelectedParam(song, host, selInst, -1);
    CHECK(song.instruments[0].automation[2].type == Instrument::AutoType::Envelope);
    view.adjustSelectedParam(song, host, selInst, -1);
    CHECK(song.instruments[0].automation[2].type == Instrument::AutoType::Lfo);
    view.adjustSelectedParam(song, host, selInst, 1);
    CHECK(song.instruments[0].automation[2].type == Instrument::AutoType::Envelope);

    // Verify screen buttons generation
    auto btns = view.getScreenButtons(song, host, selInst);
    REQUIRE(btns.size() == 8);
    CHECK(btns[0].label == "Destination");
    CHECK(btns[2].label == "Type");
}

TEST_CASE("Instrument JSON serialization parity roundtrip", "[instrument]") {
    Song orig = makeDefaultSong();
    orig.instruments[0].volume = 65;
    orig.instruments[0].panning = -25;
    orig.instruments[0].tune = -4;
    orig.instruments[0].finetune = 15;
    orig.instruments[0].filterType = Instrument::FilterType::Highpass;
    orig.instruments[0].cutoff = 75;
    orig.instruments[0].resonance = 40;
    orig.instruments[0].overdrive = 35;
    orig.instruments[0].bitDepth = 12;
    orig.instruments[0].delaySend = 50;
    orig.instruments[0].reverbSend = 60;

    std::string tmpPath = "/tmp/trakr_test_inst_parity.json";
    REQUIRE(saveSongJson(orig, tmpPath));

    Song loaded;
    REQUIRE(loadSongJson(loaded, tmpPath));
    std::filesystem::remove(tmpPath);

    REQUIRE(!loaded.instruments.empty());
    const auto& in = loaded.instruments[0];
    CHECK(in.volume == 65);
    CHECK(in.panning == -25);
    CHECK(in.tune == -4);
    CHECK(in.finetune == 15);
    CHECK(in.filterType == Instrument::FilterType::Highpass);
    CHECK(in.cutoff == 75);
    CHECK(in.resonance == 40);
    CHECK(in.overdrive == 35);
    CHECK(in.bitDepth == 12);
    CHECK(in.delaySend == 50);
    CHECK(in.reverbSend == 60);
}

#include <catch2/catch_test_macros.hpp>
#include "PolyendImport.h"
#include "PolyendExport.h"
#include "SongJson.h"
#include "Render.h"
#include <filesystem>
#include <fstream>
#include <vector>

TEST_CASE("Audio file and Polyend import", "[io][import]") {
    // 1. Generate a test WAV file (16-bit and 24-bit studio formats)
    trakr::Song song = trakr::makeDefaultSong();
    const std::string wavPath = "test_import.wav";
    bool rendered = trakr::renderToWav(song, wavPath, 44100, 16);
    REQUIRE(rendered);

    trakr::SampleData loadedSample;
    bool loadedWav = trakr::io::loadAudioFile(wavPath, loadedSample);
    REQUIRE(loadedWav);
    REQUIRE(loadedSample.sampleRate == 44100);
    REQUIRE(!loadedSample.ch[0].empty());

    const std::string wav24Path = "test_import_24bit.wav";
    bool rendered24 = trakr::renderToWav(song, wav24Path, 48000, 24);
    REQUIRE(rendered24);
    trakr::SampleData loaded24Sample;
    REQUIRE(trakr::io::loadAudioFile(wav24Path, loaded24Sample));
    REQUIRE(loaded24Sample.sampleRate == 48000);
    std::filesystem::remove(wav24Path);

    // 2. Synthesize a mock .pti file
    const std::string ptiPath = "test_synth.pti";
    {
        std::ofstream out(ptiPath, std::ios::binary);
        std::vector<char> header(392, 0);
        header[0] = 'T'; header[1] = 'I';
        const char* name = "Kick909";
        std::memcpy(&header[16], name, std::strlen(name));
        out.write(header.data(), header.size());

        // 500 samples of 16-bit PCM
        std::vector<int16_t> pcm(500, 1000);
        out.write(reinterpret_cast<const char*>(pcm.data()), pcm.size() * sizeof(int16_t));
    }

    trakr::Instrument ptiInst;
    bool loadedPti = trakr::io::loadPtiInstrument(ptiPath, ptiInst);
    REQUIRE(loadedPti);
    REQUIRE(ptiInst.name == "Kick909");
    REQUIRE(ptiInst.sample != nullptr);
    REQUIRE(ptiInst.sample->ch[0].size() == 500);

    // 3. Synthesize a mock .mtp pattern file
    const std::string mtpPath = "test_patt.mtp";
    {
        std::ofstream out(mtpPath, std::ios::binary);
        std::vector<char> header(24, 0);
        header[0] = 'M'; header[1] = 'T';
        out.write(header.data(), header.size());

        // 8 tracks of 128 steps
        // MtpTrack: uint8_t length; MtpStep step[128];
        for (int t = 0; t < 8; ++t) {
            uint8_t length = 63; // 64 rows
            out.write(reinterpret_cast<const char*>(&length), 1);
            for (int r = 0; r < 128; ++r) {
                int8_t note = (r == 0 && t == 0) ? 60 : -1;
                uint8_t inst = 0; // Instrument 1 (0-indexed in Polyend Tracker)
                uint8_t fx1_type = 0, fx1_val = 0;
                uint8_t fx2_type = 0, fx2_val = 0;
                int8_t volume = 40;
                int8_t micromove = -1;

                out.write(reinterpret_cast<const char*>(&note), 1);
                out.write(reinterpret_cast<const char*>(&inst), 1);
                out.write(reinterpret_cast<const char*>(&fx1_type), 1);
                out.write(reinterpret_cast<const char*>(&fx1_val), 1);
                out.write(reinterpret_cast<const char*>(&fx2_type), 1);
                out.write(reinterpret_cast<const char*>(&fx2_val), 1);
                out.write(reinterpret_cast<const char*>(&volume), 1);
                out.write(reinterpret_cast<const char*>(&micromove), 1);
            }
        }
    }

    trakr::Pattern mtpPat;
    bool loadedMtp = trakr::io::loadMtpPattern(mtpPath, mtpPat);
    REQUIRE(loadedMtp);
    REQUIRE(mtpPat.tracks.size() == 8);
    REQUIRE(mtpPat.tracks[0][0].note == 60);
    REQUIRE(mtpPat.tracks[0][0].instrument == 0);

    // 4. Test project.mt and project folder loading
    const std::string projDir = "test_poly_proj";
    std::filesystem::create_directories(projDir);
    {
        std::ofstream pmt(projDir + "/project.mt", std::ios::binary);
        std::vector<uint8_t> pmtData(16 + 256, 0);
        pmtData[0] = 'M'; pmtData[1] = 'T';
        pmtData[16] = 1; // Pattern 1 (index 0)
        pmtData[17] = 2; // Pattern 2 (index 1)
        pmt.write(reinterpret_cast<const char*>(pmtData.data()), pmtData.size());
    }
    trakr::Song polySong;
    bool loadedProj = trakr::io::loadPolyendProject(projDir, polySong);
    REQUIRE(loadedProj);
    REQUIRE(polySong.order.size() == 2);
    REQUIRE(polySong.order[0] == 0);
    REQUIRE(polySong.order[1] == 1);

    // Clean up temporary files
    std::filesystem::remove(wavPath);
    std::filesystem::remove(ptiPath);
    std::filesystem::remove(mtpPath);
    std::filesystem::remove_all(projDir);
}

TEST_CASE("Song JSON serialization round-trip", "[io][json]") {
    trakr::Song orig = trakr::makeDefaultSong();
    orig.bpm = 142.5f;
    orig.lpb = 8;
    orig.tracks[0].name = "LeadSynth";
    orig.tracks[0].gainDb = -2.5f;
    orig.tracks[0].delaySend = 0.45f;
    orig.tracks[0].reverbSend = 0.30f;

    orig.patterns[0].tracks[0][0].note = 72;
    orig.patterns[0].tracks[0][0].instrument = 0;
    orig.patterns[0].tracks[0][0].volume = 64;
    orig.patterns[0].tracks[0][0].fx[0] = {19, 50};

    orig.masterGainDb = -3.0f;
    orig.masterEq[2] = 2.5f;
    orig.headroom = true;
    orig.auxVolume[0] = 88;
    orig.auxMute[1] = true;

    orig.instruments[0].name = "AcidBass";
    orig.instruments[0].attackMs = 15.0f;
    orig.instruments[0].decayMs = 80.0f;
    orig.instruments[0].sustain = 0.6f;
    orig.instruments[0].releaseMs = 120.0f;
    orig.instruments[0].filterType = trakr::Instrument::FilterType::Lowpass;
    orig.instruments[0].filterCutoffHz = 1500.0f;
    orig.instruments[0].filterResonance = 2.5f;
    orig.instruments[0].lfoWave = trakr::Instrument::LfoWave::Triangle;
    orig.instruments[0].lfoTarget = trakr::Instrument::LfoTarget::Cutoff;
    orig.instruments[0].lfoRateHz = 2.0f;
    orig.instruments[0].lfoDepth = 0.4f;

    const std::string jsonFile = "test_roundtrip.trakr.json";
    bool saved = trakr::saveSongJson(orig, jsonFile);
    REQUIRE(saved);

    trakr::Song loaded;
    bool ok = trakr::loadSongJson(loaded, jsonFile);
    REQUIRE(ok);

    REQUIRE(loaded.bpm == 142.5f);
    REQUIRE(loaded.lpb == 8);
    REQUIRE(loaded.tracks.size() == orig.tracks.size());
    REQUIRE(loaded.tracks[0].name == "LeadSynth");
    REQUIRE(loaded.tracks[0].gainDb == -2.5f);
    REQUIRE(loaded.tracks[0].delaySend == 0.45f);
    REQUIRE(loaded.tracks[0].reverbSend == 0.30f);
    REQUIRE(loaded.masterGainDb == -3.0f);
    REQUIRE(loaded.masterEq[2] == 2.5f);
    REQUIRE(loaded.headroom);
    REQUIRE(loaded.auxVolume[0] == 88);
    REQUIRE(loaded.auxMute[1]);

    REQUIRE(loaded.patterns.size() == orig.patterns.size());
    REQUIRE(loaded.patterns[0].tracks[0][0].note == 72);
    REQUIRE(loaded.patterns[0].tracks[0][0].instrument == 0);
    REQUIRE(loaded.patterns[0].tracks[0][0].volume == 64);
    REQUIRE(loaded.patterns[0].tracks[0][0].fx[0].type == 19);
    REQUIRE(loaded.patterns[0].tracks[0][0].fx[0].value == 50);

    REQUIRE(loaded.instruments.size() == orig.instruments.size());
    REQUIRE(loaded.instruments[0].name == "AcidBass");
    REQUIRE(loaded.instruments[0].attackMs == 15.0f);
    REQUIRE(loaded.instruments[0].decayMs == 80.0f);
    REQUIRE(loaded.instruments[0].sustain == 0.6f);
    REQUIRE(loaded.instruments[0].releaseMs == 120.0f);
    REQUIRE(loaded.instruments[0].filterType == trakr::Instrument::FilterType::Lowpass);
    REQUIRE(loaded.instruments[0].filterCutoffHz == 1500.0f);
    REQUIRE(loaded.instruments[0].filterResonance == 2.5f);
    REQUIRE(loaded.instruments[0].lfoWave == trakr::Instrument::LfoWave::Triangle);
    REQUIRE(loaded.instruments[0].lfoTarget == trakr::Instrument::LfoTarget::Cutoff);
    REQUIRE(loaded.instruments[0].lfoRateHz == 2.0f);
    REQUIRE(loaded.instruments[0].lfoDepth == 0.4f);

    std::filesystem::remove(jsonFile);
}

TEST_CASE("Polyend export and import round-trip", "[io][polyend]") {
    // 1. PTI Export & Import
    trakr::Instrument origInst;
    origInst.name = "TestKick";
    origInst.loop = trakr::Instrument::Loop::Forward;
    auto s = std::make_shared<trakr::SampleData>();
    s->channels = 1;
    s->sampleRate = 44100;
    s->ch[0].resize(200);
    for (size_t i = 0; i < 200; ++i) {
        s->ch[0][i] = std::sin(static_cast<float>(i) * 0.1f) * 0.8f;
    }
    origInst.sample = s;
    origInst.startFrame = 0;
    origInst.loopStart = 50;
    origInst.loopEnd = 150;
    origInst.endFrame = 200;

    const std::string ptiFile = "roundtrip_test.pti";
    REQUIRE(trakr::io::exportPtiInstrument(ptiFile, origInst));

    trakr::Instrument loadedInst;
    REQUIRE(trakr::io::loadPtiInstrument(ptiFile, loadedInst));
    REQUIRE(loadedInst.name == "TestKick");
    REQUIRE(loadedInst.loop == trakr::Instrument::Loop::Forward);
    REQUIRE(loadedInst.sample != nullptr);
    REQUIRE(loadedInst.sample->ch[0].size() == 200);
    std::filesystem::remove(ptiFile);

    // 2. MTP Pattern Export & Import
    trakr::Pattern origPat;
    origPat.rows = 64;
    origPat.tracks.resize(8);
    for (int t = 0; t < 8; ++t) {
        origPat.tracks[t].resize(64);
    }
    origPat.tracks[0][0].note = 60; // C-4
    origPat.tracks[0][0].instrument = 3;
    origPat.tracks[0][0].volume = 48;
    origPat.tracks[0][0].fx[0].type = 18; // Volume
    origPat.tracks[0][0].fx[0].value = 50;
    origPat.tracks[0][4].note = trakr::kNoteOff;

    const std::string mtpFile = "roundtrip_test.mtp";
    REQUIRE(trakr::io::exportMtpPattern(mtpFile, origPat));

    trakr::Pattern loadedPat;
    REQUIRE(trakr::io::loadMtpPattern(mtpFile, loadedPat));
    REQUIRE(loadedPat.rows == 64);
    REQUIRE(loadedPat.tracks.size() == 8);
    REQUIRE(loadedPat.tracks[0][0].note == 60);
    REQUIRE(loadedPat.tracks[0][0].instrument == 3);
    REQUIRE(loadedPat.tracks[0][0].volume == 48);
    REQUIRE(loadedPat.tracks[0][4].note == trakr::kNoteOff);
    std::filesystem::remove(mtpFile);

    // 3. Complete Project Folder Export & Import
    trakr::Song song = trakr::makeDefaultSong();
    song.bpm = 128.0f;
    song.patterns[0] = origPat;
    song.instruments[0] = origInst;
    song.order = {0, 0};

    const std::string projDir = "roundtrip_polyend_project";
    REQUIRE(trakr::io::exportPolyendProject(projDir, song));

    trakr::Song loadedSong;
    REQUIRE(trakr::io::loadPolyendProject(projDir, loadedSong));
    REQUIRE(loadedSong.bpm == 128.0f);
    REQUIRE(!loadedSong.patterns.empty());
    REQUIRE(!loadedSong.instruments.empty());

    std::filesystem::remove_all(projDir);
}

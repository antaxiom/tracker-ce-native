#include <catch2/catch_test_macros.hpp>
#include "BackgroundImporter.h"
#include "PolyendImport.h"
#include "Render.h"
#include "views/SampleLoaderView.h"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <thread>
#include <vector>

namespace {

// Poll up to `deadline` (default 5s) for a completed result.
bool waitForResult(trakr::io::BackgroundImporter& imp, trakr::io::BackgroundImporter::Result& out,
                   std::chrono::milliseconds deadline = std::chrono::milliseconds(5000)) {
    const auto stop = std::chrono::steady_clock::now() + deadline;
    while (std::chrono::steady_clock::now() < stop) {
        if (imp.poll(out)) return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    return false;
}

}  // namespace

TEST_CASE("BackgroundImporter loads a WAV off-thread and reports it", "[io][background][wav]") {
    // Reuse the render pipeline to produce a small, real WAV in the CWD.
    trakr::Song song = trakr::makeDefaultSong();
    const std::string wavPath = "test_bg_import.wav";
    REQUIRE(trakr::renderToWav(song, wavPath, 44100, 16));

    trakr::io::BackgroundImporter imp;
    REQUIRE(imp.enqueue({wavPath, 3}));

    trakr::io::BackgroundImporter::Result r;
    REQUIRE(waitForResult(imp, r));
    REQUIRE(r.ok);
    CHECK(r.slot == 3);
    CHECK(r.inst.sample != nullptr);
    CHECK(!r.inst.sample->ch[0].empty());
    CHECK(r.inst.sample->sampleRate == 44100);
    CHECK(!r.name.empty());

    std::filesystem::remove(wavPath);
}

TEST_CASE("BackgroundImporter rejects unknown extensions up front", "[io][background]") {
    trakr::io::BackgroundImporter imp;
    CHECK_FALSE(imp.enqueue({"notes.txt", 0}));
    CHECK_FALSE(imp.enqueue({"", 0}));
}

TEST_CASE("BackgroundImporter reports a missing file as a failed result", "[io][background]") {
    trakr::io::BackgroundImporter imp;
    REQUIRE(imp.enqueue({"does_not_exist_12345.wav", 1}));

    trakr::io::BackgroundImporter::Result r;
    REQUIRE(waitForResult(imp, r));
    CHECK_FALSE(r.ok);
    CHECK(r.slot == 1);
}

TEST_CASE("BackgroundImporter stops and joins cleanly", "[io][background]") {
    {
        trakr::io::BackgroundImporter imp;
        REQUIRE(imp.enqueue({"test_bg_missing_a.wav", 0}));
    }  // dtor sets stop_ and joins without deadlock
    SUCCEED();
}

TEST_CASE("SampleLoaderView background import applies and publishes", "[loader][background]") {
    trakr::Song song = trakr::makeDefaultSong();
    trakr::History history;
    trakr::EngineHost engineHost;
    trakr::SampleLoaderView loader;
    int selectedInst = -1;

    // A dedicated temp dir with exactly one file we can select deterministically
    // (".." is entry 0 from the parent-navigation entry; the wav is entry 1).
    const std::filesystem::path tmp = std::filesystem::temp_directory_path() / "trakr_bg_import_test";
    std::filesystem::create_directories(tmp);
    const std::string wavPath = (tmp / "A_sample.wav").string();
    REQUIRE(trakr::renderToWav(song, wavPath, 44100, 16));

    loader.setDirectory(tmp);
    loader.selectFile(1);
    REQUIRE(loader.startBackgroundImport(0));

    // Drain on the main thread until the import lands.
    const auto stop = std::chrono::steady_clock::now() + std::chrono::milliseconds(5000);
    while (std::chrono::steady_clock::now() < stop && selectedInst != 0) {
        loader.pollBackgroundImports(song, history, engineHost, selectedInst);
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    CHECK(selectedInst == 0);
    REQUIRE(song.instruments.size() > 0);
    REQUIRE(song.instruments[0].sample != nullptr);
    REQUIRE_FALSE(song.instruments[0].sample->ch[0].empty());

    std::filesystem::remove_all(tmp);
}
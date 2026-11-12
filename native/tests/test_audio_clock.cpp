#include <catch2/catch_test_macros.hpp>

#include "AudioBackend.h"
#include "EngineHost.h"
#include "Song.h"

#include <chrono>
#include <memory>
#include <thread>

// Headless/remote runs use NullAudioBackend. It must still advance the engine
// clock, otherwise transport-dependent flows (Space -> playhead move) are
// structurally impossible in gui_ctl/functional_smoke runs.

TEST_CASE("NullAudioBackend pumps the engine clock", "[audio][transport]")
{
    trakr::EngineHost host(std::make_unique<trakr::NullAudioBackend>());
    REQUIRE(host.start(44100, 256));
    REQUIRE(host.isRunning());

    trakr::Song song = trakr::makeDefaultSong();
    host.publish(song);
    host.play(0, 0);

    std::this_thread::sleep_for(std::chrono::milliseconds(350));

    auto ph = host.getPlayhead();
    CHECK(ph.playing);
    CHECK(ph.row > 0);

    host.stop();
}

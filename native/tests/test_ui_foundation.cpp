#include <catch2/catch_test_macros.hpp>
#include "Theme.h"
#include "ShortcutMap.h"
#include "RenderScheduler.h"
#include <filesystem>
#include <thread>
#include <chrono>

TEST_CASE("Theme creation and JSON round-trip", "[ui][theme]") {
    trakr::Theme theme = trakr::makeRenoiseDarkTheme();

    REQUIRE(theme.bg0.w == 1.0f);
    REQUIRE(theme.accentAmber.w == 1.0f);
    REQUIRE(theme.accentCyan.w == 1.0f);

    const std::string tmpPath = "test_theme.json";
    bool saved = trakr::saveThemeJson(theme, tmpPath);
    REQUIRE(saved);

    trakr::Theme loadedTheme;
    bool loaded = trakr::loadThemeJson(loadedTheme, tmpPath);
    REQUIRE(loaded);

    // Verify properties match
    REQUIRE(loadedTheme.bg0.x == theme.bg0.x);
    REQUIRE(loadedTheme.bg0.y == theme.bg0.y);
    REQUIRE(loadedTheme.accentAmber.x == theme.accentAmber.x);
    REQUIRE(loadedTheme.cursor.y == theme.cursor.y);

    std::filesystem::remove(tmpPath);
}

TEST_CASE("ShortcutMap defaults and custom chords", "[ui][shortcuts]") {
    trakr::ShortcutMap map;

    REQUIRE(map.getShortcutText(trakr::ActionId::PlayStop) == "Space");
    REQUIRE(map.getShortcutText(trakr::ActionId::Undo) == "Ctrl+Z");
    REQUIRE(map.getShortcutText(trakr::ActionId::ToggleKitGallery) == "F11");
    // F1..F8 belong to the eight soft buttons; F8 must not also open Export.
    REQUIRE(map.getShortcutText(trakr::ActionId::ExportDeck) == "Ctrl+E");

    // Custom rebinding
    map.bind(trakr::ActionId::PlayStop, {ImGuiKey_Enter, true, false, false});
    REQUIRE(map.getShortcutText(trakr::ActionId::PlayStop) == "Ctrl+Enter");

    map.resetToDefaults();
    REQUIRE(map.getShortcutText(trakr::ActionId::PlayStop) == "Space");
}

TEST_CASE("RenderScheduler requestRedraw and playing throttle", "[ui][scheduler]") {
    trakr::RenderScheduler scheduler;

    REQUIRE(scheduler.shouldRedraw() == true);

    scheduler.onFrameRendered();
    REQUIRE(scheduler.shouldRedraw() == false);

    scheduler.requestRedraw();
    REQUIRE(scheduler.shouldRedraw() == true);

    scheduler.onFrameRendered();
    REQUIRE(scheduler.shouldRedraw() == false);

    // When playing, after 18ms it should flag redraw for ~60fps
    scheduler.setPlaying(true);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    REQUIRE(scheduler.shouldRedraw() == true);
}

cmake_minimum_required(VERSION 3.24)
project(trakr VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Compile flags
add_compile_definitions(
    PLATFORM_NATIVE=1
    NATIVE_DESKTOP=1
)

# Pull in third-party dependencies
include(cmake/Deps.cmake)

# 1. trakr_model (pure domain model, zero UI/SDL dependency)
add_library(trakr_model STATIC
    model/Song.cpp
    model/Song.h
    model/Command.cpp
    model/Command.h
    model/History.cpp
    model/History.h
    model/Scale.cpp
    model/Scale.h
    model/FillGenerator.cpp
    model/FillGenerator.h
)
target_include_directories(trakr_model PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/model
)
target_compile_features(trakr_model PUBLIC cxx_std_20)

# 2. trakr_engine (audio engine & sequencer, depends on model)
add_library(trakr_engine STATIC
    engine/Engine.cpp
    engine/Engine.h
    engine/Voice.cpp
    engine/Voice.h
    engine/WavetableVoice.cpp
    engine/WavetableVoice.h
    engine/ChipSynthVoice.cpp
    engine/ChipSynthVoice.h
    engine/dsp/Overdrive.h
    engine/dsp/Bitcrush.h
    engine/StepFx.cpp
    engine/StepFx.h
    engine/Mixer.cpp
    engine/Mixer.h
    engine/EngineHost.cpp
    engine/EngineHost.h
    engine/Slicer.cpp
    engine/Slicer.h
    engine/SampleOps.cpp
    engine/SampleOps.h
)
target_include_directories(trakr_engine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/engine
    ${CMAKE_CURRENT_SOURCE_DIR}/engine/dsp
    ${CMAKE_CURRENT_SOURCE_DIR}/audio
)
target_link_libraries(trakr_engine PUBLIC
    trakr_model
    trakr_audio
    miniaudio
    dr_libs
)
target_compile_features(trakr_engine PUBLIC cxx_std_20)

# 3. trakr_audio (audio backend interfaces and drivers)
add_library(trakr_audio STATIC
    audio/AudioBackend.cpp
    audio/AudioBackend.h
    audio/MiniaudioBackend.cpp
    audio/MiniaudioBackend.h
    audio/MidiManager.cpp
    audio/MidiManager.h
)
target_include_directories(trakr_audio PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/audio
)
target_link_libraries(trakr_audio PUBLIC
    miniaudio
    rtmidi
)
if(APPLE)
    target_link_libraries(trakr_audio PRIVATE "-framework CoreAudio" "-framework AudioToolbox" "-framework CoreFoundation")
endif()
target_compile_features(trakr_audio PUBLIC cxx_std_20)

# 4. trakr_io (file formats, export, JSON serialization)
add_library(trakr_io STATIC
    io/SongJson.cpp
    io/SongJson.h
    io/Render.cpp
    io/Render.h
    io/PolyendImport.cpp
    io/PolyendImport.h
    io/PolyendExport.cpp
    io/PolyendExport.h
)
target_include_directories(trakr_io PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/io
    ${CMAKE_CURRENT_SOURCE_DIR}/engine
)
target_link_libraries(trakr_io PUBLIC
    trakr_model
    trakr_engine
    nlohmann_json
    dr_libs
)
target_compile_features(trakr_io PUBLIC cxx_std_20)

# 5. trakr_app (Desktop Application with SDL3 + SDL_GPU + Dear ImGui)
add_executable(trakr_app
    app/main.cpp
    app/RenderScheduler.cpp
    app/RenderScheduler.h
    app/Theme.cpp
    app/Theme.h
    app/FontLoader.cpp
    app/FontLoader.h
    app/hw/BitmapText.cpp
    app/hw/BitmapText.h
    app/ShortcutMap.cpp
    app/ShortcutMap.h
    app/ScreenManager.cpp
    app/ScreenManager.h
    app/AppLayout.cpp
    app/AppLayout.h
    app/DemoSong.cpp
    app/DemoSong.h
    app/kit/Widgets.cpp
    app/kit/Widgets.h
    app/kit/KitGallery.cpp
    app/kit/KitGallery.h
    app/views/View.h
    app/views/TransportView.cpp
    app/views/TransportView.h
    app/views/PatternView.cpp
    app/views/PatternView.h
    app/views/PatternSelectionHelper.cpp
    app/views/PatternSelectionHelper.h
    app/views/PatternEditHelper.cpp
    app/views/PatternEditHelper.h
    app/views/PatternFxHelper.cpp
    app/views/PatternFxHelper.h
    app/views/InstrumentView.cpp
    app/views/InstrumentView.h
    app/views/InstrumentBars.cpp
    app/views/InstrumentBars.h
    app/views/SampleView.cpp
    app/views/SampleView.h
    app/views/MixerView.cpp
    app/views/MixerView.h
    app/views/PerformView.cpp
    app/views/PerformView.h
    app/views/SampleLoaderView.cpp
    app/views/SampleLoaderView.h
    app/views/ConfigView.cpp
    app/views/ConfigView.h
    app/views/FileView.cpp
    app/views/FileView.h
    app/views/OrderListView.cpp
    app/views/OrderListView.h
    app/views/FillModal.cpp
    app/views/FillModal.h
    app/views/ExportDeck.cpp
    app/views/ExportDeck.h
    app/DockLayout.cpp
    app/DockLayout.h
    app/FileDialogs.cpp
    app/FileDialogs.h
    app/MenuBar.cpp
    app/MenuBar.h
    app/UiRobot.cpp
    app/UiRobot.h
    app/hw/Display.cpp
    app/hw/Display.h
    app/hw/HeaderBar.cpp
    app/hw/HeaderBar.h
    app/hw/ScreenButtons.cpp
    app/hw/ScreenButtons.h
    app/hw/ButtonDeck.cpp
    app/hw/ButtonDeck.h
    app/hw/MenuPopup.cpp
    app/hw/MenuPopup.h
    app/hw/HwWidgets.cpp
    app/hw/HwWidgets.h
    app/hw/TextFit.cpp
    app/hw/TextFit.h
    app/ui/animation.cpp
    app/ui/animation.h
    app/ui/canvas.cpp
    app/ui/canvas.h
    app/ui/control.h
    app/ui/design.h
    app/ui/display.h
    app/ui/layout.h
    app/ui/controls/primitives.cpp
    app/ui/controls/primitives.h
    app/ui/controls/tracker.cpp
    app/ui/controls/tracker.h
    app/ui/controls/song_player.cpp
    app/ui/controls/song_player.h
    app/ui/controls/waveform.cpp
    app/ui/controls/waveform.h
    app/ui/controls/param_value_list.cpp
    app/ui/controls/param_value_list.h
    app/ui/controls/multi_label.cpp
    app/ui/controls/multi_label.h
    app/ui/controls/text_box.cpp
    app/ui/controls/text_box.h
    app/ui/screens/instrument_general.cpp
    app/ui/screens/instrument_general.h
    app/ui/screens/master_page1.cpp
    app/ui/screens/master_page1.h
    app/ui/screens/pattern.cpp
    app/ui/screens/pattern.h
    app/ui/screens/song.cpp
    app/ui/screens/song.h
    app/ui/screens/sample.cpp
    app/ui/screens/sample.h
    app/ui/screens/sample_loader.cpp
    app/ui/screens/sample_loader.h
    app/ui/screens/config.cpp
    app/ui/screens/config.h
    app/ui/screens/perform.cpp
    app/ui/screens/perform.h
    app/ui/screens/file.cpp
    app/ui/screens/file.h
    app/ui/screens/track_mixer.cpp
    app/ui/screens/track_mixer.h
    app/ui/screens/mix_eq.cpp
    app/ui/screens/global_mixer.cpp
    app/ui/screens/mix_eq.h
    app/ui/screens/instrument_pages.cpp
    app/ui/screens/instrument_pages.h
)
target_include_directories(trakr_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/model
    ${CMAKE_CURRENT_SOURCE_DIR}/engine
    ${CMAKE_CURRENT_SOURCE_DIR}/audio
    ${CMAKE_CURRENT_SOURCE_DIR}/io
    ${CMAKE_CURRENT_SOURCE_DIR}/app
    ${CMAKE_CURRENT_SOURCE_DIR}/app/hw
    ${CMAKE_CURRENT_SOURCE_DIR}/app/views
    ${CMAKE_CURRENT_SOURCE_DIR}/app/kit
)
target_link_libraries(trakr_app PRIVATE
    trakr_model
    trakr_engine
    trakr_audio
    trakr_io
    trakr_fw_fonts
    imgui
    SDL3::SDL3
)
target_compile_features(trakr_app PRIVATE cxx_std_20)

option(TRAKR_BUILD_MACOS_BUNDLE "Build macOS .app bundle" OFF)
if(APPLE)
    target_link_libraries(trakr_app PRIVATE "-framework Metal" "-framework QuartzCore")
    if(TRAKR_BUILD_MACOS_BUNDLE)
        set_target_properties(trakr_app PROPERTIES
            MACOSX_BUNDLE TRUE
            MACOSX_BUNDLE_BUNDLE_NAME "Trakr"
            MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
            MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
            MACOSX_BUNDLE_GUI_IDENTIFIER "com.trakr.trakr"
        )
    endif()
endif()

# 6. trakr_fw_fonts: firmware L4 font blobs + RLE decompression (shared)
add_library(trakr_fw_fonts STATIC
    ../core/libs/NewDisplay/CompressionRLE.cpp
    ../core/libs/fonts/RobotoMono-Light_14_L4.cpp
    ../core/libs/fonts/RobotoMono-Light_17_L4.cpp
    ../core/libs/fonts/RobotoMono-Regular_14_L4.cpp
    ../core/libs/fonts/RobotoMono-Bold_17_L4.cpp
    ../core/libs/fonts/RobotoMono-Regular_12_L4.cpp
)
target_include_directories(trakr_fw_fonts PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../core/libs
    ${CMAKE_CURRENT_SOURCE_DIR}/../core/libs/fonts
)

# 7. fwref: source-true firmware reference renderer.
# Compiles the *unmodified* NewDisplay control code and evaluates the
# checked-in spec JSON (native/tools/fwref/specs).
set_source_files_properties(
    ${CMAKE_CURRENT_SOURCE_DIR}/../core/libs/NewDisplay/textBox.cpp
    PROPERTIES COMPILE_OPTIONS
    "-include;${CMAKE_CURRENT_SOURCE_DIR}/tools/fwref/shim/textbox_compat.h")

add_library(fwref_core STATIC
    tools/fwref/audit.cpp
    tools/fwref/spec.cpp
    tools/fwref/spec_parse_extras.cpp
    tools/fwref/spec_paths.cpp
    tools/fwref/scenario.cpp
    tools/fwref/scenario_load.cpp
    tools/fwref/scenario_load_sample.cpp
    tools/fwref/scenario_lists.cpp
    tools/fwref/scenario_sample.cpp
    tools/fwref/selftest.cpp
    tools/fwref/screens/render_spec.cpp
    tools/fwref/screens/spec_controls.cpp
    tools/fwref/screens/spec_controls_sample.cpp
    tools/fwref/screens/spec_controls_song.cpp
    tools/fwref/screens/instrument.cpp
    tools/fwref/screens/master.cpp
    tools/fwref/screens/master2.cpp
    tools/fwref/screens/master3.cpp
    tools/fwref/screens/pattern.cpp
    tools/fwref/screens/config.cpp
    tools/fwref/screens/perform.cpp
    tools/fwref/screens/song.cpp
    tools/fwref/screens/loader.cpp
    tools/fwref/screens/sample.cpp
    tools/fwref/screens/file.cpp
    tools/fwref/shim/display_stub.cpp
    tools/fwref/shim/fonts.cpp
    tools/fwref/shim/platform_stub.cpp
    tools/fwref/shim/ft812_api.cpp
    tools/fwref/shim/ft812_api_geometry.cpp
    tools/fwref/shim/ft812_api_stubs.cpp
    ../core/libs/NewDisplay/bar.cpp
    ../core/libs/NewDisplay/label.cpp
    ../core/libs/NewDisplay/list.cpp
    ../core/libs/NewDisplay/frame.cpp
    ../core/libs/NewDisplay/bgLabel.cpp
    ../core/libs/NewDisplay/displayControls.cpp
    ../core/libs/NewDisplay/tracker.cpp
    ../core/libs/NewDisplay/multiLabel.cpp
    ../core/libs/NewDisplay/paramValueList.cpp
    ../core/libs/NewDisplay/textBox.cpp
    ../core/libs/NewDisplay/gridEditor.cpp
    ../core/libs/NewDisplay/songPlayer.cpp
    ../core/libs/NewDisplay/progressCursor.cpp
    ../core/libs/NewDisplay/lineIndicator.cpp
    ../core/libs/NewDisplay/points.cpp
    ../core/libs/NewDisplay/slice.cpp
    ../core/libs/NewDisplay/spectrum.cpp
    ../core/libs/NewDisplay/wtProgress.cpp
    ../core/libs/NewDisplay/horizontalBar.cpp
    ../core/source/Themes.cpp
)
target_include_directories(fwref_core PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/tools/fwref/include
    ${CMAKE_CURRENT_SOURCE_DIR}/tools/fwref
    ${CMAKE_CURRENT_SOURCE_DIR}/tools/fwref/screens
    ${CMAKE_CURRENT_SOURCE_DIR}/tools/display_emulator
    ${CMAKE_CURRENT_SOURCE_DIR}/../core/libs/NewDisplay
    ${CMAKE_CURRENT_SOURCE_DIR}/../core/libs
    ${CMAKE_CURRENT_SOURCE_DIR}/../core/libs/fonts
    ${CMAKE_CURRENT_SOURCE_DIR}/../core/libs/FT812
    ${CMAKE_CURRENT_SOURCE_DIR}/../core/source
    ${CMAKE_CURRENT_SOURCE_DIR}/../core/config
    ${CMAKE_CURRENT_SOURCE_DIR}/../core/AudioEngine/ConstantValues
    ${CMAKE_CURRENT_SOURCE_DIR}/../core/include
)
target_compile_definitions(fwref_core PRIVATE
    FWREF_SPEC_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tools/fwref/specs"
)
target_compile_options(fwref_core PRIVATE
    -include ${CMAKE_CURRENT_SOURCE_DIR}/tools/fwref/include/display.h
)
target_include_directories(fwref_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/tools/fwref
    ${CMAKE_CURRENT_SOURCE_DIR}/tools/display_emulator
    ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui
)
target_link_libraries(fwref_core PUBLIC trakr_fw_fonts nlohmann_json)

add_executable(fwref_gen
    tools/fwref/gen_main.cpp
    tools/display_emulator/FT812Emulator.cpp
    tools/display_emulator/FT812ScreensCore.cpp
    tools/display_emulator/FT812ScreensExtra.cpp
)
target_link_libraries(fwref_gen PRIVATE fwref_core)

add_executable(fwref_selftest
    tools/fwref/selftest_main.cpp
    tools/display_emulator/FT812Emulator.cpp
    tools/display_emulator/FT812ScreensCore.cpp
    tools/display_emulator/FT812ScreensExtra.cpp
)
target_link_libraries(fwref_selftest PRIVATE fwref_core)

# 8. trakr_tests (Catch2 Unit Tests)
enable_testing()

add_executable(trakr_tests
    tests/test_model.cpp
    tests/test_audio_backend.cpp
    tests/test_engine.cpp
    tests/test_ui_foundation.cpp
    tests/test_history.cpp
    tests/test_dsp.cpp
    tests/test_import.cpp
    tests/test_fuzz.cpp
    tests/test_perf.cpp
    tests/test_ui_e2e.cpp
    tests/test_fill.cpp
    tests/test_slicer.cpp
    tests/test_midi.cpp
    tests/test_ui_robot.cpp
    tests/test_polyend_re_songs.cpp
    tests/test_phase6_fidelity.cpp
    tests/test_demo_levels.cpp
    tests/test_hw_framework.cpp
    tests/test_deck_tool_control.cpp
    tests/test_dpad_parity.cpp
    tests/test_display_emulator.cpp
    tests/test_fwref.cpp
    tests/test_audio_clock.cpp
    tests/test_text_fit.cpp
    tests/test_ui_core.cpp
    tests/test_canvas_lines.cpp
    tests/test_control_library.cpp
    tests/test_animation.cpp
    tests/test_config_editor.cpp
    tests/test_fwref_audit.cpp
    tools/display_emulator/FT812Emulator.cpp
    tools/display_emulator/FT812ScreensCore.cpp
    tools/display_emulator/FT812ScreensExtra.cpp
    tests/test_cursor_parity.cpp
    tests/test_pattern_parity.cpp
    tests/test_instrument_parity.cpp
    tests/test_step_fx.cpp
    tests/test_pages_parity.cpp
    tests/test_sample_ops.cpp
    tests/test_sample_loader.cpp
    tests/test_parity_e2e_tier1_a.cpp
    tests/test_parity_e2e_tier1_b.cpp
    tests/test_parity_e2e_tier2.cpp
    tests/test_parity_e2e_tier3.cpp
    app/views/PatternView.cpp
    app/views/PatternEditHelper.cpp
    app/views/PatternSelectionHelper.cpp
    app/views/PatternFxHelper.cpp
    app/views/TransportView.cpp
    app/views/InstrumentView.cpp
    app/views/InstrumentBars.cpp
    app/views/SampleView.cpp
    app/views/MixerView.cpp
    app/views/PerformView.cpp
    app/views/SampleLoaderView.cpp
    app/views/ConfigView.cpp
    app/views/FileView.cpp
    app/views/OrderListView.cpp
    app/views/FillModal.cpp
    app/DockLayout.cpp
    app/UiRobot.cpp
    app/DemoSong.cpp
    app/kit/Widgets.cpp
    app/Theme.cpp
    app/FontLoader.cpp
    app/ShortcutMap.cpp
    app/RenderScheduler.cpp
    app/ScreenManager.cpp
    app/AppLayout.cpp
    app/hw/Display.cpp
    app/hw/HeaderBar.cpp
    app/hw/ScreenButtons.cpp
    app/hw/ButtonDeck.cpp
    app/hw/MenuPopup.cpp
    app/hw/HwWidgets.cpp
    app/hw/TextFit.cpp
    app/hw/BitmapText.cpp
    app/ui/animation.cpp
    app/ui/canvas.cpp
    app/ui/controls/primitives.cpp
    app/ui/controls/tracker.cpp
    app/ui/controls/song_player.cpp
    app/ui/controls/waveform.cpp
    app/ui/controls/param_value_list.cpp
    app/ui/controls/multi_label.cpp
    app/ui/controls/text_box.cpp
    app/ui/screens/instrument_general.cpp
    app/ui/screens/master_page1.cpp
    app/ui/screens/pattern.cpp
    app/ui/screens/song.cpp
    app/ui/screens/sample.cpp
    app/ui/screens/sample_loader.cpp
    app/ui/screens/config.cpp
    app/ui/screens/perform.cpp
    app/ui/screens/file.cpp
    app/ui/screens/track_mixer.cpp
    app/ui/screens/mix_eq.cpp
    app/ui/screens/global_mixer.cpp
    app/ui/screens/instrument_pages.cpp
)
target_include_directories(trakr_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/model
    ${CMAKE_CURRENT_SOURCE_DIR}/engine
    ${CMAKE_CURRENT_SOURCE_DIR}/engine/dsp
    ${CMAKE_CURRENT_SOURCE_DIR}/audio
    ${CMAKE_CURRENT_SOURCE_DIR}/io
    ${CMAKE_CURRENT_SOURCE_DIR}/app
    ${CMAKE_CURRENT_SOURCE_DIR}/app/hw
    ${CMAKE_CURRENT_SOURCE_DIR}/tools/display_emulator
    ${CMAKE_CURRENT_SOURCE_DIR}/../core/libs
    ${CMAKE_CURRENT_SOURCE_DIR}/../core/libs/fonts
)
target_link_libraries(trakr_tests PRIVATE
    trakr_model
    trakr_engine
    trakr_audio
    trakr_io
    imgui
    SDL3::SDL3
    Catch2::Catch2WithMain
    fwref_core
)
target_compile_features(trakr_tests PRIVATE cxx_std_20)

add_test(NAME model_tests COMMAND trakr_tests)


# fwref gates: oracle self-test + source-true reference verification
add_test(NAME fwref_oracle COMMAND fwref_selftest)
find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_Interpreter_FOUND)
    add_test(NAME fwref_reference
             COMMAND ${Python3_EXECUTABLE}
                     ${CMAKE_CURRENT_SOURCE_DIR}/tools/fwref/check_reference.py
                     --bin $<TARGET_FILE:fwref_gen>)
endif()

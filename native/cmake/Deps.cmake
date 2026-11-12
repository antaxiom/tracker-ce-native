# native/cmake/Deps.cmake
# Dependency setup for Trakr

# 1. SDL3 (System / Homebrew)
find_package(SDL3 QUIET)
if(NOT SDL3_FOUND)
    find_path(SDL3_INCLUDE_DIR SDL3/SDL.h PATHS /opt/homebrew/include /usr/local/include)
    find_library(SDL3_LIBRARY NAMES SDL3 PATHS /opt/homebrew/lib /usr/local/lib)
    if(SDL3_INCLUDE_DIR AND SDL3_LIBRARY)
        message(STATUS "Found SDL3 via paths: ${SDL3_INCLUDE_DIR}, ${SDL3_LIBRARY}")
        add_library(SDL3::SDL3 SHARED IMPORTED)
        set_target_properties(SDL3::SDL3 PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${SDL3_INCLUDE_DIR}"
            IMPORTED_LOCATION "${SDL3_LIBRARY}"
        )
        set(SDL3_FOUND TRUE)
    else()
        message(FATAL_ERROR "SDL3 not found! Please check /opt/homebrew/include/SDL3")
    endif()
endif()

# 2. Dear ImGui (docking branch) + SDL3 & SDL_GPU backends
set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/imgui")
if(EXISTS "${IMGUI_DIR}/imgui.h")
    add_library(imgui STATIC
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_demo.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
        ${IMGUI_DIR}/backends/imgui_impl_sdlgpu3.cpp
    )
    target_include_directories(imgui PUBLIC
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends
    )
    target_link_libraries(imgui PUBLIC SDL3::SDL3)
    target_compile_features(imgui PUBLIC cxx_std_20)
else()
    message(FATAL_ERROR "Dear ImGui not found in ${IMGUI_DIR}")
endif()

# 3. miniaudio (header-only)
add_library(miniaudio INTERFACE)
target_include_directories(miniaudio INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/external/miniaudio")

# 4. dr_libs (dr_wav, dr_flac, dr_mp3)
add_library(dr_libs INTERFACE)
target_include_directories(dr_libs INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/external/dr_libs")

# 5. nlohmann/json (header-only)
add_library(nlohmann_json INTERFACE)
target_include_directories(nlohmann_json INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/external")

# 6. Catch2 v3 (local from util/3rdparty/catch2)
set(CATCH2_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../util/3rdparty")
if(EXISTS "${CATCH2_ROOT}/catch2/catch_all.hpp")
    file(GLOB_RECURSE CATCH2_ALL_SRCS "${CATCH2_ROOT}/catch2/*.cpp")
    list(REMOVE_ITEM CATCH2_ALL_SRCS "${CATCH2_ROOT}/catch2/internal/catch_main.cpp")
    
    add_library(Catch2 STATIC ${CATCH2_ALL_SRCS})
    target_include_directories(Catch2 PUBLIC 
        "${CATCH2_ROOT}"
        "${CATCH2_ROOT}/catch2"
    )
    target_compile_features(Catch2 PUBLIC cxx_std_20)
    add_library(Catch2::Catch2 ALIAS Catch2)

    add_library(Catch2WithMain STATIC "${CATCH2_ROOT}/catch2/internal/catch_main.cpp")
    target_link_libraries(Catch2WithMain PUBLIC Catch2)
    target_compile_features(Catch2WithMain PUBLIC cxx_std_20)
    add_library(Catch2::Catch2WithMain ALIAS Catch2WithMain)
else()
    message(FATAL_ERROR "Catch2 not found in ${CATCH2_ROOT}/catch2")
endif()

# 7. RtMidi (MIDI input and output)
set(RTMIDI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/rtmidi")
if(EXISTS "${RTMIDI_DIR}/RtMidi.h")
    add_library(rtmidi STATIC ${RTMIDI_DIR}/RtMidi.cpp)
    target_include_directories(rtmidi PUBLIC ${RTMIDI_DIR})
    if(APPLE)
        target_compile_definitions(rtmidi PUBLIC __MACOSX_CORE__)
        target_link_libraries(rtmidi PRIVATE "-framework CoreMIDI" "-framework CoreAudio" "-framework CoreFoundation")
    elseif(UNIX AND NOT APPLE)
        target_compile_definitions(rtmidi PUBLIC __LINUX_ALSA__)
        target_link_libraries(rtmidi PRIVATE asound pthread)
    elseif(WIN32)
        target_compile_definitions(rtmidi PUBLIC __WINDOWS_MM__)
        target_link_libraries(rtmidi PRIVATE winmm)
    endif()
    target_compile_features(rtmidi PUBLIC cxx_std_20)
endif()

#ifndef FWREF_H_
#define FWREF_H_

#include <array>
#include <string>
#include <utility>
#include <vector>

namespace trakr::emulator
{
class FT812DisplayEmulator;
}

namespace fwref
{

struct InstrumentScenario
{
    std::string name = "909 Kick";
    int volume = 50;
    int panning = 50; // firmware convention: 0..100, 50 = centre
    int tune = 0;
    int fineTune = 0;
    int filterEnable = 1;
    std::string filterType = "lowPass"; // disabled|lowPass|highPass|bandPass
    float cutOff = 1.0f;
    float resonance = 0.0f;
    int selectedPlace = 0;
    std::string rightLabel = "1. 909 Kick";
};

struct MasterScenario
{
    int volume = 81; // MASTER_VOLUME_DEFAULT
    int saturationType = 0;
    int bassBoost = 0;
    int stereoBoost = 0; // 0..8
    int selectedPlace = 0;

    // Track Mixer 2/3 (mtStructs.h:920) and Global Mixer 3/3 (mtStructs.h:921-926).
    std::array<std::string, 8> trackNames = { "Track 1", "Track 2", "Track 3", "Track 4",
                                              "Track 5", "Track 6", "Track 7", "Track 8" };
    std::array<int, 8> trackVolume = { 100, 100, 100, 100, 100, 100, 100, 100 };
    std::array<bool, 8> trackMute = { false, false, false, false, false, false, false, false };
    std::array<int, 8> trackLevel = { 0, 0, 0, 0, 0, 0, 0, 0 };
    int delayVolume = 100;
    int reverbVolume = 100;
    int dryMixVolume = 100;
    int lineInVolume = 0; // mtStructs.h:899-900 (muted by default)
    bool delayMute = false;
    bool reverbMute = false;
    bool dryMixMute = false;
    bool lineInMute = true;
    int delayLevel = 0;
    int reverbLevel = 0;
    int dryMixLevel = 0;
    int lineInLevel = 0;
};

struct PatternScenario
{
    // Empty/representative pattern page. Values mirror the firmware defaults:
    // mtStructs.h:865 (patternEditStep), mtConfig.cpp:557 (patternInfo),
    // patternEditor.h:195 (editMode) and fPatternEdit.cpp:155 (trackerPattern.init).
    int actualPattern = 1;
    std::string rightLabel = "1. 909 Kick"; // titleBar right label (current instrument)
    int patternLength = 64;
    int actualStep = 0; // firmware cursor is 0-based; row numbers are 1-based
    int actualTrack = 0;
    int firstVisibleTrack = 0;
    int stepDevider = 4; // mtProject.values.patternDiv + 1 (mtStructs.h:935)
    int selectState = 1;
    int selectColor = 1; // 0 red, 1 green, 2 unfocused (fPatternEdit.cpp:4991-4996)
    int selectedParam = 0;
    int viewMode = 0; // patternEditor.h:342 (all columns)
    bool showPlayhead = false; // stopped transport
    int playheadPosition = 0;
    int selectedPlace = 0;
    int editStep = 1; // mtProject.values.patternEditStep
    int timerMinutes = 0;
    int timerSeconds = 0;
    // mtStructs.cpp:3-16 setDefaultTrackName()
    std::array<std::string, 16> trackNames = {
        "Track 1", "Track 2", "Track 3", "Track 4", "Track 5", "Track 6", "Track 7", "Track 8",
        "Midi 9",  "Midi 10", "Midi 11", "Midi 12", "Midi 13", "Midi 14", "Midi 15", "Midi 16",
    };
};

struct ConfigScenario
{
    int selectedBase = 0;
    int selectedItem = 0;
    int selectedPlace = 0;
    int gridSelected = 0;
    std::vector<std::string> baseMenu;
    std::vector<std::pair<std::string, std::string>> items;
    std::string credits;
    std::string description;
};

struct PerformScenario
{
    // gPerformance.cpp initDisplayControls(): 12 fx slots, each a cLabel
    // (vertical name) and a cMultiLabel (5 text lines).
    std::array<std::string, 12> fxNames = { "Tune", "Delay send", "Reverb send",
                                            "Low-pass cutoff", "High-Pass cutoff",
                                            "Sample end", "Panning LFO Speed",
                                            "Pattern play mode", "Volume LFO Speed",
                                            "Step repeater", "Sample playback", "Bit depth" };
    // [slot][row]; row 4 is the cMultiLabel top cell (current CC value).
    std::array<std::array<std::string, 5>, 12> fxValues = { {
      { "---", "+2", "-2", "-3", "---" },
      { "---", "+94", "+60", "-100", "---" },
      { "---", "+98", "+50", "-100", "---" },
      { "---", "-22", "-51", "-62", "---" },
      { "---", "-25", "-44", "-61", "---" },
      { "---", "-60", "-82", "-95", "---" },
      { "---", "+20", "-12", "+8", "---" },
      { "Fwd", "Back", "Rnd", "C19", "---" },
      { "---", "+23", "+20", "+18", "---" },
      { "Off", "1/2", "1/4", "1/6", "---" },
      { ">>>", "<<<", "<<<", "<<<", "---" },
      { "Off", "1", "2", "3", "---" },
    } };
    std::array<int, 12> selectedRows = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
    std::array<std::string, 8> trackNames = { "Track 1", "Track 2", "Track 3", "Track 4",
                                              "Track 5", "Track 6", "Track 7", "Track 8" };
    std::array<bool, 8> activeTracks = { true, true, true, false, false, false, false, false };
    std::array<bool, 8> trackMuted = { false, false, false, false, false, false, false, false };
    std::array<std::string, 8> trackPatterns = { "Patt: 1", "Patt: 1", "Patt: 1", "",
                                                 "", "", "", "" };
    int editPlace = -1; // hideEditFrame()
};

struct SongScenario
{
    // gSongEdit.cpp showDefaultScreen() / readSong(): playlist slots, track
    // strip and list on the right. Track counts mirror kSampleTracks (8) and
    // kMidiTracks (8); the bitmask bytes are indexed by playlist pattern - 1.
    int selectedSlot = 0; // patternsList.start / cSongPlayer list position
    int linesCount = 12;  // refreshPatternsList(): patternsList.linesCount
    std::vector<int> playlist = { 1 };
    std::vector<std::string> patternNames; // empty -> no name text
    int normalTracksCount = 8;
    int midiTracksCount = 8;
    std::array<std::string, 16> trackNames = {
        "Track 1", "Track 2", "Track 3", "Track 4", "Track 5", "Track 6", "Track 7", "Track 8",
        "Midi 9",  "Midi 10", "Midi 11", "Midi 12", "Midi 13", "Midi 14", "Midi 15", "Midi 16",
    };
    std::array<bool, 8> normalMuted = { false, false, false, false, false, false, false, false };
    std::array<bool, 8> midiMuted = { false, false, false, false, false, false, false, false };
    std::array<int, 32> normalPatternMask = {}; // allPatternsBitmask[p - 1]
    std::array<int, 32> midiPatternMask = {};   // allPatternsBitmaskMidi[p - 1]
    int isListActive = 1;
    int editMode = 0;
    int selectionIsActive = 0;
    int selectionStartSongPosition = 0;
    int selectionPatternLength = 1;
    int selectionStartTrack = 0;
    int selectionTrackLength = 1;
    int progressIsPlaying = 0;
    int progressTrackPreview = 255; // 255 disables preview-track mode
    int progressCurrentSongPosition = 0;
    int progressPatternLength = 64;
    int progressPositionInPattern = 0;
    float globalTempo = 130.0f; // mtStructs.h DEFAULT_TEMPO
    bool hexNumbers = false;    // mtConfig.general.numberingMode
};

struct LoaderScenario
{
    // gSampleImport.cpp showDefaultScreen(): SD-card file browser on the left,
    // 48-slot instrument pool in the middle, memory + preview volume bars right.
    int selectedFile = 1;  // a .wav entry (not a '/'-prefixed folder)
    int selectedSlot = 0;
    int selectedPlace = 0; // frame place (0 = file browser)
    int memoryUsage = 12;       // calculateMemUsage(): used_memory / max_memory * 100
    int memoryUsageAdd = 14;    // memoryUsageAdd.value (used + selected file)
    int previewVolume = 86;     // PREVIEW_VOLUME_DEFAULT (mtStructs.h:260)
    int slotCount = 48;         // kSampleInstrumentsCount
    std::vector<std::string> files;
    std::vector<std::string> slots; // "1.<name>" / "2." (listInstrumentSlots)
};

struct SampleScenario
{
    // gSamplePlay.cpp showDefaultScreen(): a mono wave file in Forward Loop.
    int playMode = 1;       // playModeForwardLoop (samplePlayback.h)
    int selectedPlace = 0;
    int progress = 180;     // playProgressInSpectrum -> cProgressCursor::value
    int startPoint = 40;    // cPoints display coordinates (0..599)
    int endPoint = 580;
    int loop1 = 150;
    int loop2 = 450;
    int selected = 0;       // points.selected bitmask
    int zerocrossed = 0;    // points.zerocrossed bitmask
    float startTime = 0.089f; // formatTimeSeconds() values (seconds)
    float endTime = 1.312f;
    float loop1Time = 0.336f;
    float loop2Time = 1.008f;
    float zoom = 1.0f;      // zoomTextValue
    int wavePoints = 300;   // spectrum[0].pointsLimit == k300
    int waveAmplitude = 100;
    float waveFrequency = 0.11f;
    float waveDecay = 0.004f;
    std::vector<int> waveform; // generated deterministic upper-data buckets
    int regionStart = 460;     // cLineIndicator selection (granular overlay)
    int regionEnd = 500;
    int sliceCount = 0;
    int sliceSelected = 0;
    std::vector<int> slices;
    int wtPosition = 0;
    int wtWindow = 32;
    int wtPositionMax = 128;
    std::string rightLabel = "1. 909 Kick";
    std::vector<std::string> playModes = { "1-Shot",        "Forward Loop", "Backward Loop",
                                           "Pingpong Loop", "Slice",        "Beat Slice",
                                           "Wavetable",     "Granular" };
};

struct FileScenario
{
    // gProjEdit.cpp showProjectBrowser(): fileListControl (projects, active)
    // + recentListControl (empty) + "SD Card"/"Recent" tab labels +
    // blankSpaceCover pane. No cover image in the browser state.
    int selectedPlace = 0; // 0 = file list (framed), 1 = recent list
    std::string projectName = "New Project";
    std::vector<std::string> projects;
    std::vector<std::string> recent;
    int selectedProject = 0;
};

struct Scenario
{
    InstrumentScenario instrument;
    MasterScenario master;
    PatternScenario pattern;
    ConfigScenario config;
    PerformScenario perform;
    SongScenario song;
    LoaderScenario loader;
    SampleScenario sample;
    FileScenario file;
};

bool loadScenarioJson(const std::string& path, Scenario& out, std::string& error);

// The scenario used by the app-vs-firmware comparison harness
// (native/tools/fwref/scenario.json).
Scenario defaultComparisonScenario();

// Renders the Instrument General page (firmware "Instrument Parameters 1/2")
// using the unmodified NewDisplay control sources.
bool renderInstrumentGeneral(trakr::emulator::FT812DisplayEmulator& emu,
                             const InstrumentScenario& scenario);

// Renders Master page 1/3.
bool renderMasterPage1(trakr::emulator::FT812DisplayEmulator& emu,
                       const MasterScenario& scenario);

// Renders Master pages 2/3 (Track Mixer) and 3/3 (Global Mixer).
bool renderMasterPage2(trakr::emulator::FT812DisplayEmulator& emu,
                       const MasterScenario& scenario);
bool renderMasterPage3(trakr::emulator::FT812DisplayEmulator& emu,
                       const MasterScenario& scenario);

// Renders the Config editor default screen (cList/cParamValueList/cTextBox/
// cGridEditorPopup created from gConfigEdit.cpp).
bool renderConfig(trakr::emulator::FT812DisplayEmulator& emu, const ConfigScenario& scenario);

// Renders the Performance FX screen (cMultiLabel + vertical cLabel text).
bool renderPerform(trakr::emulator::FT812DisplayEmulator& emu, const PerformScenario& scenario);

// Renders the Song editor default screen (cSongPlayer slots, track strip,
// block matrix and progress line, plus the tempo bar and action labels).
bool renderSong(trakr::emulator::FT812DisplayEmulator& emu, const SongScenario& scenario);

// Renders the Pattern editor page (cTracker grid + action buttons + title bar).
bool renderPattern(trakr::emulator::FT812DisplayEmulator& emu, const PatternScenario& scenario);

// Renders the Sample Loader default screen (cList file browser + 48-slot pool,
// cBar memory/preview gauges, header labels and action buttons).
bool renderLoader(trakr::emulator::FT812DisplayEmulator& emu, const LoaderScenario& scenario);

// Renders the Sample Playback default screen (cSpectrum waveform, cPoints
// start/end/loop markers, cProgressCursor, play-mode cList and action labels).
bool renderSample(trakr::emulator::FT812DisplayEmulator& emu, const SampleScenario& scenario);

// Renders the File project browser (gProjEdit.cpp showProjectBrowser(): project
// list + empty recent list + "SD Card"/"Recent" tabs + blankSpaceCover pane).
bool renderFile(trakr::emulator::FT812DisplayEmulator& emu, const FileScenario& scenario);

// Phase-1 oracle gate: creates a firmware-property cBar and asserts the
// resulting pixels. Returns false and fills `error` on failure.
bool runOracleSelfTest(std::string& error);

} // namespace fwref

#endif // FWREF_H_

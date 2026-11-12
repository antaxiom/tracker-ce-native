# Trakr (Native Desktop Tracker)

**Trakr** (placeholder working name) is an enhanced C++20 desktop port and rebuild of the **Polyend Tracker+ firmware** (the `tracker-ce` open-source codebase). It is a faithful 1:1 structural port of the firmware's screen editors and audio engine — pixel fidelity is verified against source-true references rendered by compiling the unmodified firmware `NewDisplay` controls through an FT812 shim (`native/tools/fwref`) — extended with desktop capabilities: lossless JSON persistence, WAV/stem export, up to 64 tracks, live MIDI input, and native file dialogs, built on SDL3 + SDL_GPU + Dear ImGui.

---

## 1. Architecture Overview

Trakr is engineered as a clean, modular set of standalone C++20 libraries with strict decoupling between the audio engine, data model, file I/O, and UI chrome. Each screen view is a structural port of the corresponding firmware screen editor (see `core/interface/*/g*.cpp`), so firmware control decomposition, naming, geometry, and colors map 1:1 onto the app:

```
┌─────────────────────────────────────────────────────────────┐
│                 trakr_app (Dear ImGui UI)                  │
│  Transport | Pattern Grid | Instruments | Sample | Mixer    │
└──────────────┬──────────────────────────────┬───────────────┘
               │ EngineHost (Lock-free SPSC)  │ Command History
               ▼                              ▼
┌─────────────────────────────┐  ┌────────────────────────────┐
│        trakr_engine         │  │        trakr_model         │
│  SampleVoice (Catmull-Rom)  │  │ Song, TrackInfo, Pattern,  │
│  DSP: ADSR, SVF, LFO        │  │ Cell (Multi-FX),           │
│  Mixer: Delay & Reverb Send │  │ Instrument, SampleData     │
└──────────────┬──────────────┘  └────────────▲───────────────┘
               │                              │
               ▼                              │
┌─────────────────────────────┐  ┌────────────┴───────────────┐
│        trakr_audio          │  │          trakr_io          │
│ MiniaudioBackend (44.1 kHz) │  │ SongJson (Lossless JSON)   │
│ NullAudioBackend (CI/Tests) │  │ PolyendImport (.pti, .mtp) │
│                             │  │ Render (Offline WAV export)│
└─────────────────────────────┘  └────────────────────────────┘
```

- **`trakr_model`**: Pure data structures (`Song`, `TrackInfo`, `Pattern`, `Cell`, `Instrument`, `SampleData`). Zero dependencies on audio hardware, graphics, or windowing.
- **`trakr_engine`**: High-performance real-time audio engine:
  - Sequencer with row-by-row tick processing, step FX evaluation (including Chance/Probability, Roll/Retrigger, Glide, Reverse, Note Cut, and Microtuning), and sample-accurate voice triggering.
  - `SampleVoice` with Catmull-Rom cubic spline interpolation, forward/backward/ping-pong looping, reverse playback, retriggering, note cut countdowns, pitch glides, and anti-click release envelopes.
  - Per-sample DSP: amplitude ADSR envelopes, stereo State-Variable Filters (SVF: Lowpass, Highpass, Bandpass), and multi-waveform LFOs (modulating Pitch, Volume, Pan, or Filter Cutoff).
  - Multi-track `Mixer` with per-track gain, pan, mute, solo, peak meters, stereo send buses for master Delay and Reverb, and soft-knee saturation limiting.
  - Lock-free `EngineHost` coordinating the UI thread and high-priority audio callback via an atomic single-producer single-consumer (SPSC) ring buffer.
- **`trakr_audio`**: Abstraction over hardware audio output (`AudioBackend`). Includes `MiniaudioBackend` (low-latency audio via miniaudio), `NullAudioBackend` (silent backend for automated tests and headless CI), and `MidiManager` (low-latency MIDI controller input via `RtMidi`).
- **`trakr_io`**: Project persistence and file formats:
  - `SongJson`: Lossless JSON serialization and deserialization of the full tracker project.
  - `PolyendImport`: Binary parsers for Polyend Tracker `.pti` instruments (with 16-byte header and dynamic sample offset decoding), `.mtp` patterns (8 tracks, notes, volumes, FX), and `project.mt` playlist sequences.
  - `PolyendExport`: Binary serializes for hardware `.pti` instruments, `.mtp` pattern tracks, and full `project.mt` directory trees for physical Polyend Tracker SD cards.
  - `Render`: Offline master WAV and multi-track stem rendering (16-bit and 32-bit float PCM) via `dr_wav`.
- **`trakr_app`**: Cross-platform desktop interface built on SDL3 + SDL_GPU + Dear ImGui Docking:
  - Hardware-accelerated graphics using native backends: Metal on macOS, Vulkan on Linux, Direct3D 12 on Windows.
  - Firmware-faithful screen structure: each view mirrors its `core/interface/*/g*.cpp` editor (screen mode bar, track headers, dual FX columns, and stopped-mode direct step editing), with firmware device-pixel geometry mapped once via a local `px()/py()`.
  - Theme anchored to the firmware `Themes.cpp` palette (`#000000`, mint note `#00E8BE`, instrument `#FEF749`, green volume `#30CB3F`, violet FX1 `#BB58F1`, sky cyan FX2 `#57F1FF`, red record cursor `#CD1818`), plus an accessible high-contrast variant.
  - High-performance, responsive `RenderScheduler`: burst rendering on user interaction, up to 60 FPS during audio playback, 0% CPU consumption when idle.

---

## 2. Building

### Prerequisites

- **CMake**: >= 3.25
- **C++ Compiler**: Apple Clang 15+, GCC 12+, or MSVC 2022+ with C++20 support
- **SDL3**: Installed system-wide (e.g. `brew install sdl3` on macOS or package manager on Linux)
- **FreeType**: (Optional, system or Homebrew)

### Build Commands

```bash
# Configure the build directory
cmake -B native/build -S native -DCMAKE_BUILD_TYPE=Release

# Build the test suite
cmake --build native/build --target trakr_tests

# Build the desktop application
cmake --build native/build --target trakr_app
```

On macOS, to generate an application bundle (`Trakr.app`), pass `-DTRAKR_BUILD_MACOS_BUNDLE=ON`:
```bash
cmake -B native/build -S native -DTRAKR_BUILD_MACOS_BUNDLE=ON
cmake --build native/build --target trakr_app
```

---

## 3. How to Test Everything

### A. Run Automated Unit Tests & Benchmarks

Run the complete Catch2 test suite (113 test cases):

```bash
./native/build/trakr_tests
```

Or using `ctest`:
```bash
ctest --test-dir native/build --output-on-failure
```

#### What the 35 Test Suites Cover:
1. **`test_model`**: Song invariants, pattern dimensions, cell allocation, default project factory.
2. **`test_engine`**: Sequencer tick timing, row advance, voice triggering, cubic interpolation, click-free release envelopes, Chance/Probability step gating, and mixer summing.
3. **`test_audio_backend`**: NullAudioBackend and MiniaudioBackend initialization, start, and stop cycles.
4. **`test_midi`**: RtMidi integration, virtual/hardware port enumeration, ring-buffer event queuing and timestamping.
5. **`test_fill`**: Polyend Fill generator, Euclidean rhythm generation ($E(K, N)$ Bjorklund algorithm), random-in-scale note selection, linear ramps, arpeggios, and 13 musical scales.
6. **`test_slicer`**: Grid 4/8/16 sample slicing, transient peak detection, slice bounds clamping.
7. **`test_ui_foundation`**: Theme palette JSON serialization, Cousine/Roboto font loading, and widget layout boundaries.
8. **`test_history`**: Command pattern undo/redo stack (`SetCellCommand`, `SetTrackCommand`, `InsertRowsCommand`, `DeleteRowsCommand`), pattern resizing, multi-level history navigation.
9. **`test_dsp`**:
   - `Adsr`: Attack, Decay, Sustain, and Release envelope states and sample curves.
   - `Svf`: Lowpass, Highpass, and Bandpass frequency filtering.
   - `Lfo`: Sine, Triangle, Sawtooth, and Square waveforms, rate/depth scaling.
   - `Delay`: Circular buffer delay lines and feedback saturation.
   - `Reverb`: Schroeder-Moorer comb and all-pass filter matrix.
10. **`test_import`**:
   - WAV and FLAC decoding via `dr_wav`.
   - Polyend `.pti` instrument parsing and export (round-trip verification with sample payload, loop markers, fine tuning).
   - Polyend `.mtp` pattern file parsing and export across 8 tracks.
   - Full Polyend project directory structure import/export (`project.mt`).
   - Lossless `SongJson` serialization and deserialization round-trip.
11. **`test_fuzz`**:
   - Audio engine fuzz testing across 200 blocks (25,600 samples) with randomized notes, instruments, volumes, and step effects. Verifies all outputs are strictly finite (`std::isfinite`), free of NaNs/Infs, and bounded by the master limiter.
   - History engine fuzz testing across 150 randomized interleaved edits, undos, and redos.
12. **`test_perf`**:
   - **Engine Real-Time Load**: Benchmarks 32 active concurrent voices with cubic interpolation and mixer summing running at 128-frame buffers (~0.14 ms per buffer, ~4.9% of one CPU core).
   - **Pattern Grid Viewport Culling**: Benchmarks drawing a 64-track × 512-row tracker grid with viewport culling (< 0.002 ms per frame).
13. **`test_ui_e2e` (Automated End-to-End Note Entry & Editing)**:
    - Piano note entry (`Z..M` lower octave, `Q..U` upper octave) with sample-accurate preview.
    - Automatic step advance by variable `editStep`.
    - Hex parameter entry on Instrument, Volume, and FX columns.
    - Intelligent auto-jump to note column on piano keypress from non-note sub-columns.
    - Note-Off (`A` / `1`) and cell clear (`.` / `Delete` / `Backspace`).
    - Polyend stopped-mode invariant: steps are directly editable without arming record when stopped.
    - Full undo/redo rollback and rollforward integration.
    - Semitone and octave transposition (`Ctrl+Up/Down`, `Ctrl+Shift+Up/Down`).
    - Clipboard operations (`Ctrl+C`, `Ctrl+X`, `Ctrl+V`).

---

### B. Headless Smoke Test & Automated UI Robot

To verify the graphics subsystem, ImGui views, and audio engine integration without an interactive display:

```bash
# Quick headless smoke test
./native/build/trakr_app --smoke-test

# Full Autonomous UI Robot (exercises all buttons, hovers 2,800+ controls, audits ID conflicts)
./native/build/trakr_app --auto-test
```
*Expected output*:
```text
[UI ROBOT] Autonomous UI testing and conflict audit started...
[UI ROBOT] Completed 2929 simulated frames and 2855 widget checks across 6 views.
[UI ROBOT AUDIT PASSED] 0 ID conflicts, 0 stack errors, 0 numerical errors.
```

---

### C. Running the Desktop Application

Launch the desktop tracker:

```bash
./native/build/trakr_app
```

#### Key Views & How to Use Them:

- **Transport Bar & Polyend Mode Navigation**:
  - **Screen Mode Switcher**: Quick-toggle tabs `[Pattern]`, `[Instrument]`, `[Sample]`, `[Mixer]`, and `[Order]` matching the firmware screen mode buttons.
  - `Play / Stop`: Click or press `Space` to toggle real-time playback.
  - `BPM / LPB`: Drag or double-click to set tempo and lines-per-beat.
  - `Octave`: Choose current octave (0–8) using `/` and `*` or `[` and `]`.
  - `Edit Step`: Controls cursor advance step after entering notes (0–16).
  - `Master Meter`: Displays real-time audio output levels (`OUT L/R`) with peak decay.

- **Pattern Editor Grid**:
  - **Polyend Stopped-Mode Invariant**: Steps are directly editable immediately upon stopping playback without needing to arm record mode.
  - **Track Headers**: Top bar displays `STEP` and `TRACK 01` through `TRACK 08`.
  - **Dual Effect Columns**: Each track provides sub-columns for Note, Instrument, Volume, `FX1` (type + value), and `FX2` (type + value).
  - **Expanded Step Effects**:
    - `R` (Roll / Retrigger): Multi-trigger sample hits within a single step row (2–16 retriggers).
    - `P` (Chance / Probability): Sets probability of note execution (0%–100%).
    - `G` (Glide / Portamento): Smooth pitch slide towards target note over $N$ ticks.
    - `C` (Note Cut): Cuts playing note after $N$ ticks.
    - `V` (Reverse): Reverses sample playback direction on trigger.
    - `M` (Microtuning): Detunes note by fractional semitones (-100 to +100 cents).
  - **Fill Generator & Musical Scales (`Ctrl+F`)**:
    - Opens the Polyend Fill modal popover.
    - **Euclidean Rhythms**: Generates distributed rhythms via the Bjorklund algorithm with configurable hits ($K$) and rotation.
    - **Random in Scale**: Generates random melodies constrained to 13 scales (Major, Minor, Pentatonic, Blues, Dorian, Phrygian, Lydian, Mixolydian, Harmonic Minor, Melodic Minor, Whole Tone, Diminished).
    - **Linear Ramp & Arpeggios**: Creates ascending/descending pitch or volume sweeps and broken chords.
    - Complete undo/redo support via `SetTrackCommand`.
  - **Live MIDI Controller & Keyboard Input**:
    - Connect any USB/hardware MIDI controller or keyboard via `View > MIDI Input`.
    - Live note preview with velocity sensitivity and real-time step recording into the pattern cursor.
  - **Piano Keyboard Entry**:
    - Lower octave: `Z` (C) .. `M` (B)
    - Upper octave: `Q` (C+1) .. `U` (B+1)
    - Note-Off: `A` or `1`
    - Clear cell: `.` or `Delete` / `Backspace`
    - Smart auto-jump: Typing a piano key while positioned in an instrument or volume sub-column automatically returns the cursor to the Note column.
  - **Hex Parameter Entry**:
    - Instrument column: type 2 hex digits (`00`–`FF`)
    - Volume column: type 2 hex digits (`00`–`7F`)
    - Effect columns (`FX1` / `FX2`): type 2 hex digits for the parameter value
  - **Mouse & Navigation**:
    - Click any sub-column or track to jump directly to it.
    - Scroll mouse wheel to scroll smoothly through pattern rows.
    - `Ctrl + Up/Down`: Transpose selected note by semitone.
    - `Ctrl + Shift + Up/Down`: Transpose selected note by octave.
  - **Clipboard & History**:
    - `Ctrl/Cmd + C`: Copy cell
    - `Ctrl/Cmd + X`: Cut cell
    - `Ctrl/Cmd + V`: Paste cell
    - `Ctrl/Cmd + Z`: Undo
    - `Ctrl/Cmd + Shift + Z` (or `Ctrl/Cmd + Y`): Redo

- **Sample & Synth Editor**:
  - Interactive waveform with draggable `Start`, `Loop Start`, `Loop End`, and `End` markers.
  - **Sample Slicing Toolbar**:
    - `Grid 4 / 8 / 16`: Automatically splits sample into equal rhythmic slice intervals.
    - `Detect Transients`: Scans audio energy peaks to detect drum hit transients.
    - `Clear`: Clears slice points back to single sample playback.
    - Slices render as cyan vertical markers on the interactive waveform display.
  - `Normalize` and `Reverse` one-click audio processing buttons.
  - `Loop Mode`: Toggle between `Off`, `Forward`, `Backward`, and `PingPong`.
  - **Amplitude Envelope (ADSR)**: Interactive sliders for Attack, Decay, Sustain, and Release.
  - **SVF Filter**: Toggle between `Off`, `LP`, `HP`, and `BP` filters; set Cutoff Hz (20–20,000 Hz) and Resonance (Q).
  - **LFO Modulation**: Route LFO to `Pitch`, `Volume`, `Pan`, or `Cutoff` with `Sine`, `Triangle`, `Saw`, or `Square` waveforms.

- **Mixer View**:
  - 8-track channel strips with `Mute`, `Solo`, `Pan` knob, and volume faders.
  - Real-time stereo peak meters per channel.
  - Per-track **Delay Send (`Dly`)** and **Reverb Send (`Rev`)** knobs routing into master effects buses.
  - Master strip with master pan, master gain, and stereo limiter meters.

- **Instrument Slots & Order List**:
  - Manage instrument slots (`+ New`, `Dup`, `Del`, `Audition`). Adding or duplicating slots automatically focuses the newly created instrument.
  - Sequence pattern playback in the Order List with live playhead position indicators. Adding or duplicating song positions immediately selects the new position.

- **Theme Selection**:
  - Access `View > Theme` to switch between:
    - **Tracker Classic** (the firmware `Themes.cpp` palette, matching the Tracker+ display)
    - **High Contrast / Accessible** (high visibility palette for dark/bright rooms)
  - Manage instrument slots (`+ New`, `Dup`, `Del`, `Audition`).
  - Sequence pattern playback in the Order List.

- **File Operations & Hardware Export**:
  - `File > New Song` (`Ctrl/Cmd + N`)
  - `File > Save Song` (`Ctrl/Cmd + S`) — saves lossless `.trakr` JSON project via native file dialog.
  - `File > Open Song...` (`Ctrl/Cmd + O`) — loads project via native file dialog.
  - `File > Export WAV (Mix)...` — renders master stereo audio mix to 16/32-bit WAV.
  - `File > Export WAV Stems...` — renders separate WAV audio files for every track (`Track_01.wav` .. `Track_08.wav`) into a destination folder.
  - `File > Export Polyend Project (.mt)...` — exports complete hardware project ready for Polyend Tracker SD card:
    - `project.mt` playlist file
    - `patterns/` folder with `.mtp` 8-track binary patterns
    - `instruments/` folder with 16-bit PCM `.pti` instruments with embedded loop points and tuning.
  - **Drag-and-Drop**: Drag any `.pti` instrument, `.mtp` pattern, or `.wav`/`.flac` audio file directly into the window.


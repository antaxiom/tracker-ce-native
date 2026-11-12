# Trakr Native Development Notes

> **Trakr** (placeholder working name) is an enhanced C++20 desktop port/rebuild of
> the **Polyend Tracker+ firmware** (`tracker-ce`). These notes are the chronological
> development history of that port. Some earlier phases describe the pre-port
> Renoise-style UI and hand-drawn references that have since been superseded by the
> firmware-faithful port (`native/app/views/*`) and source-true `fwref` references.

## Phase 0-3: Core Engine, Model & Headless Architecture
- [x] CMake build system (`trakr_model`, `trakr_engine`, `trakr_audio`, `trakr_io`, `trakr_app`, `trakr_tests`).
- [x] Miniaudio & Null backends, sample-accurate sequencer, Catmull-Rom cubic `SampleVoice`, equal-power mixer.
- [x] StepFxRegistry (Volume, Panning, Position, Microtuning, Tune), lock-free `EngineHost`, offline WAV render.
- [x] Custom flat zero-rounding widget kit, Cousine/Roboto fonts, RenderScheduler on-demand 60Hz loop.
- [x] 100-step command undo/redo history, state machine, and project serialization.

## Phase 4: Views & Interactive Tracker Engine (Checkpoint 3 / MVP)
- [x] Task 1.2 / Commands & History System:
  - `Command` base interface (`model/Command.h`, `model/Command.cpp`) with undo/redo/merge contracts.
  - Concrete commands: `SetCellCommand`, `SetCellRangeCommand`, `InsertRowsCommand`, `DeleteRowsCommand`, `SetPatternLengthCommand`, `SetOrderCommand`, `SetTrackInfoCommand`, `SetInstrumentCommand`, `SetSongParamCommand`.
  - `History` (`model/History.h`, `model/History.cpp`): 100-step undo/redo stack with continuous cell edit merging (< 500 ms).
  - Verified with property tests in `test_history.cpp` (50 random sequential edits; undo restored exact original, redo restored exact modified).
- [x] Task 4.6 / 4.8: DSP Engine Modules (`engine/dsp/`):
  - `Adsr.h`: Linear/exponential envelope generator with Attack, Decay, Sustain, Release states.
  - `Svf.h`: State Variable Filter with Lowpass, Highpass, Bandpass, Notch modes and resonance control.
  - `Lfo.h`: Multi-waveform LFO (Sine, Triangle, Saw, Square) with target routing (Pitch, Volume, Pan, Cutoff).
  - `Delay.h`: Tempo-synced stereo delay buffer with feedback, damping lowpass, and soft saturation.
  - `Reverb.h`: Algorithmic stereo Schroeder-Moorer reverb with 8 prime comb filters and 4 all-pass diffusers, strictly blow-up proof.
  - Verified with Catch2 unit tests in `test_dsp.cpp` (including 44,100-sample max feedback test).
- [x] Task 4.10: Polyend & Audio Import (`io/PolyendImport.h`, `io/PolyendImport.cpp`):
  - `loadAudioFile`: WAV/FLAC audio loading via `dr_wav` into normalized stereo `SampleData`.
  - `loadPtiInstrument`: Polyend `.pti` file parser (392-byte header + signed 16-bit PCM payload, loop modes, start/end/loop points).
  - `loadMtpPattern`: Polyend `.mtp` pattern file parser (8 tracks, notes, instruments, volumes, FX).
  - `loadPolyendProject`: Polyend `.mt` directory project scanner.
  - Verified in `test_import.cpp`.
- [x] Task 4.1: Transport Bar (`app/views/TransportView.cpp`):
  - Live Play / Stop through `EngineHost`.
  - Record mode and pattern loop toggles.
  - BPM and LPB value boxes connected to `SetSongParamCommand` and published to audio engine.
  - Octave (0..8) and Edit Step (0..16) selectors.
  - Live master peak meter connected to `engineHost.getMasterMeters()`.
- [x] Task 4.2: Pattern Editor Grid (`app/views/PatternView.cpp`):
  - Static grid drawing with beat (every 4 rows) and bar (every 16 rows) background shading.
  - Per-column colors for Note (cyan), Instrument (amber), Volume (green), FX (magenta/yellow).
  - Tracker-standard two-row piano keyboard layout:
    Lower octave: `Z` (C) .. `M` (B); Upper octave: `Q` (C+1) .. `U` (B+1).
  - Live audition preview through `engineHost.previewNoteOn`.
  - Edit-mode note entry with auto-advance by `editStep` and `SetCellCommand`.
  - Note off (`OFF`) via `A` or `1`. Clear cell via `.` / `Delete` / `Backspace`.
  - Octave and semitone transposition via `Ctrl+Up`/`Down`.
  - Live playhead indicator line following `engineHost.getPlayhead()`.
- [x] Task 4.3: Order List (`app/views/OrderListView.cpp`):
  - Dynamic sequence slots connected to `song.order`.
  - Add, duplicate, delete slots with `SetOrderCommand`.
  - Click-to-jump playback and active pattern selection.
- [x] Task 4.4: Instrument List (`app/views/InstrumentView.cpp`):
  - Slot list with Add, Duplicate, Delete, and Audition preview buttons.
- [x] Task 4.5: Sample View (`app/views/SampleView.cpp`):
  - Interactive waveform display with draggable Start, Loop Start, Loop End, and End markers.
  - Normalization, Reverse, Gain dB, Root note, Finetune, and Loop mode controls.
- [x] Task 4.7: Mixer View (`app/views/MixerView.cpp`):
  - 8-track channel strips with Mute, Solo, Pan, Gain dB, and real-time audio peak meters.
  - Master channel strip with master meters.
- [x] Task 4.9: File Menu & Shortcuts:
  - New, Save, Open (`.trakr` JSON), Export WAV (`renderToWav`).
  - Undo / Redo shortcuts (`Ctrl+Z`, `Ctrl+Y`).
  - Keyboard Shortcuts help modal.
- [x] Automated Verification:
  - `ctest` 100% pass: all tests across `test_model`, `test_engine`, `test_audio_backend`, `test_ui_foundation`, `test_history`, `test_dsp`, `test_import`.
  - Smoke test with 2560x1600 screenshot capture saved to `native/docs/phase4_views.png`.

## Phase 5: Polish & Robustness
- [x] Task 5.1: Property & Fuzz Testing (`tests/test_fuzz.cpp`):
  - Audio engine fuzzed with randomized songs (notes 0..119, instruments 0..7, volumes 0..64, step FX 0..25, random values) across 200 blocks (25,600 frames). All output samples verified strictly finite (`std::isfinite`), 0 NaNs, 0 Infs, bounded by master limiter.
  - History engine fuzzed with 150 random interleaved operations (`SetCell`, `InsertRows`, `DeleteRows`, `undo`, `redo`). Pattern dimensions and track invariants fully preserved.
- [x] Task 5.2: Performance Benchmarking (`tests/test_perf.cpp`):
  - Engine benchmark: 32 active concurrent voices with cubic interpolation and mixer summing running at 128-frame buffers.
    - Result: **0.14 ms per buffer (~4.9% of one CPU core)**. Target was < 10% (0.29 ms).
  - Pattern view culling benchmark: 64 tracks × 512 rows pattern grid viewport culling.
    - Result: **0.0016 ms per frame**. Target was < 2.0 ms.
- [x] Task 5.3: Accessibility & UI Scalability:
  - Added high-contrast colorblind-safe theme (`makeColorblindTheme()`) in `app/Theme.h` and `app/Theme.cpp`.
  - Added dynamic UI scaling (100%, 125%, 150%, 200%) via `io.FontGlobalScale` in `app/main.cpp`.
  - Added theme and UI scale switchers in the View menu.
- [x] Task 5.4: macOS Packaging & Multi-Platform CI:
  - CMake macOS bundle support (`MACOSX_BUNDLE`, `com.trakr.trakr`, `Trakr.app`) with `-DTRAKR_BUILD_MACOS_BUNDLE=ON`.
  - Added packaging, code-signing, and notarization documentation in `docs/packaging.md`.
  - Created multi-platform GitHub Actions CI workflow in `.github/workflows/native-ci.yml` supporting macOS (Clang), Ubuntu Linux (GCC), and Windows (MSVC).

## Post-Phase 5: Code Review & Polyend RE Alignment
- [x] Task 6.1: Thread-Safety & Real-Time SPSC Atomics:
  - Converted `Engine::eventReadIndex_` to `std::atomic<size_t>` with `memory_order_acquire`/`release`.
  - Changed `Engine::isPlaying()` to return `publishedPlaying_.load(std::memory_order_acquire)`.
- [x] Task 6.2: Polyend Binary Import Invariants (`io/PolyendImport.cpp`):
  - Fixed `.pti` header to dynamic 16-byte format with dynamic `instrumentSize` and CRC offset.
  - Added dual header detection for `.mtp` patterns (28-byte firmware header & 24-byte mock header).
  - Corrected 0-indexed instrument numbering (0 = Instrument 1).
  - Implemented `loadProjectMt()` for `project.mt` playlist arrays into `Song::orderList`.
  - Added drag-and-drop file/folder import in `app/main.cpp`.
- [x] Task 6.3: Lossless JSON Project Round-Trip (`io/SongJson.cpp`):
  - Complete serialization/deserialization for Song, TrackInfo, Pattern, Cell, and Instrument.
  - Full round-trip unit test coverage in `tests/test_import.cpp`.
- [x] Task 6.4: DSP Wiring & Sound Engine:
  - Connected `dsp::Adsr`, stereo `dsp::Svf`, and `dsp::Lfo` per-sample in `SampleVoice`.
  - Added master `dsp::Delay` and `dsp::Reverb` return buses fed by track sends in `Mixer`.
- [x] Task 6.5: StepFX Firmware Enum Alignment (`engine/StepFx.h`, `engine/StepFx.cpp`):
  - Aligned FX IDs with Polyend Tracker CE `enFxType` (`Roll = 3`, `Volume = 18`, `Glide = 19`, `Position = 22`, `Reverse = 26`, `Panning = 31`, `Microtuning = 34`, `Tune = 40`, `NoteCut = 43`).
- [x] Task 6.6: Interactive View Ergonomics & DSP Controls:
  - Pattern Editor: 2-digit hex entry (`0-9`, `A-F`), mouse scrolling/clicking, clipboard (`Ctrl+C`/`X`/`V`), Shift+Arrow transposition.
  - Sample View: ADSR sliders (Attack, Decay, Sustain, Release), SVF Filter toggles and Cutoff/Resonance sliders, LFO Destination/Wave/Rate/Depth controls.
  - Mixer View: Per-track Delay and Reverb send knobs (`Dly`, `Rev`) alongside Gain and Pan.
- [x] Task 6.7: UI Robustness, Polyend Workflow Alignment & Automated E2E Testing:
  - Resolved Dear ImGui ID collision warning by namespacing toggle buttons in `SampleView.cpp` with `PushID`/`PopID`.
  - Clarified Master Output meters in `TransportView.cpp` with dedicated `OUT` header and L/R tooltips.
  - Implemented Polyend Stopped-Mode editing invariant (steps directly editable when stopped without needing record arm).
  - Added intelligent auto-jump to Note column on piano keypress from non-note sub-columns.
  - Added Polyend Hardware Screen Mode Bar in `TransportView.cpp` (`[Pattern] [Instrument] [Sample] [Mixer] [Order]`).
  - Added authentic Polyend Hardware (OLED Dark) theme based on firmware color palette.
  - Created automated end-to-end UI test suite in `tests/test_ui_e2e.cpp` covering note entry, hex editing, clipboard, transposition, and history.
- [x] Automated Verification:
  - 100% pass across all 28 Catch2 test cases (534,139 assertions).
  - Smoke test verified with headless standby support.
  - All files strictly under 400 lines.

## Phase 6: Firmware Fidelity, Synthesis & Provenance (Completed)
- [x] Task 6.8: Polyend Play Modes (Wavetable & Granular Synthesis):
  - Created `engine/WavetableVoice.h` and `engine/WavetableVoice.cpp`.
  - Implemented windowed wavetable scanning (window sizes 32, 64, 128, 256, 512, 1024, 2048 samples).
  - Implemented granular synthesis with half-sine grain envelope smoothing.
- [x] Task 6.9: Open-Source `CHIP` Synthesizer Engine:
  - Created `engine/ChipSynthVoice.h` and `engine/ChipSynthVoice.cpp`.
  - Implemented all 8 Polyend CE chiptune models: `Pulse2A03` (NES pulse with PWM & polyBLEP), `SidSawTri` (C64 SID saw/tri blend), `NoiseLfsr` (LFSR noise), `FmBell` (2-op OPL FM), `SuperSquare` (3-osc detuned square), `ZapLaser` (exponential downward pitch drop), `ArpChord` (hardware-clocked arpeggios), and `ChipDrum` (percussion transient).
- [x] Task 6.10: Full Step-FX Firmware Alignment:
  - Added `FilterLowpass`, `FilterHighpass`, `FilterBandpass`, `SlideUp`, `SlideDown`, `RandomNote`, and `RandomVolume` to `engine/StepFx.cpp`.
- [x] Task 6.11: Overdrive & Bitcrush Processors:
  - Created `engine/dsp/Overdrive.h` (hyperbolic tangent soft-clipping) and `engine/dsp/Bitcrush.h` (bit depth reduction & sample decimation).
- [x] Task 6.12: Dynamic Engine Voice Dispatch:
  - Updated `engine/Engine.cpp` to dynamically route notes to `SampleVoice`, `WavetableVoice`, or `ChipSynthVoice` based on instrument configuration.
- [x] Task 6.13: License Compliance & Attribution:
  - Created `native/CREDITS.md` documenting upstream references to `polyend/tracker-ce` (MIT), `sandlbn/polyend-tracker-re` (Apache 2.0), `polyend/tracker-lib` (MIT), ImGui, SDL3, miniaudio, dr_libs, and Catch2.
- [x] Task 6.14: Comprehensive Verification:
  - Created `tests/test_phase6_fidelity.cpp` testing wavetable, granular, 8 chip models, step FX, and overdrive/bitcrush.
  - All 46 test cases (537,165 assertions) passing with 100% success.
  - All files strictly under 400 lines.

## Phase 0 (Parity Roadmap): Bugs, Theme & Ground Truth Alignment (Completed)
- [x] Gain Staging & Drone Elimination (`app/DemoSong.cpp`):
  - Fixed Pattern 1 loudness blowout by properly gain-staging tracks: Kick (-3.0 dB), Snare (-3.5 dB), HiHat (-5.0 dB), Bass (-3.5 dB), Lead (-5.0 dB), Chip (-5.5 dB), Arp (-5.5 dB), Noise (-6.0 dB), Master (-3.5 dB).
  - Replaced infinite sustain with punchy, snappy ADSR envelopes on synth bass and lead so tones decay naturally.
  - Added clean note-offs at pattern boundaries to prevent drones from bleeding across pattern loops or song order changes.
  - Created `tests/test_demo_levels.cpp` rendering all patterns to verify peak <= -3.0 dBFS and healthy RMS.
- [x] Dedicated Preview Voice Lane & Note-Off Lifecycle (`engine/Engine.*`, `engine/EngineHost.*`):
  - Added isolated `previewVoice_` in `Engine` so auditions never hijack song track voices (e.g. track 0 kick).
  - Plumbed key-release note-offs in `PatternView.cpp`, mouse-release note-offs in `InstrumentView.cpp`, space-release note-offs in `SampleView.cpp`, and MIDI note-offs in `main.cpp`.
  - Added `Engine::allNotesOff()` to cleanly silence voices on screen switch, Escape, or stop.
- [x] Eliminated User-Facing Brand / "Hardware" Mentions (Rule #6):
  - Cleaned user-facing labels in `MenuBar.cpp`, `ExportDeck.cpp`, `FillModal.cpp`, `SoftKeys.cpp`.
  - Theme named "Tracker Classic" instead of "Polyend Hardware".
- [x] Multi-Colored Empty Dash Matrix & Cursor Hues (`Theme.*`, `PatternView.cpp`):
  - Added exact hex values from firmware `Themes.cpp` (`patternTrackerColors`): Note dim (`#003933`), Instr dim (`#453B1B`), Vol dim (`#103F26`), FX1 dim (`#321F3A`), FX2 dim (`#183744`).
  - Active cursor displays green in play/navigate mode (`#30CB3F`) and red in record mode (`#CD1818`), matching hardware reference.
- [x] Instrument Highlight & Song View ID Conflicts:
  - Replaced text tags ("▶ PLAYING", "● ACTIVELY SOUNDING ON BEAT") in `InstrumentView.cpp` with visual row highlighting.
  - Fixed ImGui table row ID scoping in `OrderListView.cpp` to prevent duplicate ID warnings on repeated patterns.
- [x] Verification:
  - 49 test cases with 537,247 assertions passing 100%.
  - Autonomous UI robot passes with 0 ID conflicts, 0 stack errors.
  - All authored files strictly < 400 lines.

## Phase 1 (Parity Roadmap): Display Framework & Contextual Deck (Completed)
- [x] Display Canvas (`app/hw/Display.h`, `app/hw/Display.cpp`):
  - Logical 800x480 screen canvas with integer/aspect scaling and letterboxing/pillarboxing centered in window.
  - Support for Desktop Wide Mode (`--wide`, `F10`, or View menu toggle), expanding track columns horizontally across viewport while preserving header and 8-button screen strip.
  - Exposes `screenRect`, `headerRect`, `workspaceRect`, `buttonsRect`, `deckRect` and coordinate transformation helpers (`screenToCanvas`, `canvasToScreen`).
- [x] Firmware-Parity Header Bar (`app/hw/HeaderBar.h`, `app/hw/HeaderBar.cpp`):
  - Parity with firmware `core/interface/core/titleBar.cpp` and `songTimer.cpp`.
  - Left: Screen Title e.g. "Pattern", "Instrument Parameters [1/2]", "Song", "Master Track Mixer", "Sample Playback", "Performance Mode", with red "REC" badge when armed.
  - Center: Play time `0m:00s`, transport state glyph ("■" stopped, "▶" play, "↻" loop), and pattern/step readout `P%02d.%03d`.
  - Right: Current instrument name (`%d. %s` e.g. "1. 909 Kick") or project name.
  - 1px separator line (`#575757`) at y=28 matching firmware `barLabelData.line`.
  - Master peak audio clipping alert line.
- [x] Contextual Screen Buttons (`app/hw/ScreenButtons.h`, `app/hw/ScreenButtons.cpp`):
  - Parity with firmware `gPatternEdit.cpp` (lines 95-135) and manual lines 716-724.
  - 8 buttons spanning across bottom of screen (y=424..480).
  - 2-line rendering: dim label on top, bright value on bottom.
  - Colored underline indicators: Green (`#30CB3F`) for available actions, Red (`#CD1818`) for edit/destructive actions, Orange (`#D67022`) for tracks, Dimmed for disabled.
  - Shift support: when Shift is held, button labels switch to secondary functions.
  - Clickable via mouse and keyboard shortcuts `F1..F8` (and `Shift+F1..F8`).
- [x] On-Screen Hardware Button & Pad Deck (`app/hw/ButtonDeck.h`, `app/hw/ButtonDeck.cpp`):
  - Physical layout parity: 4x12 silicone pad grid ("Keyboard") positioned across left/center directly under the screen buttons; Rotary Jog Wheel positioned prominently on the right.
  - Sub-column jump keys with firmware colors: Note (teal `#00E8BE`), Instr (yellow `#FEF749`), FX1 (violet `#BB58F1`), FX2 (cyan `#57F1FF`).
  - Transport keys: Play (illuminated green when running), Rec (illuminated red when armed).
  - Editing actions wired:
    - Insert: inserts row shifting cells down; Shift+Insert jumps Home (row 0).
    - Delete: clears cell/component at cursor; Shift+Delete deletes row shifting cells up.
    - Copy & Paste: Copy copies active cell or selection; Shift+Copy (Paste) pastes clipboard at cursor.
    - Menu: opens shortcuts/context options.
  - Modifiers: Shift (illuminated yellow when active), numeric Octave buttons 1, 2, 3, 4 with active yellow `#FEF749` illumination for the current octave (Shift + 1..4 selects octaves 5..8).
  - Rotary Jog Wheel widget: prominent metallic dial on right with tactile notch and knurled perimeter; supports both mouse wheel and smooth mouse dragging (scrolling rows in navigate mode; editing values in record/shift mode).
  - 4x12 Silicone Pad Matrix (48 pads): note names (C3..B6), natural vs accidental styling, cyan root markers, real instrument note audition via preview voice (`selectedInstIdx`), note recording into active pattern cell with `editStep` advance when armed in Record mode, and clean note-on/note-off lifecycle on mouse down/up.
  - Toggleable via View menu ("Hardware Deck") or shortcut `F9`.
- [x] Automated Testing via Tool Control & Machine Vision:
  - Integrated into autonomous robot (`trakr_app --auto-test`) and Catch2 unit test suite (`tests/test_deck_tool_control.cpp`).
  - Machine vision geometric checks: asserts pad grid is located on the left (`padArea.Max.x < jogCenter.x - radius`), jog wheel is in right half, all button bounding boxes are non-overlapping with valid non-zero geometry.
  - Tool control simulated interaction: validates mouse wheel and angular drag on Jog Wheel, octave switching and illumination on buttons 1..4, Shift modifier state, Play/Stop transport, Insert row shift, Delete cell clear, Copy/Paste duplication, and pad note audition/release.
- [x] Verification:
  - 57 Catch2 test cases (537,324 assertions) passing 100%.
  - UI robot (`trakr_app --auto-test`) passing with 0 ID conflicts, 0 stack errors, 0 numerical errors.
  - All authored files strictly under 400 lines (top files: `Widgets.cpp` 398, `main.cpp` 397, `PatternView.cpp` 388).
  - Visual verification with screenshot captured (`screen_pattern_phase1_deck_fixed.png`).

## Phase 2: Pattern Page Parity & Revision
- [x] Sub-Column Cursor with Solid Fill & Contrasting Text:
  - Outer cell outline bounding box in `colCursorBox` (`#30CB3F` in Green mode, `#CD1818` in Record mode).
  - Solid filled rectangle highlighting active sub-column (Note, Instr, Vol, FX1, FX2).
  - High-contrast text: black text on bright green in Green mode, white text on red in Record mode.
- [x] Double Row Number Gutters (Left & Right):
  - Step numbers displayed on both left and right edges (`STEP 01..64`), 1-based decimal matching Tracker hardware video.
- [x] Yellow Wrap Line:
  - Yellow boundary line (`#FEF749`, `Themes.cpp`) at row 0 separating pattern start/wrap.
- [x] Multi-Page Screen Buttons & Shift-Mute Mode:
  - Button 8 ("More ...") cycles through Page 1 (Sequencing), Page 2 (Transforms: Render, Invert, Duplicate, Expand, Shrink, Copy, Paste), and Page 3 (Names).
  - Holding Shift reveals Track 1..8 Mute/Unmute buttons.
- [x] Floating Centered Fill Modal:
  - Floating centered modal overlay (`540x175px`) that adapts to active selection without shifting panels or docking.
- [x] Automated Testing & Revision:
  - 59 Catch2 test cases (537,353 assertions) passing 100% (`tests/test_pattern_parity.cpp`).
  - Autonomous robot (`trakr_app --auto-test`) passing across 954 frames with 0 errors.
  - Visual inspection against reference video frame (`screen_pattern_revised.png`).

## Phase 3: Instrument Parameters Parity
- [x] Full-Screen 8-Column Vertical Bar Layout (`InstrumentView.h`, `InstrumentView.cpp`, `InstrumentBars.h`, `InstrumentBars.cpp`):
  - Parity with manual Section 6.3 - 6.5 (lines 5732-6250) and firmware `core/interface/instrumentEditor/gInstrEdit.cpp` & `fIinstrEdit.cpp`.
  - Replaces old Renoise-style side-docked panels with full 800x480 workspace 8-column layout.
  - Red bounding selection box (`#CD1818`) around the active parameter column.
  - Page 1/2 General: Volume, Panning, Tune, Finetune, Filter Type, Cutoff, Resonance, More.
  - Instrument Effects Page: Overdrive, Bit depth, Reverb send, Delay send, Back.
  - Page 2/2 Automation: Destination list, Type (Off/Envelope/LFO), Shape, Speed, Amount bar.
- [x] Shift & Visual Cursor Commands Parity:
  - Unified keyboard Shift and on-screen button deck Shift toggle.
  - D-pad Up/Down/Left/Right and Jog wheel with Shift held expands rectangular pattern selection, identical to physical keyboard `[Shift] + Arrow` behavior.
- [x] Verification:
  - 61 Catch2 test cases (537,386 assertions) passing 100% (`tests/test_instrument_parity.cpp`).
  - Autonomous robot (`trakr_app --auto-test`) passing across 941 frames with 0 errors.

## Phase 4: Step FX Parity
- [x] 1:1 Firmware Enum IDs & Descriptors (`StepFx.h`, `StepFx.cpp`, `PatternFxHelper.h`, `PatternFxHelper.cpp`):
  - Mapped directly to firmware `core/source/mtSequencer.h` (`enFxType`), `core/interface/core/interfaceDefs.h` (`fxNames` and `fxIDs`), and `interfaceFxDesc.h`.
  - Removed outdated conflicting legacy alias registrations.
  - Added new dedicated handlers:
    - `OffFx` (`!`, ID 1): Resets voice FX state (glide, retrigger, cuts).
    - `OverdriveFx` (`D`, ID 38): Sets voice overdrive saturation (0..100%).
    - `BitDepthFx` (`E`, ID 39): Sets voice bit depth reduction (4..16 bits).
    - `GateLengthFx` (`q`, ID 20): Truncates note gate duration to percentage of step frames.
    - `SampleSliceFx` (`S`, ID 25): Sets active slice index.
    - `ChanceFx` (`C`, ID 4): Evaluates play probability condition.
    - Existing handlers: `Volume` (`V`), `Panning` (`P`), `Microtuning` (`M`), `Glide` (`G`), `Roll` (`R`), `FilterLowpass` (`L`), `FilterBandpass` (`B`), `FilterHighpass` (`H`), `SlideUp` (`F`), `SlideDown` (`J`), `RandomNote` (`n`), `RandomVolume` (`v`), `Position` (`p`), `Reverse` (`r`), `Tune` (`U`).
  - Implemented exact case-sensitive lookup in `fxCharToType` (e.g. `'T'` = Tempo, `'t'` = Reverb Send; `'V'` = Volume, `'v'` = Random Volume; `'P'` = Panning, `'p'` = Position; `'R'` = Roll, `'r'` = Reverse).
- [x] Verification:
  - 65 Catch2 test cases (537,456 assertions) passing 100% (`tests/test_step_fx.cpp`).
  - Autonomous UI robot (`trakr_app --auto-test`) passing across 941 frames with 0 errors.
  - All authored files strictly under 400 lines.
  - Screenshot verified: `screen_pattern_phase4.png`.

## Tracker Mini Hardware & Interface Parity
- [x] Authentic 4-Column Track Architecture (`PatternView.cpp`, `PatternEditHelper.cpp`, `PatternSelectionHelper.cpp`, `ButtonDeck.cpp`):
  - Completely eliminated Renoise-style dedicated Volume column from pattern tracks.
  - Aligned strictly with Tracker / Tracker Mini hardware (4 subcolumns: `Note`, `Instr`, `FX1`, `FX2`).
  - Mapped button deck keys 1:1:
    - `Note` (Cyan `#00E8BE`) -> Subcolumn 0
    - `Instr` (Yellow `#FEF749`) -> Subcolumn 1
    - `FX1` (Purple `#BB58F1`) -> Subcolumn 2
    - `FX2` (Sky Cyan `#57F1FF`) -> Subcolumn 3
- [x] Color Scheme & Dash Parity (`Themes.cpp` lines 118-126 `patternTrackerColors`):
  - Populated values:
    - Note: Mint Cyan (`0x00E8BE`)
    - Instrument: Decimal 2-digit Yellow (`0xFEF749`), e.g. `05`, `17`
    - FX1: Violet (`0xBB58F1`), e.g. `RV 1`, `V 50`
    - FX2: Sky Cyan (`0x57F1FF`), e.g. `C 50`, `D 20`
  - Empty dashes per column:
    - Note: Dark teal `--` (`0x003933`)
    - Instrument: Dark yellow `--` (`0x453B1B`)
    - FX1: Dark violet `----` (`0x321F3A`)
    - FX2: Dark cyan `----` (`0x183744`)
  - Cursor selection box:
    - Play/navigate mode: Green rounded box (`0x30CB3F` / `one_true_green`)
    - Record mode: Red rounded box (`0xCD1818` / `one_true_red`)
- [x] Shift & Visual Cursor Hold-to-Repeat Harmonization (`ButtonDeck.cpp`, `PatternView.cpp`):
  - Implemented continuous hold-to-repeat for visual D-pad buttons (`DpadUp`, `DpadDown`, `DpadLeft`, `DpadRight`) with initial delay (0.28s) and smooth repeat rate (0.06s), matching keyboard arrow keys.
  - Preserved 2D multi-track and multi-row selection ranges when using D-pad or Jog wheel with Shift held.
  - Synchronized visual Shift latching and physical keyboard Shift modifier across pattern mouse clicks, keyboard shortcuts, and button deck actions.
- [x] Verification:
  - 68 Catch2 test cases (537,539 assertions) passing 100%.
  - Autonomous robot (`trakr_app --auto-test`) passing across 996 frames and 858 widget checks with 0 errors.
  - All authored files strictly `< 400` lines.
  - Screenshots verified: `screen_pattern_4col.png`, `screen_perform.png`.

## Phase 5: Sample Playback & Remaining Pages Parity
- [x] Authentic Sample Playback Page (`SampleView.h`, `SampleView.cpp`):
  - Manual Section 6.6-6.11 (lines 6322-6450) and firmware `samplePlayback/gSamplePlayback.cpp`.
  - Replaced legacy Renoise-style docked panels and slider rows with authentic full-screen layout.
  - Full-width waveform display using firmware `waveformColors` (`grey_08` `#080808` canvas, `#FFFFFF` wave peaks, `#30CB3F` start marker `S`, `#CD1818` end marker `E`, `#3FFFFF` loop markers `LS`/`LE`, `#FEF749` slice markers).
  - Right-side Play Mode list (`1-Shot`, `Forward loop`, `Backward loop`, `Pingpong loop`, `Slice`, `Beat Slice`, `Wavetable`, `Granular`) with active red bounding box (`#CD1818`).
  - Contextual 8 screen buttons (`Preview`, `Start`, `End`, `Loop Start / Slices`, `Loop End / Detect`, `Zoom`, `Play Mode`, `More`).
  - Active parameter adjustment via Up/Down, Jog wheel, or mouse drag, with fine precision when Shift is held.
- [x] Verification:
  - Screenshot verified: `screen_sample_phase5.png`.
  - All authored files strictly `< 400` lines.
  - All 68 Catch2 test cases passing (537,539 assertions).

## Phase 6: Input Workflow Parity, Module Menu Popup & Sample Editor UI
- [x] Note Auditioning vs Recording Parity:
  - Fixed `canEdit` in `PatternSelectionHelper.cpp` and `main.cpp` to strictly require `recordMode == true` before committing notes to pattern cells.
  - In Play mode (green cursor box), playing piano keys or MIDI notes auditions live samples without overwriting step data.
  - In Rec mode (red cursor box), playing keys step-records notes and instruments into the pattern, stepping forward by `editStep`.
- [x] Selected Instrument Isolation & Header Interaction:
  - Eliminated automatic track fallback (`selectedInstIdx = cursorTrack_`) in `syncInstrumentFromCursor`, ensuring instrument selection persists across row and track navigation.
  - Added interactive click and mouse-wheel scrolling directly on the Header bar instrument label (`"%d. %s"`), with hover highlighting in Tracker Yellow (`#FEF749`).
- [x] Authentic Tracker Mini Function Buttons 1..4 & Menu Button:
  - Mapped hardware button deck keys 1..4 to page navigation:
    `[1]` Pattern, `[2]` Song, `[3]` Instrument Parameters, `[4]` Sample Playback.
    `[Shift] + [1]` Master, `[Shift] + [2]` Perform, `[Shift] + [3]` Sample Editor, `[Shift] + [4]` Config.
  - Preserved octave selection synchronization (`octave = isShift ? num + 4 : num`).
- [x] Authentic Module Menu Popup (`hw/MenuPopup.h`, `hw/MenuPopup.cpp`):
  - 1:1 parity with firmware `core/interface/core/interfaceMenuPopup.cpp` (4-column x 3-row grid, 440x330, `miniBoxMenuColors`: red frame `#CD1818`, box bg `#222222`, white text).
  - Toggled via `Menu` button deck key or keyboard shortcuts, with full arrow key, mouse, and Jog wheel navigation.
- [x] Destructive Sample Editor UI Wiring in `SampleView`:
  - Added `SubPage` toggle (`Playback` <-> `Editor`) via screen button 7 (`Editor` / `Playback`) or the Menu popup.
  - Right-side list presents the 7 destructive audio DSP operations (Manual Section 8.9): `Crop`, `Reverse`, `Normalize`, `Amplify`, `Fade In`, `Fade Out`, `Silence`.
  - Added `SetSampleDataCommand` to `History` providing non-destructive project undo/redo.
- [x] Authentic Sample Loader & 48-Slot Instrument Pool (`SampleLoaderView.h`, `SampleLoaderView.cpp`):
  - 1:1 parity with firmware `core/interface/sampleImporter/cSampleImporter.h` and Manual Section 3.6 (lines 1886-2055).
  - Left panel File Browser ("SD Card" / local filesystem) with folder navigation and audio file discovery (`.wav`, `.pti`, `.flac`, `.aif`, `.aiff`, `.mp3`).
  - Right panel 48-Slot Project Instrument Pool displaying slot number, instrument name, and sample duration.
  - Live auditioning (`Preview`), import to highlighted slot (`Import`), and auto-allocation to the next free slot (`Add Next`).
  - Memory usage bar along bottom calculating cumulative sample duration vs. Tracker 133.0s budget.
  - Destructive project slot cleanup (`Delete Slot`, `Delete Unused`).
- [x] Verification:
  - 70 Catch2 test cases (537,592 assertions) passing 100% (`tests/test_sample_loader.cpp`).
  - Autonomous robot (`trakr_app --auto-test`) passing with 0 errors across all views.
  - All authored files strictly `< 400` lines.
  - Verified screenshot: `screen_sample_loader.png`.

## Phase 7: Cursor, Selection & Hardware Navigation Harmonization
- [x] Visual D-pad & Keyboard Shift Harmonization:
  - Harmonized selection anchors, cursor movement, and multi-step expansion across visual D-pad, Jog wheel, pattern grid mouse drag, and physical keyboard arrows.
  - Polyend Tracker Manual 1.7.0 lines 964-971 parity: `Shift + Up` on step 0 selects the entire current track on 1st press, and selects all tracks on 2nd press.
  - Ensured Jog Wheel in Rec mode with Shift expands selection rather than adjusting cell values, matching hardware behavior.
  - Propagated visual Shift latch state across all views (`PatternView`, `InstrumentView`, `SampleView`).
  - Synchronized `selectedInstIdx` from the cursor cell upon unshifted D-pad and Jog wheel navigation.
- [x] Verification:
  - 72 Catch2 test cases (537,621 assertions) passing 100% (`tests/test_dpad_parity.cpp`).
  - Autonomous robot (`trakr_app --auto-test`) passing across 164 frames and 59 widget checks with 0 errors.
  - All authored files strictly `< 400` lines.

## Phase 8: 1:1 Parity Realignment & Unified Input Engine
- [x] 100% Unified Input & Selection Engine (`PatternSelectionHelper.h`, `PatternSelectionHelper.cpp`, `PatternView.h`, `PatternView.cpp`, `ButtonDeck.cpp`):
  - Extracted single unified 2D step and selection router `navigateCursor` called identically by:
    1. Physical keyboard `Shift + Arrow` keys (`PatternSelectionHelper::handleSelectionShortcuts`).
    2. Physical keyboard arrow keys (`PatternView::handleKeyboardInput`).
    3. On-screen visual D-pad (`ButtonDeck::renderDpad`).
    4. On-screen rotary Jog Wheel (`ButtonDeck::renderJogWheel`).
    5. Grid mouse clicks and drags.
  - Pinned 2D selection anchor: the initial selection anchor `(sel.startTrack, sel.startRow)` remains fixed while cursor coordinates `(cursorTrack, cursorRow)` and bounds `(sel.endTrack, sel.endRow)` expand in both dimensions simultaneously without collapsing previously selected tracks or rows.
  - Step 0 upward expansion (Tracker Manual 1.7.0 lines 964-971): `Shift + Up` at step 0 expands to full track (0..numRows-1) on 1st press, and expands across all 8 tracks (0..numTracks-1, 0..numRows-1) on 2nd press.
  - Deselection parity: unshifted movement immediately clears selection (`hasSelection = false`), updates the cursor position, and synchronizes `selectedInstIdx` to the active cell's instrument.
- [x] Authentic Polyend Bounding Box (Manual lines 716, 964, 3610):
  - Replaced Renoise-style orange box with authentic Polyend hardware bounding box:
    - **Play / Navigate Mode**: `#30CB3F` (`colModePlay`) border with subtle translucent green fill (`rgba(48, 203, 63, 0.18)`).
    - **Edit / Rec Mode**: `#CD1818` (`colModeRec`) border with subtle translucent red fill (`rgba(205, 24, 24, 0.18)`).
    - Subcolumn focus indicator: inverted solid block (`A6` black text on green in Play mode, on red in Rec mode) inside active cell.
- [x] Dash Character & Palette Parity (Tracker Mini Photo Alignment):
  - Note empty dash formatted as 3 dashes `---` matching note character width (`C-5`, `G#6`), rendered in Dim Teal (`#005C4E`, `colNoteDim`).
  - Instrument empty dash: 2 dashes `--`, rendered in Dim Amber (`#5C5220`, `colInstrDim`).
  - FX1 empty dash: 4 dashes `----`, rendered in Dim Magenta (`#4A2359`, `colFx1Dim`).
  - FX2 empty dash: 4 dashes `----`, rendered in Dim Sky Cyan (`#214C57`, `colFx2Dim`).
- [x] Screen Button 8 (`More`) Hardware Parity (`ScreenButtons.cpp`):
  - Implemented firmware `core/source/Themes.cpp` line 179 (`.moreButtonColors = { grey_77, grey_22, grey_77, grey_22 }`):
  - Button 8 renders inverted with light grey background (`#777777`) and dark text (`#141414`), vertically centered as a single "More" label matching the Tracker Mini photo.
- [x] SD Card Browser Direct File Auditioning (`SampleLoaderView.cpp`):
  - Pressing `[Preview]` with a file highlighted in the left SD Card browser auditions the raw file directly before importing.
- [x] Verification:
  - 72 Catch2 test cases (537,631 assertions) passing 100% (`tests/test_dpad_parity.cpp`).
  - Autonomous robot (`trakr_app --auto-test`) passing across 164 frames and 59 widget checks with 0 errors.
  - All authored files strictly `< 400` lines.

- [x] Pattern 1 Playback Headroom & Synthetic Fallback Attenuation (`Voice.cpp`, `Engine.cpp`):
  - Solved piercing playback levels when playing default patterns: attenuated fallback synthetic tone by -10 dB (`0.30f`) with warmer harmonic decay ($1/h^2$).
  - Corrected `Engine::processRowEvents`: cell volume now scales through instrument `dbGain` and instrument volume parameter rather than overriding it at maximum gain.
- [x] Instrument Parameter Screen D-pad, Jog Wheel & Delete Parity (`ButtonDeck.cpp`, `test_dpad_parity.cpp`):
  - In `ButtonDeck.cpp`, wired D-pad when on `Screen::Instrument`: `DpadLeft` / `DpadRight` selects parameter column, `DpadUp` / `DpadDown` increments/decrements value, and `DpadCenter` toggles General/Automation subpages.
  - Wired `Delete` button on deck to reset focused parameter to default (Manual line 5800).
  - Added unit test cases verifying instrument D-pad navigation, stepping, and parameter reset.
- [x] Unified Shift State & Cross-Screen D-Pad / Jog Wheel Parity (`AppLayout.cpp`, `ButtonDeck.cpp`, `ButtonDeck.h`, `MixerView.h`, `SampleView.h`, `test_dpad_parity.cpp`):
  - Solved discrepancy between holding physical Shift key and visual Shift toggle: synchronized `layoutState.shiftHeld = effectiveShift` in `AppLayout.cpp` and propagated `effectiveShift` to contextual screen buttons, shortcut handlers, and deck controls.
  - Implemented cross-screen contextual D-pad and Jog wheel dispatch:
    - `Screen::Pattern`: D-pad Up/Down navigates rows or expands selection with Shift; Left/Right steps subcolumns or expands selection across tracks with Shift; Jog wheel steps rows or adjusts notes by 12 semitones (octaves) in Rec mode with Shift (Manual lines 4778, 4888).
    - `Screen::Song`: D-pad Up/Down navigates order list slots; Left/Right and Jog wheel step patterns for the selected slot.
    - `Screen::Instrument`: D-pad Left/Right selects parameter column; Up/Down / Jog steps value; D-pad Center toggles General/Automation subpages; Delete resets parameter to default (Manual line 5800).
    - `Screen::Sample`: D-pad Left/Right selects parameter/op; Up/Down / Jog steps value (fine with Shift); D-pad Center toggles Playback/Editor subpages.
    - `Screen::Master`: D-pad Left/Right selects track (1..8) or master param; Up/Down / Jog adjusts gain dB (1.0 dB coarse, 0.1 dB fine with Shift); D-pad Center cycles Track Mixer / Master / Master FX; Delete resets focused track gain to 0.0 dB.
  - Added comprehensive Catch2 unit tests in `tests/test_dpad_parity.cpp` verifying physical Shift vs visual Shift selection equivalence, Master mixer D-pad & Delete reset, Sample D-pad & togglePage, and Song slot navigation.
- [x] Verification:
  - 72 Catch2 test cases (537,652 assertions) passing 100%.
  - Autonomous robot (`trakr_app --auto-test`) passing across 164 frames and 59 widget checks with 0 errors.
  - All authored files strictly `< 400` lines.

- [x] Shift Latching & D-Pad Center Stepping Parity (`AppLayout.cpp`, `main.cpp`, `ButtonDeck.cpp`, `test_dpad_parity.cpp`):
  - Solved physical vs visual Shift toggle modifier discrepancy: decoupled visual deck latch state from physical keyboard events so visual `[Shift]` toggles state reliably without getting clobbered by physical key transitions.
  - Passed `layoutState.shiftHeld` directly by reference into `ButtonDeck::render`, ensuring clicking visual `[Shift]` stays active across frames.
  - Implemented Pattern screen `DpadCenter` stepping by `editStep` (or `-editStep` with Shift), matching physical `Enter` (Tracker Mini Essentials lines 1383–1456).
- [x] Master Bus DSP EQ & Saturation Integration (`dsp/MasterEq.h`, `Mixer.h/.cpp`, `Engine.cpp`, `Song.h/.cpp`, `MixerView.h/.cpp`):
  - Implemented 5-band Graphic EQ with biquad peaking filters matching Polyend hardware frequencies (115 Hz, 330 Hz, 990 Hz, 3 kHz, 9.9 kHz) with $\pm 12\text{ dB}$ range.
  - Added live master DSP processing for Bass Boost (+3.5 dB low shelf), Tape Saturation drive, and Stereo Space mid-side widening, clamped through the master soft limiter (-0.4 dBFS ceiling).
  - Bound Master 1/3 and Master FX 3/3 controls directly to thread-safe lock-free `SongSnapshot`, enabling live audible parameter shaping during playback.
  - Added Catch2 unit tests in `tests/test_dpad_parity.cpp` verifying Master EQ stepping, DSP audio filtering, and parameter reset.
- [x] Verification:
  - 72 Catch2 test cases (537,662 assertions) passing 100%.
  - Autonomous UI robot (`trakr_app --auto-test`) passing across 164 frames and 59 checks with 0 ID conflicts, 0 stack errors, and 0 numerical errors.
  - All authored files strictly `< 400` lines.

- [x] Unified Shift & Visual Cursor Command Harmonization (`ButtonDeck.cpp`, `PatternView.cpp`, `InstrumentView.cpp`, `SampleView.cpp`, `MixerView.cpp`, `test_dpad_parity.cpp`):
  - In `ButtonDeck.cpp`, unified Jog wheel navigation/selection in Record Mode when Shift is active (`pv.navigate(0, step, 0, isShift, ...)`), making Jog wheel and D-pad behave identically under Shift in both Play and Record modes (Tracker Manual 1.7.0 lines 3407, 3431).
  - In `PatternView.cpp`, removed destructive instrument synchronization on Shift-click selection (`syncInstrumentFromCursor`), preserving the user's active instrument selection across all mouse, D-pad, and keyboard selection operations.
  - In `InstrumentView.cpp` and `SampleView.cpp`, relaxed strict window focus gates with `!ImGui::IsAnyItemActive()` so physical cursor keys continue functioning when interacting between the visual button deck and the display canvas.
  - In `MixerView.cpp`, added native keyboard arrow navigation and Delete/Reset handling matching visual D-pad controls across Track Mixer, Master, and Master FX subpages.
  - Added unit test in `test_dpad_parity.cpp` verifying Jog wheel selection expansion in Record Mode with Shift.
- [x] Verification:
  - 74 Catch2 test cases (537,681 assertions) passing 100%.
  - Autonomous UI robot (`trakr_app --auto-test`) passing across 164 frames and 59 checks with 0 ID conflicts, 0 stack errors, and 0 numerical errors.
  - All authored files strictly `< 400` lines.

- [x] Dynamic Track Expansion Beyond 8 (Desktop Mode) (`Command.h`, `MenuBar.cpp`, `test_model.cpp`):
  - Implemented `AddTrackCommand` and `RemoveTrackCommand` providing undoable/redoable track count expansion up to 64 tracks while maintaining the 8-track hardware parity default.
  - Integrated into Main Menu Bar (`Edit -> Add Track (Desktop)`, `Edit -> Remove Track`).
  - Added unit test in `test_model.cpp` verifying track insertion, pattern track row allocation, and complete undo/redo reversal.

- [x] Configuration Page MIDI Device Selection (`ConfigView.h`, `ConfigView.cpp`, `test_midi.cpp`):
  - In `ConfigView.cpp`, wired the `Group::Midi` parameter rows to enumerate hardware MIDI ports via `MidiManager::instance().getPortNames()`.
  - Added cycling of active MIDI Input device in `adjustSelectedValue(delta)` between disconnected and all detected system MIDI inputs.
  - Added Catch2 unit test in `test_midi.cpp` verifying MIDI In device selection and configuration buttons.

- [x] Visual Cursor Commands & Keyboard Parity Under Shift (`PatternView.cpp`, `ButtonDeck.cpp`, `HeaderBar.cpp`, `AppLayout.cpp`, `PatternFxHelper.cpp`, `test_cursor_parity.cpp`):
  - In `PatternView.cpp`, decoupled pattern grid mouse interactions from window focus (`ImGui::IsMouseHoveringRect(...)`), ensuring clicks and drags register accurately when interacting with the button deck.
  - Updated mouse click and drag under Shift to synchronize `cursorSubCol_` with the clicked subcolumn (`cSub`).
  - In `ButtonDeck.cpp`, updated `isShiftActive` immediately upon clicking the visual "Shift" button so D-Pad, Jog Wheel, and function buttons receive the toggled state in that same frame.
  - Sized the 4x12 pad matrix with square aspect ratio (`padW == padH`) and tight 2.5px gaps, placing controls immediately adjacent to the grid.
  - In `PatternView.cpp` and `ButtonDeck.cpp`, added support for auditioning notes in Record mode when Shift is held (`canEdit && !isShift`), allowing users to test notes before committing them to the pattern.
  - In `PatternView.cpp`, enhanced `syncInstrumentFromCursor` to search backward on the current track for the most recently triggered instrument when landing on an empty cell (`--`).
  - Added `renderPatternWaveformStrip` in `PatternFxHelper.cpp` and integrated real-time sample waveform visualization into Pattern mode.
  - Updated `HeaderBar.cpp` stopped state to show current cursor row and pause glyph (`0m:00s  ||  P15.011`), matching reference Tracker Mini hardware.
  - Added `tests/test_cursor_parity.cpp` verifying square pad geometry, mouse Shift-selection synchronization, note auditioning without overwriting, and waveform strip rendering.
## Phase 7: Visual Parity & Typography Refinement Loop

### Typography & Font Subsystem Alignment
- [x] Authentic Monospace Advance & Glyph Dimensions (`FontLoader.h`, `FontLoader.cpp`, `FT812Emulator.cpp`):
  - Solved tiny font / broken kerning issue: Polyend firmware L4 compressed fonts have 8px character advance for Regular/Light 14, and 10px character advance for Bold 17 with 12px glyph height.
  - Implemented exact TTF point sizes in `FontLoader`:
    - Regular / Light: `17.5f * dpiScale` -> exactly 8.0px character advance, matching `RobotoMono-Regular_14_L4`.
    - Bold: `22.0f * dpiScale` -> exactly 10.0px character advance and 12px glyph height, matching `RobotoMono-Bold_17_L4`.
  - Updated `FT812Emulator.cpp`: Integrated `imstb_truetype.h` (`loadTtfIntoDecodedFont`) to render authentic Google Fonts Roboto Mono glyphs into the display emulator font slots, eliminating broken emulator kerning.

### Global Header & Screen Buttons Shift Refinements
- [x] HeaderBar Vertical Centering (`HeaderBar.cpp`):
  - Switched font to `getMonoBoldFont()`.
  - Shifted vertical baseline from `+3.0f * scale` to `+1.0f * scale`, perfectly centering in the 27px content band and eliminating the gap above the divider.
- [x] ScreenButtons Hotspot Corrections (`ScreenButtons.cpp`):
  - Switched font to `getMonoBoldFont()`.
  - Applied hotspot analyzer shift hints: `centerTopY` to `15.0f/15.5f/26.0f`, `centerBtmY` to `36.5f`, `topX/btmX` shifted by `-0.5f * scale`.

### Screen-Specific Parity Fixes
- [x] Sample View Waveform Parity (`SampleView.cpp`, `FT812ScreensExtra.cpp`):
  - Converted waveform rendering to pixel-solid vertical rectangles `AddRectFilled(ImVec2(x, midY - h), ImVec2(x + 1, midY + h), #30CB3F)` without anti-aliasing edge fading.
  - Removed interior center line (`y = 228`) cutting through waveform.
  - Aligned timing tick marks and time readouts to the top 10px band (`y = 32..42`).
- [x] Song View Alignment (`OrderListView.cpp`, `FT812ScreensCore.cpp`):
  - Removed selection dot `●` from slot numbers (`01` instead of `● 01`).
  - Aligned track headers and columns to `196.0f + t * 75.0f` (73px width), fitting track 8 against the screen edge.
  - Added alternating darkness on `--` symbol (`#575757` on even rows, `#343434` on odd rows).
- [x] Performance View Typography & Contrast (`PerformView.cpp`):
  - Switched pads and column labels to `getMonoBoldFont()` at `font->LegacySize` (22px), resolving dark/unbolded appearance.
- [x] Instrument View Parameter Bars (`InstrumentBars.cpp`, `FT812ScreensExtra.cpp`):
  - Selection box starts at `y = 29` (`selH = 393.0f * sy`).
  - 0% line removed; bars fill continuously upward from bottom baseline.
- [x] Mixer View Font & Geometry Alignment (`MixerView.cpp`):
  - Track names use `getMonoBoldFont()`, dB readout uses `getMonoFont()`, Pan readout uses `getMonoLightFont()`.
  - Repositioned Pan readout to `cPos.y + 360.0f * sy` so it never clips at the bottom boundary.
- [x] Sample Loader View Parity Initializer (`SampleLoaderView.h`, `SampleLoaderView.cpp`):
  - Added `setParityState()` initializing demo `/samples/drums` path and sample pool.
- [x] Parity Boot Initializations (`main.cpp`):
  - `init_screen == "config"` sets `activeGroup_ = Group::General`.
  - `init_screen == "pattern"` clears pattern 0 note data for empty state comparison.
  - `init_screen == "loader"` initializes sample pool demo instruments.

### Current Visual Similarity Scores (All >= 90% Threshold)
- **SAMPLE**: **93.94%** (MAE: 2.93%) — *Up from 91.60%*
- **CONFIG**: **93.86%** (MAE: 3.29%) — *Up from 93.72%*
- **SONG**: **93.67%** (MAE: 2.54%) — *Up from 93.54%*
- **LOADER**: **93.51%** (MAE: 3.13%) — *Up from 93.34%*
- **INSTRUMENT**: **93.12%** (MAE: 3.63%) — *Up from 90.83%*
- **PATTERN**: **92.93%** (MAE: 4.47%) — *Up from 92.81%*
- **MASTER**: **92.20%** (MAE: 3.73%) — *Up from 92.06%*
- **PERFORM**: **90.87%** (MAE: 4.83%) — *Up from 90.54%*

---

## What To Do Next (Roadmap to >= 95% Parity)

### 1. Measure Fresh Similarity Scores
Run the OS comparator runner outside sandbox (`BypassSandbox: true` required for macOS Metal WindowServer allocation):
```bash
python3 native/tools/os_input/os_input_runner.py --no-emu
```
Or execute the automated refinement ratchet:
```bash
python3 native/tools/visual_refine.py --screen all
```

### 2. View-Specific Fine-Tuning Hotspots
Target each screen's remaining hotspots in priority order (lowest-scoring screens first):

1. **PERFORM Screen (Target: 90.54% -> 95.0%)**:
   - Location: `native/app/views/PerformView.cpp`
   - Target: Shift vertical rotated column text upward by 2px (`vertTextBottomY = origin.y + 214.0f * sy`) to eliminate hotspot #1/#2 in upper body.
   - Adjust performance pad grid vertical offset and pad border thickness (1px solid `#404040`).

2. **INSTRUMENT Screen (Target: 90.83% -> 95.0%)**:
   - Location: `native/app/views/InstrumentBars.cpp` and `InstrumentView.cpp`
   - Target: Verify horizontal bar column widths (12 columns: Volume, Panning, Tune, Finetune, etc.).
   - Ensure parameter value text numbers sit exactly 4px above the bottom bounding line.

3. **SAMPLE Screen (Target: 91.60% -> 95.0%)**:
   - Location: `native/app/views/SampleView.cpp`
   - Target: Fine-tune waveform bottom loop marker flags (`S`, `LS`, `LE`, `E`) and zoom level indicator.
   - Match bottom parameter boxes (Start, End, Loop Start, Loop End) border colors and text alignment.

4. **MASTER / MIXER Screen (Target: 92.06% -> 95.0%)**:
   - Location: `native/app/views/MixerView.cpp`
   - Target: Fine-tune peak meter column widths (width: 12px, spacing: 4px) and segment divider lines.
   - Check Master FX 3/3 EQ curve rendering against firmware reference.

5. **PATTERN Screen (Target: 92.81% -> 95.0%)**:
   - Location: `native/app/views/PatternView.cpp`
   - Target: Fine-tune row 8 cursor box outline padding (`tx + 2.0f * sx`, `y + 1.0f`).
   - Check track header number centering (`T1`..`T8`).

6. **LOADER, SONG, and CONFIG Screens (Target: 93.3% - 93.7% -> 95.0%)**:
   - **Loader**: Verify file list item row height (20px vs 22px) and scrollbar thumb width.
   - **Song**: Match slot number column width and border divider line with reference frame.
   - **Config**: Verify right-pane parameter box width (790px) and eliminate any lingering 1px black outline.

### 3. Bottom Screen Interface & Deck Layout
- Ensure the rotary Jog Wheel and D-pad layout in `native/app/hw/ButtonDeck.cpp` maintain distinct touch bounds so the wheel never overlaps the D-pad across responsive window sizes.
- Verify that bottom contextual screen menus contain all submenus and extra options inside `More...` matching Polyend Tracker 1.7.0 firmware.

### 4. Hard Constraints Checklist for Every Change
- **Edits strictly inside `native/`**: Never touch `core/`, `platforms/`, `synth/`, `util/`.
- **File Length Constraint**: Every authored source file must remain strictly `< 400` lines (`find native/app -name "*.cpp" -o -name "*.h" | xargs wc -l | sort -n`).
- **Brand String Parity**: Zero mentions of "hardware" or "Polyend" in user-facing UI strings (window titles, tooltips, labels).
- **Test Suite**: Always verify clean build, 101 Catch2 test cases, and autonomous UI robot:
  ```bash
  cmake --build native/build -j8 && ./native/build/trakr_tests && ./native/build/trakr_app --auto-test
  ```
- **Monotonic Ratchet**: Visual similarity scores must strictly improve or stay equal; never regress.

---

## Phase 8: Teamwork Multi-Agent Swarm Refinement (In Progress)

### Completed Screen Targets (>= 95.0% Achieved)
- [x] **Loader Screen**: **95.46%** (SSIM: 87.44%, MAE: 1.96%) — *Milestone 6 Passed Gate*
- [x] **Pattern Screen**: **95.35%** — *Milestone 4 Passed Gate*
- [x] **Master Screen**: **95.03%** (SSIM: 86.81%, FG: 82.14%) — *Milestone 3 Passed Gate*

### Intermediate Scores (All Upward Monotonic Lift)
- **Instrument Screen**: **94.19%** (SSIM: 83.41%) — *Lifted from 93.12%*
- **Sample Screen**: **94.18%** — *Lifted from 93.94%*
- **Config Screen**: **93.98%** — *Lifted from 93.86%*
- **Song Screen**: **93.77%** — *Active Milestone 7*
- **Perform Screen**: **93.11%** (SSIM: 82.59%, FG: 84.55%) — *Lifted from 91.72%*

### Active Swarm State
- Teamwork Subagents: All idle background subagents and scheduled cron loops terminated to eliminate background memory accumulation.
- Catch2 Test Suite: 103/103 test cases passing (538,003 assertions).
- Robot UI Audit (`--auto-test`): 1,103 frames, 992 widget checks, 0 ID conflicts, 0 stack errors, 0 numerical errors.
- RtMidi Fix: Removed invalid `throw()` specification from `RtMidi.cpp` CoreMIDI singleton methods and added fallback to dummy MIDI backend in `MidiManager.cpp` to prevent unhandled CoreMIDI client aborts.
- Constraints Verified: Zero edits outside `native/`; all authored files strictly < 400 lines (max: `Widgets.cpp` at 398 lines, `main.cpp` at 396 lines).
- Current Screen Benchmark Scores (8-Screen Average: 94.38%):
  - [x] **Loader Screen**: **95.46%** (SSIM: 87.16%, MAE: 2.03%) — *Passed 95% Gate*
  - [x] **Pattern Screen**: **95.35%** (SSIM: 65.95%, MAE: 3.59%) — *Passed 95% Gate*
  - [x] **Master Screen**: **95.03%** (SSIM: 86.64%, MAE: 2.49%) — *Passed 95% Gate*
  - [ ] **Instrument Screen**: **94.18%** (SSIM: 83.20%)
  - [ ] **Sample Screen**: **94.18%** (SSIM: 79.40%)
  - [ ] **Config Screen**: **93.98%** (SSIM: 73.40%)
  - [ ] **Song Screen**: **93.77%** (SSIM: 74.10%)
  - [ ] **Perform Screen**: **93.09%** (SSIM: 82.36%)











## 2026-11-11 — Source-true firmware reference for Instrument and Master

- Replaced the hand-drawn Instrument/Master emulator references with `fwref`:
  the unmodified firmware `core/libs/NewDisplay` control sources are compiled
  on the host and driven through an EVE/FT812 API shim.
- Layout/style/colors come from hash-locked specs extracted from
  `gInstrEdit.cpp`, `gMasterParams.cpp`, `titleBar.cpp`, `Themes.cpp`
  (`native/tools/fwref/specs`, `extract_spec.py --check`).
- Ported the app: Instrument General (74px bar math, filter cList, frame,
  bottom labels, More dots) and Master 1/3 (subpage order, bars, cover,
  saturation list, firmware labels).
- `NullAudioBackend` now pumps the engine clock so `--remote` transport flows
  advance; functional smoke is 17/17.
- Measured against the new reference: Instrument 97.24%, Master 97.66%.

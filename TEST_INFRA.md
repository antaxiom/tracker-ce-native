# Trakr Native Test Infrastructure & Verification Harness

## 1. Overview & Verification Architecture
Trakr Native maintains a multi-tiered test infrastructure designed to guarantee audio-engine mathematical correctness, UI hierarchy stability, and pixel-exact visual parity against Polyend Tracker hardware firmware.

Visual parity is verified against the authentic FT812 hardware display emulator (`native/tools/display_emulator/`), which decodes Polyend L4 RLE compressed fonts and renders exact firmware UI states onto an 800x480 virtual LCD buffer.

```
+-----------------------------------------------------------------------------+
|                         TEST INFRASTRUCTURE TIERS                           |
+-----------------------------------------------------------------------------+
| Tier 1: Catch2 Unit & DSP Suite (101 Tests)                                  |
|   - Audio Engine, DSP filters, voice synthesis, MIDI, JSON/Polyend I/O      |
|   - Screen feature models, undo/redo history, fuzz tests, benchmarks        |
+-----------------------------------------------------------------------------+
| Tier 2: Autonomous UI Robot Audit (--auto-test)                             |
|   - Headless ImGui frame pump (1,103 frames, 992 widget checks)             |
|   - Zero ImGui ID conflicts, zero stack imbalances, zero numerical errors   |
+-----------------------------------------------------------------------------+
| Tier 3: Opaque-Box Visual Parity & Hotspot Telemetry Harness                |
|   - Live CoreGraphics OS event injection (CGEventPost)                      |
|   - Side-by-side 1:1 image comparison (Total Pixel %, FG Match, SSIM, MAE)  |
|   - Hotspot bounding box error budgets and translation shift hints          |
+-----------------------------------------------------------------------------+
```

---

## 2. Opaque-Box Visual Parity Harness (`os_input_runner.py`)
- **Location**: `native/tools/os_input/os_input_runner.py`
- **Helper Modules**: `native/tools/os_input/hotspot_analyzer.py`, `analyze_diff.py`
- **Zero External Dependencies**: Pure Python stdlib (`ctypes`, `struct`, `zlib`, `subprocess`, `numpy`).

### 2.1 Operational Pipeline
1. **Reference Frame Generation**:
   Executes `./native/build/trakr_tests "[emulator]"` to synthesize reference 800x480 frames (`/tmp/emu_<screen>.png`) using `FT812DisplayEmulator`.
2. **OS Input Emulation**:
   `OSInputDriver` uses macOS `CoreGraphics` (`/System/Library/Frameworks/CoreGraphics.framework`) to post raw OS input events (`CGEventPost`) to verify window event boundaries.
3. **App Screenshot Capture**:
   Executes `./native/build/trakr_app --screen <screen> --polyend-font --screenshot <path>` to capture the native Metal/ImGui output.
4. **LCD Buffer Extraction**:
   `extract_lcd_800x480()` crops and normalizes the 800x480 display area from the captured frame into `/tmp/app_lcd_<screen>.png`.
5. **Multi-Metric Image Comparison**:
   `compare_and_composite()` computes four objective similarity metrics:
   - **Total Pixel Similarity (%)**: Percentage of pixels where RGB channel average difference $\le 24.0$ ($0.0$ to $255.0$ scale).
   - **Foreground Match (%)**: Match percentage evaluated specifically on non-background pixels (`max(R,G,B) > 20`).
   - **SSIM (%)**: Structural Similarity Index calculated across $8 \times 8$ local image blocks.
   - **MAE (%)**: Mean Absolute Error across all pixels normalized to $0..100\%$.
6. **Artifact Output**:
   - `/tmp/diff_<screen>.png`: False-color visual diff highlighting error regions.
   - `comparison_<screen>_1to1.png`: $1610 \times 480$ side-by-side composite (Emulator on left, 10px `#22242a` separator, Native App on right).

### 2.2 Deterministic Hotspot Telemetry Engine
`hotspot_analyzer.py` breaks down discrepancies into actionable developer hints:
- **Band Analysis**: Error distribution across HeaderBar (`y=0..30`), UpperBody (`y=30..230`), LowerBody (`y=230..430`), and ScreenButtons (`y=430..480`).
- **Top Hotspots**: Top 5 mismatched bounding boxes `[x0..x1, y0..y1]` with mismatch counts.
- **Color Extraction**: Dominant mismatched color pairs in `IM_COL32(r, g, b, 255)` format.
- **Translation Shift Hints**: Search in $[-4..+4]$ pixels returning recommended coordinate offsets `(dx, dy)`.

---

## 3. The 8 Screen Test Targets
The visual parity suite covers all 8 functional screens of the instrument:

| Screen | Target View File | Authored Lines | Visual Focus Areas |
|---|---|---|---|
| **PERFORM** | `native/app/views/PerformView.cpp` | 157 | 12 performance FX columns, rotated vertical labels (`y=244`), 4-row value blocks (`y=287`), CC top block (`y=34`). |
| **MASTER** | `native/app/views/MixerView.cpp` | 262 | 8 channel strips, dB readouts (`y=68`), pan readouts (`y=390`), VU meter fills (`h=280`), track selection outline. |
| **PATTERN** | `native/app/views/PatternView.cpp` | 397 | 8-track step sequencer grid (`y=28, h=395`), cursor outline stroke, track header badges, status lines. |
| **INSTRUMENT** | `native/app/views/InstrumentView.cpp`, `InstrumentBars.cpp` | 371 + 75 | Parameter bars (`#30CB3F`), bipolar center line, tune (`st`) / finetune (`ct`) unit formatting. |
| **LOADER** | `native/app/views/SampleLoaderView.cpp` | 309 | Two-pane sample browser and slot pool (48 slots), selection boxes, header alignment. |
| **SONG** | `native/app/views/OrderListView.cpp` | 187 | Song arranger grid, slot cell highlights, pattern sequence slots, right-edge coordinate scaling. |
| **CONFIG** | `native/app/views/ConfigView.cpp` | 247 | Two-pane settings tree (250px split), category navigation, parameter row separators. |
| **SAMPLE** | `native/app/views/SampleView.cpp` | 378 | Waveform rendering, 0.15s time ruler gridlines with second markers, START/END boundary banners. |

---

## 4. Baseline Measurements & Target Thresholds

### 4.1 Threshold Criteria
- **Target Parity**: $\ge 95.0\%$ pixel similarity for all 8 screens.
- **Floor Threshold**: $\ge 90.0\%$ pixel similarity minimum acceptable score.
- **Monotonic Ratchet**: Zero regressions permitted during visual refinement. Any change decreasing similarity must be rolled back immediately.

### 4.2 Empirical Baseline Scores (Measured 2026-11-10)

| Screen Target | Pixel Match % | FG Match % | SSIM % | MAE % | Status vs 90% Floor | Status vs 95% Target |
|---|---|---|---|---|---|---|
| **PERFORM** | **91.40%** | 81.25% | 75.64% | 4.52% | PASS (+1.40%) | Iteration Target (Priority 1) |
| **MASTER** | **92.45%** | 74.39% | 77.84% | 3.53% | PASS (+2.45%) | Iteration Target (Priority 2) |
| **PATTERN** | **92.99%** | 82.45% | 56.02% | 4.42% | PASS (+2.99%) | Iteration Target (Priority 3) |
| **INSTRUMENT** | **93.40%** | 76.94% | 80.52% | 3.39% | PASS (+3.40%) | Iteration Target (Priority 4) |
| **LOADER** | **93.46%** | 92.19% | 76.24% | 3.17% | PASS (+3.46%) | Iteration Target (Priority 5) |
| **SONG** | **93.73%** | 57.77% | 74.24% | 2.49% | PASS (+3.73%) | Iteration Target (Priority 6) |
| **CONFIG** | **93.93%** | 82.61% | 73.58% | 3.23% | PASS (+3.93%) | Iteration Target (Priority 7) |
| **SAMPLE** | **94.14%** | 66.24% | 79.54% | 2.76% | PASS (+4.14%) | Iteration Target (Priority 8) |

All 8 screens currently exceed the $\ge 90.0\%$ project floor. Refinement efforts systematically focus on lower-scoring screens to elevate all to $\ge 95.0\%$.

---

## 5. Catch2 Unit & Integration Test Suite
- **Binary**: `./native/build/trakr_tests`
- **Status**: 101 test cases, 537,967 assertions, 100% passing.
- **Coverage Map**:
  - `[model]`: Song, track, cell defaults, scale intervals, and snapping.
  - `[engine]`: Audio engine, 32 voices playback, pitch transposition, looping, dynamic allocation.
  - `[dsp]`: ADSR envelope stages, SVF filter stability, LFO range, delay, reverb, overdrive, bitcrush.
  - `[io]`: JSON serialization round-trips, WAV import/export, Polyend SD project structure round-trip.
  - `[fuzz]`: Audio engine randomized song data stress, history undo/redo fuzzing.
  - `[perf]`: 32-voice engine benchmark (< 0.15 ms / 128 frames), 64x512 pattern view culling benchmark.
  - `[hw]`: Button deck pads, octave keys, jog wheel, edit keys, D-pad, and display layout aspect scaling.
  - `[emulator]`: Authentic FT812 emulator framebuffer verification and reference frame synthesis.
  - `[tier1]`: Screen feature coverage across Perform, Pattern, Song, Master, Instrument, Sample, Loader, Config.
  - `[tier2]`: Numerical boundary and clamping tests (gain, rows, BPM, zoom, pitch, mute matrix, song slots).
  - `[tier3]`: Cross-module integration flows (screen cycle state persistence, mute during playback, modulation).

---

## 6. Autonomous UI Robot Audit
- **Command**: `./native/build/trakr_app --auto-test`
- **Status**: 1,103 simulated frames and 992 widget checks across 6 views.
- **Audit Invariants**:
  - 0 ImGui ID conflicts across active and background widgets.
  - 0 ImGui stack push/pop imbalances.
  - 0 numerical errors (NaN, Inf, or out-of-bounds coordinate rendering).

---

## 7. Execution Commands Summary

```zsh
# 1. Build test and application binaries
cmake --build native/build -j8

# 2. Run full Catch2 unit test suite (101 tests)
./native/build/trakr_tests

# 3. Run autonomous UI robot audit
./native/build/trakr_app --auto-test

# 4. Generate FT812 emulator reference screenshots
./native/build/trakr_tests "[emulator]"

# 5. Run full 8-screen visual parity comparator
python3 native/tools/os_input/os_input_runner.py

# 6. Run fast single-screen inner-loop with hotspot telemetry
python3 native/tools/os_input/os_input_runner.py --only perform --no-emu --hotspots
```

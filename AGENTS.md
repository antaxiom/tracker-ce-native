# Kilo Agent Configuration

## Project Overview
**Trakr** (placeholder working name — subject to rename) is an enhanced C++20
desktop port and rebuild of the **Polyend Tracker+ firmware** (the `tracker-ce`
open-source codebase). The desktop app reuses the firmware's core engine and UI
structure: each `native/app/views/*` screen is a faithful 1:1 structural port of
the corresponding firmware screen editor (`core/interface/*/g*.cpp`), and pixel
fidelity is verified against **source-true** references rendered by compiling the
unmodified firmware `NewDisplay` controls through an FT812/EVE API shim
(`native/tools/fwref`). On top of the faithful port it layers desktop enhancements
(lossless JSON persistence, WAV/stem export, up to 64 tracks, MIDI input, native
file dialogs).

## Key Constraints
- **Edit Scope**: ONLY edit files in `native/` directory (never `core/`, `platforms/`, `synth/`, `util/`)
- **Fidelity**: source-true screens must hold ≥95.0% pixel similarity against their `fwref` reference
- **Testing**: 100% test pass rate required

## Commands
```bash
# Build application and tests
cmake --build native/build -j8

# Run unit tests (125 test cases)
./native/build/trakr_tests

# Run UI robot audit (1,103 frames)
./native/build/trakr_app --auto-test

# Generate FT812 reference images
python3 native/tools/os_input/os_input_runner.py

# New semantic UI path (Instrument General vertical slice)
./native/build/trakr_app --screen instrument --gui-next --lcd-shot --screenshot /tmp/instrument.png

# Fast single-screen visual testing
python3 native/tools/os_input/os_input_runner.py --only <screen> --no-emu --hotspots

# Check file line counts
find native/app native/audio native/model native/engine native/io native/tests -name "*.cpp" -o -name "*.h" | xargs wc -l | sort -nr | head -n 15

# Functional smoke suite (17 real editing flows, pixel-exact restore checks)
python3 native/tools/gui_audit/functional_smoke.py --out /tmp/smoke

# Tofu guard: UI codepoints must exist in the bundled fonts
python3 native/tools/check_glyphs.py

# GUI interaction audit (batch: contact sheet per scenario)
python3 native/tools/gui_audit/gui_audit.py --spec native/tools/gui_audit/specs/core.json --out /tmp/gui_audit

# Live operator control (one key/screenshot per turn)
python3 native/tools/gui_audit/gui_ctl.py --out /tmp/op start --screen config
python3 native/tools/gui_audit/gui_ctl.py --out /tmp/op shot s1
python3 native/tools/gui_audit/gui_ctl.py --out /tmp/op key Down
python3 native/tools/gui_audit/gui_ctl.py --out /tmp/op stop
```

`--gui-next` is the opt-in semantic UI path.
`--script`/`--remote` accept `Ctrl+Z`, `Shift+Up`, `Alt+Up` chords. On macOS the
Ctrl modifier is emitted as Cmd-equivalent (ImGui swaps them), matching shortcuts.
`--remote` also accepts `click <x> <y>` (hover-then-click, pointer is held) and
`type <text>` (UTF-8 characters into the focused InputText); `gui_ctl.py` exposes
`click`/`type` and `--live` (omit `--lcd-shot` to get the menu bar/deck).
`--loader-dir <path>` starts the Sample Loader in a real directory instead of the
synthetic parity listing.

**File/Edit menus**: File mirrors the firmware File page (New/Open/Save/Save As/
Delete/Export Song|Stems|Pattern|Pattern Stems|Project/Quit) and Edit mirrors the
firmware edit ops (Undo/Redo/Cut/Copy/Paste/Fill). Dialogs are in-app ImGui modals:
macOS SDL native file dialogs post SDL_EVENT_QUIT for non-bundled binaries and kill
the app, so `dialogs::show*` only sets state and `renderDialogs()` draws the modal.

## Visual Fidelity & Parity

Measured with the native 1:1 LCD capture (`trakr_app --lcd-shot`, exact 800x480 frame, no crop/resample).

Two tiers of reference exist:

1. **Source-true references** (`native/tools/fwref`): the unmodified firmware
   `NewDisplay` controls are compiled on the host and driven by hash-stamped specs,
   producing the authoritative frame. Currently covers **Instrument (General)**,
   **Master (1/3)**, and **Pattern**.
2. **Hand-drawn emulator references** (`native/tools/display_emulator/FT812ScreensCore/Extra.cpp`):
   the remaining screens still measure against hand-drawn approximations while they
   are migrated to source-true fwref.

Current source-true scores (≥95.0% target):

| Screen | Pixel % | Status |
|--------|---------|---------|
| Pattern | 98.36% | ✅ |
| Master | 98.14% | ✅ |
| Instrument | 97.75% | ✅ |

**Measurement note**: screenshots are rendered with `--lcd-shot`, an exact 800x480
framebuffer with no DPI scaling, no menu bar, and no button deck. The PNG IS the LCD,
so no cropping or resampling is involved.

## Known Residual Gap
Remaining source-true mismatch is sub-pixel glyph rasterization: the reference uses
embedded RobotoMonoBold17 L4 bitmaps, while the app renders RobotoMono TTFs with
`PixelSnapH` (10px bold / 8px regular advance). Layout, header band, screen buttons,
and markers align within ~1px.

## Agent Rules
1. Never edit files in `core/`, `platforms/`, `synth/`, or `util/` directories
2. Maintain brand parity (no "hardware" or "Polyend" in the user-facing UI; "Trakr" is a placeholder name)
3. Follow the firmware-faithful port structure: each view mirrors its `core/interface/*/g*.cpp` screen editor
4. Use source-true `fwref` references (`render/fwref/*.png` + `native/tools/fwref/specs`) as ground truth for colors/geometry, not the hand-drawn `FT812ScreensCore/Extra.cpp`
5. Perform visual review after each significant change
6. Use a cheap vision operator (Qwen3 VL 8B) for interactive audits; reserve Hy4 for stuck decisions only
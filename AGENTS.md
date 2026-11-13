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
- **Fidelity**: the Classic profile must pass calibrated visual regression and layout invariants; `fwref` is an audited advisory oracle (ADR-0003), not ground truth
- **Testing**: 100% test pass rate required

## Commands
```bash
# Build application and tests
cmake --build native/build -j8

# Run unit tests (139 test cases)
./native/build/trakr_tests

# Run UI robot audit (1,103 frames)
./native/build/trakr_app --auto-test

# Generate FT812 reference images
python3 native/tools/os_input/os_input_runner.py

# New semantic UI path (Instrument General, Master 1/3)
./native/build/trakr_app --screen instrument --gui-next --lcd-shot --screenshot /tmp/instrument.png
./native/build/trakr_app --screen master --gui-next --lcd-shot --screenshot /tmp/master.png
./native/build/trakr_app --screen pattern --gui-next --lcd-shot --screenshot /tmp/pattern.png
./native/build/trakr_app --screen config --config-edit "Anti-aliasing" --lcd-shot --screenshot /tmp/config_edit.png

# Fast single-screen visual testing
python3 native/tools/os_input/os_input_runner.py --only <screen> --no-emu --hotspots

# Classic visual-regression gate (16 screen/sub-page states vs fwref, >= 95%)
python3 native/tools/visreg/visreg.py --all --deterministic

# Calibrated hardware structural checks (needs ImageMagick, numpy, scipy)
python3 native/tools/refcal/refcal.py --all --out /tmp/refcal
python3 -m unittest discover native/tools/refcal

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

The semantic UI path is the only renderer: all nine main workspaces and every
app-modeled sub-page render through it, and `--gui-next` is accepted but no
longer required.
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
   producing the authoritative frame. Covers every Classic screen and app-modeled
   sub-page: **Instrument (General/Effects/Automation)**, **Master (1/3, 2/3 Track
   Mixer, 3/3 Global Mixer, EQ)**, **Pattern**, **Song**, **Sample Playback**,
   **Sample Loader**, **Config**, **Performance** and **File**.
2. **Hand-drawn emulator references** (`native/tools/display_emulator/FT812ScreensCore/Extra.cpp`):
   retired from all gates; they remain only inside the legacy emulator tooling.

Current source-true scores (≥95.0% target):

| Screen / state | Similarity | Status |
|----------------|-----------|--------|
| Pattern | 99.70% | ✅ |
| Song | 98.98% | ✅ |
| Instrument General | 99.56% | ✅ |
| Instrument Effects | 99.99% | ✅ |
| Instrument Automation | 99.98% | ✅ |
| Master 1/3 | 99.66% | ✅ |
| Master 2/3 (Track Mixer) | 99.53% | ✅ |
| Master 3/3 (Global Mixer) | 99.32% | ✅ |
| Master EQ | 99.78% | ✅ |
| Sample Playback | 99.46% | ✅ |
| Sample Slice | 99.38% | ✅ |
| Sample Granular | 99.35% | ✅ |
| Sample Loader | 99.53% | ✅ |
| Config | 99.66% | ✅ |
| Config item editor | 99.65% | ✅ |
| Performance | 98.35% | ✅ |
| File | 99.36% | ✅ |

These are full-frame comparisons against the audited fwref scenario, measured by
`native/tools/visreg/visreg.py`. The remaining difference is mostly app chrome
and rotated/bitmap glyph rasterization. See
`native/docs/gates/phase4-*.md` for per-screen scope and conditions.

**Measurement note**: screenshots are rendered with `--lcd-shot`, an exact 800x480
framebuffer with no DPI scaling, no menu bar, and no button deck. The PNG IS the LCD,
so no cropping or resampling is involved.

## Known Residual Gap
Remaining source-true mismatch is sub-pixel glyph rasterization: the reference uses
embedded RobotoMonoBold17 L4 bitmaps, while the app renders RobotoMono TTFs with
`PixelSnapH` (10px bold / 8px regular advance). Layout, header band, screen buttons,
and markers align within ~1px.

## Semantic UI Renderer Rules
- `Canvas::line` draws hard-edged EVE-style segments (orthogonal segments as
  exact device-pixel rects, diagonals with AA disabled). Do not switch back to
  `ImDrawList::AddLine` without disabling `ImDrawFlags_AALines`: its texture
  fringe overdraws the control's box.
- `cLabel` with `controlStyleCenterX` takes `prop.x` as the label **centre** and
  paints its background across `[x - w/2, x + w/2]`. When porting a centred
  label, subtract `w/2` from the firmware x.
- Vertical rotated text (`Canvas::textVertical270`, cSongPlayer track names):
  glyphs rasterise at `screenY(0)` (= `origin.y - deviceTop * scale`), so the
  rotation reference must be that same y. Using `origin.y` shifted every glyph
  left by `deviceTop` device px, which hid the first Song track name past the
  left edge. The matrix idiom `translate(+h/2) / ROTATE(270) / translate(-h/2)`
  nets a `(I - R) * h/2` offset (+h in y), so the name baseline is
  `posY + 42 + fontHeight`.
- Bottom shadows (`drawBottomShadow`, 12 px: `grad_y = bottom - 10`,
  `grad_h = 12`) are theme-gated like the firmware: `theme.bottomScreenShadow`
  enables cList/cBar/cSongPlayer shadows and `theme.barShadow` the bar
  gradient. Classic enables both; other themes must not inherit them.
- Animation: sample `ui::animationNowMs()` at draw time (never accumulate
  per-frame deltas), report animated controls through `Control::animatingAt`,
  and use `ui::Tween`/`ui::Blinker` (`native/app/ui/animation.h`). Captures
  freeze the clock.

## Agent Rules
1. Never edit files in `core/`, `platforms/`, `synth/`, or `util/` directories
2. Maintain brand parity (no "hardware" or "Polyend" in the user-facing UI; "Trakr" is a placeholder name)
3. Follow the firmware-faithful port structure: each view mirrors its `core/interface/*/g*.cpp` screen editor
4. Use firmware source (`core/interface/*/g*.cpp`, `NewDisplay`, `Themes.cpp`) as ground truth; `fwref` is advisory (audited, source-derived), and the hand-drawn `FT812ScreensCore/Extra.cpp` is retired
5. Perform visual review after each significant change
6. Use a cheap vision operator (Qwen3 VL 8B) for interactive audits; reserve Hy4 for stuck decisions only
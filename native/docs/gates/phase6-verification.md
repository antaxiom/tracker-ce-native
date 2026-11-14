# Phase 6 — Verification, retirement and handoff

- Date: 2026-11-12
- Status: **PASS with documented residuals**

## Purpose

Phase 6 closes the GUI restart: the semantic UI is the production renderer,
firmware source remains ground truth, fwref is an audited advisory oracle, and
legacy code is retained only as dialog/state plumbing and a debug escape hatch.

## Verification matrix

| Gate | Command | Result |
|---|---|---|
| Unit tests | `./native/build/trakr_tests` | 139/139, 538,763 assertions |
| UI robot (semantic default) | `./native/build/trakr_app --auto-test` | 114 frames, 0 ID conflicts/stack/numeric errors |
| Functional smoke | `python3 native/tools/gui_audit/functional_smoke.py` | 17/17 scenarios pass |
| fwref strict oracle | `cd native/tools/fwref && python3 check_reference.py` | all gates pass, 0 unsupported / 0 approximations |
| Hardware structural (calibrated) | `python3 native/tools/refcal/refcal.py --all` | 8/8 tiles (Instrument x2, Instrument Automation, Performance, Pattern, Track Mixer, Sample Playback, Master EQ) |
| Glyph coverage | `python3 native/tools/check_glyphs.py` | 0 non-ASCII codepoints, all covered |
| Classic visual regression | `python3 native/tools/visreg/visreg.py --all --deterministic` | 17/17 cases >= 95% (97.93-100.00%), deterministic |
| Canvas line raster | `./native/build/trakr_tests "[canvas]"` | hard-edged, no AA fringe, exact endpoint coverage |
| Classic parity | per-screen gate docs | all nine workspaces >= 95%; best 99.9% |

## Screen coverage

| Workspace | Semantic path | Main state vs fwref | Sub-pages |
|---|---|---|---|
| Pattern | default | 98.4% | popups via app fill modal |
| Song | default | 98.9% | — |
| Instrument | default | General 99.98% | Effects/Automation legacy-equivalent (no fwref) |
| Master | default | 1/3 99.7%; 2/3 99.3%; 3/3 99.2% | Global Mixer ported (model + engine aux gains and meters); Master EQ 99.7% (source-true fwref + hardware tile) |
| Sample | default | Playback 98.3% | Editor app-defined (structural) |
| Sample Loader | default | 99.1% | importer crop page not modeled |
| Config | default | 99.6% | item editor/grid page not modeled |
| Perform | default | 98.3% | grid/pad page not modeled |
| File | default | 98.9% | browser only by design |

## Retirement decisions

1. `--gui-classic` is retired: semantic rendering is unconditional in
   `AppLayout`, and the legacy `renderActiveScreen` path is no longer reachable
   from the app (it remains only for the legacy regression harness and
   dialogs/state plumbing).
2. Legacy views remain for view state, edit commands, dialogs and the
   `--gui-classic` path. Deleting them requires porting the remaining
   app-defined dialogs (export deck, fill modal, import progress) first; this
   is tracked as post-Phase-6 cleanup rather than blocking the restart.
3. Hand-drawn `FT812ScreensCore/Extra.cpp` references are retired from all
   gates; they remain only inside the legacy display-emulator tooling.
5. `native/tools/visreg/visreg.py` replaces pixel parity as an architecture
   constraint: it renders every semantic screen/sub-page, checks 800x480 and
   determinism, and enforces a >= 95% similarity threshold against the audited
   fwref references.
4. fwref is labeled an audited advisory oracle: strict mode is on, and every
   used call is implemented or whitelisted. It is never an app dependency.

## Renderer findings (alpha, overdraw, animation)

### 1. Dark pages: no alpha-blend defect, opaque geometry was misplaced

A full-frame colour histogram against the source-true references showed no
alpha-accumulation error: the LCD path draws every fill opaque, the theme
colours match `Themes.cpp` byte for byte, and the per-screen mean luminance
agrees with fwref except where backgrounds were missing. The cause was
geometry, in two places:

- **Centred labels.** `cLabel` with `controlStyleCenterX` treats `prop.x` as the
  label *centre* (`label.cpp:114`), and its background spans
  `[x - w/2, x + w/2]`. `FileScreen` and `SampleLoaderScreen` treated `x` as a
  left edge, so the grey_0A label bands (and their text) were shifted right by
  `w/2` and the left half of the page stayed black.
- **File blank-space cover.** `gProjEdit.cpp:253-254` places the cover at
  `(800/8)*6 + 1` with width 200; the app used `701/98`, leaving a 100px black
  strip. The Sample page also framed place 6 by default while the scenario had
  `selectedPlace = 0`.

Result: File 98.88% -> 99.41%, Loader 99.15% -> 99.48%, Sample 98.26% -> 98.44%,
and the remaining mean-luminance delta is < 0.2% on every screen except the
waveform pages, where it is content (waveform shape), not darkness.

Alpha *is* load-bearing in one firmware path worth remembering: `cPoints::update`
fills the loop/selection band with `BLEND_FUNC(SRC_ALPHA, ONE)` (additive). The
app approximates it by painting grey_0A underneath the spectrum, which is
equivalent because the band only covers black and white pixels.

### 2. Box overdraw: ImGui line fringe

`Canvas::line` used `ImDrawList::AddLine`, which in this ImGui version draws a
textured stroke with an anti-aliasing fringe that spills about a pixel outside
the segment - past the edge of the control that owns it. EVE (`LINE_WIDTH`)
rasterises hard edges: the emulator fills `[min(x), y - t/2, |dx| + 1, t]` for
orthogonal segments. `Canvas::line` now draws orthogonal segments as exact
device-pixel rects (inclusive endpoints) and disables `ImDrawFlags_AALines` for
diagonals. `test_canvas_lines.cpp` locks the behaviour.

### 3. Animation contract

`native/app/ui/animation.h` provides the shared clock (`setAnimationNowMs`,
`animationFrame`), easing, `Tween` and `Blinker`, plus the firmware timings
(`kLabelBlinkMs` 200, `kPlayheadBlinkMs` 500). Rules for new screens:

- sample animation state from `animationNowMs()` at draw time; never accumulate
  per-frame deltas, so any frame rate renders identical pixels;
- report animated controls via `Control::animatingAt()`; `AppLayout` then keeps
  the 60 Hz loop alive through `RenderScheduler::requestAnimation()`;
- captures (`--lcd-shot`, `--screenshot`, `--auto-test`, smoke) freeze the clock
  so reference output stays deterministic.

## Known residuals

- fwref multi-state coverage now includes Instrument Effects/Automation and the
  Config item editor (`config_edit.json`, 99.51%).
- The Perform page has no additional firmware grid page (the earlier residual
  was incorrect); the app models every firmware Performance control.
- Global Mixer (Master 3/3) is ported: aux volume/mute persist with the project
  and drive the mixer's delay/reverb return and dry levels; Line In remains a
  silent bus because the desktop build has no input monitoring. The Master EQ
  page is reachable from the EQ label on Master 1/3, exactly like the firmware.
- Sample Loader import progress is not a missing firmware screen: the importer
  never shows its cProgressCursor (gSampleImport.cpp:851, 857, 925 comment out
  show/refresh and hide it in hideImportPreview), so the direct desktop import
  is a workflow shortcut rather than a fidelity gap. The Config item editor is ported (below); settings the desktop
  engine does not consume (anti-aliasing, limiter mode, snap-to-zero, notes
  scale/root/layout, metronome, clock routing) update the config state but do
  not yet drive audio/MIDI behavior.
- Functional smoke is fully green; its comparisons now crop to the workspace so
  chrome-only changes (Undo availability, transport timer) are not mistaken for
  screen state. Fixed along the way: stop rewinds the transport row, Song delete
  returns the selection to the previous slot, and the Sample parameter selector
  wraps like the legacy screen.
- `ScreenButtons` deck labels sit at the firmware device centres (name 442
  two-line / 452 single, value 462) and honor `ScreenButton::dimText`
  (grey_33 name with the red underline still drawn, as in gSongEdit's
  Add/Delete labels). The Song screen's deck labels were previously missing
  entirely (the bitmap offsets were absolute and pushed them below the frame);
  Master 2/3 deck labels now carry the gMasterParams content ("%.2f dB" name,
  Unmute/Mute value) instead of Track N/dB.
- Chrome text is pre-blended against the flat title-bar (#080808) and deck
  (#0A0A0A) backgrounds with `fwDrawTextOn`, reproducing the emulator's integer
  blend; the title bar now matches the reference exactly (0 differing pixels).
- Chrome text is drawn with the firmware L4 bitmap fonts instead of ImGui
  TTFs: `HeaderBar` (title, transport with the Bold17 `\x18\x19` stop icons,
  right label) and the `ScreenButtons` deck labels (Regular14 name at y 442/452,
  Light17 value at y 462). This lifted Pattern 99.70, Instrument 99.80,
  Master 1/3 99.82, Master 2/3 99.53, Master EQ 99.78, Sample 99.46,
  Loader 99.53, Config 99.66 and Perform 98.55.
- Song reference corrected: `cSongPlayer`'s rotated track names were shifted one
  column left (the emulator matrix ignored the `(I - R) * h/2` translation of the
  rotate-about-centre idiom and the app measured glyph offsets from the wrong
  raster origin), pushing the first track name off the left edge and hiding the
  last glyphs under the title bar. Both the fwref matrix/shim and
  `Canvas::textVertical270` now compose the rotation correctly, the names match
  the hardware manual (full "Track 1".."Midi 16"), and the Song screen shows the
  transport status like every non-Pattern module (interface.cpp:417-421).
- Bottom shadows are 12 device px (`grad_y = bottom - 10`, `grad_h = 12`) and are
  theme-gated like the firmware: `bottomScreenShadow` for cList/cBar/cSongPlayer,
  `barShadow` for the bar gradient (themeDefault enables both). `ui::setChromeShadowFlags`
  mirrors the theme, so themes with shadows off no longer inherit them.
- Sample parity raised to 99.28% (slice/granular 99.22/99.16) by filling the LCD
  background black (the desktop clear colour used to show through uncovered
  pixels), matching cGraphicProcessing's 6-sample column resolution and int8 peak
  scale in the demo sample, rounding EVE line endpoints, and aligning the fwref
  scenario markers/region to the demo project's frames (the stopped editor has
  no progress cursor). Slice/granular states now unhide `cSlicePoints`
  (`specs/sample_slice.json`) and keep cPoints' loop region visible in every
  play mode (points.cpp `pointsType`), matching the firmware; the granular
  cLineIndicator grain window is drawn by the desktop from the gSamplePlay
  formula but is still absent from the fwref reference (emulator compositing
  gap).
- Calibrated hardware tiles exist for seven views (including Master EQ), not every sub-page.
- ~~Header song timer.~~ Resolved: `Interface::activateModule()` shows
  `songTimer` on every module except the pattern editor
  (`interface.cpp:418-443`), and the Master EQ hardware photo confirms it. The
  timer entry now lives in the shared chrome spec, every non-Pattern screen
  draws it, and its value comes from the audio clock
  (`PlayheadPosition::songSeconds`) instead of accumulated frame deltas.

## Handoff

- Run `cmake --build native/build -j8`, `./native/build/trakr_tests`, and
  `./native/build/trakr_app --auto-test` before changing UI code.
- Screens live in `native/app/ui/screens/*` and mirror
  `core/interface/*/g*.cpp`; controls live in `native/app/ui/controls/*`.
- Use `--lcd-shot --screenshot` plus `compare -metric AE` against
  `render/fwref/*.png` (regenerate with `fwref_gen --strict`).

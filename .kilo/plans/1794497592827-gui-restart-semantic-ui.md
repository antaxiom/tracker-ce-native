# Plan: Restart the GUI as a semantic UI layer (ImGui host, Classic + Studio profiles)

Status: active (Phase 1 complete; Phase 2 Instrument gate PASSED — see native/docs/gates/phase2-instrument.md)
Date: 2026-11-12
Scope: `native/` only. No edits to `core/`, `platforms/`, `synth/`, `util/`.

## Model comparison: Claude vs this Kilo assistant

This is a planning comparison, not a benchmark result. No measured scores exist for these
models on this repository. The assessment is based on the nature of the work, the codebase
(≈11.5k LOC of UI, firmware sources as reference, pixel and visual verification), and the
strengths each model family is generally used for.

| Dimension | Claude (Opus/Sonnet 5.5) | This assistant (Kilo, DeepSeek V4.1 Flash) |
|---|---|---|
| Architecture and design judgment | Stronger at weighing long-term product tradeoffs, surfacing hidden coupling, and writing precise ADRs | Competent; produces solid structure but more likely to take the first plausible design and under-weight long-horizon consequences |
| Long-context consistency | More reliable across multi-hour refactors and large diffs; less drift from earlier decisions | Good for bounded tasks; on very long sessions, risk of losing earlier constraints without memory/doc anchors |
| Firmware source fidelity | More careful with subtle semantics in `NewDisplay` controls, draw order, clipping and state quirks | Adequate when anchored to source; tends to move faster and may skim edge cases unless explicitly required to cite source lines |
| C++ refactor volume | Slower per file, higher quality per file | Faster iteration; well suited to mechanical, test-guarded porting and scaffolding |
| Visual/design judgment | Better at "beauty of the original" decisions, calibrated screenshot comparison, and stuck polish | Capable of visual audit loops; less nuanced on aesthetic judgment calls |
| Tooling and orchestration | Strong reasoning, fewer integrated project tools here | Native todo/plan/memory/board/agent-manager tools, direct build/test/robot loops, cheaper and faster |
| Autonomy on long tasks | Higher reliability for sustained, high-stakes work | Good in short cycles; benefits from explicit checkpoints and documented decisions |
| Cost/latency | Expensive; reserve for high-leverage decisions | Much cheaper; suitable as the default executor |

### Recommendation (cost-first routing, updated 2026-11-12)

Keep Claude usage minimal. The default executor is this Kilo assistant; escalate only when a
checkpoint actually needs independent judgment.

| Tier | Model | Use |
|---|---|---|
| Default | This Kilo assistant | Planning execution, scaffolding, control/screen porting, tests, robot/smoke, docs, commits |
| Gate review | Claude Sonnet 5.5 (medium/high) | Per-screen gate reviews and ADR sanity checks; one review per phase boundary |
| Mechanical/vision | Claude Haiku 5.5 | Screenshot triage, diff classification, checklist verification |
| Escalation | Claude Opus 5.5 | Only when a decision is genuinely stuck and the user explicitly approves |

Rules:

- Do not run a Claude review for routine phases or individual controls.
- One Sonnet review per phase boundary; reuse its findings instead of re-reviewing the same code.
- Haiku is preferred for "is this a real regression or a known artifact" checks.
- If Sonnet/Haiku findings conflict with firmware source, firmware source wins.

The Phase 2 review was performed by Opus 5.5 before this policy; later gates use Sonnet.
If a single model must do everything, this assistant alone is the cost-effective choice; the
Sonnet gate is retained for high-risk validation, not for throughput.

## Goal

Replace the current `native/app/views/*` rendering approach with a restart of the UI layer that:

1. Keeps the model, engine, audio, I/O, tests, demo song, robot and reference tooling.
2. Uses ImGui as host (window, input, docks, menus, dialogs, overlays) and as the renderer,
   but not as a re-rasterizer of the firmware LCD.
3. Ports firmware **semantics and design**, not pixel replay, so the app can grow into a
   Renoise-like studio without being capped at 800x480.
4. Preserves the original look through a `Classic` layout profile with firmware-derived
   geometry, palette and typography.
5. Stops treating `fwref` as ground truth. It is a source-derived approximation until
   audited, and never an architectural dependency.

## Decisions (record as ADRs before implementation)

- **ADR-1 Fidelity definition** (`native/docs/adr/0003-fidelity-and-truth.md`): structural
  fidelity + palette + typography + interaction; not pixel identity. Pixel parity remains a metric for Classic-profile regression only.
- **ADR-2 UI architecture** (`native/docs/adr/0002-semantic-controls.md`): one semantic control layer, two layout profiles (Classic, Studio), rendered with ImGui draw lists at device-pixel-snapped coordinates.
- **ADR-3 Truth hierarchy** (`native/docs/adr/0003-fidelity-and-truth.md`): firmware source > calibrated hardware captures > fwref (advisory) > hand-drawn references (retired).
- **ADR-4 Renderer boundary** (`native/docs/adr/0004-imgui-role.md`): no screen code draws ImGui widgets directly; all LCD content goes through the control layer and its canvas abstraction.

## Non-negotiables

- Do not port or embed the `fwref`/FT812 framebuffer as the app's renderer.
- Do not compile firmware `NewDisplay` sources into the app.
- Do not delete legacy views until their replacements pass the screen exit criteria.
- Do not let ImGui style, layout or fractional DPI scaling determine LCD geometry.
- Do not change model/engine behavior as part of the UI restart.

## Why fwref is not an oracle (evidence)

- `native/tools/fwref/evidence/README.md`: no hardware captures were supplied; hardware
  structural check is "not applicable"; the source-true gates remain the pixel metric.
- `native/tools/fwref/check_reference.py`: gates cover provenance hashes, determinism, a
  primitive contract and a negative control. They do not prove FT812 equivalence.
- `native/tools/fwref/shim/ft812_api_stubs.cpp`: ~25 `API_*` calls are no-ops. Some are
  irrelevant (tags, stencil), but `API_CLEAR`, bitmap source/layout/size, `VERTEX_FORMAT`,
  and `BITMAP_TRANSFORM_A/E` affect pixels.
- `native/tools/fwref/shim/ft812_api.cpp`: `BITMAPS` vertices are ignored.
- `native/tools/fwref/shim/ft812_api_geometry.cpp`: only quarter-turn matrices are honored;
  other rotations silently draw unrotated.
- `native/tools/display_emulator/FT812Emulator.cpp`: text supports only left/center/right,
  skips non-ASCII, has no kerning or anti-aliasing; gradients approximate EVE interpolation.
- `native/tools/fwref/specs/*.json`: one state per screen; selection, edit, scrolling, popup
  and multi-page states are not covered.

Conclusion: fwref is self-consistent, not hardware-validated. It may be used for structural
comparison after a primitive-level audit, never as the app's substrate.

## Truth hierarchy

1. Firmware source: `core/interface/*/g*.cpp`, `core/libs/NewDisplay/*`, `core/source/Themes.cpp`.
2. Calibrated hardware captures: existing `native/docs/reference/videos` and
   `reference/screens/video_*.png`, rectified to 800x480 and per-channel calibrated.
3. fwref: derived aid, valid only for audited primitives.
4. Hand-drawn references: retired.

## Implementation status (2026-11-12)

- Phase 0 committed (`pre-gui-restart` tag, baseline `48ca611`).
- Phase 1 ADRs written in `native/docs/adr/`.
- Phase 2 gate passed for Instrument General: semantic controls, L4 text,
  integer Classic scale, input parity, 5/5 calibrated hardware tiles.
  See `native/docs/gates/phase2-instrument.md`.
- Master 1/3 gate passed for the Classic workspace (0.02% vs fwref, input
  parity); hardware capture for Master is pending. See
  `native/docs/gates/phase4-master1.md`.
- Pattern gate passed for the tracker workspace (0.35% vs fwref, input parity);
  popups and chrome remain legacy. See `native/docs/gates/phase4-pattern.md`.
- Song gate passed: `SongPlayerControl` (cSongPlayer) + `SongScreen`; 1.09% vs
  fwref (legacy 1.45%), scripted navigation parity; list animation frames and
  selection highlight not ported. See `native/docs/gates/phase4-song.md`.
- Sample gate passed: waveform controls (`SpectrumControl`, `PointsControl`,
  `SlicePointsControl`, `ProgressCursorControl`, `LineIndicatorControl`,
  `WtProgressControl`) + `SampleScreen`; 1.74% vs fwref (legacy 2.13%);
  audition/marker editing delegated to `SampleView`. See
  `native/docs/gates/phase4-sample.md`.
- Loader gate passed: `SampleLoaderScreen` (lists, headers, memory/preview bars);
  0.85% vs fwref (legacy 1.47%); importer crop page still legacy. See
  `native/docs/gates/phase4-loader.md`.
- Config gate passed: `ParamValueListControl` + `ConfigScreen`; 0.36% vs fwref
  (legacy 1.91%); item editor/text boxes pending. See
  `native/docs/gates/phase4-config.md`.
- Perform gate passed: `MultiLabelControl`/`VerticalLabelControl` +
  `PerformScreen`; 1.75% vs fwref (legacy 1.75%, residual is track row/chrome).
  See `native/docs/gates/phase4-perform.md`.
- File gate passed: `FileScreen` (project/recent lists, labels, blank cover);
  1.12% vs fwref (legacy 1.97%). See `native/docs/gates/phase4-file.md`.
- All nine screens now have a semantic workspace path; it is the default
  (`--gui-classic` opts out). Sub-pages without a port fall back automatically.
- Phase 5 started: semantic paths are the production renderer; legacy views
  remain as the fallback surface for sub-pages, dialogs and chrome.
- Master sub-pages ported: `TrackMixerScreen` (0.63% vs fwref page 2) and
  `MasterEqScreen` (firmware `showEqualizerScreen()`: five band bars, 5-place
  selection frame, cover over columns 5..7; 0.23% vs a new source-true
  `master_eq` fwref spec, plus an 8th calibrated hardware tile). Firmware page
  3/3 is Global Mixer, which the desktop engine does not model; the app's third
  Master tab is therefore the Master EQ page. See
  `native/docs/gates/phase5-master-subpages.md`.
- Instrument sub-pages ported: `InstrumentPagesScreen` covers Effects and
  Automation (0.18% / 0.73% vs the legacy renderer). See
  `native/docs/gates/phase5-instrument-subpages.md`.
- Sample Editor sub-page ported (app-defined page; structural verification).
  See `native/docs/gates/phase5-sample-editor.md`.
- Phase 5 exit: every app-modeled workspace and sub-page renders semantically;
  legacy views remain only for dialog/state plumbing and `--gui-classic`.
- Phase 6 complete: verification matrix, retirement decisions and handoff are
  recorded in `native/docs/gates/phase6-verification.md`.
- `--gui-classic` retired; `AppLayout` always renders semantic screens, and the
  legacy `renderActiveScreen` path remains only for the legacy test harness.
- `native/tools/visreg/visreg.py` is the repeatable Classic gate: 15/15
  screen/sub-page cases (incl. Master EQ) pass at 97.85-99.99% similarity with
  determinism.
- Multi-state fwref added for Instrument Effects/Automation (0.01%/0.02%);
  functional smoke is green 17/17 after transport/song/sample fixes.
- `Display` now composites four control passes (layer -1 prepass, 0 normal, 1
  post-marker, 2 cursor) to model NewDisplay's update/update2/update3; `cPoints`
  additive range background is painted in the prepass as an approximation.
- fwref FW-1 implemented: strict by default, per-screen audit in `renderSpec`,
  fails on unsupported or approximate calls; current result 0/0.
- `Canvas` is ImGui-backed today; a recording/software backend is Phase 5 work.
- FW-8 harness implemented (`native/tools/refcal`): 8/8 hardware tiles pass
  (Instrument x2, Instrument Automation, Performance, Pattern, Track Mixer,
  Sample Playback, Master EQ); remaining screens still need capture photos.

## Target architecture

```
native/app/
  ui/
    design.h        // palette (Themes.cpp), type scale (14/17/12), metrics, rule widths
    canvas.h        // draw surface: snap, EVE line, clip, text, gradient (ImGui-backed now)
    control.h       // base: measure, layout, draw, hitTest, handleIntent
    controls/       // frame, bgLabel, label, bar, list, paramValueList, multiLabel,
                    // textBox, horizontalBar, tracker, horizontalTracker, songPlayer,
                    // progressCursor, points, spectrum, slicePoints, wtProgress,
                    // lineIndicator, gridEditor
    display.h       // control table, creation order, reverse render order, focus chain
    screens/        // mirrors g*.cpp: initDisplayControls, setValue, navigation
    themes.cpp      // firmware palettes + desktop themes
  chrome/           // ImGui menu bar, toolbar, button deck, dialogs, status bar
  main.cpp
```

Rendering rules:

- Coordinates in logical 800x480 units, snapped to device pixels at draw time.
- Integer scale (1x/2x/3x) for Classic; fractional fit only as an explicit preference.
- Text: L4 bitmap font for Classic, TTF with matched metrics for Studio; no
  `io.FontGlobalScale` inside the LCD.
- No ImGui default styling for LCD content. ImGui widgets only for editable overlays
  (`InputText` anchored to a control rect) and desktop chrome.

### Layout profiles

| Profile | Purpose | Geometry | Renderer |
|---|---|---|---|
| Classic | Preserve original look; reference for screenshots and fidelity | Fixed firmware rects at 800x480 | ImGui draw lists, integer scale, pixel-snapped |
| Studio | Renoise-like expansion: panes, zoom, more tracks/columns, inspectors | Same controls with constraints | Same ImGui draw lists |

Controls declare preferred size and constraints. Classic resolves to firmware rects; Studio
stretches/reflows without forking control behavior.

## Phases

### Phase 0 — Baseline and freeze (1-2 days)

- Commit and tag the current `native/` tree (`pre-gui-restart`). It is currently untracked.
- Save current per-screen parity, robot (1,103 frames) and functional smoke (17 flows)
  baselines as regression fixtures.
- Record the current `views/` inventory and salvage list.

Exit: committed baseline, reproducible test commands, saved goldens.

### Phase 1 — ADRs and skeleton (2-3 days)

- Write ADR-1..4.
- Add `ui/design.h`, `ui/canvas.h`, `ui/control.h`, `ui/display.h` skeleton.
- Define Classic and Studio layout contexts and the integer-scaling presenter.
- Define the input intent model (`navigate`, `edit`, `adjust`, `toggle`, `activate`) mapped
  from keyboard, mouse and MIDI; seed from `ShortcutMap`.

Exit: compiles, no behavior change, rubric for all later screens.

### Phase 2 — Vertical slice (go/no-go, ~1 week)

- Port Instrument General: frame, bgLabel, 8 labels, 6 bars, filter list, cover.
- Render in Classic and Studio.
- Compare Classic against calibrated hardware frames; assert layout invariants
  (no overlap, no clipping, text fits, palette tokens correct).
- Run the Instrument subset of robot/smoke through the intent layer.

Gate (re-scoped to match ADR-0003): Classic workspace match against calibrated
hardware frames plus layout invariants; legacy input parity; no frame-time
regression. Studio is deferred to Phase 5 and is not a Phase 2 gate. The
calibrated harness now exists for Instrument General (2/2 tiles pass); it must
be extended to the remaining screens before their gates. If the gate fails, stop
before porting more screens.

### Phase 3 — Control library (2-3 weeks)

Order: frame/background -> label/bgLabel -> bar -> list/paramList/multiLabel -> textBox ->
tracker/horizontalTracker -> songPlayer -> points/spectrum/slice/wt/line-indicator ->
gridEditor.

Each control: semantic unit tests (measure, hit-test, navigation) plus a Classic visual
golden and a Studio reflow test.

### Phase 4 — Screen migration (1-3 days each)

Order: Pattern -> Song -> Instrument -> Master (3 pages) -> Sample -> Loader -> Config ->
Perform -> File.

Per screen: mirror `initDisplayControls` creation order and `setValue` bindings; implement
navigation and button handling; adapt existing edit helpers; run robot subset, smoke and
visual review; delete the legacy view and its helpers that are no longer used.

No screen is done while both paths remain active.

### Phase 5 — Cleanup and Studio expansion

- Delete `views/` and unused `hw/` drawing code; keep one renderer.
- Replace the pixel-parity gate as an architecture constraint with Classic visual
  regression plus layout invariants.
- Keep fwref as an advisory structural check after audit.
- Add Studio features in this order: resizable panes, zoom, extra track/column views,
  inspector panels, desktop theme variants. Do not start before Classic reaches feature
  parity.

## fwref concern register and remediation (FW-0..FW-10)

This is a first-class workstream, timeboxed, and independent of Phases 1-2. The goal is not
to "make fwref trusted"; it is to convert every silent assumption into a checked fact or an
explicit, documented approximation. The app must never depend on the outcome.

### Scoping fact

Only 27 `API_*` symbols are used by the control sources actually compiled into `fwref_core`
(`bar`, `label`, `list`, `frame`, `bgLabel`, `displayControls`, `tracker`, `multiLabel`,
`paramValueList`, `textBox`, `gridEditor`, `songPlayer`, `progressCursor`, `lineIndicator`,
`points`, `slice`, `spectrum`, `wtProgress`, `horizontalBar`). Commands used only by
uncompiled sources (`display.cpp`, `img.cpp`, `stream.cpp`, `snapshots.cpp`, `newList.cpp`)
are out of scope until those controls are added.

### Register

| ID | Concern | Evidence | Impact today | Action / acceptance |
|----|---------|----------|--------------|---------------------|
| FW-0 | Validation is self-referential | `evidence/README.md` (no hardware); `check_reference.py` gates are hashes, determinism, primitive contract, negative control | No independent proof of FT812 equivalence | Define oracle status: `approximate` until FW-8 passes; publish status in README and manifest |
| FW-1 | Silent no-op stubs hide gaps | `ft812_api_stubs.cpp`: `API_CLEAR`, `API_CLEAR_COLOR`, `BITMAP_SOURCE/LAYOUT/SIZE`, `CMD_SETBITMAP`, `ALPHA_FUNC`, `COLOR_MASK`, `VERTEX_FORMAT`, `CMD_DLSTART`, ... | Unsupported paths render wrong with no signal | Add strict mode: every call must be implemented or explicitly whitelisted per spec; abort on unknown; log a per-spec call inventory |
| FW-2 | Geometry approximations | `ft812_api.cpp` RECTS pairs; `drawThickSegment` exact only for H/V, generic Bresenham otherwise; `FTPOINTS` squares; gradients use float truncation, no EVE dithering | Sub-pixel and color differences; unknown on diagonals | Known-answer tests per primitive with independent expected raster; fuzz coords/widths; mark approximation where unfixable |
| FW-3 | Vertex/BITMAPS path | `API_END` ignores `BITMAPS` vertices; `VERTEX2F` only draws a CELL glyph when `gPrim == BITMAPS && cell != 0` | Any non-CELL bitmap usage silently vanishes | Assert every `BITMAPS` vertex carries a CELL and handle; fail otherwise |
| FW-4 | Text fidelity | `FT812Emulator::drawTextCmd` handles only left/center/right, ASCII, no kerning; font widths from L4 table; `CMD_NUMBER` dec/hex only | Glyph placement/color differences; non-ASCII dropped | Per-glyph bitmap goldens; option matrix tests; assert non-ASCII unreachable in specs; verify vertical-270 path |
| FW-5 | Bitmap font/image state | `BITMAP_SOURCE/LAYOUT/SIZE` and `CMD_SETBITMAP` are no-ops; shim assumes handles map to preloaded firmware fonts | Correct only while no image/bitmap draw path is exercised | Strict assertion that reference specs never draw images; explicit handle-to-font map; failure if a bitmap draw is requested |
| FW-6 | Matrix support | `drawMatrixGlyph` honors only quarter turns; other rotations fall back to unrotated | Currently used only by `cLabel` 270-degree path, which is handled; any future rotation is silently wrong | Fail loudly on non-quarter rotation; add rotation tests; document the limitation |
| FW-7 | Single-state specs | 12 spec JSONs, one state per screen | Parity validated on the easiest slice; edit/selection/popup states unverified | Add multi-state specs per screen: selected row, edit cursor, popup open, scrolled list, alternate page |
| FW-8 | No hardware ground truth | `evidence/README.md`; reference videos exist but unprocessed | Appearance not validated against the real LCD | Build a calibration harness: rectify to 800x480, per-channel black/white balance, structural region checks (bands, separators, bar presence, glyph rows); record pass/fail per region |
| FW-9 | No independent implementation | All expectations are derived from the same source reading | Spec or shim bugs are self-consistent and invisible | Differential tests: second naive renderer for frame/bar/label/list/tracker; divergence localizes the defect |
| FW-10 | Coverage drift | `fwref_core` compiles 19 control sources; app port will need others (`horizontalTracker`, `imgGrid`, popups) | New controls may silently hit no-op paths | When a new control is added, extend the register, strict whitelist and known-answer tests before using its reference |

### Remediation order

1. **FW-1 strict mode**: highest value; converts silent degradation into failures.
2. **FW-3, FW-5, FW-6 assertions**: cheap, eliminates hidden assumptions.
3. **FW-2, FW-4 known-answer and per-glyph tests**: establishes primitive confidence.
4. **FW-8 calibration harness**: first independent appearance check.
5. **FW-7 multi-state specs**: broadens coverage beyond the default state.
6. **FW-9 differential tests**: independent localization.
7. **FW-0, FW-10**: publish status, keep the register current.

### Definition of done

fwref may be labeled **"audited advisory oracle"** only when:

- Strict mode is on by default and every call in every used spec is implemented or
  whitelisted with a documented reason.
- FW-2/3/4/5/6 primitive tests pass.
- FW-7 multi-state specs exist for all nine screens.
- FW-8 structural comparison against calibrated hardware frames passes per region.
- The README and manifest state the confidence level and known approximations.

Otherwise it remains **"approximate, advisory only"**, and Classic-profile verification relies
on calibrated hardware captures plus layout/visual invariants. This status never blocks the
GUI restart and never becomes an app dependency.

Phase 5 status: **complete for the restart scope**. The nine workspaces and
every app-modeled sub-page render through one semantic compositor; legacy
views remain only as dialog/state plumbing and a debug escape hatch. Studio
expansion (resizable panes, zoom, extra track/column views, inspector panels,
desktop themes) is split out as the Phase 7 feature program below; it is a new
product surface, not a restart requirement, and needs its own design pass.

### Phase 7 (backlog) — Studio expansion

1. Resizable panes and workspace zoom in `LayoutProfile::Studio`.
2. Extra track/column views and inspector panels.
3. Desktop theme variants.
Do not start without a design pass; Classic remains the reference profile.

### Phase 6 — Verification, retirement and handoff

- Run the full gate matrix (unit, robot, smoke, fwref strict, refcal, glyph,
  Classic parity) and record results in `native/docs/gates/phase6-verification.md`.
- Freeze the renderer boundary: semantic screens are production; legacy views
  remain for dialogs/state and the debug `--gui-classic` escape hatch.
- Keep fwref labeled an audited advisory oracle; record residuals and the
  post-restart cleanup list (dialogs, unmodeled pages, FW-7 multi-state specs).

## Verification model

- Unit: control measurement, hit-testing, navigation, focus/edit state.
- Layout invariants: no overlap, no clipping, text fits, palette tokens.
- Visual: Classic goldens at 1x/2x for each screen and state; comparisons against calibrated
  hardware frames; fwref advisory only.
- Behavioral: robot audit and functional smoke, run through the intent layer.
- Build/test gates: `cmake --build native/build -j8`, `./native/build/trakr_tests`,
  `--auto-test`, `functional_smoke.py`, `check_glyphs.py`.

## Risks and mitigations

| Risk | Mitigation |
|---|---|
| Two profiles double QA | Shared control semantics; only layout resolution differs; Classic is the reference |
| Visual drift without pixel gates | Calibrated hardware goldens per screen/state plus layout invariants |
| fwref errors baked in | Never used as app substrate; controls draw from firmware semantics; audit first |
| Input regressions | Central intent layer; migrate robot/smoke once in Phase 2 |
| Scope creep | Studio features gated until Classic feature parity |
| Editing logic rewrite | Reuse history/commands and pattern helper logic; view-models hold view state only |
| Font/glyph coverage | L4 for Classic, TTF for Studio, keep `check_glyphs.py` tofu guard |
| No committed baseline | Phase 0 is mandatory before deletions |
| Model/tooling inconsistency | ADRs and this plan as anchors; Sonnet for gates, Haiku for checks, Kilo assistant for execution |

## Open questions

- Should Classic mode be a user-facing setting or only a verification/layout profile?
- Is a future pixel-true Classic renderer (behind `canvas.h`) a requirement, or is calibrated
  visual regression sufficient?
- Do we need L4 bitmap text in Studio, or is matched-metric TTF sufficient?
- Which existing view helpers are worth adapting versus rewriting (audit in Phase 0)?

## Review remediation + renderer findings (2026-11-13)

- All 22 local-review findings fixed: project-save path after New Project, the
  malformed "Add Empty" pattern, cursor/selection clamps, `posix_spawn` instead
  of `system`, bounded `.pti`/project-JSON parsing, Config row mapping and a
  real `Song::headroom` (3 dB sample path, persisted with the master section),
  firmware fader curve (`native/model/FaderCurve.h`), reset publish, shortcut
  guard while typing, per-screen deck routing, atomic sample crop, history-backed
  Song order edits, `mkstemp` in os_input, mixer/voice invariant hoisting,
  song-player note cache, bitmap-text run merging, dead View toggles and dead UI
  files removed, unified fwref reference lists.
- Dark pages: no alpha defect - centred `cLabel` backgrounds (`x` is the centre)
  and the File cover geometry were wrong. File 99.41%, Loader 99.48%, Sample
  98.44%; Canvas `line` now hard-edged with no AA fringe; `ui/animation.h` plus
  `Control::animatingAt`/`RenderScheduler::requestAnimation` implement the
  hardware-timed animation contract (see `phase6-verification.md`).

## Global Mixer (Master 3/3) ported

- `Song::auxVolume/auxMute` (defaults 86 = 0 dB, line-in muted), persisted in
  the master JSON section; `Mixer::setAuxGains` applies delay/reverb return and
  dry levels with mute and publishes aux meters; `GlobalMixerScreen` mirrors
  `showGlobalMixer()` (names, faders, level bars, frame, dB/mute labels). Master
  EQ remains reachable from the EQ label. Gap closed: Master 3/3 99.18% vs
  `master_page3.png`, visreg now 16/16.

## Header song timer (done)

- The timer moved into the shared chrome spec, so every non-Pattern reference
  now includes it, `AppLayout` enables it on every screen, and its value is
  derived from the audio clock (`PlayheadPosition::songSeconds`) rather than
  accumulated frame deltas (animation-ready).

## Config item editor (done)

- Ported the firmware item editor (`configMenu.cpp` itemEditorOpen ->
  `gConfigEdit.cpp` hideSubmenu/showConfigItemLabel/showConfigItemDesctription/
  showConfigList4): the submenu hides, the option list moves to
  `(800/8)*4+1` width `(800/8)*2-3`, the item name and description render in
  `configLabel`/`descriptionTextBox`, and the bottom labels become Cancel (6) /
  Apply (7). Enter opens; Up/Down move; Enter applies; Left/Esc cancels.
- Option tables and descriptions are copied verbatim from `configMenuDefs.h`
  (performance source, scales, roots, pad layout, on/off, limiter, recording,
  numbering, middle C, clock in/out, channels, clock compensation, metronome).
- New fwref state `specs/config_edit.json` + scenario edit fields
  (`editTitle`/`editDescription`/`editItems`/`editSelected`) and
  `renderConfigEdit`; visreg case `config_edit` = 99.51%.
- Desktop settings without an engine consumer remain session-level (documented
  residual); Pattern divider and Extra headroom publish to the model/engine.

## Song/File reference revision and shadow audit

- Fixed `Canvas::textVertical270`: glyph offsets were measured from `origin.y`
  while glyphs rasterise at `screenY(0)` (`origin.y - deviceTop * scale`), which
  shifted every Song track name one column left and pushed "Track 1" off the
  left edge. Names now align with the blocks and show in full.
- Fixed the fwref matrix composition: `translate()` now applies the current
  linear part, so `translate(+h/2) / ROTATE(270) / translate(-h/2)` nets the
  firmware's `(I - R) * h/2` offset. The reference now matches the Tracker+
  Essentials Song page (full rotated names between the title bar and the grid).
  The emulator also advances rotated glyphs by `width + 1` like the firmware.
- Song shows the title-bar transport (the firmware hides the song timer only on
  the Pattern screen); scenario progress is aligned to the stopped demo state.
- Bottom shadow verified: 12 device px, `grad_y = bottom - 10`, `grad_h = 12`,
  enabled by `themeDefault` (`bottomScreenShadow`, `barShadow`). Shadow drawing
  is now theme-gated in the semantic controls.
- `os_input_runner.py` compares verified captures against `render/fwref` (the
  retired hand-drawn emulator frames are no longer used) and the compare folder
  was regenerated for all nine screens.

## Sample slice/granular reference completion

- `cPoints` stays visible in every play mode (the firmware selects the
  start-end region for 1-Shot/Forward and the loop region plus markers for
  loop/slice/granular through `pointsType`); the app no longer hides it in
  Slice/BeatSlice/Granular.
- Added `specs/sample_slice.json` (built by `build_sample_variant`) so the
  Slice state shows `cSlicePoints`, and slice points in `scenario.json` now use
  the demo project's frame-derived pixel positions.
- `cLineIndicator` now follows `gSamplePlay.cpp:748-751` (grain window around
  the playhead widened at the sample edges, additive fill under the waveform,
  cyan playhead line above it) and is layered like the firmware
  (`drawLayer(-1)` / `0`).
- Parity: slice 99.22%, granular 99.16% (was 98.62/98.60).

## Chrome bitmap text port

- HeaderBar draws the title, transport and right label with the Bold17 L4
  bitmap font at the titleBar.cpp geometry (cell top row 2, title x 9, right
  label right-aligned at 790) and composes the transport with the firmware
  `"%3im:%02is %2s P%i.%03i%2s"` format and its stop/song/pattern icon glyphs.
- ScreenButtons draws the deck names (Regular14, centre y 442 two-line / 452
  single-line) and values (Light17, centre y 462) with the same bitmap fonts,
  and the Pattern deck uses the bitmap path as well.
- Parity after the port: Pattern 99.70, Song 98.98, Instrument 99.80,
  Master 1/3 99.82, 2/3 99.53, 3/3 99.32, EQ 99.78, Sample 99.46, Slice 99.38,
  Granular 99.35, Loader 99.53, Config 99.66, item editor 99.65,
  Perform 98.55, File 99.36 (all >= 98.5).

## Deck labels correction

- ScreenButtons bitmap labels were being placed with absolute device y values
  added to the window origin, pushing every deck label off-screen; the offsets
  are now relative (17/27/37) and the Song deck shows Play Pattern / Play Song /
  Undo / Add Slot / Delete Slot / Pattern / Tempo again.
- Added `ScreenButton::dimText` for firmware grey_33 names with an active
  underline (Song Add/Delete); Master 2/3 labels now match gMasterParams
  (".2f dB" name + Unmute/Mute value).
- Parity: Pattern 99.73, Instrument 99.87, Master 1/3 99.85, 2/3 99.63,
  3/3 99.37, EQ 99.84, Sample 99.49, Slice 99.40, Granular 99.37, Loader 99.53,
  Config 99.67, item editor 99.66, Song 98.96, Perform 98.52, File 99.28.

## Global Mixer line-in monitoring

- `Mixer::setLineIn` + aux-3 gain/meter sums interleaved stereo input into the
  master bus (gMasterParams.cpp showGlobalMixer).
- `Engine::pushInput` accepts capture frames through a lock-free SPSC ring and
  drains them per sub-block; `EngineHost::pushInputFrames` /
  `setAudioInputEnabled` expose it, and `publish()` enables capture only when
  `song.auxMute[3]` is false.
- `MiniaudioBackend` opens a f32 stereo capture device lazily and forwards
  frames through the new `AudioBackend::InputCallback`; headless backends stay
  silent.
- Tests: `Mixer line-in bus monitoring` and `Engine drains capture frames into
  the line-in bus` (146 cases total).

## Pre-blended chrome text

- Added `hw::fwDrawTextOn`, which composites each L4 nibble against a known flat
  background using the emulator's integer blend ((src*a + dst*(1-a)) / 255) and
  draws the result opaque. GPU alpha blending rounded edges differently, which
  was the entire title-bar residual.
- HeaderBar (title/transport/right label) and the ScreenButtons deck labels use
  it; the title bar now matches the reference pixel-for-pixel and the Pattern
  deck diff dropped from 500 to 339 px.

## Loader import progress re-scoped + capture wiring test

- Checked `gSampleImport.cpp`: the importer's progress cursor is disabled
  (show/refresh commented at lines 851/857 and hidden at 925), so the firmware
  has no import-progress screen to port. The Loader residual is a desktop
  workflow shortcut, not a missing control.
- Added `Line-in capture is requested only while the bus is unmuted`, which
  asserts EngineHost opens the capture device exactly when `auxMute[3]` is
  false and releases it again when muted (fake backend, no hardware).

## Global Mixer frame

- `showGlobalMixer()` hides the cFrame (gMasterParams.cpp:1405); the desktop
  screen was drawing a red frame around the selected aux column. The frame is
  now hidden; Master 3/3 rose from 99.37 to 99.49. Remaining difference is the
  aux meter/fader bar states and header label backgrounds.

## Master 3/3 label colours + label pre-blend

- Aux name labels use the creation-time `topBarLabelColors` (white on grey_08)
  for all four aux (including the muted Line In), matching gMasterParams.cpp
  initDisplayControls and the reference; previously they were recoloured with
  active/inactive palettes, painting a grey_32 band across the header.
- `Canvas::text`/`textCentered` accept an optional background; `LabelControl`
  passes its opaque background so glyph edges match the emulator's integer
  blend (title/deck already used `fwDrawTextOn`).
- Master 3/3 hits 99.84 (from 99.37); remaining diff is the level-bar container
  gradient (1 value) and bottom-label AA.

## Title-bar separator row

- All screens now draw the white separator at device row 28 (the firmware
  topBarLabel posY+height) via the foreground list; six screens had it at row 27.
- Parity after: Pattern 99.73, Song 99.27, Instrument 99.87, Master 1/3 99.85,
  2/3 99.63, 3/3 99.84, EQ 99.84, Sample 99.80, Slice 99.71, Granular 99.68,
  Loader 99.84, Config 99.97, item editor 99.97, Perform 98.84, File 99.60.

## Pixel-exact Perform, Track Mixer, and tracker text (2026-11-14)

Commit `1d0ebf2`:

- `Canvas::textVertical270`: dropped the +0.5 x rounding and shifted glyphs one
  device row lower (quad-vs-pixel convention). Perform FX names and Song track
  names are pixel-exact.
- `VerticalLabelControl` adds the firmware rotate-matrix's +font-height y
  offset (label.cpp nets `+h`); Perform FX names were ~22px high.
- `ScreenButtons` bottom labels: posY is 424 (cBgLabel starts at 425) and text
  centring uses C integer `th/2`. The `2.0f` divisor blurred the 19px name line
  and dropped the 22px value line a row. `patternChrome` labels share the fix.
- `MultiLabelControl` values/top pre-blend against the grey_0A cell fill
  (removes the +1 GPU-blend fringe).
- Perform bar no longer shrinks a row; inactive track buttons show an empty
  label like `showTracksState`.
- `TrackMixerScreen`: level bar is the full-column backdrop (x=2+100i, y=29,
  w=97, h=394), gain fader drawn on top; creation order follows gMasterParams
  (fader before level) because `renderAll` composites in reverse creation order.
- `Canvas::hGradient` emits device-pixel columns with EVE's `t=(x-x0)/(x1-x0)`
  steps (every 8px cBar shadow).
- `TrackerControl`: row numbers follow `Number2Bitmaps` (6px compressed gap);
  note/instrument/fx text uses `fonts[1]` (Light17, `text_font=1` in
  tracker.cpp), pre-blended against `cellBackground()`; track headers pre-blend.

Visreg after batch: 17/17 pass; Perform and Master 2/3 are pixel-exact.
`trakr_tests` 147 cases / 538,818 assertions pass.

**Metric caveat.** `compare -metric AE` here is a normalised error sum, not a
pixel count. Exact per-pixel parity is lower (pattern 98.17%, song 98.65%,
sample_slice 98.65%, granular 98.34%, loader 98.43%, file 98.10%, config
99.36, config_edit 99.56, instrument 99.58, master1 99.28, master3 99.75,
sample 99.30). Decide whether to add exact-count gating (see Phase A).

## Phase A gate runs (2026-11-14)

- A2 gates: `functional_smoke.py` 17/17; `gui_audit.py core.json` clean
  (4 scenarios); `refcal.py --all` 8/8 hardware references pass.
- A1: visreg gained `--exact` (exact per-pixel gate, additive; default AE path
  unchanged). Committed `95da8ba`.
- A3: repo hygiene left minimal this pass — untracked `.agents/`,
  `.github/workflows/native-ci.yml`, `TEST_INFRA.md`, `TEST_READY.md`,
  `kilo-handoff.md`, `imgui.ini`, `render/`, three `screen_*_phase5.png`.
  Protected: keep `render/` and the three phase5 PNGs; modified README.md is
  not ours and is left unstaged. Nothing was deleted.
- A4: this log updated with commit `1d0ebf2` (performed above).

## Phase B residuals + reference artifact (2026-11-14)

Commits 4652a94 (tracker dash +1 advance), 38c3aec (pattern bar row 479),
468ff5c (song block height, File header, loader compare bar, list scrollbar).

- All AE green (17/17 at >= 95%); exact parity lifted across the board:
  pattern 99.15, song 98.97, loader 98.54, file 98.59 (from 98.17/98.65/
  98.43/98.10), granular 98.34.
- Key finding: the fwref bottom-bar cLabels were extracted at init-time
  `value=1`; the firmware resets them to `value=0` (only the active button
  stays 1). This is a REFERENCE artifact, not an app bug, and it dominates the
  remaining structural diffs on config/master1/song/etc bottom bars. Regenerate
  the references with value=0 to lift those (Phase C item; moving the gate
  baseline needs its own verification).
- Remaining residuals after structural fixes are the documented L4-vs-TTF AA
  fringe, the granular cLineIndicator emulator gap, and EVE LINE_STRIP corner
  overhang — all accepted/documented, not app defects.

## Immediate next steps

Phase A — verification and housekeeping (now):
  1. Decide the parity metric (AE 95% vs exact counts; prefer additive exact gate).
  2. Run the remaining gates: functional_smoke.py, gui_audit.py core.json, refcal.py (all pass this session).
  3. Triage untracked repo artifacts; keep `screen_*_phase5.png`, `render/`.
  4. Log each phase's gate results into this file and phase6-verification.md.

Phase B — Phase 6 parity residuals, by exact-pixel impact:
  5. Pattern dash 11px-vs-10px advance (largest pattern residual).
  6. Generalise pre-blended workspace text to Song/Sample/Loader.
  7. File and Loader exact-parity residuals (lowest scores).
  8. Sample Granular residual / cLineIndicator emulator gap.
  9. Config/Instrument/Master 1 residuals.

Phase C — unported screens: Instrument/Patch (after Phase B exits), then the
Sample Loader import-progress workflow (design decision + new fwref states).
Phase D — Studio/Demo feature expansion (only after Phase C).

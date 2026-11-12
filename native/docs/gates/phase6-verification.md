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
| Unit tests | `./native/build/trakr_tests` | 134/134, 538,204 assertions |
| UI robot (semantic default) | `./native/build/trakr_app --auto-test` | 114 frames, 0 ID conflicts/stack/numeric errors |
| Functional smoke | `python3 native/tools/gui_audit/functional_smoke.py` | 17/17 scenarios pass |
| fwref strict oracle | `cd native/tools/fwref && python3 check_reference.py` | all gates pass, 0 unsupported / 0 approximations |
| Hardware structural (calibrated) | `python3 native/tools/refcal/refcal.py --all` | 8/8 tiles (Instrument x2, Instrument Automation, Performance, Pattern, Track Mixer, Sample Playback, Master EQ) |
| Glyph coverage | `python3 native/tools/check_glyphs.py` | 0 non-ASCII codepoints, all covered |
| Classic visual regression | `python3 native/tools/visreg/visreg.py --all --deterministic` | 15/15 cases >= 95% (97.85-99.99%), deterministic |
| Classic parity | per-screen gate docs | all nine workspaces >= 95%; best 99.9% |

## Screen coverage

| Workspace | Semantic path | Main state vs fwref | Sub-pages |
|---|---|---|---|
| Pattern | default | 98.4% | popups via app fill modal |
| Song | default | 98.9% | — |
| Instrument | default | General 99.98% | Effects/Automation legacy-equivalent (no fwref) |
| Master | default | 1/3 99.8%; 2/3 99.4% | Master EQ 99.8% (source-true fwref + hardware tile); Global Mixer 3/3 not modeled |
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

## Known residuals

- fwref multi-state coverage now includes Instrument Effects/Automation; the
  Config item-editor popup remains app-defined and has no reference.
- The Perform page has no additional firmware grid page (the earlier residual
  was incorrect); the app models every firmware Performance control.
- Firmware Global Mixer (Master 3/3) is not modeled by the desktop engine
  (no global aux volume/mute/line-in in the desktop mixer), so the app's third
  Master tab is the firmware Master EQ page (`showEqualizerScreen()`, reached in
  the firmware from the EQ label on Master 1/3). Both pages now have a
  source-true fwref reference; Master EQ also has a calibrated hardware tile.
- Sample Loader import progress and the Config item-editor popup are desktop
  workflow divergences (direct file open and direct value editing) rather than
  missing screen controls.
- Functional smoke is fully green; its comparisons now crop to the workspace so
  chrome-only changes (Undo availability, transport timer) are not mistaken for
  screen state. Fixed along the way: stop rewinds the transport row, Song delete
  returns the selection to the previous slot, and the Sample parameter selector
  wraps like the legacy screen.
- Calibrated hardware tiles exist for seven views (including Master EQ), not every sub-page.

## Handoff

- Run `cmake --build native/build -j8`, `./native/build/trakr_tests`, and
  `./native/build/trakr_app --auto-test` before changing UI code.
- Screens live in `native/app/ui/screens/*` and mirror
  `core/interface/*/g*.cpp`; controls live in `native/app/ui/controls/*`.
- Use `--lcd-shot --screenshot` plus `compare -metric AE` against
  `render/fwref/*.png` (regenerate with `fwref_gen --strict`).

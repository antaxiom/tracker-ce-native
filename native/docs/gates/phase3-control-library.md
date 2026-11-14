# Phase 3 — Control library (gate)

Status: completed. Every Phase 4/5 screen renders through this library, and the
per-control deliverables (semantic unit tests, Classic visual golden, Studio
reflow render) are satisfied or explicitly mapped to existing coverage.

## Deliverable matrix

Each control is mapped to its semantic unit test, its Classic visual golden
(where its output is rendered at device-exact scale), and the Studio reflow
render. "semantic unit test" = measure (`rect()`), hit-test
(`Display::hitTest` / `onPress`) and navigation (selection/scroll anchor) where
the control is interactive.

| Plan control | native port | Semantic unit test | Classic golden | Studio reflow |
|---|---|---|---|---|
| frame | `FrameControl` (primitives.h) | `test_ui_core.cpp`, `[frame][phase3]` | every screen | `[ui][phase3]` |
| background | `BlankCoverControl`/fill via `Canvas` | `test_ui_core.cpp` compositing | every screen | `[ui][phase3]` |
| label / bgLabel | `LabelControl`, `VerticalLabelControl` | `test_ui_core.cpp`, `[phase3]` measure | Perform/Song | `[ui][phase3]` |
| bar | `BarControl` | `test_ui_core.cpp` (press, curve) | Master/Config | `[ui][phase3]` |
| list | `ListControl` | `test_ui_core.cpp` (select, anchor, scroll) | Loader/Filter | `[ui][phase3]` |
| paramList | `ParamValueListControl` | `test_ui_core.cpp` | Config | `[ui][phase3]` |
| multiLabel | `MultiLabelControl` | `test_ui_core.cpp`, `[multilabel]` | Performance | `[ui][phase3]` |
| textBox | `TextBoxControl` | `[textbox][phase3]` (word-wrap) | Config edit/Info | `[ui][phase3]` |
| tracker | `TrackerControl` | `test_pattern_parity.cpp` + robot | Pattern | screen e2e |
| songPlayer | `SongPlayerControl` | `test_ui_core.cpp` (window, press) | Song | screen e2e |
| points/spectrum/slice/wt/line-indicator | `waveform.cpp` helpers | `test_ui_core.cpp` point-map; `test_pages_parity.cpp` | Sample/granular | screen e2e |
| gridEditor | `InstrumentView` edit state | `test_instrument_parity.cpp` | Instrument | screen e2e |

## New in this Phase 3 closure

`native/tests/test_control_library.cpp` (`[ui][phase3]`, `[ui][textbox][phase3]`,
`[ui][frame][phase3]`):

- `Control library renders in place and reflows under Studio`: builds each
  self-contained control (frame, label, bar, list, paramValueList, multiLabel,
  textBox) and renders it through the semantic compositor under **Classic**
  (device-exact, firmware workspace) and **Studio** (fit-scale host reflow).
  Asserts each emits non-degenerate, finite vertices under both profiles.
- `TextBoxControl word-wraps within its width`: proves a long description
  wraps onto multiple glyph rows inside a narrow panel (no clipping plumbing).
- `Controls expose measure (rect) and hit-test placement`: measure returns the
  device rect; only interactive controls appear in `Display::hitTest`.
- `FrameControl exposes its selected index`: measure/state accessor.

## Studio reflow

Studio is deferred to Phase 5/7 (ADR-0002, re-scoped Phase 2 gate):
`LayoutProfile::Studio` in `native/app/ui/layout.h` stores a fit scale and
host-sized workspace until it grows a reflow layout. The
`[ui][phase3]` render case exercises every control under the Studio profile
now, locking that each control renders non-degenerate in a host-sized context
so the future reflow work starts from a verified base.

## Result

`trakr_tests`: 151 cases / 538,982 assertions pass, including the four new
Phase 3 cases (164 Phase 3 assertions). No production code was changed.

## Residual

The only per-control note is firmware-authentic overhang (e.g. the rotated
vertical FX name extends one character of its cell width left of its rect).
The `[ui][phase3]` render test asserts finiteness and non-degeneracy and allows
that overhang rather than treating it as clipping.
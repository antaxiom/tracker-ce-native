# Gate: Phase 2 — Instrument General Vertical Slice

- Date: 2026-11-12
- Profile: Classic (800x480)
- Path: `trakr_app --screen instrument --gui-next`
- Decision: **PASS (with stated conditions)**

## Scope

Instrument Parameters 1/2 (General) is the first screen rendered by the semantic
UI layer: `app/ui` controls, `Canvas`, reverse creation order, L4 bitmap text,
integer Classic scale, firmware geometry and bar/list semantics. App chrome
(header, bottom ScreenButtons, deck) remains in the existing layer for this
slice.

## Evidence

| Check | Result |
|---|---|
| Unit tests | 127/127 pass (538,167 assertions) |
| fwref strict mode | 0 unsupported, 0 approximations |
| fwref reference gates | provenance, determinism, primitive contract, negative control pass |
| Hardware structural (refcal) | 5/5 tiles pass; Instrument 2/2 |
| App vs fwref (Instrument workspace) | 68 px differ (0.02%); legacy path 0.65% |
| Input parity (remote FIFO) | Down changes Volume; Right+Down changes Panning and frame column |
| Robot audit | 114 frames pass with and without `--gui-next` |
| Wall-time proxy | robot loop ~0.12s both paths; no measurable regression |
| Functional smoke | 9/17; identical failures with the header clamp reverted (pre-existing) |

Hardware tiles: two Instrument tiles in
`native/docs/reference/screens/video_perform_instrument_pattern.png`, plus
Performance, Pattern and Track Mixer tiles. refcal checks header row 28, bottom
label cells, frame grid and filter rows; it does not claim pixel identity.

## Conditions

1. Studio layout, reflow and the intent layer are deferred; Classic parity is the
   gate for this slice.
2. Hardware evidence covers 5 tiles across 4 screens; remaining screens and
   multi-state captures still need refcal specs.
3. App chrome (header/title bar, ScreenButtons) still uses the legacy layer.
   A screen is not fully migrated until chrome also moves to the control layer.
4. Functional smoke failures are pre-existing and tracked separately.

## Next

Port the remaining control library (multiLabel, textBox, tracker,
horizontalTracker, songPlayer, gridEditor) and then migrate Pattern, which
exercises tracker/list/text much more heavily than Instrument.

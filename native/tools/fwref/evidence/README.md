# Hardware evidence

The plan's low-frequency structural check uses user-supplied photos or video
frames of the Tracker LCD. No hardware captures were supplied with this task,
so this step is recorded as **not applicable** rather than fabricated.

When captures are available:

1. Rectify the LCD to an exact 800x480 frame (four corner homography).
2. Save as `hw_instrument.png` / `hw_master.png` in this directory.
3. Compare region masks against `render/fwref/*.png`:
   * bar presence and white/green classification per column,
   * filter/saturation row band positions,
   * bottom-bar separator positions and label rows.
4. Record pass/fail per region here. Do not use hardware as a pixel metric.

## Audit status (2026-11-12)

FW-1 strict mode is implemented (`native/tools/fwref/audit.{h,cpp}`):

- `fwref_gen --strict` fails on any unsupported pixel-affecting call or
  approximation; `check_reference.py` now runs with `--strict`.
- Current result: **0 unsupported, 0 approximations** across all ten reference
  screens.
- Whitelisted as ignored-safe with a documented reason: `API_BITMAP_SOURCE`,
  `API_BITMAP_LAYOUT`, `API_BITMAP_SIZE`, `API_CMD_SETBITMAP` (font handles are
  preloaded; image/bitmap draws are audited at `API_END`).
- Audited and failing when exercised: non-cell `BITMAPS` vertices, non-ASCII
  text, unsupported text option bits, non-quarter matrix rotations, unsupported
  number bases.

## Known spec/runtime nuances

- Instrument General: the control is created with 8 frame places, but
  `showInstrumentParams()` / `refreshFramePosition()` sets `placesCount = 7` at
  runtime and `activateLabelsBorder` refuses place 7 ("More"). The app clamps
  the frame to places 0..6. fwref specs keep the creation-time 8 places; the
  default renders place 0, so references are unaffected. Multi-state specs must
  model the runtime `placesCount`.
- Strict mode is enforced per screen in `renderSpec`; a failing screen returns
  false and prints the audit report.

## Hardware structural results (2026-11-12)

Harness: `native/tools/refcal` (rectify, per-channel black/white calibration,
structural checks against fwref).

- Frames: two Instrument Parameters 1/2 tiles, one Performance FX tile, one
  Pattern tile (`video_perform_instrument_pattern.png`), and one Track Mixer
  2/3 tile (`video_pattern_mute_selection_mixer.png`).
- Result: **5/5 passed**. Header separator at firmware row 28; 8/8 bottom label
  cells present; Instrument adds frame column/grid and 4/4 filter-list rows.
- Caveat: the photo frame height reads ~361px vs 391px in fwref (the lower edge
  is obscured/compressed in the capture), reported as an advisory delta.
- Not pixel identity; values/states differ between the photo and the reference.

Remaining: extend `refcal/specs.json` to Perform, Pattern, Master, Sample,
Config, Song, Loader and File, and add multi-state captures.

## Hardware status

Still **not fully validated against hardware**. fwref remains an approximate,
advisory-only oracle (ADR-0003). The source-true gates
(`check_reference.py`) prove provenance, determinism, the primitive contract
and the negative control; they do not prove FT812 equivalence. Instrument
General now has calibrated structural evidence; other screens do not.

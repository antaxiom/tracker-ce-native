# refcal — hardware reference calibration (FW-8)

`refcal.py` turns a photo/video frame of the real LCD into a calibrated
800x480 image and checks **structure** against an audited fwref reference.

It does not do pixel-identity comparison. ADR-0003 makes calibrated structure
the hardware check; pixel parity remains an advisory metric.

## What it does

1. Finds the LCD header separator inside a tile and derives the LCD bounds.
2. Rectifies the tile to exactly 800x480 with ImageMagick.
3. Calibrates per channel: 1st-percentile black point, 99th-percentile white.
4. Checks:
   - header separator row (~28),
   - red frame box presence, width, grid column and top row,
   - four filter-list text cells at the 27px pitch,
   - bottom button label cells for the eight columns.
5. Writes `<name>_rectified.png`, `<name>_calibrated.png` and
   `<name>_metrics.json` under `--out`.

Row/column profile correlations are reported as **advisory** only; values differ
between captures and valid states.

## Usage

```bash
python3 native/tools/refcal/refcal.py --all --out /tmp/refcal
python3 native/tools/refcal/refcal.py \
  --frame native/docs/reference/screens/video_perform_instrument_pattern.png \
  --tile 0,382,632,381 \
  --ref render/fwref/instrument_params.png \
  --name instrument_hw_left \
  --out /tmp/refcal
```

Requires ImageMagick (`magick`), `numpy` and `scipy`.

## Current results (2026-11-12)

`specs.json` contains five tiles across four screens:

- 2 Instrument Parameters 1/2 tiles (`video_perform_instrument_pattern.png`),
- 1 Performance FX tile,
- 1 Pattern tile,
- 1 Track Mixer 2/3 tile (`video_pattern_mute_selection_mixer.png`).

All 5 pass the required checks: header separator at firmware row 28, bottom
label cells present, and (for Instrument) frame grid and four filter-list rows.
The header check compares against firmware geometry (row 28), not against the
reference image's brightest row, because some reference headers have no bright
separator line.

Known photo artifact: the red frame height reads ~361px on the captures versus
391px in fwref. The frame's lower edge is obscured/compressed in the photo, so
height is checked as a presence/plausibility range and reported as an advisory
delta. Do not use this as evidence that the app frame height is wrong.

## Adding screens

Add an entry to `specs.json`:

```json
{"name": "perform_hw", "frame": "native/docs/reference/screens/....png",
 "tile": [0, 10, 632, 381], "ref": "render/fwref/perform.png"}
```

Tile rectangles are the panel crop inside a collage; the tool finds the LCD
inside it. Use a reference rendered for the same page/state where possible.

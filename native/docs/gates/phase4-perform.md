# Gate: Performance Mode

- Date: 2026-11-12
- Profile: Classic (800x480)
- Path: `trakr_app --screen perform --gui-next`
- Decision: **PASS for the FX workspace (track row/chrome pending)**

## Scope

`PerformScreen` mirrors `gPerformance.cpp::initDisplayControls()`: frame (12
places) plus, per column, `textLabel[i]` (vertical cLabel) and `value1Label[i]`
(cMultiLabel). Creation order and reverse composite match NewDisplay; the app
draws one extra black workspace clear like the firmware background.

`MultiLabelControl` ports `multiLabel.cpp`: CC value cell (35px), FX hole,
four 30px preset rows with the active row in `valueNormalMLabelColors[2]`,
edit-mode red palette and the bottom shadow gradient.
`VerticalLabelControl` uses `Canvas::textVertical270` (Light17).

Pad presses are delegated to `PerformView::selectTop`/`selectRow`, which reuse
`triggerSlotPad` and `applyToEngine`; latch/engine behavior is unchanged.

## Evidence

| Check | Result |
| --- | --- |
| Unit tests | 132/132 pass; includes top/row cell routing |
| App vs fwref (full frame) | 1.75% pixels differ; legacy path 1.75% (same) |
| Input parity (script) | Top-cell and preset clicks toggle the edit frame and active row |
| Robot audit | 114 frames pass with and without `--gui-next` |
| Hardware structural (refcal) | Performance tile passes header/bottom checks (5/5 suite) |

## Conditions

1. The remaining delta is the bottom track/pattern row and app chrome, plus
   L4-vs-TTF vertical glyph rasterization of the FX names.
2. Grid-editor producer page and pad backlight indicators are not ported.
3. No calibrated per-column hardware check yet (suite covers header/bottom).

## Next

Add a per-column calibrated tile, then finish the grid/pad page if enabled.

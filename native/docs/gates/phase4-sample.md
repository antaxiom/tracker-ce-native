# Gate: Sample Playback (waveform)

- Date: 2026-11-12
- Profile: Classic (800x480)
- Path: `trakr_app --screen sample --gui-next`
- Decision: **PASS for the playback workspace (editor sub-page/chrome pending)**

## Scope

`SampleScreen` mirrors `gSamplePlay.cpp::initDisplayControls()` creation order:
frame (7 places; place 6 = play-mode list), cLabel[8] + cBgLabel (app chrome),
cList play-mode list, cProgressCursor, cLineIndicator, cPoints, cSlicePoints,
cSpectrum[0..1], cWtProgress. `showDefaultScreen()` visibility is reproduced:
wave present -> progress cursor; slice/beat-slice -> slice markers; granular ->
line indicator; otherwise points; no wave -> wavetable cursor.

Waveform controls (`ui/controls/waveform.*`) port:

- `cSpectrum` 300/200-point min/max columns over the 600x218 field with the
  zoom window and stereo half-height split.
- `cPoints` additive range background plus start/end/loop markers and
  zero-cross colours (0x01/0x02/0x04/0x08 selection bits).
- `cSlicePoints`, `cProgressCursor`, `cLineIndicator`, `cWtProgress`.

Navigation, parameter clamping, audition, history and marker dragging are
delegated to `SampleView`; play-mode clicks write `Instrument::loop` and publish.

## Evidence

| Check | Result |
|---|---|
| Unit tests | 130/130 pass; includes zoom-window point mapping |
| App vs fwref (full frame) | 1.74% pixels differ; legacy path 2.13% |
| Input parity (script) | Right selects Play Mode, Up advances Forward -> Backward Loop |
| Robot audit | 114 frames pass with and without `--gui-next` |
| Hardware structural (refcal) | No Sample tile yet |

## Conditions

1. `cPoints`' `BLEND_FUNC(SRC_ALPHA, ONE)` additive background cannot be
   expressed in ImGui; it is painted in the `Display` prepass (layer -1) before
   the spectrum, matching the legacy approximation.
2. The zoom window is centred (`MAX_16BIT/2`); firmware follows the edit cursor
   when zoomed. Non-1.0 zoom is not independently compared.
3. The Sample Editor sub-page and the 8 bottom labels remain legacy/app chrome.
4. No calibrated hardware tile for Sample; refcal coverage pending.

## Next

Add a Sample hardware tile and a multi-state (zoom/slice/granular) fwref spec.

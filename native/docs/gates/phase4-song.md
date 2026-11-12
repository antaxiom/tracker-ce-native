# Gate: Song (cSongPlayer)

- Date: 2026-11-12
- Profile: Classic (800x480)
- Path: `trakr_app --screen song --gui-next`
- Decision: **PASS for the arranger workspace (list animation/chrome pending)**

## Scope

`SongScreen` mirrors `gSongEdit.cpp::initDisplayControls()`: frame (3 places,
startPlace 2), tempo bar (col 7), and `cSongPlayer` over cols 0..6. The 8 bottom
cLabels and cBgLabel are app chrome and stay in the legacy layer.

`SongPlayerControl` ports `core/libs/NewDisplay/songPlayer.cpp`:

- update(): background, bottom shadow, "Pattern" header, playlist window with
  27px rows, 210px column, scrollbar, selected-row frame in the active/edit/idle
  colour (`colors[9]` / `colors[5]` / `colors[1]`).
- update2(): per-track names rotated 270 degrees in RobotoMono Regular 12,
  muted tracks in `colors[4]`.
- update3(): `fillBlocks()` occupancy matrix (block width/spacing 486/5/4/23,
  12 visible patterns) and the progress line.

Esc/left/right/insert/delete edit slots through `SetOrderCommand`, so undo,
history and engine publish are unchanged. Mouse presses on the list select a
slot.

## Evidence

| Check | Result |
|---|---|
| Unit tests | 130/130 pass (538,182 assertions) |
| App vs fwref (full frame) | 1.09% pixels differ; legacy path 1.45% |
| Input parity (script) | Down x2 moves the selection frame from slot 1 to slot 3 |
| Robot audit | 114 frames pass with and without `--gui-next` |
| fwref strict audit | 0 unsupported, 0 approximations |
| Hardware structural (refcal) | No Song tile yet |

## Conditions

1. The firmware playlist scroll animation (`listAnimationStep`, `textListPos`
   interpolation) is not ported; the settled window is drawn, matching the
   reference frame.
2. `cSongPlayer` selection highlight (`controlData->selection`) is not drawn;
   the reference scenario has no active selection, but marquee selection on the
   Song screen is still legacy-only.
3. Header/title bar and bottom ScreenButtons remain app chrome.
4. No calibrated hardware tile for Song; refcal coverage pending.

## Next

Add a Song hardware tile, then port the selection highlight and Config
(`cParamValueList`) or Perform/File.

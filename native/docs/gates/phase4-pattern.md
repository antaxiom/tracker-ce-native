# Gate: Pattern Editor (cTracker)

- Date: 2026-11-12
- Profile: Classic (800x480)
- Path: `trakr_app --screen pattern --gui-next`
- Decision: **PASS for the tracker workspace (popups/chrome pending)**

## Scope

`PatternScreen` renders the firmware cTracker through `TrackerControl`:
row-number gutters, four 186px track columns, 15 rows at 26px, beat dividers,
track-name bars, note/instrument/FX1/FX2 cells, cursor box, selection fill and
playhead gradients. Navigation, editing, history, clipboard and selection
semantics are delegated to `PatternView`, so key behavior is unchanged.
Bottom ScreenButtons and the pattern header remain app chrome.

## Evidence

| Check | Result |
|---|---|
| Unit tests | 127/127 pass |
| App vs fwref (workspace) | 0.35% pixels differ; legacy path 0.32% (comparable) |
| Input parity (remote FIFO) | Down x2 + Right moves the cursor from row 1 to row 3 and track 1 to track 2 |
| Robot audit | 114 frames pass with `--gui-next` |
| Hardware structural (refcal) | Pattern tile passes header/bottom checks (5/5 tile suite) |

## Conditions

1. Fill/note/instrument popups and their lists are not re-rendered; they remain
   in the legacy path until the popup controls are ported.
2. Selection drag rendering is implemented; mouse drag selection parity is not
   independently tested.
3. Hardware coverage for Pattern is header/bottom only (no per-cell structural
   check yet).
4. Studio/reflow is deferred.

## Next

Port `cSongPlayer` for the Song screen, or finish the popup/list controls first
if Pattern modal parity is prioritized.

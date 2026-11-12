# Gate: Sample Editor (app sub-page)

- Date: 2026-11-12
- Profile: Classic (800x480)
- Path: `trakr_app --screen sample --script Tab`
- Decision: **PASS (app-defined page; verified by source/layout equivalence)**

## Scope

The Sample Editor is an app-defined sub-page (the firmware has no editor page
with this layout). `SampleScreen` now renders both sub-pages:

- Playback: play-mode list, frame, points/slices/granular cursors (unchanged).
- Editor: seven-row edit-op list (Crop, Reverse, Normalize, Amplify, Fade In,
  Fade Out, Silence) at the firmware list geometry, same waveform controls,
  no play-mode list and no frame.

Tab toggles the sub-page from the semantic screen; double-clicking an edit row
applies it through `SampleView::applyEditOp`, and left/right keys move the
selected operation.

## Evidence

| Check | Result |
|---|---|
| Unit tests | 133/133 pass |
| Layout | Reuses the ListControl/waveform geometry of the playback gate |
| Robot audit | 114 frames pass on the default semantic path |
| fwref | Not applicable (app-defined page) |

## Conditions

1. No firmware reference exists for this layout; verification is structural.
2. The legacy editor was reachable from the menu/R0 only, so there is no
   direct screenshot baseline; the semantic page is now reachable with Tab.
3. Bottom buttons (Apply/Start/End/Zoom/...) remain app chrome.

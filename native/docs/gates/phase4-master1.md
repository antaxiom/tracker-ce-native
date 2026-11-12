# Gate: Master 1/3 (Audio Control)

- Date: 2026-11-12
- Profile: Classic (800x480)
- Path: `trakr_app --screen master --gui-next`
- Decision: **PASS for the Classic workspace (hardware capture pending)**

## Scope

Master 1/3 is rendered by `MasterPage1Screen` using the existing semantic
controls: frame (8 places), volume bar (col 0), saturation list (col 5),
bass/space bars (cols 6/7) and the blank cover over cols 1..4. Editing is
delegated to `MixerView` (Master sub-page), so clamping and publish behavior
are unchanged. Header/title bar and bottom ScreenButtons remain app chrome.

## Evidence

| Check | Result |
|---|---|
| Unit tests | 127/127 pass |
| App vs fwref (workspace) | 68 px differ (0.02%); legacy path 0.78% |
| Input parity (remote FIFO) | Right selects col 5 and Up changes saturation; frame and list update |
| Robot audit | 114 frames pass with `--gui-next` |
| Hardware structural (refcal) | No hardware tile for Master 1/3 yet |

## Conditions

1. No calibrated hardware tile for Master 1/3 exists in the reference library;
   refcal coverage for this screen is pending.
2. The screen gate covers the Classic workspace only. Studio/reflow is deferred.
3. App chrome still uses the legacy layer.

## Next

Add a Master 1/3 hardware tile (or a verified video frame) to refcal, then
proceed to Song or Pattern. Pattern remains the high-risk screen and requires
the tracker control.

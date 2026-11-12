# Gate: Config

- Date: 2026-11-12
- Profile: Classic (800x480)
- Path: `trakr_app --screen config --gui-next`
- Decision: **PASS for the default screens (item editor/grid page pending)**

## Scope

`ConfigScreen` mirrors `gConfigEdit.cpp::initDisplayControls()`: frame (4
places), basemenu cList (14 rows), submenu `cParamValueList`, blank space
covers, config label/list stubs and the popout label. The 8 bottom cLabels and
cBgLabel are app chrome.

`ParamValueListControl` ports `core/libs/NewDisplay/paramValueList.cpp`: 27px
rows, left name / right-aligned value, `textListPos` composite anchor window,
selection frame in `activeListColors` and bottom row shadow.

Group switching, item navigation and value edits are delegated to
`ConfigView` (`moveSelection`, `switchGroup`, `adjustSelectedValue`); row data
comes from `ConfigView::currentRows`, shared with the legacy path.

## Evidence

| Check | Result |
|---|---|
| Unit tests | 132/132 pass (538,191 assertions) |
| App vs fwref (full frame) | 0.36% pixels differ; legacy path 1.91% |
| Input parity (script) | Down x2 moves the item frame; `=` promotes focus to the submenu (frame place 1) |
| Robot audit | 114 frames pass with and without `--gui-next` |
| fwref strict audit | 0 unsupported, 0 approximations |
| Hardware structural (refcal) | No Config tile yet |

## Conditions

1. The item-editor list, description/credits text boxes and grid-editor page
   are not re-rendered; `configListShown` remains false in the app.
2. Frame place 2 (config list) and place 3 (producer grid) are wired but only
   places 0/1 are reachable from the default screen.
3. Header/title bar and bottom ScreenButtons remain app chrome.
4. No calibrated hardware tile for Config.

## Next

Port the item-editor list/description boxes, then close remaining conditions.

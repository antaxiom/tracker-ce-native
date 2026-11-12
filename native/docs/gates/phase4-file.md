# Gate: File (project browser)

- Date: 2026-11-12
- Profile: Classic (800x480)
- Path: `trakr_app --screen file --gui-next`
- Decision: **PASS for the project browser**

## Scope

`FileScreen` mirrors `gProjEdit.cpp::initDisplayControls()` and
`showProjectBrowser()`: frame (2 places), "SD Card"/"Recent" top labels
(Bold17, `topBarLabelColors`), project file cList with
`projectActiveListColors`, empty recent cList with
`projectInactiveListColors` (firmware still paints a selection row) and the
blank cover over the right column. The cover `cImg` is hidden and draws
nothing.

Selection, navigation and Open are delegated to `FileView`
(`moveSelection`, `selectProject`, `openSelected`); the bottom buttons remain
app chrome.

## Evidence

| Check | Result |
|---|---|
| Unit tests | 132/132 pass |
| App vs fwref (full frame) | 1.12% pixels differ; legacy path 1.97% |
| Input parity (script) | Up/Down moves the project frame; click/double-click selects and opens |
| Robot audit | 114 frames pass with and without `--gui-next` |
| fwref strict audit | 0 unsupported, 0 approximations (file reference now generated) |

## Conditions

1. The browser stage is ported; the project-open dialog/export deck stay in the
   app layer by design.
2. The recent list is empty because the desktop app keeps no recent-project
   history.
3. No cover bitmap is loaded, matching `hideProjectCover()`.
4. No calibrated hardware tile for File.

## Next

Add a File hardware tile and wire recent-project history if desired.

# Gate: Sample Loader

- Date: 2026-11-12
- Profile: Classic (800x480)
- Path: `trakr_app --screen loader --gui-next`
- Decision: **PASS for the default screen (crop/importer pages pending)**

## Scope

`SampleLoaderScreen` mirrors `gSampleImport.cpp::initDisplayControls()`:
frame (3 places: SD card, instruments, preview volume), top-bar labels
("SD Card", "Instruments" in Bold17 with `topBarLabelColors`), explorer and
instrument cLists (13 rows, cList anchor window), memory cBar
(CompareTwoValues) and preview-volume cBar. `activateLabelsBorder()` focus
semantics are reproduced: active/inactive `sampleLoader*ListColors`, header
colour swap, and frame place 0/1.

Navigation is delegated to `SampleLoaderView`: Left/Right switch panes,
Up/Down move the selection, Enter enters a directory, clicks select a row or an
instrument slot. The hidden crop controls stay hidden on the default screen.

## Evidence

| Check | Result |
|---|---|
| Unit tests | 130/130 pass; includes cList anchor-window behaviour |
| App vs fwref (full frame) | 0.85% pixels differ; legacy path 1.47% |
| Input parity (script) | Down x2 moves the file selection, Right+Down selects slot 2 |
| Robot audit | 114 frames pass with and without `--gui-next` |
| Hardware structural (refcal) | No Loader tile yet |

## Conditions

1. The importer crop page (rename/import progress, editor waveform) is not
   ported; those controls remain named stubs in the legacy path.
2. The parity memory "compare" bar uses the scenario's `memoryUsageAdd`
   (12% -> 13%) because the synthetic SD-card path has no real file sizes.
3. Bottom labels remain app chrome.
4. No calibrated hardware tile for Loader; refcal coverage pending.

## Next

Port the importer crop page, then Config (`cParamValueList` / grid editor).

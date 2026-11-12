# fwref — source-true firmware reference

`fwref` renders the firmware's Instrument General, Master 1/3, Master 2/3 (Track
Mixer), Master 3/3 (Global Mixer), Config, Perform, Pattern, Song, Sample Loader
and Sample Playback screens on the host by compiling the **unmodified**
`core/libs/NewDisplay` control sources and driving them with checked-in specs
extracted from the firmware source.

This is the only reference used for those screens by
`native/tools/os_input/os_input_runner.py`; the previous hand-drawn renderers
were removed in `FT812ScreensExtra.cpp` / `FT812ScreensCore.cpp`.

## Truth chain

```
core/interface/instrumentEditor/gInstrEdit.cpp    ─┐
core/interface/masterParams/gMasterParams.cpp     ─┤ extract_spec.py
core/interface/configEditor/gConfigEdit.cpp       ─┤ (expressions + SHA-256)
core/interface/performanceMode/gPerformance.cpp   ─┤
core/interface/patternEditor/gPatternEdit.cpp     ─┤
core/interface/songEditor/gSongEdit.cpp           ─┤
core/interface/sampleImporter/gSampleImport.cpp   ─┤
core/interface/samplePlayback/gSamplePlay.cpp     ─┤
core/interface/core/songTimer.cpp                 ─┤
core/source/Themes.cpp                            ─┤
core/interface/core/titleBar.cpp                  ─┘
        │
        ▼
native/tools/fwref/specs/*.json        checked-in, hash-stamped property specs
        │                              (evaluated verbatim; no hand-entered coords)
        ▼
core/libs/NewDisplay/{bar,label,list,multiLabel,paramValueList,textBox,
                      gridEditor,frame,bgLabel,tracker,songPlayer,
                      progressCursor,lineIndicator,points,slice,spectrum,
                      wtProgress,horizontalBar,displayControls}.cpp
        │        compiled unmodified into fwref_core
        ▼
native/tools/fwref/shim/ft812_api.cpp  EVE/FT81x immediate-mode shim
        │                              (scissor, blend incl. SRC_ALPHA/ONE,
        │                               gradient, CMD_TEXT, CMD_NUMBER/
        │                               CMD_SETBASE, LINES/LINE_STRIP/RECTS/
        │                               FTPOINTS, matrix stack, CELL+VERTEX2F
        │                               glyphs for x > 511)
        ▼
native/tools/display_emulator/FT812Emulator  exact 800x480 framebuffer
        │
        ▼
render/fwref/{instrument_params,master_page1,master_page2,master_page3,
               config,perform,pattern,song,loader,sample}.png
```

`include/display.h` is a host shim for the `cDisplay` surface only
(`createControl<T>()`, `getTheme()`, reverse-creation-order `renderAll()`).
It defines the real header's include guard so the control sources' quoted
`#include "display.h"` becomes a no-op; the shim is force-included first.
`include/Tracker.h` stubs the timer used by `cList` while scrolling is off, and
`include/mtStructs.h` mirrors the `kSampleTracks`/`kMidiTracks`/`kAllTracks`
constants that `cSongPlayer` reaches through the real `display.h`, plus
`MAX_SLICE_NUMBER` (sliceControl.h) and `MAX_16BIT` (spectrum.cpp).

## Files

| Path | Purpose |
|------|---------|
| `extract_spec.py` | Extracts and hash-stamps layout/style/text expressions. `--check` fails on source drift. |
| `specs/instrument_general.json` | Instrument General control list (frame, 8 labels, 6 bars, bgLabel, filter list, cover). |
| `specs/master_page1.json` | Master 1/3 control list (frame, 8 labels, bgLabel, 3 bars, cover, saturation list). |
| `specs/master_page2.json` | Track Mixer 2/3 (8 labels, 8 volume bars, 8 level bars, bgLabel). |
| `specs/master_page3.json` | Global Mixer 3/3 (4 aux name labels, 4 volume + 8 level bars, labels). |
| `specs/config.json` | Config default screen (frame, 8 labels, cList/cParamValueList, hidden cTextBox/cGridEditorPopup/popout, covers). |
| `specs/perform.json` | Performance FX (12 vertical cLabels + 12 cMultiLabels, track labels, bgLabel). |
| `specs/pattern.json` | Pattern control list (frame, 8 action labels, bgLabel, cTracker grid, title-bar song timer). |
| `specs/song.json` | Song editor default screen (hidden frame, tempo bar, 8 action labels, bgLabel, cSongPlayer). |
| `specs/loader.json` | Sample Loader default screen (3-place frame, 8 action labels, bgLabel, 2 pane headers, file/slot cLists, memory + preview cBars, hidden waveform controls). |
| `specs/sample.json` | Sample Playback default screen (7-place frame, 8 action labels, bgLabel, play-mode cList, cProgressCursor, cPoints, cSpectrum; hidden cLineIndicator/cSlicePoints/cWtProgress). |
| `specs/chrome.json` | Title bar (background, title, optional right label). |
| `scenario.json` | Shared instrument/master/config/perform/pattern/song/loader/sample state used by the reference and the comparison harness. |
| `spec.cpp` | Spec loader + integer expression evaluator + style-name map (`MAX_16BIT` expands to 65535). |
| `scenario.cpp` | Firmware value formatters and resolvers (dB, cutoff, resonance, master/aux/perform mapping, song tempo). |
| `scenario_sample.cpp` | Loader/Sample resolvers, preview-dB/memory/time/zoom formatters, deterministic waveform generator. |
| `scenario_load.cpp`, `scenario_load_sample.cpp`, `scenario_lists.cpp` | Scenario JSON loader and list/row/bar mappings. |
| `spec_parse_extras.cpp` | Optional list/param_list/multi/grid/song/points/spectrum/slice/wt/line-indicator spec blocks. |
| `screens/render_spec.cpp` | Creates controls via the shim `cDisplay`, then `renderAll()` (optional `chrome_on_top`). |
| `screens/{instrument,master,master2,master3,config,perform,pattern,song,loader,sample}.cpp` | Public render entry points. |
| `screens/spec_controls.cpp` | Creates cFrame/cLabel/cBar/cList/cBgLabel/cTracker/cMultiLabel/cParamValueList/cTextBox/cGridEditorPopup/cHorizontalBar. |
| `screens/spec_controls_song.cpp` | Creates cSongPlayer with its playlist, bitmask, mute, name and progress data. |
| `screens/spec_controls_sample.cpp` | Creates cPoints/cSpectrum/cProgressCursor/cLineIndicator/cSlicePoints/cWtProgress with waveform, slice and wavetable data. |
| `shim/matrix.h` | EVE matrix stack used by cLabel 270° vertical text. |
| `selftest.cpp` | Phase-1 gate: unmodified `cBar` pixel contract. |
| `check_reference.py` | Provenance + determinism + pixel contract + negative-control gates. |

## Commands

```bash
cmake --build native/build --target fwref_gen fwref_selftest trakr_tests -j8
./native/build/fwref_selftest
python3 native/tools/fwref/extract_spec.py --check
python3 native/tools/fwref/check_reference.py
./native/build/fwref_gen --out render/fwref
python3 native/tools/fwref/make_manifest.py --out render/fwref
ctest --test-dir native/build -R fwref
```

## Conventions

* **Rects are half-open**: an EVE `RECTS` pair `(x0,y0)-(x1,y1)` fills
  `x0 <= x < x1`, `y0 <= y < y1`. The 75px bar fill and 97px column follow
  directly from this.
* **Line widths are 1/16 px**, rounded to whole pixels in the host rasterizer:
  `24 -> 2px`, `16 -> 1px`, `8 -> 1px`. The app uses a 1.5px `AddRect` for the
  same selection frame; this is the documented residual difference.
* **Draw order is reverse creation order** (`cDisplay::step3()`): a control
  created earlier paints later. Spec `order` values are the firmware slot
  indices.
* **Fonts**: firmware order is Light14=0, Light17=1, Regular14=2, Bold17=3,
  Regular12=4. The emulator decodes all five.
* **Master saturation frame**: `cList::setValue()` does not update `barPos`, so
  the selection frame stays on row 0; this quirk is reproduced.
* **Matrix stack**: `API_CMD_LOADIDENTITY/TRANSLATE/ROTATE/SETMATRIX` implement the
  EVE 16.16 matrix. `cLabel`'s `controlStyleVerticalText` path rotates each glyph
  270°; the shim maps that to the emulator's `drawTextVertical270()`. `cSongPlayer`
  reuses the same stack for its track-name strip.
* **Number command**: `API_CMD_SETBASE` records the base (10 or 16) and
  `API_CMD_NUMBER` formats the slot/pattern number in it, then draws through the
  same text path as `API_CMD_TEXT`. This is the number column of the Song playlist.
* **Chrome on top**: Song sets `"chrome_on_top": true`. The title-bar controls are
  created before the screen body (as in the firmware), so the vertical track-name
  glyphs that extend above y=28 are clipped by the title bar exactly as on device.
* **Hidden controls**: spec entries with `"hidden": true` are created but never
  shown (Config's cTextBox/cGridEditorPopup/popout, Master's edit frame, Song's
  edit frame, Loader's waveform controls, Sample's cLineIndicator/cSlicePoints/
  cWtProgress/cSpectrum[1]).
* **Stages**: `cDisplay::renderAll()` runs only the stages a control declares
  (`stagesCount`), matching `display.cpp`; this matters for `cSpectrum`
  (`stagesCount == 1`) whose `update2/update3` are never called on device.
* **Waveform geometry**: `75 + ((300 - (MAX_16BIT / 300)) / 2)` is kept verbatim
  in the specs; `evaluateExpression()` substitutes `MAX_16BIT = 65535`, and the
  extractor's `--check` gate compares against the same expansion.
* **Sample waveform data**: `scenario.json` stores a deterministic generator
  (`points`/`amplitude`/`frequency`/`decay`), expanded into int8 upper/lower
  buckets at load time, so the Sample reference is never empty and byte-identical
  across runs.
* **Additive blend**: `SRC_ALPHA, ONE` maps to `BlendMode::SrcAlphaOne` in the
  emulator. `cPoints`' loop-region fill and `cLineIndicator` rely on additive
  blending over the spectrum; plain alpha blend would hide the waveform.

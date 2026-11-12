# Project: Trakr Native — Enhanced Port/Rebuild of Tracker+ Firmware

**Trakr** (placeholder working name) is an enhanced C++20 desktop port and rebuild
of the **Polyend Tracker+ firmware** (the `tracker-ce` open-source codebase). The
desktop app reuses the firmware's core engine and UI structure: each
`native/app/views/*` screen is a faithful 1:1 structural port of the corresponding
firmware screen editor (`core/interface/*/g*.cpp`), and pixel fidelity is verified
against **source-true** references rendered by compiling the unmodified firmware
`NewDisplay` controls through an FT812/EVE API shim (`native/tools/fwref`). On top
of the faithful port it layers desktop enhancements.

## Architecture

The app renders an 800x480 logical LCD on a desktop GPU backend (Metal/Vulkan/
Direct3D 12 via SDL3 + SDL_GPU + Dear ImGui). Each screen view mirrors its firmware
screen editor structurally: firmware control decomposition, naming, device-pixel
geometry (mapped once via a local `px()/py()`), draw order, and colors map 1:1 from
the firmware source.

### Screen ↔ firmware editor mapping

| App view | Firmware screen editor |
|----------|------------------------|
| `PatternView` (+ helpers) | `core/interface/patternEditor/gPatternEdit.cpp` |
| `OrderListView` (Song)    | `core/interface/songEditor/gSongEdit.cpp` |
| `SampleView`              | `core/interface/samplePlayback/gSamplePlay.cpp` |
| `InstrumentView`/`Bars`   | `core/interface/instrumentEditor/gInstrEdit.cpp` |
| `MixerView` (Master)      | `core/interface/masterParams/gMasterParams.cpp` |
| `PerformView`             | `core/interface/performanceMode/gPerformance.cpp` |
| `SampleLoaderView`        | `core/interface/sampleImporter/gSampleImport.cpp` |
| `ConfigView`              | `core/interface/configEditor/gConfigEdit.cpp` |
| `FileView`                | `core/interface/projectEditor/gProjEdit.cpp` |

### Shared chrome

- `HeaderBar` mirrors `core/interface/core/titleBar.cpp` + `songTimer.cpp`.
- `ScreenButtons` mirrors the firmware bottom-button bar.
- `ButtonDeck` mirrors the hardware pad/D-pad/jog controls.

### Data flow & component hierarchy

- `AppLayout`: global logical dimensions (800x480: 28px header, 396px workspace, 56px soft buttons).
- `HeaderBar`: title, transport, and instrument readouts at `y=0..28`.
- `ScreenButtons`: 8 contextual bottom buttons at `y=424..480`.
- View implementations (each a firmware screen port; see table above).

## UI Restart (2026-11)

The UI layer is being restarted around semantic controls; decisions are recorded
in `native/docs/adr/`:

- `0001-ui-layer-restart.md` — restart UI only; keep model/engine/io/tests.
- `0002-semantic-controls.md` — `app/ui/` controls, `Canvas`, Classic/Studio profiles.
- `0003-fidelity-and-truth.md` — fidelity definition and fwref demotion.
- `0004-imgui-role.md` — ImGui is the host, not the LCD compositor.

The Instrument General screen is the first vertical slice, opt-in with
`--gui-next`; `views/InstrumentBars.cpp` remains the legacy path until the gate
passes. Do not add new screens to the legacy renderer.

## Reference Tiers (ground truth)

1. **Source-true** (`native/tools/fwref`): the unmodified firmware `NewDisplay`
   controls compiled on the host, driven by hash-stamped specs extracted from the
   firmware screen editors. Specs exist for all nine screens; references
   remain advisory until the fwref audit (ADR-0003) is complete.

2. **Hand-drawn** (`native/tools/display_emulator/FT812ScreensCore/Extra.cpp`): the
   remaining screens still measure against hand-drawn approximations while they are
   migrated to source-true fwref.

## Fidelity Targets

Measured with `trakr_app --lcd-shot` (exact 800x480 framebuffer), source-true
screens must hold ≥95.0% pixel similarity. Current source-true scores:

| Screen | Pixel % | Status |
|--------|---------|--------|
| Pattern | 98.36% | ✅ |
| Master | 98.14% | ✅ |
| Instrument | 97.75% | ✅ |

## Code Layout

- `native/app/hw/HeaderBar.h/.cpp` — firmware title bar port.
- `native/app/hw/ScreenButtons.h/.cpp` — firmware bottom-button bar port.
- `native/app/views/*.cpp` — per-screen firmware ports (< 400 lines each).
- `native/tools/fwref/` — source-true reference renderer + specs + gates.
- `native/tools/display_emulator/` — FT812 framebuffer emulator + legacy hand-drawn references.
- `native/tests/` — 125 Catch2 test cases + fwref gates.
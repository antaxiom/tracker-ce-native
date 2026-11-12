# Gate: Instrument Effects and Automation

- Date: 2026-11-12
- Profile: Classic (800x480)
- Paths: `--screen instrument`, click More (Effects), click Automation Page
- Decision: **PASS (source-true fwref states added)**

## Scope

`InstrumentPagesScreen` mirrors `gInstrEdit.cpp::showEffectScreen()` and
`showInstrumentEnv()`:

- Effects: Overdrive / Bit depth / Reverb send / Delay send bars, the blank
  cover over columns 4..7, and the frame places for columns 0..3.
- Automation: destination list (two columns, 6 rows), type list (3 rows),
  LFO shape/speed lists (5/18 rows) when the slot is an LFO, envelope bars
  (attack/decay/sustain/release/amount, inactive when the type is Off), LFO
  amount bar, and the 7 automation frame places.

List clicks and keyboard edits are delegated to `InstrumentView`
(`setSelectedAutoDest`, `adjustSelectedParam`, `resetSelectedParam`); list
selection writes the instrument automation slot and publishes to the engine.

## Evidence

| Check | Result |
| --- | --- |
| Unit tests | 133/133 pass |
| Effects vs fwref workspace | 0.01% pixels differ (99.99% similarity) |
| Automation vs fwref workspace | 0.02% pixels differ (99.98% similarity) |
| Robot audit | 114 frames pass on the default semantic path |
| fwref | `instrument_effects.json` and `instrument_automation.json`; strict audit 0/0 |

## Conditions

1. References are workspace-cropped because the bottom page buttons differ by
   design (the desktop deck spells out Overdrive/Bit depth/... labels).
2. The bottom labels and page buttons remain app chrome.
3. Effects page hides columns 4..7 with the firmware blank cover; automation
   lists are recreated by the semantic display rather than reused per page.

## Next

Both states now exist; next is retiring the legacy Instrument renderer.

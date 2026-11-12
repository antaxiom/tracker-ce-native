# Gate: Instrument Effects and Automation

- Date: 2026-11-12
- Profile: Classic (800x480)
- Paths: `--screen instrument`, click More (Effects), click Automation Page
- Decision: **PASS (legacy-equivalent; no fwref state for these pages)**

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
| Effects vs legacy render | 0.18% pixels differ (L4 vs TTF glyphs) |
| Automation vs legacy render | 0.73% pixels differ (L4 vs TTF glyphs) |
| Robot audit | 114 frames pass on the default semantic path |
| fwref | No Effects/Automation state in the instrument spec (FW-7 gap) |

## Conditions

1. No source-true fwref state for these pages; parity is legacy-equivalence
   plus layout invariants, not pixel comparison against the firmware oracle.
2. The bottom labels and page buttons remain app chrome.
3. Effects page hides columns 4..7 with the firmware blank cover; automation
   lists are recreated by the semantic display rather than reused per page.

## Next

Add Effects/Automation fwref states (requires new spec entries), then retire
the legacy Instrument renderer.

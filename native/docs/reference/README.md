# Polyend reference library (read this before touching any UI/workflow code)

Trakr Native is "the Polyend Tracker, desktop, without limits". Every screen, key
binding, colour and workflow should be traceable to something in this folder or in
the read-only firmware tree at `core/` (Tracker CE). When in doubt: **copy the
hardware behaviour first, then lift the limit.**

## Source-of-truth order
1. **Firmware source** (read-only, never edit): `core/interface/<module>/` and
   `core/source/Themes.cpp`. Colours, layouts and labels live here.
2. **Manual** `Polyend-Tracker-Manual-1.7.0.pdf` (searchable text:
   `tracker-manual-1.7.0.txt`). Feature-parity checklist = this document.
3. **Essentials guides** for Tracker+ and Tracker Mini (`*-Essentials-*.pdf`/`.txt`).
4. **Screenshots** in `screens/` (frames from the reference video
   https://youtu.be/sgvRkWcgTh8 and the Tracker Mini render).

## Manual quick index (text file line numbers, manual 1.7.0)
| Topic | Lines | Topic | Lines |
|---|---|---|---|
| Page layout / header | 700 | Quick reference commands | 859-1100 |
| Page map | 1464 | Project / file menu | 1554 |
| Config options | 2068 | Pattern page layouts | 2847 |
| Step params (colour coding) | 3503 | Rec mode (green/red box) | 3610 |
| Fill / step jump | 3821-3887 | Note / Instrument / FX steps | 4762 / 4976 / 5212 |
| Instrument params p1 | 5732 | Instrument FX | 5870 |
| Automation p2 | 5983 | Play modes | 6322-7338 |
| Step FX reference (37 FX) | 7458-10248 | Master pages | 10561 |
| Sample editor | 11798-12490 | Song mode | 12561-13593 |
| Master FX | 13593-14331 | MIDI | 14331-16182 |
| Performance mode | 16182-16972 | UI style / hex option | 17188 / 17357 |

## Video analysis notes
See `docs/reference/screens/` and the plan artifact. Highlights:
* One 800x480 screen: black bg, white header bar with page title left, play time,
  transport glyph, `P<pattern>.<step>` and selected instrument right.
* Bottom row of 8 "screen buttons": label + current value + coloured underline
  (green = usable, red = disabled/alt). Dimmed when unavailable.
* Cursor box: **green = play mode, red = REC (edit) mode**, with a *solid fill on the
  active sub-column* (note / instr / FX1 / FX2).
* Empty-cell dashes are tinted per sub-column (dim teal / olive / violet / blue).

## Licensing
The PDFs are © Polyend sp. z o.o. They are kept here for development reference only
and must not be shipped inside release bundles (excluded from installers). Firmware
source is under the Polyend Source Available License (see `NOTICE.md`,
`native/CREDITS.md`).


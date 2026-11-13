# Gate: Master 2/3 and Master EQ

- Date: 2026-11-12
- Profile: Classic (800x480)
- Paths: `trakr_app --screen master --script Tab` (2/3), `--master-page eq` (EQ page)
- Decision: **PASS for Track Mixer (2/3) and Master EQ**

## Scope

`TrackMixerScreen` mirrors `gMasterParams.cpp::showMixerScreen()`: eight
top-bar track name labels (Bold17), eight level bars (right of the fader) and
eight gain faders with `inactiveBarColors` when muted. Frame and blank covers
are hidden on this page, matching the firmware.

`MasterEqScreen` mirrors `showEqualizerScreen()`: five band bars (columns 0..4)
mapped from `masterEq` (-12..+12 dB -> 0..100), the selected-band frame
(`framesPlaces[0..4]`, `refreshEqualizerFrame()`), the blank cover over columns
5..7 and the eight bottom labels (five band names with `"%.2f dB"`, two blanks
and `Back`). Its reference is `specs/master_eq.json`, extracted from the
firmware source by `extract_spec.py`.

Page switching: Tab is handled by all three Master screens
(`MixerView::togglePage`), and the app Page buttons keep working.

## Evidence

| Check | Result |
|---|---|
| Unit tests | 134/134 pass (538,204 assertions); gain curve and EQ frame covered |
| Track Mixer vs fwref page 2 | 0.63% pixels differ (legacy path 4.24% on old master data) |
| Master EQ vs fwref `master_eq.png` | 0.23% pixels differ (99.77% similarity) |
| Global Mixer vs fwref page 3 | 0.82% pixels differ (99.18% similarity) |
| Master EQ vs hardware photo | `refcal` tile `master_eq_hw_left` passes 8 structural checks (frame on the selected band, band fills, labels) |
| Master 1/3 sanity | 0.22% vs fwref page 1 |
| Robot audit | 114 frames pass on the default semantic path |

Parity data: `--screen master` now seeds the fwref scenario track names and
`trackVolume = 100` (gain +6 dB), which is what the reference frame shows.

## Conditions

1. Page 3 (Global Mixer) is ported as `GlobalMixerScreen` with model, engine
   and UI in place: `Song::auxVolume/auxMute` (firmware defaults 86 = 0 dB,
   line-in muted), `Mixer::setAuxGains` applying delay/reverb return and dry
   levels with mute, aux bus meters, and the four-column page with name
   labels, faders, level bars, frame and bottom dB/mute labels. Line In is a
   silent bus in the desktop build (no input monitoring), so its meter reads 0.
2. The bottom track mute/level labels are app chrome (ScreenButtons).
3. Live level meters render empty at idle; the reference frame is idle too.

## Next

Port Instrument Effects/Automation, then retire the legacy workspace renderer.

# Gate: Master 2/3 and Master FX

- Date: 2026-11-12
- Profile: Classic (800x480)
- Paths: `trakr_app --screen master --script Tab` (2/3), `Tab,Tab` (FX page)
- Decision: **PASS for Track Mixer (2/3); Master FX pass by layout/legacy equivalence**

## Scope

`TrackMixerScreen` mirrors `gMasterParams.cpp::showMixerScreen()`: eight
top-bar track name labels (Bold17), eight level bars (right of the fader) and
eight gain faders with `inactiveBarColors` when muted. Frame and blank covers
are hidden on this page, matching the firmware.

`MasterEqScreen` mirrors `showEqualizerScreen()`: five band bars (columns 0..4)
mapped from `masterEq` (-12..+12 dB -> 0..100) plus the blank cover over
columns 5..7.

Page switching: Tab is handled by all three Master screens
(`MixerView::togglePage`), and the app Page buttons keep working.

## Evidence

| Check | Result |
|---|---|
| Unit tests | 133/133 pass (538,195 assertions); gain curve covered |
| Track Mixer vs fwref page 2 | 0.63% pixels differ (legacy path 4.24% on old master data) |
| Master FX vs legacy | 0 pixels differ (same geometry, L4 glyphs) |
| Master 1/3 sanity | 0.22% vs fwref page 1 |
| Robot audit | 114 frames pass on the default semantic path |

Parity data: `--screen master` now seeds the fwref scenario track names and
`trackVolume = 100` (gain +6 dB), which is what the reference frame shows.

## Conditions

1. Firmware page 3 is *Global Mixer* (Delay/Reverb/Dry Mix/Line In). The app
   exposes a Master EQ page instead because the desktop engine has no muteable
   FX buses; page 3 therefore has no fwref comparison and is verified against
   the legacy layout only.
2. The bottom track mute/level labels are app chrome (ScreenButtons).
3. Live level meters render empty at idle; the reference frame is idle too.

## Next

Port Instrument Effects/Automation, then retire the legacy workspace renderer.

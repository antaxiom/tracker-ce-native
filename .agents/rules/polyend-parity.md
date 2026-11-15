---
trigger: always_on
---

# Polyend Tracker parity rule

Trakr Native must behave like a Polyend Tracker / Tracker+ with the limits removed.

Before designing or changing ANY screen, key binding, colour, menu or workflow in
`native/app/`:

1. Read `native/docs/reference/README.md` (index of manual line numbers).
2. Look up the matching firmware module in `core/interface/<module>/` and colours in
   `core/source/Themes.cpp` (READ ONLY - never edit `core/`, `platforms/`, `synth/`, `util/`).
3. Look up the matching manual section (`native/docs/reference/tracker-manual-1.7.0.txt`)
   and the screenshots in `native/docs/reference/screens/`.
4. Cite the manual page / firmware file in the PR/commit message or code comment.
5. Do NOT introduce Renoise-style patterns (side panels, docking, tab strips, sliders)
   unless the hardware has no equivalent; then document why in `native/NOTES.md`.
6. Never mention "hardware" / "Polyend" in user-facing UI strings (window titles,
   tooltips, labels). Keep brand references to About/Credits only.
7. Verify with a screenshot compared against the reference frame
   (`trakr_app --screen <name> --screenshot out.png`) before declaring done.

Hard constraints still apply: edits only inside `native/`, every authored file < 400 lines,
Catch2 + `--auto-test` must pass.


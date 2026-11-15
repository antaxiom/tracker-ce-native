# Autonomous >95% Visual Refinement & Model-Routing Protocol

When running unattended visual parity refinement (`/goal` or `/teamwork`) targeting $\ge 95\%$ similarity across the 8 FT812 reference screens:

## 1. Fast Deterministic Telemetry Over Visual Guessing
Never guess coordinates or colors from full-screen screenshots alone. Always use the single-screen hotspot analyzer for inner-loop iterations:
```zsh
python3 native/tools/os_input/os_input_runner.py --only <screen> --no-emu --hotspots
```
- Note: Requires `BypassSandbox: true` on macOS so Metal can allocate a drawable from WindowServer.
- Read the `[BANDS]` and `[HOTSPOT #1..5]` output lines directly:
  - Apply the exact `Emu=IM_COL32(R,G,B,255)` color when a color mismatch is reported.
  - Apply the exact `SHIFT HINT: move App (dx=..., dy=...)` offset to the corresponding `ImDrawList` / `ImGui::SetCursorPos` coordinates in `native/app/views/`.

## 2. Monotonic Score Ratchet (Zero Regression Policy)
Before editing any view file for `<screen>`:
1. Copy the file to `/tmp/ratchet_<File>.bak` and record the baseline similarity `%`.
2. Build (`cmake --build native/build --target trakr_app -j4`) and run `os_input_runner.py --only <screen> --no-emu --hotspots`.
3. **If similarity decreases**: immediately restore `/tmp/ratchet_<File>.bak` before trying another hypothesis. Never stack a second edit on top of a regression.

## 3. Model Consciousness & Dynamic Escalation (`flash` -> `pro`)
Be conscious of the active model tier and route subagents deliberately via `invoke_subagent`:
- **Fast Inner Loop (`Model: "flash"`)**:
  - Use `flash` for mechanical coordinate nudges, rectangle sizing, and `IM_COL32` color replacements guided by `[HOTSPOT]` telemetry.
- **Plateau Escalation (`Model: "pro"`)**:
  - If 2 consecutive edits on a screen yield `< +0.15%` improvement (local maximum / plateau), **STOP** iterating with `flash`.
  - Immediately invoke a `pro` subagent (`invoke_subagent(TypeName="self", Role="Visual Parity Escalation Architect", Model="pro", ...)`) with the `[HOTSPOT]` telemetry, the `/tmp/diff_<screen>.png` path, and the target `.cpp` file + `FT812Screens*.cpp` reference function to diagnose deeper layout or font-metric mismatches.

## 4. Hard Constraints Gate
Before declaring any milestone or `/goal` complete:
- Every authored file in `native/` must be strictly `< 400` lines (`wc -l`).
- Zero edits outside `native/`.
- `./native/build/trakr_tests` and `./native/build/trakr_app --auto-test` must pass 100%.
- Full 8-screen verification (`python3 native/tools/os_input/os_input_runner.py`) must confirm all 8 screens meet target similarity.

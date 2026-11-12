# GUI interaction audit

Two tools, both token-lean:

| Tool | Purpose | Cost profile |
|---|---|---|
| `gui_audit.py` | Scripted key sequence -> per-step frames -> **one contact sheet** per scenario | cheap: one image per scenario |
| `gui_ctl.py` | Live remote control: `start / shot / key / stop` against the running app | one screenshot per turn |

The app itself is driven through `trakr_app --lcd-shot --remote <fifo>` (`--script` for
batch runs). No OS-level automation and no window-focus flakiness.

## Model tiers

Give the operator role (`operator_prompt.md`) to a **cheap vision model** by default:

| Tier | Model (OpenRouter) | Use |
|---|---|---|
| default | `Nemotron 3.5 Super VL (free)` | interactive navigation loops, smoke checks |
| cheap | `Qwen3 VL 8B Instruct` | scripts with denser text |
| strong | `Hy4` + `--variant low` | only when a decision needs reasoning about firmware semantics |

Keep Hy4 out of routine loops. A navigation turn only needs "which key next"; an 8B/free
VL model is enough. Escalate a *single stuck step* to Hy4 rather than the whole run.

## Scripted audit (batch)

```bash
python3 native/tools/gui_audit/gui_audit.py \
  --spec native/tools/gui_audit/specs/core.json --out /tmp/gui_audit
# -> /tmp/gui_audit/<scenario>_contact.png and review_prompt.md
```

Scenario JSON: `screen`, `keys`, `expect` (one line per frame, first = initial frame),
`manual` (short excerpt). Keep expectations **verified against the manual**; a wrong
expectation produces false findings (e.g. Config Left/Right switches category, it does
not adjust the value).

## Live operator loop (interactive)

```bash
python3 native/tools/gui_audit/gui_ctl.py --out /tmp/s --screen loader start
python3 native/tools/gui_audit/gui_ctl.py --out /tmp/s shot s1     # prints /tmp/s/s1.png
python3 native/tools/gui_audit/gui_ctl.py --out /tmp/s key Down
python3 native/tools/gui_audit/gui_ctl.py --out /tmp/s shot s2
python3 native/tools/gui_audit/gui_ctl.py --out /tmp/s stop
```

Agent-side loop: read only the newest shot, ask the operator model for one key,
send it, repeat. Record findings as
`[{"scenario","step","issue","severity","manual_ref"}]`.

## Known functional gaps found by the first run

- Sample Loader had no keyboard navigation (fixed: Up/Down/Left/Right + Enter to open dirs).
- File/menu actions still use native dialogs; the firmware uses the in-app Sample Loader
  and Song screens. Menu items should route to `Screen::SampleLoader` / `Screen::Song`
  (project load/save) instead of opening desktop dialogs.

## Functional smoke suite (regression)

```bash
python3 native/tools/gui_audit/functional_smoke.py --out /tmp/smoke
python3 native/tools/gui_audit/functional_smoke.py --only pattern_copy_paste
```

17 flows drive the real app through `gui_ctl` and assert either a visible change
or a pixel-exact restore of a named prior frame (undo, delete, reset, cancel,
navigation round-trips). Use it after touching any view input/focus gating.
`--loader-dir <path>` starts the loader in a real directory instead of the
synthetic parity listing; the smoke suite does this for its loader scenarios.

## Menu/dialog testing

`gui_ctl.py --live` runs the real window (menu bar and button deck visible) so the
File/Edit menus can be driven: `click <x> <y>` does a hover-then-click and keeps
the pointer there; `type <text>` fills ImGui InputText fields. Coordinates are in
ImGui display units (on this project's window the capture is 2000x1250 for a
1280-unit layout). File dialogs are in-app modals, so they can be cancelled with
`Escape` and confirmed with `Enter`.

## Font/tofu audit

```bash
python3 native/tools/check_glyphs.py
```

Scans `native/app` + display emulator sources for non-ASCII codepoints and fails
if any are missing from the bundled RobotoMono fonts. RobotoMono lacks ▶ ↻ ● ✕ ✓ ✗
and the geometric/dingbat blocks, so icons are vector-drawn (header transport) or
ASCII (`X`, `[OK]`/`[ERR]`) and the font fallback is pinned to `?` — never U+FFFD,
which RobotoMono renders as a hollow box.

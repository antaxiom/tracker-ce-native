#!/usr/bin/env python3
"""Token-lean GUI interaction audit.

Runs scripted key sequences against the app in 1:1 LCD mode, captures one frame
per step, tiles them into a single contact sheet, and writes a compact review
prompt for a vision model (e.g. Hy4). Only the contact sheet + prompt are sent
to the reviewer, keeping context small.

Spec format (JSON list):
  [{
    "name": "loader_browse",
    "screen": "loader",
    "keys": ["Down", "Down", "Down"],
    "expect": ["initial frame", "cursor moves to next file", "..."],
    "manual": "short manual excerpt"
  }]

Usage:
  python3 native/tools/gui_audit/gui_audit.py --spec native/tools/gui_audit/specs/loader.json --out /tmp/gui_audit
"""

import argparse
import json
import os
import subprocess
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "os_input"))
from os_input_runner import read_image_rgb, write_png_rgb  # noqa: E402

TILE_W, TILE_H = 400, 240
MAX_COLS = 3


def tile_image(src, dst, tw=TILE_W, th=TILE_H):
    """Nearest-neighbour downscale to a tile; returns (tw, th, rgb)."""
    w, h, rgb = read_image_rgb(src)
    out = bytearray(tw * th * 3)
    for r in range(th):
        sy = min(h - 1, int((r + 0.5) * h / th))
        for c in range(tw):
            sx = min(w - 1, int((c + 0.5) * w / tw))
            si = (sy * w + sx) * 3
            di = (r * tw + c) * 3
            out[di:di + 3] = rgb[si:si + 3]
    write_png_rgb(dst, tw, th, bytes(out))
    return tw, th, bytes(out)


def build_contact_sheet(tiles, dst):
    """tiles: list of (tw, th, rgb). Writes a grid contact sheet."""
    n = len(tiles)
    cols = min(MAX_COLS, n)
    rows = (n + cols - 1) // cols
    gap = 4
    W = cols * TILE_W + (cols - 1) * gap
    H = rows * TILE_H + (rows - 1) * gap
    canvas = bytearray(W * H * 3)
    for i, (tw, th, rgb) in enumerate(tiles):
        r0 = (i // cols) * (TILE_H + gap)
        c0 = (i % cols) * (TILE_W + gap)
        for y in range(th):
            src = y * tw * 3
            dst_off = ((r0 + y) * W + c0) * 3
            canvas[dst_off:dst_off + tw * 3] = rgb[src:src + tw * 3]
    write_png_rgb(dst, W, H, bytes(canvas))
    return W, H


def run_scenario(app_bin, sc, out_dir):
    name = sc["name"]
    prefix = os.path.join(out_dir, name)
    steps = sc.get("keys", [])
    if steps:
        cmd = [app_bin, "--screen", sc["screen"], "--polyend-font", "--lcd-shot",
               "--screenshot", prefix + ".png", "--script", ",".join(steps)]
        subprocess.run(cmd, check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        shots = [prefix + "_step%02d.png" % (i + 1) for i in range(len(steps) + 1)]
    else:
        cmd = [app_bin, "--screen", sc["screen"], "--polyend-font", "--lcd-shot",
               "--screenshot", prefix + ".png"]
        subprocess.run(cmd, check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        shots = [prefix + ".png"]
    return [p for p in shots if os.path.exists(p)]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--spec", required=True)
    ap.add_argument("--out", default="/tmp/gui_audit")
    ap.add_argument("--app", default="./native/build/trakr_app")
    args = ap.parse_args()

    os.makedirs(args.out, exist_ok=True)
    with open(args.spec) as f:
        scenarios = json.load(f)
    if isinstance(scenarios, dict):
        scenarios = [scenarios]

    prompt = ["# GUI interaction audit", "",
              "Compare the captured frames (reading order left->right, top->bottom)"
              " against the firmware manual description. Report behavioural or visual"
              " mismatches only.", ""]

    for sc in scenarios:
        shots = run_scenario(args.app, sc, args.out)
        if not shots:
            print("[gui_audit] %s: no frames captured" % sc["name"])
            continue
        tiles = [tile_image(p, os.path.join(args.out, "%s_tile%02d.png" % (sc["name"], i + 1)))
                 for i, p in enumerate(shots)]
        sheet = os.path.join(args.out, "%s_contact.png" % sc["name"])
        W, H = build_contact_sheet(tiles, sheet)
        print("[gui_audit] %s -> %s (%dx%d, %d steps)" % (sc["name"], sheet, W, H, len(shots)))

        prompt.append("## scenario: %s (screen=%s)" % (sc["name"], sc["screen"]))
        prompt.append("contact sheet: %s" % sheet)
        prompt.append("")
        prompt.append("| step | key | expected |")
        prompt.append("|---|---|---|")
        keys = sc.get("keys", []) or []
        expect = sc.get("expect", []) or []
        for i in range(len(keys) + 1):
            k = "initial" if i == 0 else keys[i - 1]
            exp = expect[i] if i < len(expect) else "(no expectation)"
            prompt.append("| %d | %s | %s |" % (i + 1, k, exp))
        if sc.get("manual"):
            prompt += ["", "Manual excerpt:", sc["manual"]]
        prompt.append("")

    prompt += [
        "## Output",
        "Write JSON to the path given by the caller:",
        '[{"scenario":"...","step":1,"issue":"...","severity":"low|med|high","manual_ref":"..."}]',
        "Only list real mismatches. Empty list is a valid answer.",
    ]
    dst = os.path.join(args.out, "review_prompt.md")
    with open(dst, "w") as f:
        f.write("\n".join(prompt) + "\n")
    print("[gui_audit] prompt -> %s" % dst)


if __name__ == "__main__":
    main()

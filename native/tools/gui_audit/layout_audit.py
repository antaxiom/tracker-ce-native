#!/usr/bin/env python3
"""Systematic fixed-cell text-fit audit for the tracker UI.

Runs the app for every screen and secondary page with TRAKR_LAYOUT_AUDIT=1.
The shared TextFit helper reports labels that had to be shrunk or clipped to
stay inside their cell, so overflow regressions are found without eyeballing
each screenshot.

Usage:
  python3 native/tools/gui_audit/layout_audit.py
  python3 native/tools/gui_audit/layout_audit.py --strict   # leftovers fail
  python3 native/tools/gui_audit/layout_audit.py --out render/subpages

Exit codes: 0 = all cells fit at firmware size, 1 = some labels were shrunk,
2 = a label had to be clipped (hard overflow).
"""

import argparse
import os
import re
import subprocess
import sys
import tempfile

SCENARIOS = [
    ("pattern", ""),
    ("pattern", "F8,F8"),
    ("song", ""),
    ("song", "F8"),
    ("sample", ""),
    ("sample", "F7"),
    ("instrument", ""),
    ("instrument", "F8"),
    ("instrument", "F8,F5"),
    ("master", ""),
    ("perform", ""),
    ("loader", ""),
    ("loader", "F8"),
    ("config", ""),
    ("config", "F1,F1,F1,F1"),
    ("config", "F8"),
    ("file", ""),
]

LINE_RE = re.compile(r"\[LAYOUT\]\s+(\S+)\s+'(.*)'\s+box=(\d+)px advance=([\d.]+)px\s+(shrunk|CLIPPED)")


def run_scenario(app, screen, keys, outdir):
    shot = os.path.join(outdir, f"audit_{screen}_{keys.replace(',', '_') or 'main'}.png")
    cmd = [app, "--screen", screen, "--polyend-font", "--lcd-shot", "--screenshot", shot]
    if keys:
        cmd += ["--script", keys]
    env = dict(os.environ)
    env["TRAKR_LAYOUT_AUDIT"] = "1"
    proc = subprocess.run(cmd, env=env, stdout=subprocess.DEVNULL,
                          stderr=subprocess.PIPE, text=True, timeout=120)
    events = []
    for line in proc.stderr.splitlines():
        m = LINE_RE.search(line)
        if m:
            events.append({
                "cell": m.group(1),
                "text": m.group(2),
                "box": int(m.group(3)),
                "advance": float(m.group(4)),
                "clipped": m.group(5) == "CLIPPED",
            })
    return events


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--app", default="./native/build/trakr_app")
    ap.add_argument("--strict", action="store_true",
                    help="exit non-zero when any label is shrunk (not just clipped)")
    ap.add_argument("--out", default=None,
                    help="keep per-page screenshots in this directory")
    args = ap.parse_args()

    if not os.path.exists(args.app):
        sys.exit(f"app not found: {args.app}")

    seen = set()
    report = []
    outdir = args.out
    cleanup = None
    if outdir:
        os.makedirs(outdir, exist_ok=True)
    else:
        cleanup = tempfile.TemporaryDirectory(prefix="trakr_layout_")
        outdir = cleanup.name
    try:
        for screen, keys in SCENARIOS:
            label = keys or "main"
            try:
                events = run_scenario(args.app, screen, keys, outdir)
            except subprocess.TimeoutExpired:
                report.append((screen, label, None, "TIMEOUT"))
                continue
            for e in events:
                key = (e["cell"], e["text"], e["box"], e["clipped"])
                if key in seen:
                    continue
                seen.add(key)
                report.append((screen, label, e, None))
    finally:
        if cleanup:
            cleanup.cleanup()

    clipped = [r for r in report if isinstance(r[2], dict) and r[2]["clipped"]]
    shrunk = [r for r in report if isinstance(r[2], dict) and not r[2]["clipped"]]
    errors = [r for r in report if r[2] is None]

    print(f"Scenarios: {len(SCENARIOS)}  offenders: {len(shrunk) + len(clipped)}")
    for screen, label, e, err in report:
        if err:
            print(f"  {screen:<11} {label:<10} {err}")
        else:
            mode = "CLIPPED" if e["clipped"] else "shrunk"
            print(f"  {screen:<11} {label:<10} {e['cell']:<14} '{e['text']}' "
                  f"box={e['box']}px advance={e['advance']:.1f}px {mode}")

    if clipped or errors:
        return 2
    if shrunk and args.strict:
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())

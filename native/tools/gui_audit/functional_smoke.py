#!/usr/bin/env python3
"""Functional smoke tests for the tracker replica.

Drives the real app through gui_ctl and verifies each core editing flow either
changes the screen or returns it to an exact prior state (pixel-exact when idle,
which catches no-op keys and broken undo/reset paths).

Usage:
  python3 native/tools/gui_audit/functional_smoke.py [--only name] [--out /tmp/smoke]
"""
import argparse
import os
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
sys.path.insert(0, os.path.join(ROOT, "native", "tools", "os_input"))
from os_input_runner import read_image_rgb  # noqa: E402
import numpy as np  # noqa: E402

CTL = os.path.join(ROOT, "native", "tools", "gui_audit", "gui_ctl.py")
APP = os.path.join(ROOT, "native", "build", "trakr_app")

# steps: (keys, expectation, reference) where expectation is change|same and
# reference is prev|base.
SCENARIOS = [
    ("pattern_record_undo", "pattern", [
        (["Ctrl+R"], "change", "base", None),
        (["Z"], "change", "prev", None),
        (["Ctrl+Z"], "change", "prev", None),
        (["Ctrl+Y"], "same", 2, None),
        (["Ctrl+R"], "change", "prev", None),
        (["Ctrl+Z"], "change", "prev", None),
        (["Up"], "same", "base", None),
    ]),
    ("pattern_delete", "pattern", [
        (["Ctrl+R"], "change", "base", None),
        (["Z"], "change", "prev", None),
        (["Up"], "change", "prev", None),
        (["Delete"], "change", "prev", None),
        (["Up"], "same", 1, None),
    ]),
    ("pattern_copy_paste", "pattern", [
        (["Ctrl+R"], "change", "base", None),
        (["Z"], "change", "prev", None),
        (["Up"], "change", "prev", None),
        (["Ctrl+C"], "same", "prev", None),
        (["Down"], "change", "prev", None),
        (["Ctrl+V"], "change", "prev", None),
        (["Ctrl+Z"], "change", "prev", None),
        (["Ctrl+Z"], "change", "prev", None),
        (["Ctrl+R"], "change", "prev", None),
        (["Up"], "change", "prev", None),
        (["Up"], "same", "base", None),
    ]),
    ("pattern_nav", "pattern", [
        (["Down"], "change", "base", None),
        (["Up"], "same", "base", None),
    ]),
    ("pattern_transport", "pattern", [
        (["Space"], "change", "base", None),
        (["Space"], "same", "base", [250, 0, 220, 24]),
    ]),
    ("song_cursor", "song", [
        (["Insert"], "change", "base", None),
        (["Up"], "change", "prev", None),
        (["Down"], "change", "prev", None),
        (["Delete"], "same", "base", None),
    ]),
    ("song_pattern", "song", [
        (["Right"], "change", "base", None),
        (["Left"], "same", "base", None),
    ]),
    ("song_insert", "song", [
        (["Insert"], "change", "base", None),
        (["Delete"], "same", "base", None),
    ]),
    ("instrument_value", "instrument", [
        (["Up"], "change", "base", None),
        (["Delete"], "same", "base", None),
    ]),
    ("instrument_col", "instrument", [
        (["Right"], "change", "base", None),
        (["Left"], "same", "base", None),
    ]),
    ("sample_param", "sample", [
        (["Right"], "change", "base", None),
        (["Left"], "same", "base", None),
    ]),
    ("sample_value", "sample", [
        (["Up"], "change", "base", None),
        (["Down"], "same", "base", None),
    ]),
    ("config_item", "config", [
        (["Down"], "change", "base", None),
        (["Up"], "same", "base", None),
    ]),
    ("config_category", "config", [
        (["Right"], "change", "base", None),
        (["Left"], "same", "base", None),
    ]),
    ("loader_cursor", "loader", [
        (["Down"], "change", "base", None),
        (["Up"], "same", "base", None),
    ]),
    ("loader_dir", "loader", [
        (["Down"], "change", "base", None),
        (["Enter"], "change", "prev", None),
        (["Enter"], "same", "base", None),
        (["Down"], "change", "base", None),
        (["Up"], "same", "base", None),
    ]),
    ("master_track", "master", [
        (["Right"], "change", "base", None),
        (["Left"], "same", "base", None),
        (["Up"], "change", "base", None),
        (["Delete"], "same", "base", None),
    ]),
]


def ctl(out, *args):
    subprocess.run([sys.executable, CTL, "--out", out, *args], check=True,
                   stdout=subprocess.DEVNULL)


def load(path):
    w, h, b = read_image_rgb(path)
    return np.frombuffer(b, dtype=np.uint8).reshape(h, w, 3).astype(np.int16)


def diff(a, b):
    return int((np.abs(a - b).max(axis=2) > 24).sum())


def run_one(out, name, screen, steps):
    shot_dir = os.path.join(out, name)
    os.makedirs(shot_dir, exist_ok=True)
    start_args = ["start", "--screen", screen, "--app", APP]
    if screen == "loader":
        tree = os.path.join(out, "loader_tree")
        os.makedirs(os.path.join(tree, "Alpha"), exist_ok=True)
        os.makedirs(os.path.join(tree, "Beta"), exist_ok=True)
        start_args += ["--loader-dir", tree]
    ctl(shot_dir, *start_args)
    try:
        base_path = os.path.join(shot_dir, "base.png")
        ctl(shot_dir, "shot", base_path)
        images = [load(base_path)]
        prev_index = 0
        failures = []
        for i, (keys, expect, ref, region) in enumerate(steps, 1):
            for k in keys:
                ctl(shot_dir, "key", k)
            path = os.path.join(shot_dir, f"step{i:02d}.png")
            ctl(shot_dir, "shot", path)
            images.append(load(path))
            if ref == "base":
                ref_index = 0
            elif ref == "prev":
                ref_index = prev_index
            else:
                ref_index = int(ref)
            a, b = images[ref_index], images[-1]
            if region:
                x, y, w, h = region
                a, b = a[y:y + h, x:x + w], b[y:y + h, x:x + w]
            d = diff(a, b)
            ok = (d > 20) if expect == "change" else (d == 0)
            if not ok:
                failures.append(f"step{i} {keys} expected {expect} vs {ref}, diff={d}")
            prev_index = len(images) - 1
        return failures
    finally:
        try:
            ctl(shot_dir, "stop")
        except SystemExit:
            pass


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--only")
    ap.add_argument("--out", default="/tmp/smoke")
    args = ap.parse_args()
    os.makedirs(args.out, exist_ok=True)
    selected = [s for s in SCENARIOS if not args.only or s[0] == args.only]
    failed = 0
    for name, screen, steps in selected:
        failures = run_one(args.out, name, screen, steps)
        status = "PASS" if not failures else "FAIL"
        print(f"[{status}] {name}")
        for f in failures:
            print("   ", f)
        failed += bool(failures)
    print(f"{len(selected) - failed}/{len(selected)} scenarios passed")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())

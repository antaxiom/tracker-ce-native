#!/usr/bin/env python3
"""Classic visual-regression gate.

Renders each semantic screen at the native 800x480 LCD size and compares it to
the audited fwref references (advisory) with a similarity threshold. This is the
Phase 6 replacement for treating pixel parity as an architecture constraint:
Classic output is checked for regressions, while layout invariants and the unit
suite carry the structural guarantees.

Usage:
    python3 native/tools/visreg/visreg.py --all
    python3 native/tools/visreg/visreg.py --only song,master2
    python3 native/tools/visreg/visreg.py --all --threshold 0.98
    python3 native/tools/visreg/visreg.py --all --update  # write current shots

References are produced by:
    ./native/build/fwref_gen --out render/fwref --strict
    ./native/build/fwref_gen --out render/fwref --sample-state slice --strict
    ./native/build/fwref_gen --out render/fwref --sample-state granular --strict
"""

from __future__ import annotations

import argparse
import hashlib
import os
import struct
import subprocess
import sys
import tempfile
import zlib
from dataclasses import dataclass


@dataclass(frozen=True)
class Case:
    name: str
    screen: str
    ref: str | None
    script: str | None = None
    args: tuple[str, ...] = ()
    crop: tuple[int, int, int, int] | None = None


CASES = [
    Case("pattern", "pattern", "pattern.png"),
    Case("song", "song", "song.png"),
    Case("instrument", "instrument", "instrument_params.png"),
    Case("instrument_effects", "instrument", "instrument_effects.png",
         args=("--instrument-page", "effects"), crop=(0, 28, 800, 396)),
    Case("instrument_automation", "instrument", "instrument_automation.png",
         args=("--instrument-page", "automation"), crop=(0, 28, 800, 396)),
    Case("master1", "master", "master_page1.png"),
    Case("master2", "master", "master_page2.png", "Tab"),
    Case("master3", "master", "master_page3.png", args=("--master-page", "global")),
    Case("master_eq", "master", "master_eq.png", args=("--master-page", "eq")),
    Case("sample", "sample", "sample.png"),
    Case("sample_slice", "sample", "sample_slice.png", "Up,Up,Up"),
    Case("sample_granular", "sample", "sample_granular.png", "Up,Up,Up,Up,Up,Up"),
    Case("loader", "loader", "loader.png"),
    Case("config", "config", "config.png"),
    Case("perform", "perform", "perform.png"),
    Case("file", "file", "file.png"),
]

WIDTH, HEIGHT = 800, 480


def image_info(path: str) -> tuple[int, int]:
    out = subprocess.run(["magick", "identify", "-format", "%w %h", path],
                         check=True, capture_output=True, text=True)
    w, h = out.stdout.split()
    return int(w), int(h)


def mismatch(a: str, b: str) -> float:
    """Fraction of differing pixels via ImageMagick (project dependency)."""
    w, h = image_info(a)
    wb, hb = image_info(b)
    if (w, h) != (wb, hb):
        return 1.0
    out = subprocess.run(["compare", "-metric", "AE", a, b, "null:"],
                         capture_output=True, text=True)
    text = (out.stderr or out.stdout).strip().split()
    if not text:
        raise RuntimeError("compare produced no metric")
    diff = float(text[0])
    total = w * h
    if total <= 0:
        return 1.0
    return min(1.0, diff / total)


def run_case(case: Case, app: str, refs: str, out_dir: str) -> tuple[str, float | None, float, str]:
    shot = os.path.join(out_dir, case.name + ".png")
    cmd = [app, "--screen", case.screen, "--lcd-shot"]
    if case.script:
        cmd += ["--script", case.script]
    cmd += list(case.args)
    cmd += ["--screenshot", shot]
    subprocess.run(cmd, check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

    produced = shot
    if case.script:
        base = shot[:-4]
        steps = sorted(
            (p for p in os.listdir(out_dir) if p.startswith(os.path.basename(base) + "_step")),
            key=lambda p: int(p.rsplit("_step", 1)[1].split(".")[0]),
        )
        if not steps:
            return case.name, None, 0.0, "no script capture"
        produced = os.path.join(out_dir, steps[-1])

    width, height = image_info(produced)
    if (width, height) != (WIDTH, HEIGHT):
        return case.name, None, 0.0, f"bad size {width}x{height}"

    if not case.ref:
        return case.name, None, 0.0, "rendered (no reference)"

    ref_path = os.path.join(refs, case.ref)
    if not os.path.exists(ref_path):
        return case.name, None, 0.0, f"missing reference {case.ref}"

    if case.crop:
        x, y, w, h = case.crop
        for src, dst in ((produced, produced + ".crop.png"), (ref_path, ref_path + ".crop.png")):
            subprocess.run(["magick", src, "-crop", f"{w}x{h}+{x}+{y}", "+repage", dst],
                           check=True)
        produced, ref_path = produced + ".crop.png", ref_path + ".crop.png"

    diff = mismatch(produced, ref_path)
    return case.name, diff, 1.0 - diff, ""


def sha256(path: str) -> str:
    with open(path, "rb") as f:
        return hashlib.sha256(f.read()).hexdigest()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--app", default="native/build/trakr_app")
    parser.add_argument("--refs", default="render/fwref")
    parser.add_argument("--out", default="")
    parser.add_argument("--threshold", type=float, default=0.95,
                        help="minimum pixel similarity (default 0.95)")
    parser.add_argument("--only", default="", help="comma separated case names")
    parser.add_argument("--all", action="store_true", help="run every case (default)")
    parser.add_argument("--deterministic", action="store_true",
                        help="render one screen twice and require identical bytes")
    args = parser.parse_args()

    root = os.getcwd()
    if not os.path.exists(args.app):
        args.app = os.path.join(root, args.app)
    if not os.path.isabs(args.refs):
        args.refs = os.path.join(root, args.refs)

    selected = CASES
    if args.only:
        wanted = {s.strip() for s in args.only.split(",") if s.strip()}
        selected = [c for c in CASES if c.name in wanted]
        missing = wanted - {c.name for c in selected}
        if missing:
            print(f"unknown cases: {', '.join(sorted(missing))}", file=sys.stderr)
            return 2
    if not selected:
        print("no cases selected", file=sys.stderr)
        return 2

    out_dir = args.out or tempfile.mkdtemp(prefix="trakr_visreg_")
    os.makedirs(out_dir, exist_ok=True)

    failures = 0
    print(f"{'case':16} {'similarity':>10}  status")
    for case in selected:
        name, diff, similarity, note = run_case(case, args.app, args.refs, out_dir)
        if diff is None:
            status = note or "no reference"
            ok = note.startswith("rendered")
            print(f"{name:16} {'-':>10}  {status}")
            if not ok:
                failures += 1
            continue
        ok = similarity >= args.threshold
        print(f"{name:16} {similarity * 100:9.2f}%  {'OK' if ok else 'FAIL'} ({diff * 100:.2f}% differ)")
        if not ok:
            failures += 1

    if args.deterministic:
        case = selected[0]
        a = os.path.join(out_dir, "det_a.png")
        b = os.path.join(out_dir, "det_b.png")
        for target in (a, b):
            subprocess.run([args.app, "--screen", case.screen, "--lcd-shot",
                            "--screenshot", target],
                           check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        same = sha256(a) == sha256(b)
        print(f"{'determinism':16} {'-':>10}  {'OK' if same else 'FAIL'} ({case.name})")
        if not same:
            failures += 1

    print(f"\n{len(selected) - failures}/{len(selected)} cases passed "
          f"(threshold {args.threshold * 100:.1f}%)")
    print(f"captures: {out_dir}")
    return 0 if failures == 0 else 1


if __name__ == "__main__":
    sys.exit(main())

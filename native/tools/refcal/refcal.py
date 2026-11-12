#!/usr/bin/env python3
"""Hardware reference calibration and structural checks (FW-8).

The reference videos are photographs of the real LCD: perspective, lifted
blacks and a colour cast. This tool rectifies one LCD tile to 800x480,
calibrates it per channel, and compares structure (header/bottom separators,
frame box, filter list bands) against an audited fwref reference.

It intentionally does not do pixel-identity comparison; ADR-0003 makes
calibrated structure the hardware check and pixel parity an advisory metric.

Usage:
    refcal.py --frame IMG --tile x,y,w,h --ref render/fwref/instrument_params.png
    refcal.py --spec native/tools/refcal/specs.json --out /tmp/refcal
"""

from __future__ import annotations

import argparse
import json
import shutil
import subprocess
import sys
import tempfile
from dataclasses import dataclass, field
from pathlib import Path

import numpy as np
from scipy import ndimage

ROOT = Path(__file__).resolve().parents[3]
DEFAULT_SPEC = Path(__file__).resolve().parent / "specs.json"
DEVICE_W = 800
DEVICE_H = 480
HEADER_H = 28
BUTTONS_TOP = 424


def run_magick(args: list[str]) -> bytes:
    exe = shutil.which("magick") or shutil.which("convert")
    if exe is None:
        raise RuntimeError("ImageMagick (magick) is required")
    result = subprocess.run([exe, *args], capture_output=True, check=False)
    if result.returncode != 0:
        raise RuntimeError(f"magick failed: {result.stderr.decode('utf-8', 'ignore')}")
    return result.stdout


def load_image(path: Path) -> np.ndarray:
    raw = run_magick([str(path), "-background", "black", "-alpha", "remove",
                      "-alpha", "off", "-depth", "8", "rgb:-"])
    probe = run_magick([str(path), "-format", "%w %h", "info:"]).decode().split()
    w, h = int(probe[0]), int(probe[1])
    data = np.frombuffer(raw, dtype=np.uint8)
    return data.reshape(h, w, 3)


def save_rgb(rgb: np.ndarray, path: Path) -> None:
    h, w, _ = rgb.shape
    tmp = path.with_suffix(".rgb")
    tmp.write_bytes(np.ascontiguousarray(rgb.astype(np.uint8)).tobytes())
    run_magick(["-size", f"{w}x{h}", "-depth", "8", f"rgb:{tmp}", str(path)])
    tmp.unlink(missing_ok=True)


def resize_to_device(rgb: np.ndarray, path: Path) -> np.ndarray:
    h, w, _ = rgb.shape
    tmp = path.with_suffix(".src.rgb")
    tmp.write_bytes(np.ascontiguousarray(rgb).tobytes())
    run_magick(["-size", f"{w}x{h}", "-depth", "8", f"rgb:{tmp}",
                "-filter", "Lanczos", "-resize", f"{DEVICE_W}x{DEVICE_H}!",
                str(path)])
    tmp.unlink(missing_ok=True)
    return load_image(path)


def calibrate(rgb: np.ndarray) -> tuple[np.ndarray, dict]:
    flat = rgb.reshape(-1, 3).astype(np.float32)
    black = np.percentile(flat, 1.0, axis=0)
    white = np.percentile(flat, 99.0, axis=0)
    span = np.maximum(white - black, 1.0)
    out = np.clip((rgb.astype(np.float32) - black) * (255.0 / span), 0.0, 255.0)
    return out.astype(np.uint8), {"black": black.round(2).tolist(),
                                  "white": white.round(2).tolist()}


def _groups(mask: np.ndarray, max_gap: int = 2) -> list[tuple[int, int]]:
    idx = np.where(mask)[0]
    if idx.size == 0:
        return []
    groups: list[list[int]] = [[int(idx[0]), int(idx[0])]]
    for value in idx[1:]:
        value = int(value)
        if value - groups[-1][1] <= max_gap:
            groups[-1][1] = value
        else:
            groups.append([value, value])
    return [(a, b) for a, b in groups]


def detect_lcd(tile: np.ndarray) -> dict:
    gray = tile.mean(axis=2)
    h, w = gray.shape
    counts = (gray > 90).sum(axis=1)
    search_end = max(10, int(h * 0.35))
    search = counts[:search_end]
    if int(search.max()) < int(0.5 * w):
        raise RuntimeError("cannot find the LCD header separator")

    # The separator is the longest bright horizontal line in the header area.
    # Header text can also cover >50% of a row, so pick the row with the
    # maximum extent, then the topmost row close to that maximum.
    peak = int(search.max())
    candidates = np.where(search >= int(peak * 0.95))[0]
    header = int(candidates[0])

    row = gray[header]
    xs = np.where(row > 90)[0]
    if xs.size < 0.3 * w:
        row = gray[min(h - 1, header + 1)]
        xs = np.where(row > 90)[0]
    if xs.size == 0:
        raise RuntimeError("cannot find the LCD horizontal extent")
    left, right = int(xs.min()), int(xs.max()) + 1
    scale = (right - left) / float(DEVICE_W)
    top = header - HEADER_H * scale
    bottom = top + DEVICE_H * scale

    return {
        "header_y": header,
        "left": left,
        "right": right,
        "top": max(0.0, top),
        "bottom": min(float(h), bottom),
        "scale": scale,
    }


def structural_metrics(rgb: np.ndarray) -> dict:
    gray = rgb.mean(axis=2)
    header = int(np.argmax((gray[:60] > 90).sum(axis=1)))

    red = ((rgb[:, :, 0] > 100) & (rgb[:, :, 1] < 110) & (rgb[:, :, 2] < 110) &
           (rgb[:, :, 0].astype(int) > rgb[:, :, 1].astype(int) + 40) &
           (rgb[:, :, 0].astype(int) > rgb[:, :, 2].astype(int) + 40))
    red[:20, :] = False
    red[BUTTONS_TOP:, :] = False
    # The photo frame line can break into several components, so score each
    # 100px column window and take the bbox of all red pixels inside it.
    frame = None
    scores = []
    for c in range(8):
        x0 = 1 + c * 100 - 8
        x1 = x0 + 100 + 16
        scores.append(int(red[:, max(0, x0):min(DEVICE_W, x1)].sum()))
    if scores and max(scores) > 150:
        c = int(np.argmax(scores))
        x0 = max(0, 1 + c * 100 - 8)
        x1 = min(DEVICE_W, x0 + 100 + 16)
        ys, xs = np.where(red[:, x0:x1])
        if ys.size:
            frame = {"x": int(xs.min()) + x0, "y": int(ys.min()),
                     "w": int(xs.max() - xs.min() + 1),
                     "h": int(ys.max() - ys.min() + 1)}

    def cells(x0: int, x1: int, y0: int, y1: int, rows: int, cols: int,
              row_h: int, col_w: int) -> list[float]:
        out: list[float] = []
        for r in range(rows):
            for c in range(cols):
                region = gray[y0 + r * row_h:y0 + (r + 1) * row_h,
                              x0 + c * col_w:x0 + (c + 1) * col_w]
                out.append(float((region > 60).mean()) if region.size else 0.0)
        return out

    # cList (filter) rows use the firmware 27px pitch; cLabel bottom buttons
    # use the eight 100px screen cells. Energies are value-independent.
    filter_cells = cells(401, 498, HEADER_H + 5, 29, 4, 1, 27, 97)
    bottom_cells = cells(0, 800, 431, 455, 1, 8, 24, 100)

    return {
        "header_sep_y": header,
        "frame": frame,
        "filter_cells": [round(v, 4) for v in filter_cells],
        "bottom_cells": [round(v, 4) for v in bottom_cells],
        "row_profile": gray.mean(axis=1).round(2).tolist(),
        "col_profile": gray.mean(axis=0).round(2).tolist(),
    }


def compare(hw: dict, ref: dict, tolerance: int = 3, check_filter: bool = True,
            check_frame: bool = True) -> list[dict]:
    checks: list[dict] = []

    def check(name: str, ok: bool, detail: str) -> None:
        checks.append({"name": name, "ok": bool(ok), "detail": detail})

    check("header_separator", abs(hw["header_sep_y"] - HEADER_H) <= tolerance,
          f"{hw['header_sep_y']} vs firmware {HEADER_H}")

    frame = hw["frame"]
    if not check_frame:
        pass
    elif frame is None:
        check("frame_box", False, "no red frame detected")
    else:
        check("frame_box", 90 <= frame["w"] <= 110 and 340 <= frame["h"] <= 400,
              f"{frame}")
        col = round((frame["x"] + frame["w"] / 2.0 - 1 - 48.5) / 100.0)
        expected_x = 1 + 100 * col
        check("frame_grid_x", abs(frame["x"] - expected_x) <= 8,
              f"x={frame['x']} expected~{expected_x}")
        check("frame_grid_y", abs(frame["y"] - 30) <= 8, f"y={frame['y']}")
        ref_frame = ref["frame"]
        if ref_frame:
            checks.append({"name": "frame_height_advisory", "ok": True,
                           "detail": f"hw {frame['h']} vs ref {ref_frame['h']}"})

    if check_filter:
        hw_filter = hw["filter_cells"]
        ref_filter = ref["filter_cells"]
        present = sum(1 for v in hw_filter if v > 0.015)
        check("filter_list_rows", present >= 4,
              f"present {present}/4 (hw={hw_filter}, ref={ref_filter})")

    hw_bottom = hw["bottom_cells"]
    ref_bottom = ref["bottom_cells"]
    present = sum(1 for v in hw_bottom if v > 0.010)
    check("bottom_label_cells", present >= 6,
          f"present {present}/8 (hw={hw_bottom}, ref={ref_bottom})")

    def corr(a, b) -> float:
        a = np.asarray(a, dtype=float)
        b = np.asarray(b, dtype=float)
        if a.size != b.size or a.std() == 0 or b.std() == 0:
            return 0.0
        return float(np.corrcoef(a, b)[0, 1])

    checks.append({"name": "row_profile_correlation", "ok": True,
                   "detail": f"advisory {corr(hw['row_profile'], ref['row_profile']):.3f}"})
    checks.append({"name": "column_profile_correlation", "ok": True,
                   "detail": f"advisory {corr(hw['col_profile'], ref['col_profile']):.3f}"})
    return checks


@dataclass
class Result:
    name: str
    frame: str
    ref: str
    ok: bool = False
    calibration: dict = field(default_factory=dict)
    detection: dict = field(default_factory=dict)
    checks: list[dict] = field(default_factory=list)
    artifacts: dict = field(default_factory=dict)


def run_one(entry: dict, out_dir: Path) -> Result:
    name = entry["name"]
    frame_path = (ROOT / entry["frame"]).resolve()
    ref_path = (ROOT / entry["ref"]).resolve()
    if not frame_path.exists():
        raise RuntimeError(f"missing frame: {frame_path}")
    if not ref_path.exists():
        raise RuntimeError(f"missing reference: {ref_path}")

    frame = load_image(frame_path)
    tile = entry.get("tile")
    if tile:
        x, y, w, h = [int(v) for v in tile]
        frame = frame[y:y + h, x:x + w]
    detection = detect_lcd(frame)
    x0, x1 = detection["left"], detection["right"]
    y0, y1 = int(round(detection["top"])), int(round(detection["bottom"]))
    crop = frame[max(0, y0):min(frame.shape[0], y1), max(0, x0):min(frame.shape[1], x1)]

    out_dir.mkdir(parents=True, exist_ok=True)
    rectified_path = out_dir / f"{name}_rectified.png"
    calibrated_path = out_dir / f"{name}_calibrated.png"
    rectified = resize_to_device(crop, rectified_path)
    calibrated, calibration = calibrate(rectified)
    save_rgb(calibrated, calibrated_path)

    ref = load_image(ref_path)
    ref_metrics = structural_metrics(ref)
    hw_metrics = structural_metrics(calibrated)
    checks = compare(hw_metrics, ref_metrics, tolerance=int(entry.get("tolerance", 3)),
                     check_filter=bool(entry.get("check_filter", True)),
                     check_frame=bool(entry.get("check_frame", True)))
    result = Result(name=name, frame=str(frame_path), ref=str(ref_path),
                    calibration=calibration, detection=detection, checks=checks)
    result.ok = all(c["ok"] for c in checks)
    result.artifacts = {"rectified": str(rectified_path), "calibrated": str(calibrated_path)}
    (out_dir / f"{name}_metrics.json").write_text(json.dumps(
        {"detection": detection, "ref": {k: v for k, v in ref_metrics.items() if "profile" not in k},
         "hardware": {k: v for k, v in hw_metrics.items() if "profile" not in k},
         "checks": checks, "ok": result.ok}, indent=2))
    return result


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--frame")
    parser.add_argument("--tile", help="x,y,w,h within the frame")
    parser.add_argument("--ref", default="render/fwref/instrument_params.png")
    parser.add_argument("--name", default="hardware")
    parser.add_argument("--spec", default=str(DEFAULT_SPEC))
    parser.add_argument("--out", default="/tmp/refcal")
    parser.add_argument("--all", action="store_true", help="run every spec entry")
    args = parser.parse_args()

    out_dir = Path(args.out)
    entries: list[dict] = []
    if args.all or (args.frame is None and Path(args.spec).exists()):
        spec = json.loads(Path(args.spec).read_text())
        entries = spec.get("entries", spec if isinstance(spec, list) else [])
    if args.frame:
        entry = {"name": args.name, "frame": args.frame, "ref": args.ref}
        if args.tile:
            entry["tile"] = [int(v) for v in args.tile.split(",")]
        entries = [entry]
    if not entries:
        parser.error("no entries: pass --frame or use --spec")

    failures = 0
    for entry in entries:
        try:
            result = run_one(entry, out_dir)
        except Exception as exc:  # noqa: BLE001 - CLI reports and continues
            print(f"[FAIL] {entry.get('name', '?')}: {exc}")
            failures += 1
            continue
        status = "PASS" if result.ok else "FAIL"
        print(f"[{status}] {result.name}  ({len(result.checks)} checks)")
        for check in result.checks:
            if not check["ok"]:
                print(f"    - {check['name']}: {check['detail']}")
        if not result.ok:
            failures += 1

    print(f"{len(entries) - failures}/{len(entries)} hardware references passed")
    return 0 if failures == 0 else 1


if __name__ == "__main__":
    sys.exit(main())

#!/usr/bin/env python3
"""Phase-4 gates for the source-true firmware reference.

1. Spec provenance: extract_spec.py --check (hash drift fails).
2. Determinism: two renders must be byte-identical.
3. Independent pixel contract: checks the rendered PNGs, not the renderer.
4. Negative control: a deliberately tampered spec must fail the pixel contract.

Usage: check_reference.py [--bin native/build/fwref_gen]
"""

import json
import os
import shutil
import struct
import subprocess
import sys
import tempfile
import zlib

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))
sys.path.insert(0, os.path.dirname(__file__))
import extract_spec  # noqa: E402
import refs  # noqa: E402
SPEC_DIR = refs.SPEC_DIR

FAILS = []

def fail(msg):
    FAILS.append(msg)
    print("FAIL:", msg)

def read_png(path):
    data = open(path, "rb").read()
    i = 8
    idat = b""
    w = h = None
    while i < len(data):
        ln = struct.unpack(">I", data[i:i + 4])[0]
        typ = data[i + 4:i + 8]
        chunk = data[i + 8:i + 8 + ln]
        if typ == b"IHDR":
            w, h = struct.unpack(">II", chunk[:8])
        elif typ == b"IDAT":
            idat += chunk
        i += 12 + ln
    raw = zlib.decompress(idat)
    bpp = 4
    stride = w * bpp + 1
    px = {}
    prev = bytearray(w * bpp)
    for y in range(h):
        off = y * stride
        filt = raw[off]
        row = bytearray(raw[off + 1:off + stride])
        for i in range(len(row)):
            a = row[i - bpp] if i >= bpp else 0
            b = prev[i]
            c = prev[i - bpp] if i >= bpp else 0
            if filt == 1:
                row[i] = (row[i] + a) & 0xFF
            elif filt == 2:
                row[i] = (row[i] + b) & 0xFF
            elif filt == 3:
                row[i] = (row[i] + ((a + b) // 2)) & 0xFF
            elif filt == 4:
                pp = a + b - c
                pa, pb, pc = abs(pp - a), abs(pp - b), abs(pp - c)
                pr = a if (pa <= pb and pa <= pc) else (b if pb <= pc else c)
                row[i] = (row[i] + pr) & 0xFF
            elif filt != 0:
                raise SystemExit(f"{path}: unknown PNG filter {filt}")
        prev = row
        for x in range(w):
            o = x * bpp
            px[(x, y)] = (row[o], row[o + 1], row[o + 2])
    return w, h, px

def eq(px, x, y, rgb, label):
    got = px.get((x, y))
    if got != tuple(rgb):
        fail(f"{label}: pixel ({x},{y}) is {got}, expected {tuple(rgb)}")

def is_white(px, x, y):
    return px.get((x, y)) == (255, 255, 255)

def first_white_y(px, x, y0, y1):
    for y in range(y0, y1):
        if px.get((x, y)) == (255, 255, 255):
            return y
    return -1

def white_count(px, x0, x1, y):
    return sum(1 for x in range(x0, x1) if px.get((x, y)) == (255, 255, 255))

def has_white_in(px, x0, x1, y0, y1):
    for y in range(y0, y1):
        for x in range(x0, x1):
            if px.get((x, y)) == (255, 255, 255):
                return True
    return False

def has_rgb_in(px, x0, x1, y0, y1, rgb):
    for y in range(y0, y1):
        for x in range(x0, x1):
            if px.get((x, y)) == rgb:
                return True
    return False

def no_green(px, label):
    for (x, y), (r, g, b) in px.items():
        if g > r + 16 and g > b + 16:
            fail(f"{label}: unexpected green pixel at ({x},{y}) = {(r, g, b)}")
            return

def check_manifest(fresh_dir=None):
    manifest_path = os.path.join(ROOT, "render", "fwref", "reference_manifest.json")
    if not os.path.exists(manifest_path):
        fail("reference_manifest.json missing (run fwref_gen and make_manifest.py)")
        return
    import hashlib
    def sha(path):
        h = hashlib.sha256()
        with open(path, "rb") as f:
            for chunk in iter(lambda: f.read(65536), b""):
                h.update(chunk)
        return h.hexdigest()
    m = json.load(open(manifest_path))
    if sha(os.path.join(os.path.dirname(SPEC_DIR), "scenario.json")) != m["scenario"]["sha256"]:
        fail("manifest: scenario.json hash mismatch")
    for name, digest in m["specs"].items():
        if sha(os.path.join(SPEC_DIR, name)) != digest:
            fail(f"manifest: spec hash mismatch for {name}")
    for name, digest in m["pngs"].items():
        path = os.path.join(ROOT, "render", "fwref", name)
        if not os.path.exists(path) or sha(path) != digest:
            fail(f"manifest: PNG hash mismatch for {name}")
    import hashlib
    for rel, digest in m.get("sources", {}).items():
        if rel.endswith("@theme"):
            continue
        path = os.path.join(ROOT, rel)
        if not os.path.exists(path):
            fail(f"manifest: missing source {rel}")
            continue
        lines = open(path, encoding="utf-8").read().split("\n")
        h = hashlib.sha256("\n".join(lines[:-1]).encode("utf-8")).hexdigest()
        if h != digest:
            fail(f"manifest: source hash mismatch for {rel}")
    spec = json.load(open(os.path.join(SPEC_DIR, "instrument_general.json")))
    theme = spec["provenance"]["theme"]
    lines = open(os.path.join(ROOT, theme["file"]), encoding="utf-8").read().split("\n")
    h = hashlib.sha256("\n".join(lines[theme["lines"][0] - 1:theme["lines"][1]]).encode()).hexdigest()
    if h != theme["sha256"]:
        fail("manifest: Themes.cpp theme range hash mismatch")
    if fresh_dir is not None:
        for name, digest in m["pngs"].items():
            fresh = os.path.join(fresh_dir, name)
            if not os.path.exists(fresh) or sha(fresh) != digest:
                fail(f"manifest: fresh render differs from checked-in {name}")
    print("manifest: checked-in artifacts match")

def check_instrument(px):
    eq(px, 1, 200, (0xCD, 0x18, 0x18), "instrument frame")
    eq(px, 50, 35, (0x0A, 0x0A, 0x0A), "instrument bar background")
    eq(px, 40, 41, (0x08, 0x08, 0x08), "instrument bar container")
    eq(px, 50, 300, (255, 255, 255), "instrument volume fill")
    fy = first_white_y(px, 50, 29, 424)
    if fy != 224:
        fail(f"instrument volume fill top is {fy}, expected 224")
    if white_count(px, 0, 100, 300) != 75:
        fail("instrument volume fill width is not 75px")
    for x in (150, 250, 350, 650):
        eq(px, x, 300, (0x08, 0x08, 0x08), "instrument empty left/right bar")
    eq(px, 550, 40, (255, 255, 255), "instrument cutoff fill (value 100)")
    eq(px, 402, 100, (0x0A, 0x0A, 0x0A), "instrument filter list bg")
    eq(px, 404, 60, (255, 255, 255), "instrument filter list selection frame")
    if px.get((404, 52)) == (255, 255, 255):
        fail("instrument filter list frame starts too high (row 52)")
    eq(px, 404, 88, (255, 255, 255), "instrument filter list frame bottom")
    eq(px, 760, 100, (0x0A, 0x0A, 0x0A), "instrument blank space cover")
    eq(px, 100, 450, (0x77, 0x77, 0x77), "instrument bottom separator")
    eq(px, 0, 450, (0, 0, 0), "instrument bottom bar left edge")
    eq(px, 799, 450, (0, 0, 0), "instrument bottom bar right edge")
    eq(px, 400, 10, (0x08, 0x08, 0x08), "instrument title bar bg")
    eq(px, 400, 28, (255, 255, 255), "instrument title bar underline")
    eq(px, 744, 463, (0x77, 0x77, 0x77), "instrument More dot 0")
    eq(px, 757, 463, (0x22, 0x22, 0x22), "instrument More dot 1")
    if not has_white_in(px, 5, 95, 455, 470):
        fail("instrument value label text missing")
    if not has_white_in(px, 6, 90, 5, 23):
        fail("instrument title text missing")
    no_green(px, "instrument")

def check_master(px):
    eq(px, 1, 200, (0xCD, 0x18, 0x18), "master frame")
    eq(px, 50, 35, (0x0A, 0x0A, 0x0A), "master volume bar background")
    fy = first_white_y(px, 50, 29, 424)
    if fy != 110:
        fail(f"master volume fill top is {fy}, expected 110 (81%)")
    if white_count(px, 0, 100, 200) != 75:
        fail("master volume fill width is not 75px")
    eq(px, 300, 200, (0x0A, 0x0A, 0x0A), "master blank space cover")
    eq(px, 505, 33, (255, 255, 255), "master saturation list selection frame")
    eq(px, 640, 300, (0x08, 0x08, 0x08), "master bass boost empty bar")
    eq(px, 760, 300, (0x08, 0x08, 0x08), "master space empty bar")
    eq(px, 100, 450, (0x77, 0x77, 0x77), "master bottom separator")
    if not has_rgb_in(px, 105, 195, 444, 459, (0x77, 0x77, 0x77)):
        fail("master Reverb single-line label text missing")
    if not has_white_in(px, 5, 100, 5, 23):
        fail("master title text missing")
    no_green(px, "master")

def any_pixel(px, x0, x1, y0, y1, pred):
    for y in range(y0, y1):
        for x in range(x0, x1):
            r, g, b = px[(x, y)]
            if pred(r, g, b):
                return True
    return False


def check_pattern(px):
    # chrome (titleBar.cpp / songTimer.cpp)
    eq(px, 400, 10, (0x08, 0x08, 0x08), "pattern title bar bg")
    eq(px, 0, 28, (255, 255, 255), "pattern title bar underline edge")
    if not has_white_in(px, 6, 90, 5, 23):
        fail("pattern title text missing")
    if not has_white_in(px, 270, 500, 5, 23):
        fail("pattern song timer text missing")
    if not has_white_in(px, 640, 795, 5, 23):
        fail("pattern title-bar right label missing")
    # tracker background / step divider (cTracker::backgroundDivider)
    eq(px, 450, 60, (0x08, 0x08, 0x08), "pattern tracker background")
    eq(px, 450, 110, (0x0E, 0x0E, 0x0E), "pattern step divider row 61")
    eq(px, 450, 140, (0x08, 0x08, 0x08), "pattern row 62 is not a divider")
    # row numbers in both gutters (colors[16] grey_57, antialiased glyphs)
    if not any_pixel(px, 1, 27, 212, 236, lambda r, g, b: r == g == b and 40 < r < 0x60):
        fail("pattern left row number missing")
    if not any_pixel(px, 770, 796, 212, 236, lambda r, g, b: r == g == b and 40 < r < 0x60):
        fail("pattern right row number missing")
    # track header boxes (colors[19] grey_57, black name text)
    eq(px, 450, 30, (0x57, 0x57, 0x57), "pattern track header box")
    if not any_pixel(px, 32, 100, 30, 52, lambda r, g, b: (r, g, b) == (0, 0, 0)):
        fail("pattern track name text missing")
    # green cursor highlight/frame (selectColor=1 -> colors[17])
    eq(px, 50, 220, (0x30, 0xCB, 0x3F), "pattern cursor param highlight")
    eq(px, 100, 210, (0x30, 0xCB, 0x3F), "pattern cursor frame top")
    # white note glyph in the selected cell, dark-green note glyphs elsewhere
    if not any_pixel(px, 33, 50, 214, 236, lambda r, g, b: r > 200 and g > 200 and b > 200):
        fail("pattern selected note text is not white")
    if not any_pixel(px, 33, 50, 54, 80,
                     lambda r, g, b: r <= 4 and 40 <= g <= 60 and 35 <= b <= 55):
        fail("pattern empty note colour missing")
    # track-end line at the row-1 boundary (colors[20] instrYellow)
    eq(px, 700, 210, (0xFE, 0xF7, 0x49), "pattern track end line")
    # bottom action bar (cBgLabel + cFrame + cLabel)
    eq(px, 500, 450, (0x77, 0x77, 0x77), "pattern bottom separator")
    eq(px, 100, 423, (0xCD, 0x18, 0x18), "pattern action frame top")
    eq(px, 1, 450, (0xCD, 0x18, 0x18), "pattern action frame left")
    eq(px, 50, 474, (0x30, 0xCB, 0x3F), "pattern action underline")
    if not has_rgb_in(px, 5, 95, 430, 455, (0x77, 0x77, 0x77)):
        fail("pattern action label text missing")
    if not any_pixel(px, 5, 95, 455, 472, lambda r, g, b: r == g == b and r > 200):
        fail("pattern action value text missing")


def check_master2(px):
    eq(px, 400, 10, (0x08, 0x08, 0x08), "master2 title bar bg")
    eq(px, 5, 35, (0x08, 0x08, 0x08), "master2 track name label bg")
    eq(px, 20, 200, (255, 255, 255), "master2 track volume fill")
    eq(px, 80, 200, (0x08, 0x08, 0x08), "master2 level bar container")
    eq(px, 410, 440, (0x0A, 0x0A, 0x0A), "master2 bottom bar bg")
    if not has_white_in(px, 30, 75, 36, 56):
        fail("master2 track name text missing")
    if not has_rgb_in(px, 5, 95, 430, 455, (0x77, 0x77, 0x77)):
        fail("master2 label value text missing")
    if not has_white_in(px, 6, 90, 5, 23):
        fail("master2 title text missing")
    no_green(px, "master2")


def check_master3(px):
    eq(px, 400, 10, (0x08, 0x08, 0x08), "master3 title bar bg")
    eq(px, 5, 35, (0x08, 0x08, 0x08), "master3 aux name label bg")
    eq(px, 20, 200, (255, 255, 255), "master3 delay volume fill")
    eq(px, 60, 200, (0x0A, 0x0A, 0x0A), "master3 delay level container")
    eq(px, 410, 440, (0x0A, 0x0A, 0x0A), "master3 bottom bar bg")
    if not has_white_in(px, 30, 70, 36, 56):
        fail("master3 aux name text missing")
    if not has_rgb_in(px, 5, 95, 430, 455, (0x77, 0x77, 0x77)):
        fail("master3 label value text missing")
    if not has_white_in(px, 6, 90, 5, 23):
        fail("master3 title text missing")
    no_green(px, "master3")


def check_master_eq(px):
    eq(px, 400, 10, (0x08, 0x08, 0x08), "master_eq title bar bg")
    eq(px, 1, 200, (0xCD, 0x18, 0x18), "master_eq selection frame")
    eq(px, 50, 35, (0x0A, 0x0A, 0x0A), "master_eq band bar background")
    eq(px, 50, 300, (255, 255, 255), "master_eq band 0 fill")
    eq(px, 350, 300, (255, 255, 255), "master_eq band 3 fill")
    eq(px, 520, 300, (0x0A, 0x0A, 0x0A), "master_eq blank space cover")
    eq(px, 410, 440, (0x0A, 0x0A, 0x0A), "master_eq bottom bar bg")
    if not has_rgb_in(px, 5, 95, 430, 455, (0x77, 0x77, 0x77)):
        fail("master_eq band label text missing")
    if not has_white_in(px, 6, 90, 5, 23):
        fail("master_eq title text missing")
    no_green(px, "master_eq")


def check_config(px):
    eq(px, 400, 10, (0x08, 0x08, 0x08), "config title bar bg")
    eq(px, 50, 200, (0x0A, 0x0A, 0x0A), "config basemenu list bg")
    eq(px, 400, 200, (0x0A, 0x0A, 0x0A), "config submenu list bg")
    eq(px, 700, 200, (0x0A, 0x0A, 0x0A), "config blank space cover")
    eq(px, 1, 200, (0xCD, 0x18, 0x18), "config frame")
    eq(px, 400, 440, (0x0A, 0x0A, 0x0A), "config bottom bar bg")
    if not has_rgb_in(px, 720, 790, 435, 470, (0x77, 0x77, 0x77)):
        fail("config Enter label text missing")
    if not has_white_in(px, 6, 90, 5, 23):
        fail("config title text missing")
    if not any_pixel(px, 10, 190, 36, 58, lambda r, g, b: r == g == b and r > 150):
        fail("config basemenu item text missing")
    if not any_pixel(px, 215, 590, 36, 58, lambda r, g, b: r == g == b and r > 150):
        fail("config submenu row text missing")
    no_green(px, "config")


def check_perform(px):
    eq(px, 400, 10, (0x08, 0x08, 0x08), "perform title bar bg")
    eq(px, 20, 50, (0x0A, 0x0A, 0x0A), "perform top value cell bg")
    eq(px, 13, 300, (0x0A, 0x0A, 0x0A), "perform value cell bg")
    eq(px, 410, 440, (0x0A, 0x0A, 0x0A), "perform bottom bar bg")
    if not any_pixel(px, 20, 60, 60, 244, lambda r, g, b: r == g == b and r > 120):
        fail("perform vertical FX name missing")
    if not has_rgb_in(px, 5, 95, 430, 455, (0x77, 0x77, 0x77)):
        fail("perform track name label missing")
    if not any_pixel(px, 5, 95, 455, 472, lambda r, g, b: r == g == b and r > 150):
        fail("perform track pattern label missing")
    if not any_pixel(px, 20, 70, 285, 312, lambda r, g, b: g > r + 40 and g > b + 40):
        fail("perform selected row green text missing")
    if not has_white_in(px, 6, 90, 5, 23):
        fail("perform title text missing")


def check_song(px):
    # chrome (titleBar.cpp): "Song" title, no right label
    eq(px, 400, 10, (0x08, 0x08, 0x08), "song title bar bg")
    if not has_white_in(px, 6, 90, 5, 23):
        fail("song title text missing")
    # cSongPlayer background (colors[3] grey_0A)
    eq(px, 1, 200, (0x0A, 0x0A, 0x0A), "song player background")
    # block matrix, pattern 1 (rows 0..4): track0 active, track2 muted,
    # midi track1 muted, track4 empty
    eq(px, 7, 100, (255, 255, 255), "song pattern1 track0 block")
    eq(px, 67, 100, (0x44, 0x43, 0x44), "song pattern1 muted track2 block")
    eq(px, 277, 100, (0x44, 0x43, 0x44), "song pattern1 muted midi track1 block")
    eq(px, 127, 100, (0x12, 0x12, 0x12), "song pattern1 empty track4 block")
    # pattern 2 (rows 5..8): track0 active, track2 empty
    eq(px, 7, 237, (255, 255, 255), "song pattern2 track0 block")
    eq(px, 67, 237, (0x12, 0x12, 0x12), "song pattern2 empty track2 block")
    # pattern 3 (rows 9..11): track3 active, track0 empty
    eq(px, 97, 345, (255, 255, 255), "song pattern3 track3 block")
    eq(px, 7, 345, (0x12, 0x12, 0x12), "song pattern3 empty track0 block")
    # progress line (colors[9] one_true_green) at pattern position 0
    eq(px, 7, 104, (0x30, 0xCB, 0x3F), "song progress line top")
    eq(px, 7, 105, (0x30, 0xCB, 0x3F), "song progress line bottom")
    # playlist selection frame (colors[9], list active) and API_CMD_NUMBER slot
    eq(px, 491, 91, (0x30, 0xCB, 0x3F), "song list frame top")
    eq(px, 491, 100, (0x30, 0xCB, 0x3F), "song list frame left")
    if not any_pixel(px, 515, 535, 95, 115, lambda r, g, b: r == g == b and r > 150):
        fail("song playlist slot number missing")
    if not any_pixel(px, 565, 645, 50, 70, lambda r, g, b: r == g == b and r > 150):
        fail("song Pattern header text missing")
    # vertical track strip: white names, muted track2 in colors[4] grey_33
    if not has_white_in(px, 9, 30, 30, 72):
        fail("song track name text missing")
    if not has_rgb_in(px, 69, 90, 30, 72, (0x33, 0x33, 0x33)):
        fail("song muted track name colour missing")
    # tempo bar (value = 130*100/800 = 16): container grey_08, white fill
    eq(px, 701, 35, (0x0A, 0x0A, 0x0A), "song tempo bar background")
    eq(px, 720, 100, (0x08, 0x08, 0x08), "song tempo bar container")
    eq(px, 720, 400, (255, 255, 255), "song tempo bar fill")
    # bottom action bar (cBgLabel value 191 skips the x=600 separator)
    eq(px, 410, 430, (0x0A, 0x0A, 0x0A), "song bottom bar bg")
    eq(px, 100, 450, (0x77, 0x77, 0x77), "song bottom separator")
    if not has_rgb_in(px, 5, 95, 430, 455, (0x77, 0x77, 0x77)):
        fail("song action label text missing")
    if not any_pixel(px, 700, 798, 440, 470, lambda r, g, b: r == g == b and r > 150):
        fail("song tempo value text missing")


def check_loader(px):
    # chrome (titleBar.cpp): "Sample Loader", right label hidden.
    eq(px, 400, 10, (0x08, 0x08, 0x08), "loader title bar bg")
    if not has_white_in(px, 6, 120, 5, 23):
        fail("loader title text missing")
    # cFrame place 0 (file browser) and both cList backgrounds (grey_0A).
    eq(px, 1, 200, (0xCD, 0x18, 0x18), "loader browser frame")
    eq(px, 100, 300, (0x0A, 0x0A, 0x0A), "loader file list bg")
    eq(px, 400, 300, (0x0A, 0x0A, 0x0A), "loader slot list bg")
    if not any_pixel(px, 5, 290, 62, 90, lambda r, g, b: r == g == b and r > 120):
        fail("loader file list text missing")
    if not any_pixel(px, 310, 590, 62, 90, lambda r, g, b: r == g == b and r > 120):
        fail("loader slot list text missing")
    if not any_pixel(px, 150, 445, 34, 56, lambda r, g, b: r == g == b and r > 150):
        fail("loader SD Card header text missing")
    if not any_pixel(px, 450, 745, 34, 56, lambda r, g, b: r == g == b and r > 150):
        fail("loader Instruments header text missing")
    # preview volume bar (86% -> fill top y=91) and memory compare bar (12/14%).
    eq(px, 620, 90, (0x08, 0x08, 0x08), "loader preview volume container")
    eq(px, 620, 100, (255, 255, 255), "loader preview volume fill")
    eq(px, 720, 350, (0x08, 0x08, 0x08), "loader memory container")
    eq(px, 720, 370, (255, 255, 255), "loader memory fill")
    eq(px, 750, 362, (0xCD, 0x18, 0x18), "loader memory compare delta")
    # bottom action bar + separators.
    eq(px, 400, 450, (0x77, 0x77, 0x77), "loader bottom separator")
    eq(px, 0, 450, (0, 0, 0), "loader bottom bar left edge")
    eq(px, 20, 450, (0x0A, 0x0A, 0x0A), "loader bottom bar bg")
    if not has_rgb_in(px, 5, 95, 430, 455, (0x77, 0x77, 0x77)):
        fail("loader action label text missing")
    if not any_pixel(px, 605, 695, 455, 472, lambda r, g, b: r == g == b and r > 150):
        fail("loader preview volume value text missing")
    if not any_pixel(px, 707, 795, 455, 472, lambda r, g, b: r == g == b and r > 150):
        fail("loader memory value text missing")
    no_green(px, "loader")


def check_sample(px):
    # chrome (titleBar.cpp): "Sample Playback" + instrument right label.
    eq(px, 400, 10, (0x08, 0x08, 0x08), "sample title bar bg")
    if not has_white_in(px, 6, 140, 5, 23):
        fail("sample title text missing")
    if not has_white_in(px, 640, 795, 5, 23):
        fail("sample title-bar right label missing")
    # cSpectrum waveform: white peaks around the centre line y=225.
    eq(px, 100, 225, (255, 255, 255), "sample waveform peak")
    eq(px, 100, 300, (0, 0, 0), "sample waveform area outside loop")
    # cPoints loop region (grey_0A fill) and start/end/loop marker lines.
    eq(px, 300, 140, (0x0A, 0x0A, 0x0A), "sample loop region fill")
    eq(px, 40, 116, (255, 255, 255), "sample start point line")
    eq(px, 580, 116, (255, 255, 255), "sample end point line")
    eq(px, 150, 340, (255, 255, 255), "sample loop point marker")
    # cProgressCursor (red) at x=180, only across the waveform height.
    eq(px, 180, 225, (0xCD, 0x18, 0x18), "sample progress cursor")
    eq(px, 180, 115, (0, 0, 0), "sample progress cursor top edge")
    # play-mode cList background + selected Forward Loop row frame.
    eq(px, 620, 100, (0x0A, 0x0A, 0x0A), "sample play mode list bg")
    if not has_white_in(px, 601, 798, 52, 80):
        fail("sample play mode selection frame missing")
    # bottom action labels: Preview/Start/Loop Start/Loop End/End/Zoom/Play Mode.
    # selectedPlace defaults to 6 (samplePlayback.h:159): the frame is on the list.
    eq(px, 601, 100, (0xCD, 0x18, 0x18), "sample action frame")
    eq(px, 1, 424, (0, 0, 0), "sample unselected action cell")
    for x0, x1 in ((5, 95), (105, 195), (205, 295), (305, 395), (405, 495),
                   (505, 595), (605, 795)):
        if not has_rgb_in(px, x0, x1, 430, 455, (0x77, 0x77, 0x77)):
            fail(f"sample action label text missing in {x0}-{x1}")
    if not any_pixel(px, 5, 95, 455, 472, lambda r, g, b: r == g == b and r > 150):
        fail("sample Preview value text missing")
    if not any_pixel(px, 105, 195, 455, 472, lambda r, g, b: r == g == b and r > 150):
        fail("sample Start time text missing")
    no_green(px, "sample")


def run(cmd, env=None):
    return subprocess.run(cmd, cwd=ROOT, env=env, check=True)

def main():
    global FAILS
    bin_path = os.path.join(ROOT, "native", "build", "fwref_gen")
    args = sys.argv[1:]
    if "--bin" in args:
        bin_path = args[args.index("--bin") + 1]

    # Direct mode: check one already-rendered PNG (Phase 8 invocation).
    screen = None
    png = None
    if "--screen" in args:
        screen = args[args.index("--screen") + 1]
    for a in args:
        if a.endswith(".png"):
            png = a
    if screen is not None and png is not None:
        _, _, px = read_png(png if os.path.isabs(png) else os.path.join(ROOT, png))
        if screen == "instrument":
            check_instrument(px)
        elif screen == "master":
            check_master(px)
        elif screen == "pattern":
            check_pattern(px)
        elif screen == "master2":
            check_master2(px)
        elif screen == "master3":
            check_master3(px)
        elif screen == "config":
            check_config(px)
        elif screen == "perform":
            check_perform(px)
        elif screen == "song":
            check_song(px)
        elif screen == "loader":
            check_loader(px)
        elif screen == "sample":
            check_sample(px)
        else:
            fail(f"unknown screen {screen}")
        if FAILS:
            print(f"\n{len(FAILS)} check(s) failed")
            return 1
        print(f"{screen}: pixel contract passed")
        return 0

    # 1. provenance
    if subprocess.run([sys.executable, os.path.join(ROOT, "native", "tools", "fwref",
                                                     "extract_spec.py"), "--check"],
                      cwd=ROOT).returncode != 0:
        fail("spec provenance check failed")
        return 1

    # 2. determinism + 3. pixel contract
    with tempfile.TemporaryDirectory() as td:
        a = os.path.join(td, "a")
        b = os.path.join(td, "b")
        sample_states = [None] + list(refs.SAMPLE_STATE_PNGS)
        for state in sample_states:
            extra = ["--sample-state", state] if state else []
            run([bin_path, "--out", a, "--strict"] + extra)
            run([bin_path, "--out", b, "--strict"] + extra)

        for name in refs.all_pngs():
            pa = os.path.join(a, name)
            pb = os.path.join(b, name)
            if not os.path.exists(pa) or not os.path.exists(pb):
                fail(f"{name}: render missing")
                continue
            if open(pa, "rb").read() != open(pb, "rb").read():
                fail(f"{name}: renders are not byte-identical")
            else:
                print(f"deterministic: {name}")

        _, _, ipx = read_png(os.path.join(a, "instrument_params.png"))
        _, _, mpx = read_png(os.path.join(a, "master_page1.png"))
        _, _, m2px = read_png(os.path.join(a, "master_page2.png"))
        _, _, m3px = read_png(os.path.join(a, "master_page3.png"))
        _, _, meqpx = read_png(os.path.join(a, "master_eq.png"))
        _, _, cfpx = read_png(os.path.join(a, "config.png"))
        _, _, pfpx = read_png(os.path.join(a, "perform.png"))
        _, _, ppx = read_png(os.path.join(a, "pattern.png"))
        _, _, spx = read_png(os.path.join(a, "song.png"))
        _, _, lpx = read_png(os.path.join(a, "loader.png"))
        _, _, smpx = read_png(os.path.join(a, "sample.png"))
        check_instrument(ipx)
        check_master(mpx)
        check_master2(m2px)
        check_master3(m3px)
        check_master_eq(meqpx)
        check_config(cfpx)
        check_perform(pfpx)
        check_pattern(ppx)
        check_song(spx)
        check_loader(lpx)
        check_sample(smpx)
        check_manifest(a)

        # 4. negative control: shift the frame +3px; the (1,200) contract must fail
        neg = os.path.join(td, "neg")
        shutil.copytree(SPEC_DIR, neg)
        with open(os.path.join(neg, "instrument_general.json")) as f:
            spec = json.load(f)
        for e in spec["entries"]:
            if e["control"] == "cFrame":
                e["x"] = "0 + 3"
                for place in e.get("places", []):
                    place["x"] = "(" + place["x"] + ") + 3"
        with open(os.path.join(neg, "instrument_general.json"), "w") as f:
            json.dump(spec, f)

        env = dict(os.environ)
        env["TRAKR_FWREF_SPECS"] = neg
        nout = os.path.join(td, "neg_out")
        run([bin_path, "--out", nout, "--scenario", os.path.join(os.path.dirname(SPEC_DIR), "scenario.json")],
            env=env)
        _, _, npx = read_png(os.path.join(nout, "instrument_params.png"))
        saved = FAILS
        FAILS = []
        check_instrument(npx)
        detected = len(FAILS) > 0
        FAILS = saved
        if not detected:
            fail("negative control: tampered spec was not detected")
        else:
            print("negative control: tampered spec detected")

    if FAILS:
        print(f"\n{len(FAILS)} check(s) failed")
        return 1
    print("\nall fwref gates passed")
    return 0

if __name__ == "__main__":
    sys.exit(main())

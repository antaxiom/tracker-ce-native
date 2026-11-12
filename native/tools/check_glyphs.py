#!/usr/bin/env python3
"""Guard against tofu: every non-ASCII codepoint used in UI source must exist
in the bundled font(s). Missing codepoints render as the U+FFFD box because
ImGui picks RobotoMono's replacement glyph as its fallback.

Usage: python3 native/tools/check_glyphs.py [src_dir ...]
Exit code 1 if any used codepoint is missing from all bundled fonts.
"""
import os
import struct
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
FONT_DIR = os.path.join(ROOT, "native", "assets", "fonts")
FONTS = ["RobotoMono-Regular.ttf", "RobotoMono-Bold.ttf", "RobotoMono-Light.ttf"]
SRC_DIRS = sys.argv[1:] or [
    os.path.join(ROOT, "native", "app"),
    os.path.join(ROOT, "native", "tools", "display_emulator"),
]


def read_cmap(path):
    data = open(path, "rb").read()
    num_tables = struct.unpack(">H", data[4:6])[0]
    cmap_off = None
    for i in range(num_tables):
        rec = 12 + i * 16
        tag = data[rec:rec + 4]
        if tag == b"cmap":
            cmap_off = struct.unpack(">I", data[rec + 8:rec + 12])[0]
            break
    if cmap_off is None:
        return set()
    n = struct.unpack(">H", data[cmap_off + 2:cmap_off + 4])[0]
    codepoints = set()
    for i in range(n):
        rec = cmap_off + 4 + i * 8
        sub_off = cmap_off + struct.unpack(">I", data[rec + 4:rec + 8])[0]
        fmt = struct.unpack(">H", data[sub_off:sub_off + 2])[0]
        if fmt == 4:
            seg_x2 = struct.unpack(">H", data[sub_off + 6:sub_off + 8])[0]
            seg = seg_x2 // 2
            ends = sub_off + 14
            starts = ends + seg_x2 + 2
            deltas = starts + seg_x2
            ranges = deltas + seg_x2
            for s in range(seg):
                end = struct.unpack(">H", data[ends + s * 2:ends + s * 2 + 2])[0]
                start = struct.unpack(">H", data[starts + s * 2:starts + s * 2 + 2])[0]
                if start == 0xFFFF:
                    continue
                delta = struct.unpack(">h", data[deltas + s * 2:deltas + s * 2 + 2])[0]
                ro = struct.unpack(">H", data[ranges + s * 2:ranges + s * 2 + 2])[0]
                for cp in range(start, end + 1):
                    if ro == 0:
                        codepoints.add((cp + delta) & 0xFFFF)
                    else:
                        gi = ranges + s * 2 + ro + (cp - start) * 2
                        if gi + 2 <= len(data):
                            g = struct.unpack(">H", data[gi:gi + 2])[0]
                            if g:
                                codepoints.add(cp)
        elif fmt == 12:
            ngroups = struct.unpack(">I", data[sub_off + 12:sub_off + 16])[0]
            for g in range(ngroups):
                base = sub_off + 16 + g * 12
                start, end, _ = struct.unpack(">III", data[base:base + 12])
                codepoints.update(range(start, end + 1))
    return codepoints


def used_codepoints(src_dir):
    used = {}
    for dirpath, _, files in os.walk(src_dir):
        for name in files:
            if not name.endswith((".cpp", ".h")):
                continue
            path = os.path.join(dirpath, name)
            for lineno, line in enumerate(open(path, encoding="utf-8", errors="replace"), 1):
                line = line.split("//", 1)[0]
                for ch in line:
                    if ord(ch) > 0x7F:
                        used.setdefault(ord(ch), []).append(f"{os.path.relpath(path, ROOT)}:{lineno}")
    return used


def main():
    cover = set()
    for f in FONTS:
        p = os.path.join(FONT_DIR, f)
        if os.path.exists(p):
            cover |= read_cmap(p)
    missing = []
    total = 0
    for d in SRC_DIRS:
        for cp, places in used_codepoints(d).items():
            total += 1
            if cp not in cover:
                missing.append((cp, places))
    if missing:
        for cp, places in missing:
            print(f"MISSING U+{cp:04X} {chr(cp)!r} at {', '.join(places[:4])}")
        print(f"{len(missing)} codepoint(s) missing from bundled fonts")
        return 1
    print(f"OK: {total} non-ASCII codepoint(s) used, all covered by bundled fonts")
    return 0


if __name__ == "__main__":
    sys.exit(main())

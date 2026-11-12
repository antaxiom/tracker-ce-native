#!/usr/bin/env python3
"""
Deterministic Spatial Telemetry & Hotspot Analyzer for 800x480 FT812 Display Parity.
Computes semantic band error budgets, Top-N mismatched bounding boxes, dominant
IM_COL32 color discrepancies, and local (dx, dy) translation alignment hints.
"""

from typing import List, Tuple, Dict

W, H = 800, 480
TOTAL_PX = W * H
MATCH_THRESH = 24.0


def _px_err(b_emu: bytes, b_app: bytes, idx: int) -> float:
    return (
        abs(b_emu[idx] - b_app[idx])
        + abs(b_emu[idx + 1] - b_app[idx + 1])
        + abs(b_emu[idx + 2] - b_app[idx + 2])
    ) / 3.0


def _dominant_mismatch_colors(
    b_emu: bytes, b_app: bytes, x0: int, y0: int, x1: int, y1: int
) -> Tuple[Tuple[int, int, int], Tuple[int, int, int]]:
    """Finds the most frequent Emu RGB and App RGB specifically among mismatched pixels."""
    emu_buckets: Dict[Tuple[int, int, int], List[int]] = {}
    app_buckets: Dict[Tuple[int, int, int], List[int]] = {}
    for y in range(y0, y1):
        row_off = y * W * 3
        for x in range(x0, x1):
            i = row_off + x * 3
            if _px_err(b_emu, b_app, i) <= MATCH_THRESH:
                continue
            er, eg, eb = b_emu[i], b_emu[i + 1], b_emu[i + 2]
            ar, ag, ab = b_app[i], b_app[i + 1], b_app[i + 2]
            ek = (er >> 3, eg >> 3, eb >> 3)
            ak = (ar >> 3, ag >> 3, ab >> 3)
            e_ent = emu_buckets.setdefault(ek, [0, 0, 0, 0])
            e_ent[0] += 1; e_ent[1] += er; e_ent[2] += eg; e_ent[3] += eb
            a_ent = app_buckets.setdefault(ak, [0, 0, 0, 0])
            a_ent[0] += 1; a_ent[1] += ar; a_ent[2] += ag; a_ent[3] += ab

    def _pick(buckets: Dict[Tuple[int, int, int], List[int]]) -> Tuple[int, int, int]:
        if not buckets:
            return (0, 0, 0)
        best = max(buckets.values(), key=lambda v: v[0])
        c = max(1, best[0])
        return (best[1] // c, best[2] // c, best[3] // c)

    return _pick(emu_buckets), _pick(app_buckets)


def _best_shift(
    b_emu: bytes, b_app: bytes, x0: int, y0: int, x1: int, y1: int, base_mismatches: int
) -> Tuple[int, int, int]:
    """Searches dx, dy in [-4..+4] to see if translating App pixels reduces mismatches."""
    best_dx, best_dy, best_mm = 0, 0, base_mismatches
    for dy in range(-4, 5):
        for dx in range(-4, 5):
            if dx == 0 and dy == 0:
                continue
            mm = 0
            for y in range(y0, y1):
                ay = y - dy
                if ay < 0 or ay >= H:
                    mm += (x1 - x0)
                    continue
                e_row = y * W * 3
                a_row = ay * W * 3
                for x in range(x0, x1):
                    ax = x - dx
                    if ax < 0 or ax >= W:
                        mm += 1
                        continue
                    ei = e_row + x * 3
                    ai = a_row + ax * 3
                    err = (
                        abs(b_emu[ei] - b_app[ai])
                        + abs(b_emu[ei + 1] - b_app[ai + 1])
                        + abs(b_emu[ei + 2] - b_app[ai + 2])
                    ) / 3.0
                    if err > MATCH_THRESH:
                        mm += 1
            if mm < best_mm:
                best_mm = mm
                best_dx = dx
                best_dy = dy
    return best_dx, best_dy, base_mismatches - best_mm


def analyze_screen_hotspots(screen: str, b_emu: bytes, b_app: bytes, top_k: int = 5) -> str:
    """Returns a concise, machine-actionable telemetry report for a screen."""
    lines: List[str] = []

    # 1. Semantic vertical band summary
    bands = [
        ("HeaderBar   (y=0..30)  ", 0, 30),
        ("UpperBody   (y=30..230)", 30, 230),
        ("LowerBody   (y=230..430)", 230, 430),
        ("SoftButtons (y=430..480)", 430, 480),
    ]
    band_parts = []
    for name, y0, y1 in bands:
        mm = 0
        for y in range(y0, y1):
            ro = y * W * 3
            for x in range(W):
                if _px_err(b_emu, b_app, ro + x * 3) > MATCH_THRESH:
                    mm += 1
        loss_pct = (mm / TOTAL_PX) * 100.0
        band_parts.append(f"{name}: -{loss_pct:.2f}% ({mm}px)")
    lines.append(f"  [BANDS] " + " | ".join(band_parts))

    # 2. Tile grid (10x12 -> 80x40px tiles)
    tw, th = 80, 40
    tiles = []
    for gy in range(H // th):
        for gx in range(W // tw):
            x0, y0 = gx * tw, gy * th
            x1, y1 = x0 + tw, y0 + th
            mm = 0
            for y in range(y0, y1):
                ro = y * W * 3
                for x in range(x0, x1):
                    if _px_err(b_emu, b_app, ro + x * 3) > MATCH_THRESH:
                        mm += 1
            if mm > 25:
                tiles.append((mm, x0, y0, x1, y1))

    tiles.sort(reverse=True, key=lambda t: t[0])

    # 3. Format Top-K hotspots with IM_COL32 and shift telemetry
    for rank, (mm, x0, y0, x1, y1) in enumerate(tiles[:top_k], 1):
        loss_pct = (mm / TOTAL_PX) * 100.0
        (er, eg, eb), (ar, ag, ab) = _dominant_mismatch_colors(b_emu, b_app, x0, y0, x1, y1)
        dx, dy, recovered = _best_shift(b_emu, b_app, x0, y0, x1, y1, mm)
        rec_pct = (recovered / TOTAL_PX) * 100.0
        shift_str = ""
        if recovered >= max(20, int(mm * 0.12)):
            shift_str = f" | SHIFT HINT: move App (dx={dx:+d}, dy={dy:+d}) -> recovers +{rec_pct:.2f}%"
        lines.append(
            f"  [HOTSPOT #{rank}] rect=({x0},{y0}..{x1},{y1}) loss=-{loss_pct:.2f}% ({mm}px) "
            f"| Emu=IM_COL32({er},{eg},{eb},255) vs App=IM_COL32({ar},{ag},{ab},255){shift_str}"
        )

    return "\n".join(lines)

#!/usr/bin/env python3
"""
Black-Box OS-Level Input Emulator & 1:1 Firmware Display Comparator for Trakr Native.
Controls mouse and keyboard inputs at the OS/Window event boundary via macOS CoreGraphics
(CGEventPost) and performs 1:1 image comparison against the FT812 Display Emulator
using pure Python stdlib (zlib, struct, subprocess/sips) with zero external dependencies.
"""

import sys
import os
import time
import struct
import zlib
import subprocess
import tempfile
import ctypes
import numpy as np
from typing import Tuple
from hotspot_analyzer import analyze_screen_hotspots

class OSInputDriver:
    def __init__(self):
        self.cg = None
        try:
            self.cg = ctypes.cdll.LoadLibrary('/System/Library/Frameworks/CoreGraphics.framework/CoreGraphics')
            self.cg.CGEventCreateMouseEvent.restype = ctypes.c_void_p
            self.cg.CGEventCreateKeyboardEvent.restype = ctypes.c_void_p
            self.cg.CGEventPost.argtypes = [ctypes.c_uint32, ctypes.c_void_p]
            self.cg.CFRelease.argtypes = [ctypes.c_void_p]
        except Exception:
            self.cg = None

    def post_mouse_move(self, x: float, y: float) -> bool:
        if not self.cg:
            return False
        event = self.cg.CGEventCreateMouseEvent(None, 5, ctypes.c_double(x), ctypes.c_double(y), 0)
        if event:
            self.cg.CGEventPost(0, event)
            self.cg.CFRelease(event)
            return True
        return False

    def post_mouse_click(self, x: float, y: float, button: int = 0) -> bool:
        if not self.cg:
            return False
        down_type = 1 if button == 0 else 3
        up_type = 2 if button == 0 else 4
        down_ev = self.cg.CGEventCreateMouseEvent(None, down_type, ctypes.c_double(x), ctypes.c_double(y), button)
        up_ev = self.cg.CGEventCreateMouseEvent(None, up_type, ctypes.c_double(x), ctypes.c_double(y), button)
        if down_ev and up_ev:
            self.cg.CGEventPost(0, down_ev)
            time.sleep(0.02)
            self.cg.CGEventPost(0, up_ev)
            self.cg.CFRelease(down_ev)
            self.cg.CFRelease(up_ev)
            return True
        return False

    def post_key(self, keycode: int, down: bool = True) -> bool:
        if not self.cg:
            return False
        ev = self.cg.CGEventCreateKeyboardEvent(None, ctypes.c_uint16(keycode), down)
        if ev:
            self.cg.CGEventPost(0, ev)
            self.cg.CFRelease(ev)
            return True
        return False

    def tap_key(self, keycode: int, delay: float = 0.03) -> bool:
        ok1 = self.post_key(keycode, True)
        time.sleep(delay)
        ok2 = self.post_key(keycode, False)
        return ok1 and ok2

def read_image_rgb(path: str) -> Tuple[int, int, bytes]:
    """Reads any PNG/BMP via sips conversion to uncompressed 24-bit/32-bit BMP."""
    fd, tmp_bmp = tempfile.mkstemp(prefix="os_input_", suffix=".bmp")
    os.close(fd)
    try:
        subprocess.run(["sips", "-s", "format", "bmp", path, "--out", tmp_bmp],
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=True)
        with open(tmp_bmp, "rb") as f:
            bmp = f.read()
    finally:
        if os.path.exists(tmp_bmp):
            os.remove(tmp_bmp)

    pixel_offset = struct.unpack_from("<I", bmp, 10)[0]
    w, h = struct.unpack_from("<ii", bmp, 18)
    bpp = struct.unpack_from("<H", bmp, 28)[0]
    top_down = (h < 0)
    h = abs(h)
    bytes_per_px = bpp // 8
    row_stride = ((w * bytes_per_px + 3) // 4) * 4

    out = bytearray(w * h * 3)
    for r in range(h):
        src_r = r if top_down else (h - 1 - r)
        row_start = pixel_offset + src_r * row_stride
        dst_start = r * w * 3
        for c in range(w):
            px_off = row_start + c * bytes_per_px
            b = bmp[px_off]
            g = bmp[px_off + 1]
            r_col = bmp[px_off + 2]
            idx = dst_start + c * 3
            out[idx] = r_col
            out[idx + 1] = g
            out[idx + 2] = b
    return w, h, bytes(out)

def write_png_rgb(path: str, w: int, h: int, rgb_bytes: bytes):
    """Writes 24-bit RGB bytes to a standard PNG file using pure Python zlib."""
    def make_chunk(chunk_type: bytes, data: bytes) -> bytes:
        crc = zlib.crc32(chunk_type + data) & 0xFFFFFFFF
        return struct.pack(">I", len(data)) + chunk_type + data + struct.pack(">I", crc)

    raw_rows = bytearray()
    stride = w * 3
    for r in range(h):
        raw_rows.append(0) # filter type 0 (None)
        raw_rows.extend(rgb_bytes[r * stride : (r + 1) * stride])

    ihdr = struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0)
    idat = zlib.compress(bytes(raw_rows), 6)
    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n")
        f.write(make_chunk(b"IHDR", ihdr))
        f.write(make_chunk(b"IDAT", idat))
        f.write(make_chunk(b"IEND", b""))

def crop_and_resize_rgb(w: int, h: int, rgb: bytes, x0: float, y0: float, cw: float, ch: float, tw: int, th: int) -> bytes:
    out = bytearray(tw * th * 3)
    for r in range(th):
        sy = int(min(h - 1, max(0, y0 + (r + 0.5) * ch / th)))
        for c in range(tw):
            sx = int(min(w - 1, max(0, x0 + (c + 0.5) * cw / tw)))
            s_idx = (sy * w + sx) * 3
            d_idx = (r * tw + c) * 3
            out[d_idx : d_idx + 3] = rgb[s_idx : s_idx + 3]
    return bytes(out)

def detect_lcd_rect(w: int, h: int, rgb: bytes):
    """Detects the physical 800x480 LCD rect from the rendered bezel rails.

    The left rail (#282A2E) is the column with the largest bezel-colour coverage;
    its top row sits 2 device rows below the true screen top (the window's top
    border covers it). The capture scale is fixed for the harness window, so the
    LCD width in source pixels is CAPTURE_LCD_W. Returns (x0, y0, cw, ch) or None.
    """
    arr = np.frombuffer(rgb, dtype=np.uint8).reshape(h, w, 3).astype(np.int16)
    bezel = np.all(np.abs(arr - np.array([40, 42, 46])) <= 3, axis=2)
    cols = bezel.sum(axis=0)
    left = int(np.argmax(cols))
    if cols[left] < h * 0.05:
        return None
    ys = np.where(bezel[:, left])[0]
    scale = CAPTURE_LCD_W / 800.0
    y0 = float(ys.min()) - 2.0
    return (left + 1.0, y0, CAPTURE_LCD_W, 480.0 * scale)

CAPTURE_LCD_W = 1682.0

def extract_lcd_800x480(full_screenshot_path: str, out_lcd_path: str) -> bool:
    w, h, rgb = read_image_rgb(full_screenshot_path)
    if (w, h) == (800, 480):
        # Native 1:1 LCD capture: the screenshot already is the 800x480 canvas.
        write_png_rgb(out_lcd_path, 800, 480, rgb)
        return True

    rect = detect_lcd_rect(w, h, rgb)
    if rect is None:
        dpi = w / 1280.0
        menu_h = 19.0 * dpi
        avail_w = w
        avail_h = h - menu_h
        scale = min(avail_w / 800.0, avail_h / (480.0 + 260.0))
        screen_w = int(round(800.0 * scale))
        screen_h = int(round(480.0 * scale))
        total_h = int(round((480.0 + 260.0) * scale))
        origin_x = int((avail_w - screen_w) * 0.5)
        origin_y = int((menu_h + (avail_h - total_h) * 0.5))
        rect = (origin_x, origin_y, screen_w, screen_h)

    lcd_rgb = crop_and_resize_rgb(w, h, rgb, *rect, 800, 480)
    write_png_rgb(out_lcd_path, 800, 480, lcd_rgb)
    return True

def compute_ssim_grayscale(im1: np.ndarray, im2: np.ndarray, k1=0.01, k2=0.03, l=255.0) -> float:
    """Computes Structural Similarity Index (SSIM) across 8x8 blocks."""
    im1 = im1.astype(np.float64)
    im2 = im2.astype(np.float64)
    c1 = (k1 * l) ** 2
    c2 = (k2 * l) ** 2
    h, w = im1.shape
    bh, bw = 8, 8
    sh, sw = h // bh, w // bw
    im1_blocks = im1[:sh*bh, :sw*bw].reshape(sh, bh, sw, bw).transpose(0, 2, 1, 3).reshape(-1, bh*bw)
    im2_blocks = im2[:sh*bh, :sw*bw].reshape(sh, bh, sw, bw).transpose(0, 2, 1, 3).reshape(-1, bh*bw)
    mu1 = np.mean(im1_blocks, axis=1)
    mu2 = np.mean(im2_blocks, axis=1)
    var1 = np.var(im1_blocks, axis=1)
    var2 = np.var(im2_blocks, axis=1)
    cov = np.mean((im1_blocks - mu1[:, None]) * (im2_blocks - mu2[:, None]), axis=1)
    num = (2 * mu1 * mu2 + c1) * (2 * cov + c2)
    den = (mu1**2 + mu2**2 + c1) * (var1 + var2 + c2)
    return float(np.mean(num / den))

def compare_and_composite(emu_path: str, app_lcd_path: str, diff_out_path: str, composite_out_path: str) -> Tuple[float, float, float, float, bytes, bytes]:
    ew, eh, b_emu = read_image_rgb(emu_path)
    aw, ah, b_app = read_image_rgb(app_lcd_path)
    if (ew, eh) != (800, 480):
        b_emu = crop_and_resize_rgb(ew, eh, b_emu, 0, 0, ew, eh, 800, 480)
    if (aw, ah) != (800, 480):
        b_app = crop_and_resize_rgb(aw, ah, b_app, 0, 0, aw, ah, 800, 480)

    diff_bytes = bytearray(len(b_emu))
    total_err = 0.0
    matched_px = 0
    total_px = 800 * 480

    for i in range(0, len(b_emu), 3):
        dr = abs(b_emu[i] - b_app[i])
        dg = abs(b_emu[i+1] - b_app[i+1])
        db = abs(b_emu[i+2] - b_app[i+2])
        err = (dr + dg + db) / 3.0
        total_err += err
        if err <= 24.0:
            matched_px += 1
            diff_bytes[i] = b_app[i] // 2
            diff_bytes[i+1] = b_app[i+1] // 2
            diff_bytes[i+2] = b_app[i+2] // 2
        else:
            diff_bytes[i] = min(255, int(err * 2))
            diff_bytes[i+1] = 20
            diff_bytes[i+2] = 60

    write_png_rgb(diff_out_path, 800, 480, bytes(diff_bytes))

    comp_w = 1610
    comp = bytearray(comp_w * 480 * 3)
    for r in range(480):
        row_off = r * comp_w * 3
        comp[row_off : row_off + 800 * 3] = b_emu[r * 800 * 3 : (r + 1) * 800 * 3]
        for gx in range(800, 810):
            comp[row_off + gx * 3 : row_off + gx * 3 + 3] = b"\x22\x24\x2a"
        comp[row_off + 810 * 3 : row_off + 1610 * 3] = b_app[r * 800 * 3 : (r + 1) * 800 * 3]
    write_png_rgb(composite_out_path, comp_w, 480, bytes(comp))

    mae_pct = (total_err / (total_px * 255.0)) * 100.0
    match_pct = (matched_px / total_px) * 100.0

    # Multi-dimensional perceptual metrics: Foreground Match & SSIM
    arr_emu = np.frombuffer(b_emu, dtype=np.uint8).reshape((480, 800, 3))
    arr_app = np.frombuffer(b_app, dtype=np.uint8).reshape((480, 800, 3))
    diff_arr = np.abs(arr_emu.astype(float) - arr_app.astype(float)).mean(axis=2)
    is_fg = (arr_emu.max(axis=2) > 20) | (arr_app.max(axis=2) > 20)
    fg_match_pct = float((diff_arr[is_fg] <= 24.0).mean() * 100.0) if np.any(is_fg) else 100.0
    gray_emu = np.dot(arr_emu[..., :3], [0.2989, 0.5870, 0.1140])
    gray_app = np.dot(arr_app[..., :3], [0.2989, 0.5870, 0.1140])
    ssim_pct = compute_ssim_grayscale(gray_emu, gray_app) * 100.0

    return mae_pct, match_pct, fg_match_pct, ssim_pct, b_emu, b_app

def run_blackbox_os_session(app_bin: str) -> bool:
    driver = OSInputDriver()
    print("[BLACK-BOX OS INPUT] Launching trakr_app for live OS input emulation...")
    proc = subprocess.Popen([app_bin, "--screen", "perform", "--polyend-font"], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    time.sleep(0.9)
    if driver.cg:
        driver.tap_key(125) # kVK_DownArrow
        time.sleep(0.05)
        driver.tap_key(126) # kVK_UpArrow
        time.sleep(0.05)
        driver.tap_key(122) # kVK_F1 (Toggle Track 1 in Perform)
        time.sleep(0.05)
        driver.tap_key(122) # kVK_F1 (Restore Track 1 in Perform)
        time.sleep(0.05)
        driver.tap_key(53)  # kVK_Escape (Quit cleanly via OS key event)
        print("[BLACK-BOX OS INPUT] Dispatched real CoreGraphics OS events (Arrows, F1, Escape).")
    try:
        proc.wait(timeout=2.0)
    except subprocess.TimeoutExpired:
        proc.terminate()
    return True

def main():
    app_bin = "./native/build/trakr_app"
    test_bin = "./native/build/trakr_tests"
    art_dir = os.path.normpath(os.path.join(
        os.path.dirname(os.path.abspath(__file__)), "..", "..", "..", "render", "compare"))
    only_screens = None
    show_hotspots = False
    skip_emu = False

    args = sys.argv[1:]
    i = 0
    while i < len(args):
        if args[i] == "--only" and i + 1 < len(args):
            only_screens = [s.strip().lower() for s in args[i + 1].split(",") if s.strip()]
            show_hotspots = True
            i += 2
        elif args[i] == "--hotspots":
            show_hotspots = True
            i += 1
        elif args[i] == "--no-emu":
            skip_emu = True
            i += 1
        elif not args[i].startswith("--"):
            art_dir = args[i]
            i += 1
        else:
            i += 1

    all_screens = ["perform", "pattern", "song", "master", "instrument", "sample", "loader", "config", "file"]
    screens = [s for s in all_screens if s in only_screens] if only_screens else all_screens

    need_emu = not skip_emu or any(not os.path.exists(f"/tmp/emu_{s}.png") for s in screens)
    if need_emu:
        print("[1:1 DISPLAY COMPARE] Generating authentic FT812 firmware reference frames...")
        subprocess.run([test_bin, "[emulator]"], check=True)

    if not only_screens:
        run_blackbox_os_session(app_bin)

    for screen in screens:
        full_png = os.path.join(art_dir, f"screen_{screen}_verified.png")
        lcd_png = f"/tmp/app_lcd_{screen}.png"
        emu_png = f"/tmp/emu_{screen}.png"
        diff_png = f"/tmp/diff_{screen}.png"
        comp_png = os.path.join(art_dir, f"comparison_{screen}_1to1.png")

        subprocess.run([app_bin, "--screen", screen, "--polyend-font", "--lcd-shot", "--screenshot", full_png], check=True)
        if os.path.exists(emu_png) and extract_lcd_800x480(full_png, lcd_png):
            mae, match, fg_match, ssim, b_emu, b_app = compare_and_composite(emu_png, lcd_png, diff_png, comp_png)
            print(f"[1:1 DISPLAY COMPARE] {screen.upper():10s} | Pixel: {match:6.2f}% | FG Match: {fg_match:5.2f}% | SSIM: {ssim:5.2f}% | MAE: {mae:4.2f}% | Composite: {comp_png}")
            if show_hotspots:
                print(analyze_screen_hotspots(screen, b_emu, b_app))

if __name__ == '__main__':
    main()

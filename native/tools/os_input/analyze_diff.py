#!/usr/bin/env python3
import sys
import os

from os_input_runner import read_image_rgb
from hotspot_analyzer import analyze_screen_hotspots

def main():
    screen = sys.argv[1] if len(sys.argv) > 1 else 'master'
    emu_png = f"/tmp/emu_{screen}.png"
    app_png = f"/tmp/app_lcd_{screen}.png"
    if not os.path.exists(emu_png) or not os.path.exists(app_png):
        print(f"Missing images for {screen}")
        return

    w, h, b_emu = read_image_rgb(emu_png)
    w, h, b_app = read_image_rgb(app_png)
    print(analyze_screen_hotspots(screen, b_emu, b_app, top_k=10))

    def px_err(idx):
        return (abs(b_emu[idx] - b_app[idx]) + abs(b_emu[idx+1] - b_app[idx+1]) + abs(b_emu[idx+2] - b_app[idx+2])) / 3.0

    print("\nBand Breakdown:")
    bands = [
        ("HeaderBar   (y=0..30)  ", 0, 30),
        ("UpperBody   (y=30..230)", 30, 230),
        ("LowerBody   (y=230..430)", 230, 430),
        ("SoftButtons (y=430..480)", 430, 480),
    ]
    for name, y0, y1 in bands:
        mm = 0
        y_counts = {}
        for y in range(y0, y1):
            ro = y * 800 * 3
            for x in range(800):
                if px_err(ro + x * 3) > 24.0:
                    mm += 1
                    y_counts[y] = y_counts.get(y, 0) + 1
        loss = mm / (800 * 480) * 100.0
        top_y = sorted(y_counts.items(), key=lambda t: t[1], reverse=True)[:5]
        top_y_str = ", ".join(f"y={y}({c}px)" for y, c in top_y)
        print(f"  {name}: {loss:5.2f}% ({mm:5d}px) | worst: {top_y_str}")

    if len(sys.argv) > 2 and sys.argv[2] == '--inspect':
        y_start = int(sys.argv[3])
        y_end = int(sys.argv[4])
        x_start = int(sys.argv[5]) if len(sys.argv) > 5 else 0
        x_end = int(sys.argv[6]) if len(sys.argv) > 6 else 100
        print(f"\nInspecting x={x_start}..{x_end}, y={y_start}..{y_end}:")
        for y in range(y_start, y_end):
            line_emu = "".join("#" if max(b_emu[(y*800+x)*3 : (y*800+x)*3+3]) > 100 else " " for x in range(x_start, x_end))
            line_app = "".join("#" if max(b_app[(y*800+x)*3 : (y*800+x)*3+3]) > 100 else " " for x in range(x_start, x_end))
            print(f"y={y:3d}: Emu|{line_emu}| App|{line_app}|")

    if len(sys.argv) > 2 and sys.argv[2] == '--inspect-rgb':
        y = int(sys.argv[3])
        x_start = int(sys.argv[4])
        x_end = int(sys.argv[5])
        print(f"\nRGB for y={y}, x={x_start}..{x_end}:")
        for x in range(x_start, x_end):
            idx = (y * 800 + x) * 3
            e_rgb = (b_emu[idx], b_emu[idx+1], b_emu[idx+2])
            a_rgb = (b_app[idx], b_app[idx+1], b_app[idx+2])
            diff = px_err(idx)
            print(f"  x={x:3d}: Emu=#{e_rgb[0]:02x}{e_rgb[1]:02x}{e_rgb[2]:02x} App=#{a_rgb[0]:02x}{a_rgb[1]:02x}{a_rgb[2]:02x} err={diff:.1f}")

if __name__ == '__main__':
    main()

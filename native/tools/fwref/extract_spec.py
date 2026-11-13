#!/usr/bin/env python3
"""Extract source-true firmware specs for the fwref oracle.

Every coordinate/style/text literal written here is taken verbatim from the
firmware source (whitespace-normalised) and validated against the recorded
line range. The emitted JSON carries a SHA-256 for each range so
`extract_spec.py --check` (and verify_spec.py) fail when the source drifts.

Usage:
  extract_spec.py            write native/tools/fwref/specs/*.json
  extract_spec.py --check    fail if the checked-in JSON is stale
"""

import hashlib
import json
import os
import re
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))
SPEC_DIR = os.path.join(ROOT, "native", "tools", "fwref", "specs")

INSTR = "core/interface/instrumentEditor/gInstrEdit.cpp"
MASTER = "core/interface/masterParams/gMasterParams.cpp"
PATTERN = "core/interface/patternEditor/gPatternEdit.cpp"
SONGTIMER = "core/interface/core/songTimer.cpp"
THEMES = "core/source/Themes.cpp"
STYLE_DEFS = "core/libs/NewDisplay/displayStructs.h"
TITLEBAR = "core/interface/core/titleBar.cpp"
GCONFIG = "core/interface/configEditor/gConfigEdit.cpp"
GCONFIGMENU = "core/interface/configEditor/configMenu.cpp"
PERFORM = "core/interface/performanceMode/gPerformance.cpp"
GSONG = "core/interface/songEditor/gSongEdit.cpp"
GLOADER = "core/interface/sampleImporter/gSampleImport.cpp"
GSAMPLE = "core/interface/samplePlayback/gSamplePlay.cpp"
GPROJ = "core/interface/projectEditor/gProjEdit.cpp"

def read(rel):
    with open(os.path.join(ROOT, rel), "r", encoding="utf-8") as f:
        return f.read().split("\n")

def norm(s):
    return re.sub(r"\s+", " ", s).strip()

def sha_range(lines, first, last):
    text = "\n".join(lines[first - 1:last])
    return hashlib.sha256(text.encode("utf-8")).hexdigest()

def range_text(lines, first, last):
    return norm(" ".join(lines[first - 1:last]))

def require(lines, first, last, expr, where):
    hay = range_text(lines, first, last)
    needle = norm(expr)
    if needle not in hay:
        raise SystemExit(
            f"extract_spec: expression not found in {where} lines {first}-{last}: {expr!r}"
        )

def src(rel, lines, first, last):
    return {"file": rel, "lines": [first, last], "sha256": sha_range(lines, first, last)}

# --------------------------------------------------------------------------
# Theme parsing
# --------------------------------------------------------------------------

COLOR_RE = re.compile(r"static constexpr uint32_t (\w+)\s*=\s*(0x[0-9A-Fa-f]+);")

def parse_colors(theme_lines):
    colors = {}
    for line in theme_lines:
        m = COLOR_RE.search(line)
        if m:
            colors[m.group(1)] = int(m.group(2), 16)
    return colors

def parse_theme_fields(theme_lines, first, last):
    text = "\n".join(theme_lines[first - 1:last])
    text = re.sub(r"//.*", "", text)
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    fields = {}
    for m in re.finditer(r"\.(\w+)\s*=\s*\{([^}]*)\}", text):
        name = m.group(1)
        raw = [t.strip() for t in m.group(2).split(",")]
        fields[name] = [t for t in raw if t]
    return fields

def resolve_colors(theme_fields, theme_colors, field):
    if field not in theme_fields:
        raise SystemExit(f"extract_spec: theme field {field!r} not found")
    out = []
    for token in theme_fields[field]:
        token = token.strip()
        if token.startswith("0x"):
            out.append(int(token, 16) & 0xFFFFFF)
        elif token in theme_colors:
            out.append(theme_colors[token] & 0xFFFFFF)
        elif re.fullmatch(r"\d+", token):
            out.append(int(token) & 0xFFFFFF)
        else:
            raise SystemExit(f"extract_spec: cannot resolve theme colour {token!r} in {field}")
    return out

# --------------------------------------------------------------------------
# Entry builders
# --------------------------------------------------------------------------

def entry(order, control, x, y, w, h, style, colors=None, **kw):
    e = {
        "order": order,
        "control": control,
        "x": x,
        "y": y,
        "w": w,
        "h": h,
        "style": style,
    }
    if colors is not None:
        e["colors"] = colors
    e.update(kw)
    return e

def parse_places(lines, first, last, expected=8):
    rows = []
    for line in lines[first - 1:last]:
        if "framesPlaces" in line:
            continue
        for m in re.finditer(r"\{\s*(.+?)\s*,\s*(.+?)\s*,\s*(.+?)\s*,\s*(.+?)\s*\}", line):
            rows.append({"x": m.group(1), "y": m.group(2), "w": m.group(3), "h": m.group(4)})
    if len(rows) != expected:
        raise SystemExit(
            f"extract_spec: expected {expected} framesPlaces rows, got {len(rows)}"
        )
    return rows

FILTER_LABELS = ["Disabled", "Low-pass", "High-pass", "Band-pass"]
SATURATION_LABELS = ["Hard Clip", "Soft Clip", "Saturate", "Overdrive", "Drive+"]

INSTR_LABELS = [
    ("Volume", "volume_db"),
    ("Panning", "panning"),
    ("Tune", "tune"),
    ("Finetune", "finetune"),
    ("Filter Type", "filter"),
    ("Cutoff", "cutoff"),
    ("Resonance", "resonance"),
    ("More", ""),
]
INSTR_BAR_VALUES = {
    0: "volume",
    1: "pan_leftright",
    2: "tune_leftright",
    3: "fine_leftright",
    5: "cutoff",
    6: "resonance",
}

MASTER_LABELS = [
    ("Volume", 1, "master_db"),
    ("Reverb", 0, " "),
    ("Delay", 0, " "),
    ("Limiter", 0, ""),
    ("EQ", 0, " "),
    ("Saturation", 1, "saturation"),
    ("Bass Boost", 1, "bass_boost"),
    ("Space", 1, "stereo_boost"),
]

def build_chrome(lines, songtimer_lines, theme_fields, colors):
    tl_top = resolve_colors(theme_fields, colors, "topBarLabelColors")
    timer_colors = resolve_colors(theme_fields, colors, "songTimerLabelColors")
    entries = []

    require(lines, 31, 37, "prop.x = 9;", TITLEBAR)
    require(lines, 31, 37, "prop.y = 13;", TITLEBAR)
    require(lines, 31, 37, "prop.w = 270;", TITLEBAR)
    e = entry(900, "cLabel", "9", "13", "270", "0",
              ["controlStyleCenterY", "controlStyleFont4", "controlStyleLimitTextWidth"],
              colors=tl_top, value=0, text="@title")
    e["source"] = src(TITLEBAR, lines, 31, 37)
    entries.append(e)

    require(lines, 39, 47, "prop.y = 13;", TITLEBAR)
    require(lines, 39, 47, "prop.x = 790;", TITLEBAR)
    # width is inherited from the reused prop (prop.w = 270 at line 35)
    e = entry(901, "cLabel", "790", "13", "270", "0",
              ["controlStyleRightX", "controlStyleCenterY", "controlStyleFont4"],
              colors=tl_top, value=0, text="@rightLabel")
    e["source"] = src(TITLEBAR, lines, 33, 47)
    entries.append(e)

    require(lines, 69, 83, "barLabelData.line.color = display.getTheme().topBarLabelColors[2];", TITLEBAR)
    require(lines, 69, 83, "prop.x = 0;", TITLEBAR)
    require(lines, 69, 83, "prop.w = 800;", TITLEBAR)
    require(lines, 69, 83, "prop.h = 28;", TITLEBAR)
    e = entry(902, "cLabel", "0", "0", "800", "28",
              ["controlStyleBackground", "controlStyleSpecialFx"],
              colors=tl_top, value=0, text="",
              line={"x1": 0, "y1": 28, "x2": 800, "y2": 28, "color": tl_top[2]})
    e["source"] = src(TITLEBAR, lines, 69, 83)
    entries.append(e)

    # Title-bar song timer. Interface::activateModule() shows it on every module
    # except the pattern editor (interface.cpp:418-443), and fPatternEdit toggles
    # it there; the chrome carries it for every screen.
    require(songtimer_lines, 15, 23, "prop1.x = 280;", SONGTIMER)
    require(songtimer_lines, 15, 23, "prop1.style = controlStyleFont4 | controlStyleCenterY;",
            SONGTIMER)
    require(songtimer_lines, 113, 135, '"%3im:%02is %2s P%i.%03i%2s"', SONGTIMER)
    e = entry(899, "cLabel", "280", "0", "0", "26",
              ["controlStyleFont4", "controlStyleCenterY"],
              colors=timer_colors, value=0, text_format="song_timer")
    e["source"] = src(SONGTIMER, songtimer_lines, 15, 23)
    e["text_source"] = src(SONGTIMER, songtimer_lines, 113, 135)
    entries.append(e)

    return entries

def build_instrument(lines, theme_fields, colors):
    entries = []
    active_labels = resolve_colors(theme_fields, colors, "activeLabelsColors")
    more_colors = resolve_colors(theme_fields, colors, "moreButtonColors")
    active_bars = resolve_colors(theme_fields, colors, "activeBarColors")
    active_buttons = resolve_colors(theme_fields, colors, "activeButtonLabelsColors")
    cover_colors = resolve_colors(theme_fields, colors, "blankSpaceCoverLabelColors")

    # frame (framesPlaces[0], gInstrEdit.cpp:15-24)
    require(lines, 16, 16, "{ 0 + 1, 31, 800 / 8 - 3, 391 }", INSTR)
    e = entry(0, "cFrame", "0 + 1", "31", "800 / 8 - 3", "391",
              ["controlStyleShow"], value="@instrument.selectedPlace")
    e["places"] = parse_places(lines, 15, 24)
    e["source"] = src(INSTR, lines, 15, 24)
    e["override_source"] = src(INSTR, lines, 1168, 1176)
    entries.append(e)

    # labels + bars (creation loop, gInstrEdit.cpp:150-173)
    for i in range(8):
        require(lines, 153, 161, "prop2.x = (800 / 8) * i + (800 / 16);", INSTR)
        name, fmt = INSTR_LABELS[i]
        label_colors = more_colors if i == 7 else (active_buttons if i == 4 else active_labels)
        kw = {"value": 1, "text": name, "text2_format": fmt}
        label_style = ["controlStyleCenterX", "controlStyleCenterY", "controlStyleFont3",
                       "controlStyleShow"]
        style2 = ["controlStyleCenterX", "controlStyleCenterY",
                  "controlStyleFont3" if i == 4 else "controlStyleFont2"]
        if i == 7:
            kw["dots"] = {"count": 2, "current": 0}
            kw["text2"] = ""
            label_style.append("controlStyleSpecialFx2")
        e = entry(2 + 2 * i, "cLabel",
                  f"(800 / 8) * {i} + (800 / 16)", "424", "800 / 8 - 6", "55",
                  label_style,
                  colors=label_colors,
                  style2=style2,
                  **kw)
        e["source"] = src(INSTR, lines, 153, 161)
        e["override_source"] = src(INSTR, lines, 798, 851)
        e["text_source"] = src(INSTR, lines, 831, 851)
        entries.append(e)

        if i not in INSTR_BAR_VALUES:
            continue
        require(lines, 163, 172, "prop2.x = (800 / 8) * i + 1;", INSTR)
        bar_style = ["controlStyleShow", "controlStyleBackground",
                     "controlStyleGradient", "controlStyleBottomShadow"]
        if i in (1, 2, 3):
            bar_style.insert(1, "controlStyleValueLeftRight_100_100")
        else:
            bar_style.insert(1, "controlStyleValue_0_100")
        e = entry(3 + 2 * i, "cBar",
                  f"(800 / 8) * {i} + 1", "29", "800 / 8 - 3", "394",
                  bar_style, colors=active_bars,
                  bar_value=INSTR_BAR_VALUES[i])
        e["source"] = src(INSTR, lines, 163, 172)
        e["override_source"] = src(INSTR, lines, 897, 920)
        entries.append(e)

    # bottom bar (gInstrEdit.cpp:175-183)
    require(lines, 175, 183, "prop2.style = controlStyleNoTransparency | controlStyleShow;", INSTR)
    e = entry(18, "cBgLabel", "0", "425", "800", "54",
              ["controlStyleNoTransparency", "controlStyleShow"], value=255)
    e["source"] = src(INSTR, lines, 175, 183)
    entries.append(e)

    # filter type list (gInstrEdit.cpp:185-197, filterModeFunctLabels 48-53)
    require(lines, 190, 197, "prop.x = (800 / 8) * (4) + 1;", INSTR)
    require(lines, 48, 53, '"Disabled"', INSTR)
    e = entry(19, "cList", "(800 / 8) * (4) + 1", "29", "800 / 8 - 3", "394",
              ["controlStyleBackground", "controlStyleBottomShadow"],
              list={"items": FILTER_LABELS, "start": "@instrument.filterListPos",
                    "length": 4, "linesCount": 5})
    e["source"] = src(INSTR, lines, 185, 197)
    e["override_source"] = src(INSTR, lines, 1612, 1643)
    e["text_source"] = src(INSTR, lines, 48, 53)
    entries.append(e)

    # blank space cover over column 7 (gInstrEdit.cpp:330-341, resized 855-856)
    require(lines, 855, 856, "display.setControlPosition(blankSpaceCover[0], (800 / 8) * 7 + 1, -1);", INSTR)
    require(lines, 855, 856, "display.setControlSize(blankSpaceCover[0], 800 / 8 - 3, 394);", INSTR)
    e = entry(36, "cLabel", "(800 / 8) * 7 + 1", "29", "800 / 8 - 3", "394",
              ["controlStyleBackground", "controlStyleBottomShadow", "controlStyleShow"],
              colors=cover_colors, value=0, text="")
    e["source"] = src(INSTR, lines, 330, 341)
    e["override_source"] = src(INSTR, lines, 855, 856)
    entries.append(e)

    return entries

def build_master(lines, theme_fields, colors):
    entries = []
    active_labels = resolve_colors(theme_fields, colors, "activeLabelsColors")
    active_bars = resolve_colors(theme_fields, colors, "activeBarColors")
    cover_colors = resolve_colors(theme_fields, colors, "blankSpaceCoverLabelColors")

    require(lines, 11, 11, "{ 0 + 1, 31, 800 / 8 - 3, 391 }", MASTER)
    e = entry(0, "cFrame", "0 + 1", "31", "800 / 8 - 3", "391",
              ["controlStyleShow"], value="@master.selectedPlace")
    e["places"] = parse_places(lines, 10, 19)
    e["source"] = src(MASTER, lines, 10, 19)
    e["override_source"] = src(MASTER, lines, 931, 939)
    entries.append(e)

    require(lines, 57, 67, "prop2.x = (800 / 8) * i + (800 / 16);", MASTER)
    for i, (name, value, fmt) in enumerate(MASTER_LABELS):
        kw = {"value": value, "text": name}
        if fmt and fmt != " ":
            kw["text2_format"] = fmt
        else:
            kw["text2"] = fmt
        e = entry(1 + i, "cLabel",
                  f"(800 / 8) * {i} + (800 / 16)", "424", "800 / 8 - 6", "55",
                  ["controlStyleCenterX", "controlStyleCenterY", "controlStyleFont3",
                   "controlStyleShow"],
                  colors=active_labels,
                  style2=["controlStyleCenterX", "controlStyleCenterY", "controlStyleFont2"],
                  **kw)
        e["source"] = src(MASTER, lines, 57, 67)
        e["override_source"] = src(MASTER, lines, 264, 322)
        e["text_source"] = src(MASTER, lines, 272, 297)
        entries.append(e)

    require(lines, 69, 77, "prop2.style = controlStyleNoTransparency | controlStyleShow;", MASTER)
    e = entry(9, "cBgLabel", "0", "425", "800", "54",
              ["controlStyleNoTransparency", "controlStyleShow"], value=255)
    e["source"] = src(MASTER, lines, 69, 77)
    entries.append(e)

    require(lines, 103, 114, "prop2.x = (800 / 8) * i + 2;", MASTER)
    for i, bar_value in ((0, "master_volume"), (6, "bass_boost"), (7, "stereo_boost_bar")):
        e = entry(26 + i, "cBar",
                  f"(800 / 8) * {i} + 2", "29", "800 / 8 - 3", "394",
                  ["controlStyleShow", "controlStyleValue_0_100", "controlStyleBackground",
                   "controlStyleGradient", "controlStyleBottomShadow"],
                  colors=active_bars, bar_value=bar_value, bar_data_white=True)
        e["source"] = src(MASTER, lines, 103, 114)
        e["override_source"] = src(MASTER, lines, 1031, 1036)
        entries.append(e)

    require(lines, 347, 348, "display.setControlPosition(blankSpaceCover[0], (800 / 8) * 1 + 2, -1);", MASTER)
    require(lines, 347, 348, "display.setControlSize(blankSpaceCover[0], (800 / 8) * 4 - 3, -1);", MASTER)
    e = entry(34, "cLabel", "(800 / 8) * 1 + 2", "29", "(800 / 8) * 4 - 3", "394",
              ["controlStyleBackground", "controlStyleBottomShadow", "controlStyleShow"],
              colors=cover_colors, value=0, text="")
    e["source"] = src(MASTER, lines, 138, 148)
    e["override_source"] = src(MASTER, lines, 347, 348)
    entries.append(e)

    require(lines, 235, 250, "saturationListProperties.x = (800 / 8) * (5) + 2;", MASTER)
    require(lines, 21, 27, '"Hard Clip"', MASTER)
    e = entry(41, "cList", "(800 / 8) * (5) + 2", "29", "800 / 8 - 3", "394",
              ["controlStyleBackground", "controlStyleBottomShadow"],
              list={"items": SATURATION_LABELS, "start": 0,
                    "length": 5, "linesCount": 5},
              value="@master.saturationType")
    e["source"] = src(MASTER, lines, 235, 250)
    e["override_source"] = src(MASTER, lines, 1189, 1196)
    e["text_source"] = src(MASTER, lines, 21, 27)
    entries.append(e)

    return entries

def mixer_bottom_labels(start_order, entries, lines, active_labels, override):
    """Master page 2/3 and 3/3 share the label[i] creation block (57-67)."""
    require(lines, 57, 67, "prop2.x = (800 / 8) * i + (800 / 16);", MASTER)
    for i in range(8):
        if start_order == "track":
            text = f"@master.trackDb.{i}"
            text2 = f"@master.mixMute.{i}"
        elif i < 4:
            text = f"@master.auxDb.{i}"
            text2 = f"@master.auxMute.{i}"
        else:
            text = ""
            text2 = " "
        e = entry(1 + i, "cLabel",
                  f"(800 / 8) * {i} + (800 / 16)", "424", "800 / 8 - 6", "55",
                  ["controlStyleCenterX", "controlStyleCenterY", "controlStyleFont3",
                   "controlStyleShow"],
                  colors=active_labels,
                  style2=["controlStyleCenterX", "controlStyleCenterY", "controlStyleFont2"],
                  value=1, text=text, text2=text2)
        e["source"] = src(MASTER, lines, 57, 67)
        e["override_source"] = src(MASTER, lines, override[0], override[1])
        entries.append(e)


def build_master2(lines, theme_fields, colors):
    entries = []
    active_labels = resolve_colors(theme_fields, colors, "activeLabelsColors")
    active_bars = resolve_colors(theme_fields, colors, "activeBarColors")
    top_labels = resolve_colors(theme_fields, colors, "topBarLabelColors")

    require(lines, 10, 19, "{ 0 + 1, 31, 800 / 8 - 3, 391 }", MASTER)
    require(lines, 1350, 1417, '"Track Mixer 2/3"', MASTER)
    require(lines, 1350, 1417, '"Mute"', MASTER)
    require(lines, 1350, 1417, "display.setControlText2(label[i], mixerLabel[i]);", MASTER)
    e = entry(0, "cFrame", "0", "0", "0", "0",
              ["controlStyleShow"], value="@master.selectedPlace", hidden=True)
    e["places"] = parse_places(lines, 10, 19)
    e["source"] = src(MASTER, lines, 10, 19)
    e["override_source"] = src(MASTER, lines, 1350, 1417)
    entries.append(e)

    mixer_bottom_labels("track", entries, lines, active_labels, (1350, 1417))

    require(lines, 69, 77, "prop2.style = controlStyleNoTransparency | controlStyleShow;", MASTER)
    e = entry(9, "cBgLabel", "0", "425", "800", "54",
              ["controlStyleNoTransparency", "controlStyleShow"], value=255)
    e["source"] = src(MASTER, lines, 69, 77)
    entries.append(e)

    require(lines, 79, 89, "prop2.x = (800 / 8) * i + (800 / 16);", MASTER)
    require(lines, 79, 89, "prop2.y = 32;", MASTER)
    require(lines, 79, 89, "prop2.h = 26;", MASTER)
    require(lines, 94, 101, "prop2.x = (800 / 8) * i + 2;", MASTER)
    require(lines, 94, 101, "prop2.y = 64;", MASTER)
    require(lines, 94, 101, "prop2.w = 800 / 16 - 3;", MASTER)
    require(lines, 94, 101, "prop2.h = 359;", MASTER)
    require(lines, 103, 114, "prop2.x = (800 / 8) * i + 2;", MASTER)
    require(lines, 103, 114, "prop2.y = 29;", MASTER)
    require(lines, 103, 114, "prop2.w = 800 / 8 - 3;", MASTER)
    require(lines, 103, 114, "prop2.h = 394;", MASTER)
    require(lines, 1046, 1062, "display.setControlPosition(trackVolumeBar[i], (800 / 8) * i + 2, 64);", MASTER)
    require(lines, 1046, 1062, "display.setControlSize(trackVolumeBar[i], 800 / 16 - 3, 359);", MASTER)

    for i in range(8):
        e = entry(10 + 3 * i, "cLabel",
                  f"(800 / 8) * {i} + (800 / 16)", "32", "800 / 8 - 4", "26",
                  ["controlStyleShow", "controlStyleBackground", "controlStyleCenterY",
                   "controlStyleCenterX", "controlStyleFont4"],
                  colors=top_labels, value=0, text=f"@master.trackName.{i}")
        e["source"] = src(MASTER, lines, 79, 89)
        e["override_source"] = src(MASTER, lines, 1350, 1417)
        entries.append(e)

        e = entry(11 + 3 * i, "cBar",
                  f"(800 / 8) * {i} + 2", "64", "800 / 16 - 3", "359",
                  ["controlStyleShow", "controlStyleValue_0_100", "controlStyleBackground",
                   "controlStyleGradient", "controlStyleBottomShadow"],
                  colors=active_bars, bar_value="track_volume", bar_arg=i)
        e["source"] = src(MASTER, lines, 94, 101)
        e["override_source"] = src(MASTER, lines, 1046, 1062)
        entries.append(e)

        e = entry(12 + 3 * i, "cBar",
                  f"(800 / 8) * {i} + 2", "29", "800 / 8 - 3", "394",
                  ["controlStyleShow", "controlStyleValue_0_100", "controlStyleBackground",
                   "controlStyleGradient", "controlStyleBottomShadow"],
                  colors=active_bars, bar_value="track_level", bar_arg=i)
        e["source"] = src(MASTER, lines, 103, 114)
        e["override_source"] = src(MASTER, lines, 1046, 1062)
        entries.append(e)

    return entries


def build_master3(lines, theme_fields, colors):
    entries = []
    active_labels = resolve_colors(theme_fields, colors, "activeLabelsColors")
    active_bars = resolve_colors(theme_fields, colors, "activeBarColors")
    inactive_bars = resolve_colors(theme_fields, colors, "inactiveBarColors")
    top_labels = resolve_colors(theme_fields, colors, "topBarLabelColors")

    require(lines, 10, 19, "{ 0 + 1, 31, 800 / 8 - 3, 391 }", MASTER)
    require(lines, 1419, 1627, '"Global Mixer 3/3"', MASTER)
    require(lines, 1419, 1627, '"Delay"', MASTER)
    require(lines, 1419, 1627, '"Reverb"', MASTER)
    require(lines, 1419, 1627, '"Dry Mix"', MASTER)
    require(lines, 1419, 1627, '"Line In"', MASTER)
    require(lines, 1110, 1125, "display.setControlSize(trackVolumeBar[i], 32, 359);", MASTER)
    require(lines, 1110, 1125, "display.setControlSize(barControl[i], 32, 359);", MASTER)
    require(lines, 1110, 1125, "display.setControlPosition(barControl[i + 4], (800 / 8) * i + 2 + 64, 64);", MASTER)

    e = entry(0, "cFrame", "0", "0", "0", "0",
              ["controlStyleShow"], value="@master.selectedPlace", hidden=True)
    e["places"] = parse_places(lines, 10, 19)
    e["source"] = src(MASTER, lines, 10, 19)
    e["override_source"] = src(MASTER, lines, 1603, 1604)
    entries.append(e)

    mixer_bottom_labels("aux", entries, lines, active_labels, (1419, 1627))

    require(lines, 69, 77, "prop2.style = controlStyleNoTransparency | controlStyleShow;", MASTER)
    e = entry(9, "cBgLabel", "0", "425", "800", "54",
              ["controlStyleNoTransparency", "controlStyleShow"], value=255)
    e["source"] = src(MASTER, lines, 69, 77)
    e["override_source"] = src(MASTER, lines, 1600, 1601)
    entries.append(e)

    require(lines, 79, 89, "prop2.x = (800 / 8) * i + (800 / 16);", MASTER)
    for i in range(8):
        hidden = i >= 4
        text = f"@master.auxName.{i}" if i < 4 else ""
        e = entry(10 + 3 * i, "cLabel",
                  f"(800 / 8) * {i} + (800 / 16)", "32", "800 / 8 - 4", "26",
                  ["controlStyleShow", "controlStyleBackground", "controlStyleCenterY",
                   "controlStyleCenterX", "controlStyleFont4"],
                  colors=top_labels, value=0, text=text, hidden=hidden)
        e["source"] = src(MASTER, lines, 79, 89)
        e["override_source"] = src(MASTER, lines, 1539, 1566)
        entries.append(e)

        e = entry(11 + 3 * i, "cBar",
                  f"(800 / 8) * {i} + 2", "64", "32", "359",
                  ["controlStyleShow", "controlStyleValue_0_100", "controlStyleBackground",
                   "controlStyleGradient", "controlStyleBottomShadow"],
                  colors=inactive_bars if i == 3 else active_bars,
                  bar_value="aux_volume", bar_arg=i, hidden=hidden)
        e["source"] = src(MASTER, lines, 94, 101)
        e["override_source"] = src(MASTER, lines, 1110, 1125)
        entries.append(e)

    require(lines, 103, 114, "prop2.x = (800 / 8) * i + 2;", MASTER)
    for i in range(8):
        group = i % 4
        x = f"(800 / 8) * {group} + 2 + " + ("32" if i < 4 else "64")
        e = entry(12 + 3 * i, "cBar", x, "64", "32", "359",
                  ["controlStyleShow", "controlStyleValue_0_100", "controlStyleBackground",
                   "controlStyleGradient", "controlStyleBottomShadow"],
                  colors=active_bars, bar_value="aux_level", bar_arg=group)
        e["source"] = src(MASTER, lines, 103, 114)
        e["override_source"] = src(MASTER, lines, 1110, 1125)
        entries.append(e)

    return entries


EQ_BAND_LABELS = ["115 Hz", "330 Hz", "990 Hz", "3000 Hz", "9900 Hz"]

def build_master_eq(lines, theme_fields, colors):
    entries = []
    active_labels = resolve_colors(theme_fields, colors, "activeLabelsColors")
    active_bars = resolve_colors(theme_fields, colors, "activeBarColors")
    cover_colors = resolve_colors(theme_fields, colors, "blankSpaceCoverLabelColors")

    # frameControl (gMasterParams.cpp:10-19) selected by selectedPlaceEqualizer
    require(lines, 10, 19, "{ 0 + 1, 31, 800 / 8 - 3, 391 }", MASTER)
    require(lines, 971, 979, "display.setControlValue(frameControl, selectedPlaceEqualizer);", MASTER)
    e = entry(0, "cFrame", "0 + 1", "31", "800 / 8 - 3", "391",
              ["controlStyleShow"], value="@master.selectedEqPlace")
    e["places"] = parse_places(lines, 10, 19)
    e["source"] = src(MASTER, lines, 10, 19)
    e["override_source"] = src(MASTER, lines, 971, 979)
    entries.append(e)

    # showEqualizerScreen(): band labels 0..4, blanks 5..6 and Back 7.
    require(lines, 57, 67, "prop2.x = (800 / 8) * i + (800 / 16);", MASTER)
    require(lines, 827, 837, 'display.setControlText(label[0], "115 Hz");', MASTER)
    require(lines, 827, 837, 'display.setControlText(label[7], "Back");', MASTER)
    require(lines, 839, 847, "display.setControlValue(label[i], 1);", MASTER)
    for i in range(8):
        text = EQ_BAND_LABELS[i] if i < 5 else ("Back" if i == 7 else " ")
        kw = {"value": 1, "text": text}
        if i < 5:
            kw["text2_format"] = "eq_band_db"
            kw["text2_arg"] = i
        else:
            kw["text2"] = " "
        e = entry(1 + i, "cLabel",
                  f"(800 / 8) * {i} + (800 / 16)", "424", "800 / 8 - 6", "55",
                  ["controlStyleCenterX", "controlStyleCenterY", "controlStyleFont3",
                   "controlStyleShow"],
                  colors=active_labels,
                  style2=["controlStyleCenterX", "controlStyleCenterY", "controlStyleFont2"],
                  **kw)
        e["source"] = src(MASTER, lines, 57, 67)
        e["override_source"] = src(MASTER, lines, 839, 847)
        e["text_source"] = src(MASTER, lines, 827, 837)
        entries.append(e)

    require(lines, 69, 77, "prop2.style = controlStyleNoTransparency | controlStyleShow;", MASTER)
    e = entry(9, "cBgLabel", "0", "425", "800", "54",
              ["controlStyleNoTransparency", "controlStyleShow"], value=255)
    e["source"] = src(MASTER, lines, 69, 77)
    e["override_source"] = src(MASTER, lines, 862, 864)
    entries.append(e)

    # Band bars 0..4 (bars 5..7 are hidden by showEqualizerScreen()).
    require(lines, 103, 114, "prop2.x = (800 / 8) * i + 2;", MASTER)
    require(lines, 1031, 1036, "display.setControlPosition(barControl[i], (800 / 8) * i + 2, 29);", MASTER)
    require(lines, 1031, 1036, "display.setControlSize(barControl[i], 800 / 8 - 3, 394);", MASTER)
    for i in range(5):
        e = entry(26 + i, "cBar",
                  f"(800 / 8) * {i} + 2", "29", "800 / 8 - 3", "394",
                  ["controlStyleShow", "controlStyleValue_0_100", "controlStyleBackground",
                   "controlStyleGradient", "controlStyleBottomShadow"],
                  colors=active_bars, bar_value="eq_band", bar_arg=i, bar_data_white=True)
        e["source"] = src(MASTER, lines, 103, 114)
        e["override_source"] = src(MASTER, lines, 1031, 1036)
        entries.append(e)

    # blankSpaceCover[0] covers columns 5..7 (gMasterParams.cpp:891-893).
    require(lines, 891, 893, "display.setControlPosition(blankSpaceCover[0], (800 / 8) * 5 + 2, -1);", MASTER)
    require(lines, 891, 893, "display.setControlSize(blankSpaceCover[0], (800 / 1) * 3 - 3, -1);", MASTER)
    e = entry(34, "cLabel", "(800 / 8) * 5 + 2", "29", "(800 / 1) * 3 - 3", "394",
              ["controlStyleBackground", "controlStyleBottomShadow", "controlStyleShow"],
              colors=cover_colors, value=0, text="")
    e["source"] = src(MASTER, lines, 138, 148)
    e["override_source"] = src(MASTER, lines, 891, 893)
    entries.append(e)

    return entries


def build_config(lines, theme_fields, colors):
    entries = []
    active_labels = resolve_colors(theme_fields, colors, "activeLabelsColors")
    blank_cover = resolve_colors(theme_fields, colors, "blankSpaceCoverLabelColors")
    item_text = resolve_colors(theme_fields, colors, "configItemEditorTextLabelColors")
    popup = resolve_colors(theme_fields, colors, "configConfirmPopUpLabelColors")
    grid_colors = resolve_colors(theme_fields, colors, "gridEditorColors")

    require(lines, 7, 24, "{ (800 / 8) * 0 + 1, 31, 800 / 4 - 3, 391 }", GCONFIG)
    require(lines, 274, 322, 'titleBar->setTitle("Config");', GCONFIG)
    require(lines, 274, 322, "display.setControlValue(bgLabel, 0b11000100);", GCONFIG)

    # popoutWindowLabel (gConfigEdit.cpp:50-59)
    require(lines, 50, 59, "prop9.style =", GCONFIG)
    e = entry(0, "cLabel", "400", "320", "800 - 10", "100",
              ["controlStyleBackground", "controlStyleCenterX", "controlStyleCenterY",
               "controlStyleFont2"],
              colors=popup, value=0, text="", hidden=True)
    e["source"] = src(GCONFIG, lines, 50, 59)
    entries.append(e)

    # frameControl (61-65)
    require(lines, 61, 65, "prop3.data = &frameData;", GCONFIG)
    e = entry(1, "cFrame", "0", "0", "0", "0",
              ["controlStyleShow"], value="@config.selectedPlace")
    e["places"] = parse_places(lines, 7, 14, 4)
    e["source"] = src(GCONFIG, lines, 61, 65)
    e["override_source"] = src(GCONFIG, lines, 305, 315)
    entries.append(e)

    # label[8] (81-106)
    require(lines, 81, 106, "prop2.x = (800 / 8) * i + ((i == 0 || i == 3) ? (800 / 8) : (800 / 16));", GCONFIG)
    require(lines, 81, 106, "prop2.y = 424;", GCONFIG)
    require(lines, 81, 106, "prop2.h = 55;", GCONFIG)
    require(lines, 274, 322, 'display.setControlText(label[7], "Enter");', GCONFIG)
    for i in range(8):
        x = f"(800 / 8) * {i} + " + ("(800 / 8)" if i in (0, 3) else "(800 / 16)")
        style = ["controlStyleCenterX", "controlStyleCenterY", "controlStyleFont3"]
        if i in (0, 3):
            style.append("controlStyleShowBitmap")
        e = entry(2 + i, "cLabel", x, "424", "800 / 8 - 6", "55", style,
                  colors=active_labels, value=0,
                  text="Enter" if i == 7 else "", text2="")
        e["source"] = src(GCONFIG, lines, 81, 106)
        e["override_source"] = src(GCONFIG, lines, 274, 322)
        entries.append(e)

    # bgLabel (108-117)
    require(lines, 108, 117, "prop2.style = controlStyleNoTransparency | controlStyleShow;", GCONFIG)
    e = entry(10, "cBgLabel", "0", "425", "800", "54",
              ["controlStyleNoTransparency", "controlStyleShow"], value=196)
    e["source"] = src(GCONFIG, lines, 108, 117)
    e["override_source"] = src(GCONFIG, lines, 312, 313)
    entries.append(e)

    # lists (119-144)
    require(lines, 119, 144, "configBasemenuListControl = display.createControl<cList>(&prop2);", GCONFIG)
    require(lines, 119, 144, "configSubmenuListControl = display.createControl<cParamValueList>(&prop2);", GCONFIG)
    require(lines, 119, 144, "configListControl = display.createControl<cList>(&prop2);", GCONFIG)
    e = entry(11, "cList", "(800 / 8) * 0 + 1", "29", "800 / 4 - 3", "394",
              ["controlStyleFont1", "controlStyleBackground", "controlStyleBottomShadow"],
              list={"from": "config.baseMenu", "start": "@config.selectedBase",
                    "length": 5, "linesCount": 14})
    e["source"] = src(GCONFIG, lines, 119, 128)
    entries.append(e)

    e = entry(12, "cParamValueList", "(800 / 8) * 2 + 1", "29", "800 / 2 - 3", "394",
              ["controlStyleFont1", "controlStyleBackground", "controlStyleBottomShadow"],
              param_list={"from": "config.items", "start": "@config.selectedItem",
                          "length": 0, "linesCount": 14},
              value="@config.selectedItem")
    e["source"] = src(GCONFIG, lines, 129, 137)
    entries.append(e)

    e = entry(13, "cList", "(800 / 8) * 5 + 1", "29", "800 / 4 - 3", "394",
              ["controlStyleFont1", "controlStyleBackground", "controlStyleBottomShadow"],
              list={"start": 0, "length": 0, "linesCount": 14}, hidden=True)
    e["source"] = src(GCONFIG, lines, 138, 145)
    entries.append(e)

    # configLabel (146-153)
    require(lines, 146, 153, "prop2.style = (controlStyleFont3);", GCONFIG)
    e = entry(14, "cLabel", "(800 / 8) * 2 + 15", "27", "800 / 4 - 18", "55",
              ["controlStyleFont3"], colors=item_text, value=0, text="", hidden=True)
    e["source"] = src(GCONFIG, lines, 146, 153)
    entries.append(e)

    # text boxes (155-173)
    require(lines, 155, 173, "creditsTextBox = display.createControl<cTextBox>(&prop2);", GCONFIG)
    require(lines, 155, 173, "descriptionTextBox = display.createControl<cTextBox>(&prop2);", GCONFIG)
    e = entry(15, "cTextBox", "0", "30", "780", "450",
              ["controlStyleFont5"], colors=item_text, value=3,
              text="@config.credits", hidden=True)
    e["source"] = src(GCONFIG, lines, 155, 164)
    entries.append(e)
    e = entry(16, "cTextBox", "(800 / 8) * 2 + 10", "60", "800 / 4 - 15", "365",
              ["controlStyleFont5"], colors=item_text, value=3,
              text="@config.description", hidden=True)
    e["source"] = src(GCONFIG, lines, 165, 173)
    entries.append(e)

    # grid editor + pad bars (175-211)
    require(lines, 175, 211, "gridPadsControl = display.createControl<cGridEditorPopup>(&gridPadsProp);", GCONFIG)
    e = entry(17, "cGridEditorPopup", "13", "130", "780", "260",
              [], colors=grid_colors, value="@config.gridSelected",
              grid={"selected": "@config.gridSelected", "count": 48}, hidden=True)
    e["source"] = src(GCONFIG, lines, 175, 196)
    entries.append(e)
    e = entry(18, "cBar", "(800 / 8) * 0 + 1", "29", "800 / 8 - 3", "394",
              ["controlStyleValue_0_100", "controlStyleBackground"], hidden=True)
    e["source"] = src(GCONFIG, lines, 198, 204)
    entries.append(e)
    e = entry(19, "cBar", "(800 / 8) * 1 + 1", "29", "800 / 8 - 3", "394",
              ["controlStyleValueLeftRight_100_100", "controlStyleBackground"], hidden=True)
    e["source"] = src(GCONFIG, lines, 206, 211)
    entries.append(e)

    # blank space covers (259-271)
    require(lines, 259, 271, "blankSpaceCover[0] = display.createControl<cLabel>(&prop2);", GCONFIG)
    e = entry(20, "cLabel", "(800 / 8) * 6 + 1", "29", "800 / 4 - 2", "394",
              ["controlStyleBackground", "controlStyleBottomShadow"],
              colors=blank_cover, value=0, text="")
    e["source"] = src(GCONFIG, lines, 259, 268)
    e["override_source"] = src(GCONFIG, lines, 302, 303)
    entries.append(e)
    e = entry(21, "cLabel", "(800 / 8) * 2 + 1", "29", "800 / 4 - 2", "394",
              ["controlStyleBackground", "controlStyleBottomShadow"],
              colors=blank_cover, value=0, text="", hidden=True)
    e["source"] = src(GCONFIG, lines, 259, 271)
    entries.append(e)

    return entries


def build_config_edit(lines, theme_fields, colors, menu_lines):
    """Config item editor state: itemEditorOpen() -> hideSubmenu(),
    showConfigItemLabel(), showConfigItemDesctription(), showConfigList4()
    (gConfigEdit.cpp) with Apply/Cancel labels set by configMenu.cpp."""
    entries = build_config(lines, theme_fields, colors)
    by_order = {e["order"]: e for e in entries}

    # activateLabelsBorder(): the frame stays on the submenu place (1) while
    # the item editor is open.
    by_order[1]["value"] = "1"

    # hideSubmenu(): the param/value list is hidden while the editor is open.
    submenu = by_order[12]
    submenu["hidden"] = True
    submenu["override_source"] = src(GCONFIG, lines, 517, 528)

    # showConfigList4(list_width=2): list at (800/8)*4+1, width (800/8)*2-3.
    lst = by_order[13]
    lst["x"] = "(800 / 8) * 4 + 1"
    lst["w"] = "((800 / 8) * 2) - 3"
    lst["hidden"] = False
    lst["list"] = {
        "from": "config.editItems",
        "start": "@config.editSelected",
        "length": 0,
        "linesCount": 14,
    }
    lst["value"] = "@config.editSelected"
    lst["override_source"] = src(GCONFIG, lines, 360, 393)

    # showConfigItemLabel(title) + the blank space cover behind it.
    label = by_order[14]
    label["hidden"] = False
    label["text"] = "@config.editTitle"
    label["override_source"] = src(GCONFIG, lines, 454, 462)
    by_order[21]["hidden"] = False
    by_order[21]["override_source"] = src(GCONFIG, lines, 454, 462)

    # showConfigItemDesctription(description).
    desc = by_order[16]
    desc["hidden"] = False
    desc["text"] = "@config.editDescription"
    desc["override_source"] = src(GCONFIG, lines, 471, 484)

    # loadConfigTextList()/loadConfigValuesList(): label[6] "Cancel",
    # label[7] "Apply" through changeLabelText().
    cancel = by_order[8]
    cancel["text"] = "Cancel"
    cancel["override_source"] = src(GCONFIGMENU, menu_lines, 470, 475)
    apply_label = by_order[9]
    apply_label["text"] = "Apply"
    apply_label["override_source"] = src(GCONFIGMENU, menu_lines, 470, 475)

    return entries


def build_sample_variant(lines, theme_fields, colors, state):
    """Slice/granular sample states unhide the state-specific overlays
    (cSlicePoints for Slice/Beat Slice, cLineIndicator for Granular) the way
    fSamplePlay showSlicesSelectValue()/showGranularPositionValue() do."""
    entries = build_sample(lines, theme_fields, colors)
    by_order = {e["order"]: e for e in entries}
    if state == "slice":
        # cSlicePoints is shown whenever playMode is Slice/BeatSlice
        # (fSamplePlay.cpp:430-450 processSlicePoints).
        by_order[14]["hidden"] = False
        by_order[14]["visibility_source"] = src(GSAMPLE, lines, 430, 450)
    # Granular keeps cLineIndicator hidden: the reference cannot yet composite
    # its additive grain window (the overlay is documented as a fwref gap; the
    # desktop draws it from the same gSamplePlay formula).
    return entries


def build_perform(lines, theme_fields, colors):
    entries = []
    active_labels = resolve_colors(theme_fields, colors, "activeLabelsColors")
    fx_name = resolve_colors(theme_fields, colors, "perfFxNameTextLabelColors")
    ml_colors = resolve_colors(theme_fields, colors, "perfValueNormalMLabelColors")

    require(lines, 32, 37, "{ 11, 31, 62, 391 }", PERFORM)
    require(lines, 119, 163, 'titleBar->setTitle("Performance FX");', PERFORM)
    require(lines, 90, 96, "controlStyleVerticalText);", PERFORM)
    require(lines, 97, 105, "multiLabelData[i].yForFxTextHole = 210;", PERFORM)
    require(lines, 97, 105, "multiLabelData[i].textLinesCount = 5;", PERFORM)
    require(lines, 97, 105, "multiLabelData[i].yOffset = 16;", PERFORM)
    require(lines, 293, 318, "multiLabelData[place].selected = performance.getSelectedRow(place) + 1;",
            PERFORM)
    require(lines, 266, 289, '"Patt: %d"', PERFORM)

    e = entry(0, "cFrame", "0", "0", "0", "0",
              ["controlStyleShow"], value=0, hidden=True)
    e["places"] = parse_places(lines, 32, 37, 12)
    e["source"] = src(PERFORM, lines, 32, 37)
    e["override_source"] = src(PERFORM, lines, 154, 154)
    entries.append(e)

    # label[8] (gPerformance.cpp:63-75), track names/patterns (217-289)
    require(lines, 63, 75, "prop2.x = (800 / 8) * i + (800 / 16);", PERFORM)
    require(lines, 63, 75, "prop2.y = 424;", PERFORM)
    require(lines, 63, 75, "prop2.h = 55;", PERFORM)
    for i in range(8):
        e = entry(1 + i, "cLabel",
                  f"(800 / 8) * {i} + (800 / 16)", "424", "800 / 8 - 6", "55",
                  ["controlStyleCenterX", "controlStyleCenterY", "controlStyleFont3",
                   "controlStyleShow"],
                  colors=active_labels,
                  style2=["controlStyleCenterX", "controlStyleCenterY", "controlStyleFont2"],
                  value=1, text=f"@perform.trackLabel.{i}",
                  text2=f"@perform.trackPatternLabel.{i}")
        e["source"] = src(PERFORM, lines, 63, 75)
        e["override_source"] = src(PERFORM, lines, 217, 289)
        entries.append(e)

    # bgLabel (77-85)
    require(lines, 77, 85, "prop2.style = controlStyleNoTransparency;", PERFORM)
    e = entry(9, "cBgLabel", "0", "425", "800", "54",
              ["controlStyleNoTransparency", "controlStyleShow"], value=255)
    e["source"] = src(PERFORM, lines, 77, 85)
    e["override_source"] = src(PERFORM, lines, 159, 160)
    entries.append(e)

    # per-slot vertical name + cMultiLabel (87-116)
    require(lines, 87, 116, "prop.x = 42 + 65 * i;", PERFORM)
    require(lines, 87, 116, "prop.y = 210 + 34;", PERFORM)
    require(lines, 87, 116, "prop.w = 62;", PERFORM)
    require(lines, 87, 116, "prop.h = 174 + 210 + 5;", PERFORM)
    require(lines, 87, 116, "value1Label[i] = display.createControl<cMultiLabel>(&prop);", PERFORM)
    for i in range(12):
        e = entry(10 + 2 * i, "cLabel",
                  f"42 + 65 * {i}", "210 + 34", "0", "0",
                  ["controlStyleCenterX", "controlStyleCenterY", "controlStyleFont2",
                   "controlStyleVerticalText"],
                  colors=fx_name, value=0, text=f"@perform.fxName.{i}")
        e["source"] = src(PERFORM, lines, 87, 96)
        e["override_source"] = src(PERFORM, lines, 119, 143)
        entries.append(e)

        e = entry(11 + 2 * i, "cMultiLabel",
                  f"42 + 65 * {i}", "34", "62", "174 + 210 + 5",
                  ["controlStyleCenterX", "controlStyleFont2", "controlStyleBackground",
                   "controlStyleBottomShadow"],
                  colors=ml_colors,
                  multi={"textLinesCount": 5,
                         "selected": f"@perform.selectedRow.{i}",
                         "xOffset": 0, "yOffset": 16, "yForFxTextHole": 210,
                         "text": [f"@perform.fxValue.{i}.{r}" for r in range(5)]})
        e["source"] = src(PERFORM, lines, 97, 116)
        e["override_source"] = src(PERFORM, lines, 293, 318)
        entries.append(e)

    return entries


SONG_LABEL_TEXTS = [
    ("Play", "Pattern"),
    ("Play", "Song"),
    ("Undo", ""),
    ("Add", "Slot"),
    ("Delete", "Slot"),
    ("Pattern", ""),
    ("", ""),
    ("Tempo", None),  # showTempoValue() formats "<tempo> BPM"
]


def build_song(lines, theme_fields, colors):
    entries = []
    active_labels = resolve_colors(theme_fields, colors, "activeLabelsColors")
    active_button = resolve_colors(theme_fields, colors, "activeButtonLabelsColors")
    inactive_button = resolve_colors(theme_fields, colors, "inactiveButtonLabelsColors")

    # frame (gSongEdit.cpp:7-27; framesPlaces[0..2], startPlace = 2)
    require(lines, 7, 11, "{ (800 / 8) * 0 + 1, 31, 500 - 3, 391 }", GSONG)
    require(lines, 13, 27, "frameData.startPlace = 2;", GSONG)
    e = entry(0, "cFrame", "0", "0", "0", "0", ["controlStyleShow"], value=0, hidden=True)
    e["places"] = parse_places(lines, 7, 11, 3)
    e["source"] = src(GSONG, lines, 13, 27)
    entries.append(e)

    # tempo bar (gSongEdit.cpp:29-37; value = globalTempo*100/MAX_TEMPO)
    require(lines, 29, 37, "prop1.x = (800 / 8) * 7 + 1;", GSONG)
    require(lines, 29, 37, "controlStyleValue_0_100", GSONG)
    require(lines, 29, 37, "controlStyleBackground", GSONG)
    require(lines, 29, 37, "controlStyleGradient", GSONG)
    require(lines, 29, 37, "controlStyleBottomShadow", GSONG)
    e = entry(1, "cBar", "(800 / 8) * 7 + 1", "29", "800 / 8 - 3", "394",
              ["controlStyleValue_0_100", "controlStyleBackground", "controlStyleGradient",
               "controlStyleBottomShadow", "controlStyleShow"],
              bar_value="song_tempo")
    e["source"] = src(GSONG, lines, 29, 37)
    entries.append(e)

    # action labels (gSongEdit.cpp:58-81, texts/colours 148-220 and 944-949)
    require(lines, 58, 81, "prop2.x = (800 / 8) * i + (800 / 16);", GSONG)
    require(lines, 58, 81, "prop2.y = 424;", GSONG)
    require(lines, 58, 81, "prop2.h = 55;", GSONG)
    require(lines, 58, 81, "controlStyleSpecialFx", GSONG)
    require(lines, 148, 220, 'display.setControlText(label[0], "Play");', GSONG)
    require(lines, 148, 220, 'display.setControlText(label[5], "Pattern");', GSONG)
    require(lines, 148, 220, 'display.setControlStyle2(label[7],', GSONG)
    for i, (text, text2) in enumerate(SONG_LABEL_TEXTS):
        style = ["controlStyleCenterX", "controlStyleCenterY", "controlStyleFont3",
                 "controlStyleShow"]
        if 2 <= i <= 5:
            style.append("controlStyleSpecialFx")
        if i in (0, 1, 6):
            label_colors = active_button
        elif i == 7:
            label_colors = active_labels
        else:
            label_colors = inactive_button
        style2 = ["controlStyleCenterX", "controlStyleCenterY",
                  "controlStyleFont2" if i == 7 else "controlStyleFont3"]
        x = f"(800 / 8) * {i} + (800 / 8)" if i == 5 else f"(800 / 8) * {i} + (800 / 16)"
        kw = {"value": 0 if i in (2, 5) else 1, "text": text}
        if text2 is None:
            kw["text2_format"] = "song_tempo_text"
        else:
            kw["text2"] = text2
        if i in (2, 3, 4):
            kw["line"] = {"x1": -40, "y1": 50, "x2": 40, "y2": 50,
                          "color": colors["one_true_red"]}
        elif i == 5:
            kw["line"] = {"x1": -90, "y1": 50, "x2": 90, "y2": 50,
                          "color": colors["one_true_red"]}
        e = entry(2 + i, "cLabel", x, "424", "800 / 8 - 6", "55", style,
                  colors=label_colors, style2=style2, **kw)
        e["source"] = src(GSONG, lines, 58, 81)
        e["text_source"] = src(GSONG, lines, 148, 220)
        entries.append(e)

    # bottom bar (gSongEdit.cpp:83-91, value 191 skips the x=600 separator)
    require(lines, 83, 91, "prop2.value = 191;", GSONG)
    require(lines, 83, 91, "prop2.style = controlStyleNoTransparency;", GSONG)
    e = entry(10, "cBgLabel", "0", "425", "800", "54",
              ["controlStyleNoTransparency", "controlStyleShow"], value=191)
    e["source"] = src(GSONG, lines, 83, 91)
    entries.append(e)

    # cSongPlayer (gSongEdit.cpp:93-107): slots/list/matrix/progress
    require(lines, 93, 107, "prop.data = nullptr;", GSONG)
    require(lines, 93, 107, "prop.x = 0 + 1;", GSONG)
    require(lines, 93, 107, "prop.y = 29;", GSONG)
    require(lines, 93, 107, "prop.w = ((800 / 8) * 7) - 3;", GSONG)
    require(lines, 93, 107, "prop.h = 394;", GSONG)
    require(lines, 93, 107, "controlStyleBackground | controlStyleVerticalText", GSONG)
    require(lines, 93, 107, "songPlayerControl = display.createControl<cSongPlayer>(&prop);",
            GSONG)
    e = entry(11, "cSongPlayer", "0 + 1", "29", "((800 / 8) * 7) - 3", "394",
              ["controlStyleBackground", "controlStyleVerticalText",
               "controlStyleStartNumbersFrom1", "controlStyleBottomShadow", "controlStyleShow"],
              song={"start": "@song.selectedSlot", "linesCount": 12})
    e["source"] = src(GSONG, lines, 93, 107)
    entries.append(e)

    return entries


PATTERN_LABELS = ["Pattern", "Length", "Step jump", "Fill", "Preview", "Undo", "Redo", "More"]


PATTERN_LINE_COLORS = {
    "red": ("one_true_red", ".red = one_true_red,"),
    "green": ("one_true_green", ".green = one_true_green,"),
}


def build_pattern(lines, theme_lines, theme_fields, colors, songtimer_lines):
    entries = []
    active_labels = resolve_colors(theme_fields, colors, "activeLabelsColors")
    inactive_labels = resolve_colors(theme_fields, colors, "inactiveLabelsColors")
    inactive_button = resolve_colors(theme_fields, colors, "inactiveButtonLabelsColors")
    more_button = resolve_colors(theme_fields, colors, "moreButtonColors")
    tracker_colors = resolve_colors(theme_fields, colors, "patternTrackerColors")
    timer_colors = resolve_colors(theme_fields, colors, "songTimerLabelColors")

    red_token, red_expr = PATTERN_LINE_COLORS["red"]
    green_token, green_expr = PATTERN_LINE_COLORS["green"]
    require(theme_lines, 168, 171, red_expr, THEMES)
    require(theme_lines, 168, 171, green_expr, THEMES)
    red = colors[red_token]
    green = colors[green_token]
    require(theme_lines, 176, 179, ".patternEditorBottomButtonsLines = 1,", THEMES)
    require(theme_lines, 176, 179, ".patternEditorPlayheadShadow = 1,", THEMES)

    # frame + 8 item places (gPatternEdit.cpp:14-23, prop zeroed at 84-91)
    require(lines, 15, 15, "{ 0 + 1, 423, 800 / 8 - 1, 55 }", PATTERN)
    require(lines, 84, 91, "prop.data = &frameData;", PATTERN)
    e = entry(0, "cFrame", "0", "0", "0", "0",
              ["controlStyleShow"], value="@pattern.selectedPlace")
    e["places"] = parse_places(lines, 14, 23)
    e["source"] = src(PATTERN, lines, 14, 23)
    e["override_source"] = src(PATTERN, lines, 84, 91)
    entries.append(e)

    # action button labels (gPatternEdit.cpp:95-107 geometry, 1436-1502 texts)
    require(lines, 95, 107, "prop2.x = (800 / 8) * i + (800 / 16);", PATTERN)
    require(lines, 1438, 1445, '"Step jump"', PATTERN)
    require(lines, 123, 136, "labelDataRed.line.color = display.getTheme().red;", PATTERN)
    require(lines, 123, 136, "labelDataGreen.line.color = display.getTheme().green;", PATTERN)
    require(lines, 123, 136, "labelDataMore.dotIndicator.count = 3;", PATTERN)
    text2_keys = { 0: "@pattern.patternNumber", 1: "@pattern.length", 2: "@pattern.editStep" }
    for i, name in enumerate(PATTERN_LABELS):
        if i in (0, 1, 2, 4):
            label_colors = active_labels
        elif i == 5:
            label_colors = inactive_button
        elif i == 7:
            label_colors = more_button
        else:
            label_colors = inactive_labels

        style = ["controlStyleCenterX", "controlStyleCenterY", "controlStyleFont3",
                 "controlStyleShow",
                 "controlStyleSpecialFx2" if i == 7 else "controlStyleSpecialFx"]
        kw = {"value": 0 if 3 <= i <= 6 else 1, "text": name}
        if i in text2_keys:
            kw["text2"] = text2_keys[i]
        else:
            kw["text2"] = ""
        if i == 7:
            kw["dots"] = {"count": 3, "current": 0}
        else:
            kw["line"] = {"x1": -40, "y1": 50, "x2": 40, "y2": 50,
                          "color": green if i in (0, 1, 2, 4) else red}
        e = entry(1 + i, "cLabel",
                  f"(800 / 8) * {i} + (800 / 16)", "424", "800 / 8 - 6", "55",
                  style, colors=label_colors,
                  style2=["controlStyleCenterX", "controlStyleCenterY", "controlStyleFont2"],
                  **kw)
        e["source"] = src(PATTERN, lines, 95, 107)
        e["override_source"] = src(PATTERN, lines, 1436, 1502)
        e["text_source"] = src(PATTERN, lines, 123, 136)
        entries.append(e)

    # bottom bar background (gPatternEdit.cpp:109-118)
    require(lines, 109, 118, "prop2.style = controlStyleNoTransparency | controlStyleShow;", PATTERN)
    e = entry(9, "cBgLabel", "0", "425", "800", "54",
              ["controlStyleNoTransparency", "controlStyleShow"], value=255)
    e["source"] = src(PATTERN, lines, 109, 118)
    e["override_source"] = src(PATTERN, lines, 287, 288)
    entries.append(e)

    # pattern grid (gPatternEdit.cpp:213-230)
    require(lines, 213, 220, "controlStyleFont4 |", PATTERN)
    require(lines, 213, 220,
            "mtConfig.general.numberingMode == NumberingMode::kDecimalWithStepsFrom1", PATTERN)
    require(lines, 213, 220, "controlStyleBottomShadow", PATTERN)
    require(lines, 213, 220, "controlStyleGradient", PATTERN)
    require(lines, 222, 226, "prop.style = commonStyle;", PATTERN)
    require(lines, 222, 226, "prop.y = mtConfig.general.patternInfo != 0 ? 0 : 28;", PATTERN)
    e = entry(22, "cTracker", "0", "28", "800", "395",
              ["controlStyleFont4", "controlStyleStartNumbersFrom1",
               "controlStyleBottomShadow", "controlStyleGradient", "controlStyleShow"],
              colors=tracker_colors, value="@pattern.viewMode")
    e["source"] = src(PATTERN, lines, 213, 230)
    entries.append(e)

    return entries


def list_header_colors(list_colors):
    # gSampleImport.cpp:424-432 explorerHeaderColors/instrumentHeaderColors
    return [list_colors[4], list_colors[3], list_colors[0], list_colors[5]]


def build_loader(lines, theme_fields, colors):
    entries = []
    active_labels = resolve_colors(theme_fields, colors, "activeLabelsColors")
    active_button = resolve_colors(theme_fields, colors, "activeButtonLabelsColors")
    inactive_button = resolve_colors(theme_fields, colors, "inactiveButtonLabelsColors")
    active_list = resolve_colors(theme_fields, colors, "sampleLoaderActiveListColors")
    inactive_list = resolve_colors(theme_fields, colors, "sampleLoaderInactiveListColors")
    mem_normal = resolve_colors(theme_fields, colors, "sampleLoadMemBarNormalColors")

    # --- cFrame: places[0] default frame is outside the page (x=1).
    places = parse_places(lines, 8, 12, 3)
    e = entry(0, "cFrame", "0", "0", "0", "0", ["controlStyleShow"],
              value="@loader.selectedPlace")
    e["places"] = places
    e["source"] = src(GLOADER, lines, 8, 25)
    entries.append(e)

    # --- action buttons: refreshActionButtons() leaves 3/5 inactive.
    label_text = [
        ("Add", 478, 500),
        ("Add next", 148, 229),
        ("Preview", 148, 229),
        ("Add", 466, 476),
        ("Import", 148, 229),
        ("Delete", 148, 229),
        ("Preview Vol", 148, 229),
        ("Memory", 148, 229),
    ]
    for i in range(8):
        col = active_button if i < 6 else active_labels
        if i in (3, 5):
            col = inactive_button
        style2 = ["controlStyleCenterX", "controlStyleCenterY",
                  "controlStyleFont3" if i == 5 else "controlStyleFont2"]
        text, text_first, text_last = label_text[i]
        e = entry(i + 1, "cLabel", "(800 / 8) * %d + (800 / 16)" % i, "424",
                  "800 / 8 - 6", "55",
                  ["controlStyleCenterX", "controlStyleCenterY", "controlStyleFont3",
                   "controlStyleShow"],
                  colors=col, style2=style2,
                  value=0 if i < 5 else 1, text=text)
        if i == 5:
            e["text2"] = "unused"
        elif i == 6:
            e["text2_format"] = "loader_preview_db"
        elif i == 7:
            e["text2_format"] = "loader_memory"
        e["source"] = src(GLOADER, lines, 27, 42)
        e["text_source"] = src(GLOADER, lines, text_first, text_last)
        entries.append(e)

    # --- bottom button background (bgLabel value 255 -> all 7 separators).
    e = entry(9, "cBgLabel", "0", "425", "800", "54",
              ["controlStyleNoTransparency", "controlStyleShow"], value=255)
    e["source"] = src(GLOADER, lines, 44, 52)
    e["override_source"] = src(GLOADER, lines, 197, 198)
    entries.append(e)

    # --- pane headers ("SD Card" / "Instruments").
    header_style = ["controlStyleShow", "controlStyleBackground", "controlStyleCenterY",
                    "controlStyleCenterX", "controlStyleFont4"]
    e = entry(10, "cLabel", "(800 / 16) * 3 - 1", "32", "(800 / 8) * 3 - 3", "26",
              header_style, colors=list_header_colors(active_list), value=0, text="SD Card")
    e["source"] = src(GLOADER, lines, 54, 62)
    e["text_source"] = src(GLOADER, lines, 200, 202)
    entries.append(e)

    e = entry(11, "cLabel", "(800 / 16) * 9 - 1", "32", "(800 / 8) * 3 - 3", "26",
              header_style, colors=list_header_colors(inactive_list), value=0,
              text="Instruments")
    e["source"] = src(GLOADER, lines, 64, 69)
    e["override_source"] = src(GLOADER, lines, 54, 62)  # y/h inherited from prop2
    e["text_source"] = src(GLOADER, lines, 203, 205)
    entries.append(e)

    # --- SD-card browser (active) and 48-slot instrument pool (inactive).
    e = entry(12, "cList", "0 + 1", "29 + 32", "(800 / 8) * 3 - 3", "394 - 32",
              ["controlStyleBackground", "controlStyleBottomShadow"], colors=active_list,
              list={"from": "loader.files", "start": "@loader.selectedFile",
                    "linesCount": 13, "selectionActive": 1,
                    "lineWidthMax": 270, "lineTextLenMax": 32})
    e["source"] = src(GLOADER, lines, 71, 84)
    entries.append(e)

    e = entry(13, "cList", "(800 / 8) * 3 + 1", "29 + 32", "(800 / 8) * 3 - 3",
              "394 - 32", ["controlStyleBackground", "controlStyleBottomShadow"],
              colors=inactive_list,
              list={"from": "loader.slots", "start": "@loader.selectedSlot",
                    "linesCount": 13, "selectionActive": 0,
                    "lineWidthMax": 270, "lineTextLenMax": 34})
    e["source"] = src(GLOADER, lines, 86, 94)
    # geometry (y/h) is inherited from the reused prop at lines 78/80.
    e["override_source"] = src(GLOADER, lines, 77, 84)
    entries.append(e)

    # --- memory gauge (compare two values) and preview volume bar.
    e = entry(14, "cBar", "(800 / 8) * 7 + 1", "29", "800 / 8 - 3", "394",
              ["controlStyleCompareTwoValues", "controlStyleBackground",
               "controlStyleGradient", "controlStyleBottomShadow", "controlStyleShow"],
              colors=mem_normal, bar_value="loader_memory",
              bar_compare="@loader.memoryUsageAdd")
    e["source"] = src(GLOADER, lines, 96, 104)
    e["override_source"] = src(GLOADER, lines, 278, 304)
    entries.append(e)

    e = entry(15, "cBar", "(800 / 8) * 6 + 1", "29", "800 / 8 - 3", "394",
              ["controlStyleValue_0_100", "controlStyleBackground", "controlStyleGradient",
               "controlStyleBottomShadow", "controlStyleShow"],
              bar_value="loader_preview_volume")
    e["source"] = src(GLOADER, lines, 106, 115)
    e["override_source"] = src(GLOADER, lines, 264, 276)
    entries.append(e)

    # --- waveform controls exist but showDefaultScreen() hides them.
    cursor_y = "75 + ((300 - (MAX_16BIT / 300)) / 2)"
    half_h = "(MAX_16BIT / 300) / 2"
    e = entry(16, "cProgressCursor", "100", cursor_y, "600", "MAX_16BIT / 300",
              [], hidden=True)
    e["source"] = src(GLOADER, lines, 124, 129)
    entries.append(e)

    e = entry(17, "cPoints", "100", cursor_y, "600", "MAX_16BIT / 300", [],
              hidden=True,
              points={"type": 0, "start": 0, "end": "MAX_16BIT",
                      "loop1": -1, "loop2": -1, "selected": 0, "zerocrossed": 0})
    e["source"] = src(GLOADER, lines, 117, 131)
    entries.append(e)

    e = entry(18, "cSpectrum", "100",
              "75 + ((300 - ((MAX_16BIT / 300))) / 2)", "600", half_h, [],
              hidden=True, spectrum={"type": 0, "pointsLimit": 300, "from": ""})
    e["source"] = src(GLOADER, lines, 133, 138)
    entries.append(e)

    e = entry(19, "cSpectrum", "100",
              "75 + ((300 - ((MAX_16BIT / 300))) / 2) + (MAX_16BIT / 300) / 2",
              "600", half_h, [], hidden=True,
              spectrum={"type": 0, "pointsLimit": 300, "from": ""})
    e["source"] = src(GLOADER, lines, 140, 145)
    entries.append(e)

    return entries


def build_sample(lines, theme_fields, colors):
    entries = []
    active_labels = resolve_colors(theme_fields, colors, "activeLabelsColors")

    places = parse_places(lines, 6, 14, 7)
    e = entry(0, "cFrame", "0", "0", "0", "0", ["controlStyleShow"],
              value="@sample.selectedPlace")
    e["places"] = places
    e["source"] = src(GSAMPLE, lines, 6, 32)
    entries.append(e)

    # Forward Loop: Preview/Start | Loop Start/Loop End | End/Zoom | Play Mode.
    labels = [
        ("Preview", 262, 300, "sample_preview", 1),
        ("Start", 262, 300, "sample_start_time", 1),
        ("Loop Start", 601, 613, "sample_loop1_time", 1),
        ("Loop End", 601, 613, "sample_loop2_time", 1),
        ("End", 262, 300, "sample_end_time", 1),
        ("Zoom", 262, 300, "sample_zoom", 1),
        ("Play Mode", 141, 154, "", 0),
        ("", 141, 154, "", 1),
    ]
    for i, (text, first, last, fmt, value) in enumerate(labels):
        x = "(800 / 8) * %d + (800 / 16)" % i
        w = "800 / 8 - 6"
        style = ["controlStyleCenterX", "controlStyleCenterY", "controlStyleFont3",
                 "controlStyleShow"]
        if i == 6:
            x = "(800 / 8) * 6 + (800 / 8)"
            w = "800 / 4 - 6"
            style.append("controlStyleShowBitmap")
        e = entry(i + 1, "cLabel", x, "424", w, "55", style, colors=active_labels,
                  style2=["controlStyleCenterX", "controlStyleCenterY",
                          "controlStyleFont2"],
                  value=value, text=text)
        if fmt:
            e["text2_format"] = fmt
        if i == 7:
            e["hidden"] = True
        e["source"] = src(GSAMPLE, lines, 44, 63)
        e["text_source"] = src(GSAMPLE, lines, first, last)
        entries.append(e)

    e = entry(9, "cBgLabel", "0", "425", "800", "54",
              ["controlStyleNoTransparency", "controlStyleShow"], value=127)
    e["source"] = src(GSAMPLE, lines, 65, 73)
    e["override_source"] = src(GSAMPLE, lines, 333, 335)
    entries.append(e)

    e = entry(10, "cList", "(800 / 8) * 6 + 1", "29", "800 / 4 - 3", "394",
              ["controlStyleBackground", "controlStyleBottomShadow"],
              list={"from": "sample.playModes", "start": "@sample.playMode",
                    "linesCount": 9, "selectionActive": 0})
    e["source"] = src(GSAMPLE, lines, 75, 86)
    e["override_source"] = src(GSAMPLE, lines, 449, 459)
    entries.append(e)

    cursor_y = "75 + ((300 - (MAX_16BIT / 300)) / 2)"
    full_h = "MAX_16BIT / 300"
    half_h = "(MAX_16BIT / 300) / 2"

    e = entry(11, "cProgressCursor", "0", cursor_y, "600", full_h, [],
              value="@sample.progress")
    e["source"] = src(GSAMPLE, lines, 88, 94)
    e["override_source"] = src(GSAMPLE, lines, 170, 173)
    entries.append(e)

    e = entry(12, "cLineIndicator", "0", cursor_y, "600", full_h, [], hidden=True,
              value="@sample.progress",
              line_indicator={"start": "@sample.regionStart", "end": "@sample.regionEnd"})
    e["source"] = src(GSAMPLE, lines, 96, 100)
    e["override_source"] = src(GSAMPLE, lines, 88, 94)  # shared cursor geometry
    entries.append(e)

    e = entry(13, "cPoints", "0", cursor_y, "600", full_h, [],
              points={"type": "@sample.playMode", "start": "@sample.startPoint",
                      "end": "@sample.endPoint", "loop1": "@sample.loop1",
                      "loop2": "@sample.loop2", "selected": "@sample.selected",
                      "zerocrossed": "@sample.zerocrossed"})
    e["source"] = src(GSAMPLE, lines, 102, 104)
    e["override_source"] = src(GSAMPLE, lines, 88, 94)
    entries.append(e)

    e = entry(14, "cSlicePoints", "0", cursor_y, "600", full_h, [], hidden=True,
              slice_points={"from": "sample.slices", "selected": "@sample.sliceSelected"})
    e["source"] = src(GSAMPLE, lines, 106, 107)
    e["override_source"] = src(GSAMPLE, lines, 88, 94)
    entries.append(e)

    e = entry(15, "cSpectrum", "0",
              "75 + ((300 - ((MAX_16BIT / 300))) / 2)", "600", full_h, [],
              spectrum={"type": 0, "pointsLimit": 300, "from": "sample.waveform"})
    e["source"] = src(GSAMPLE, lines, 112, 117)
    e["override_source"] = src(GSAMPLE, lines, 386, 390)
    entries.append(e)

    e = entry(16, "cSpectrum", "0",
              "75 + ((300 - ((MAX_16BIT / 300))) / 2) + (MAX_16BIT / 300) / 2",
              "600", half_h, [], hidden=True,
              spectrum={"type": 0, "pointsLimit": 300, "from": "sample.waveform"})
    e["source"] = src(GSAMPLE, lines, 119, 124)
    entries.append(e)

    e = entry(17, "cWtProgress", "0", "400", "600", "2", [], hidden=True,
              wt_progress={"position": "@sample.wtPosition", "window": "@sample.wtWindow",
                           "positionMax": "@sample.wtPositionMax"})
    e["source"] = src(GSAMPLE, lines, 126, 131)
    entries.append(e)

    return entries


def build_file(lines, theme_fields, colors):
    entries = []
    active_list = resolve_colors(theme_fields, colors, "projectActiveListColors")
    inactive_list = resolve_colors(theme_fields, colors, "projectInactiveListColors")
    blank_colors = resolve_colors(theme_fields, colors, "blankSpaceCoverLabelColors")
    topbar = resolve_colors(theme_fields, colors, "topBarLabelColors")
    active_button = resolve_colors(theme_fields, colors, "activeButtonLabelsColors")

    # cFrame (gProjEdit.cpp:12-15; 2 places, place 0 = file list active).
    places = parse_places(lines, 12, 15, 2)
    e = entry(0, "cFrame", "0", "0", "0", "0", ["controlStyleShow"],
              value="@file.selectedPlace")
    e["places"] = places
    e["source"] = src(GPROJ, lines, 12, 15)
    entries.append(e)

    # 8 footer labels (gProjEdit.cpp showDefaultScreen texts 140-156).
    footer = [("New", "Project"), ("Open", ""), ("Import Mod", ""), ("Save", ""),
              ("Save As", ""), ("Export", ""), ("", ""), ("", "")]
    for i in range(8):
        e = entry(i + 1, "cLabel", "(800 / 8) * %d + (800 / 16)" % i, "424",
                  "800 / 8 - 6", "55",
                  ["controlStyleCenterX", "controlStyleCenterY", "controlStyleFont3",
                   "controlStyleShow"],
                  colors=active_button, value=1, text=footer[i][0], text2=footer[i][1])
        e["source"] = src(GPROJ, lines, 25, 39)
        e["text_source"] = src(GPROJ, lines, 140, 156)
        entries.append(e)

    # footer background (gProjEdit.cpp:41-50).
    e = entry(9, "cBgLabel", "0", "425", "800", "54",
              ["controlStyleNoTransparency", "controlStyleShow"], value=255)
    e["source"] = src(GPROJ, lines, 41, 50)
    entries.append(e)

    # tab labels "SD Card" / "Recent" (gProjEdit.cpp:52-67).
    hs = ["controlStyleShow", "controlStyleBackground", "controlStyleCenterY",
          "controlStyleCenterX", "controlStyleFont4"]
    e = entry(10, "cLabel", "(800 / 16) * 3", "32", "(800 / 8) * 3 - 6", "26",
              hs, colors=topbar, value=0, text="SD Card")
    e["source"] = src(GPROJ, lines, 52, 60)
    e["text_source"] = src(GPROJ, lines, 267, 267)
    entries.append(e)
    e = entry(11, "cLabel", "(800 / 16) * 9", "32", "(800 / 8) * 3 - 6", "26",
              hs, colors=topbar, value=0, text="Recent")
    e["source"] = src(GPROJ, lines, 62, 67)
    e["override_source"] = src(GPROJ, lines, 52, 60)
    e["text_source"] = src(GPROJ, lines, 249, 249)
    entries.append(e)

    # project list (active) + recent list (empty, inactive) (gProjEdit.cpp:69-99).
    e = entry(12, "cList", "0 + 1", "29 + 32", "(800 / 8 * 3) - 3", "394 - 32",
              ["controlStyleBackground", "controlStyleBottomShadow"], colors=active_list,
              list={"from": "file.projects", "start": "@file.selectedProject",
                    "linesCount": 13, "selectionActive": 1,
                    "lineWidthMax": 270, "lineTextLenMax": 32})
    e["source"] = src(GPROJ, lines, 69, 80)
    entries.append(e)
    e = entry(13, "cList", "(800 / 8) * 3 + 1", "29 + 32", "(800 / 8 * 3) - 3", "394 - 32",
              ["controlStyleBackground", "controlStyleBottomShadow"], colors=inactive_list,
              list={"from": "file.recent", "start": "@file.selectedProject",
                    "linesCount": 10, "selectionActive": 0,
                    "lineWidthMax": 270, "lineTextLenMax": 32})
    e["source"] = src(GPROJ, lines, 82, 93)
    entries.append(e)

    # blankSpaceCover right pane (gProjEdit.cpp:105-114).
    e = entry(14, "cLabel", "600 + 1", "29", "200", "394",
              ["controlStyleBackground", "controlStyleBottomShadow"], colors=blank_colors,
              value=0)
    e["source"] = src(GPROJ, lines, 105, 114)
    e["override_source"] = src(GPROJ, lines, 249, 265)
    entries.append(e)

    return entries


def validate_entry(e, files, style_names, where):
    ranges = [e["source"]]
    for k in ("override_source", "text_source"):
        if k in e:
            ranges.append(e[k])
    hay = " ".join(range_text(files[r["file"]], r["lines"][0], r["lines"][1]) for r in ranges)
    hay += " " + " ".join(e.get("places", [{}])[0].values()) if e.get("places") else ""
    # MAX_16BIT (mtStructs.h:53) is accepted verbatim in the spec but the
    # evaluator substitutes its value; compare against the same expansion.
    hay = hay.replace("MAX_16BIT", "65535")

    for field in ("y", "w", "h"):
        val = e[field]
        # 0 is the zero-initialised strControlProperties default, not a source literal.
        if val == "0":
            continue
        needle = val.replace("MAX_16BIT", "65535")
        if not re.search(r"(?<!\d)" + re.escape(needle) + r"(?!\d)", hay):
            raise SystemExit(f"extract_spec: {where} {e['control']}: {field}={val!r} absent from cited source")

    for name in e.get("style", []) + e.get("style2", []):
        if name not in style_names:
            raise SystemExit(f"extract_spec: {where}: unknown style {name!r}")

    for key in ("text", "text2"):
        val = e.get(key, "")
        if val and val[0] != "@" and val.strip() and val not in hay:
            raise SystemExit(f"extract_spec: {where} {e['control']}: {key}={val!r} absent from cited source")

    for item in e.get("list", {}).get("items", []):
        if item not in hay:
            raise SystemExit(f"extract_spec: {where}: list item {item!r} absent from cited source")


def build():
    theme_lines = read(THEMES)
    colors = parse_colors(theme_lines)
    theme_fields = parse_theme_fields(theme_lines, 43, 180)

    instr_lines = read(INSTR)
    master_lines = read(MASTER)
    pattern_lines = read(PATTERN)
    songtimer_lines = read(SONGTIMER)
    title_lines = read(TITLEBAR)
    config_lines = read(GCONFIG)
    config_menu_lines = read(GCONFIGMENU)
    perform_lines = read(PERFORM)
    song_lines = read(GSONG)
    loader_lines = read(GLOADER)
    sample_lines = read(GSAMPLE)
    gproj_lines = read(GPROJ)

    provenance = {
        "extractor": "native/tools/fwref/extract_spec.py",
        "theme": src(THEMES, theme_lines, 43, 180),
        "files": {
            INSTR: sha_range(instr_lines, 1, len(instr_lines) - 1),
            MASTER: sha_range(master_lines, 1, len(master_lines) - 1),
            PATTERN: sha_range(pattern_lines, 1, len(pattern_lines) - 1),
            SONGTIMER: sha_range(songtimer_lines, 1, len(songtimer_lines) - 1),
            THEMES: sha_range(theme_lines, 1, len(theme_lines) - 1),
            TITLEBAR: sha_range(title_lines, 1, len(title_lines) - 1),
            GCONFIG: sha_range(config_lines, 1, len(config_lines) - 1),
            GCONFIGMENU: sha_range(config_menu_lines, 1, len(config_menu_lines) - 1),
            PERFORM: sha_range(perform_lines, 1, len(perform_lines) - 1),
            GSONG: sha_range(song_lines, 1, len(song_lines) - 1),
            GLOADER: sha_range(loader_lines, 1, len(loader_lines) - 1),
            GSAMPLE: sha_range(sample_lines, 1, len(sample_lines) - 1),
            GPROJ: sha_range(gproj_lines, 1, len(gproj_lines) - 1),
        },
    }

    style_lines = read(STYLE_DEFS)
    style_names = set(re.findall(r"\b(controlStyle\w+)\s*=", "\n".join(style_lines)))
    files = {INSTR: instr_lines, MASTER: master_lines, PATTERN: pattern_lines,
             SONGTIMER: songtimer_lines, THEMES: theme_lines, TITLEBAR: title_lines,
             GCONFIG: config_lines, GCONFIGMENU: config_menu_lines,
             PERFORM: perform_lines, GSONG: song_lines,
             GLOADER: loader_lines, GSAMPLE: sample_lines, GPROJ: gproj_lines}
    chrome_entries = build_chrome(title_lines, songtimer_lines, theme_fields, colors)
    instr_entries = build_instrument(instr_lines, theme_fields, colors)
    master_entries = build_master(master_lines, theme_fields, colors)
    master2_entries = build_master2(master_lines, theme_fields, colors)
    master3_entries = build_master3(master_lines, theme_fields, colors)
    master_eq_entries = build_master_eq(master_lines, theme_fields, colors)
    config_entries = build_config(config_lines, theme_fields, colors)
    config_edit_entries = build_config_edit(config_lines, theme_fields, colors,
                                            config_menu_lines)
    sample_slice_entries = build_sample_variant(sample_lines, theme_fields, colors,
                                                 "slice")
    perform_entries = build_perform(perform_lines, theme_fields, colors)
    pattern_entries = build_pattern(pattern_lines, theme_lines, theme_fields, colors,
                                    songtimer_lines)
    song_entries = build_song(song_lines, theme_fields, colors)
    loader_entries = build_loader(loader_lines, theme_fields, colors)
    sample_entries = build_sample(sample_lines, theme_fields, colors)
    file_entries = build_file(gproj_lines, theme_fields, colors)
    for e in chrome_entries:
        validate_entry(e, files, style_names, "chrome")
    for e in instr_entries:
        validate_entry(e, files, style_names, "instrument")
    for e in master_entries:
        validate_entry(e, files, style_names, "master")
    for e in master2_entries:
        validate_entry(e, files, style_names, "master2")
    for e in master3_entries:
        validate_entry(e, files, style_names, "master3")
    for e in master_eq_entries:
        validate_entry(e, files, style_names, "master_eq")
    for e in config_entries:
        validate_entry(e, files, style_names, "config")
    for e in config_edit_entries:
        validate_entry(e, files, style_names, "config_edit")
    for e in sample_slice_entries:
        validate_entry(e, files, style_names, "sample_slice")
    for e in perform_entries:
        validate_entry(e, files, style_names, "perform")
    for e in pattern_entries:
        validate_entry(e, files, style_names, "pattern")
    for e in song_entries:
        validate_entry(e, files, style_names, "song")
    for e in loader_entries:
        validate_entry(e, files, style_names, "loader")
    for e in sample_entries:
        validate_entry(e, files, style_names, "sample")
    for e in file_entries:
        validate_entry(e, files, style_names, "file")

    chrome = {
        "screen": "chrome",
        "theme": "default",
        "provenance": provenance,
        "entries": chrome_entries,
    }
    instrument = {
        "screen": "instrument_general",
        "scenario": "instrument",
        "theme": "default",
        "chrome": {"title": "Instrument Parameters 1/2",
                   "rightLabel": "@instrument.rightLabel"},
        "provenance": provenance,
        "entries": instr_entries,
    }
    master = {
        "screen": "master_page1",
        "scenario": "master",
        "theme": "default",
        "chrome": {"title": "Master 1/3", "rightLabel": ""},
        "provenance": provenance,
        "entries": master_entries,
    }
    master_page2 = {
        "screen": "master_page2",
        "scenario": "master",
        "theme": "default",
        "chrome": {"title": "Track Mixer 2/3", "rightLabel": ""},
        "provenance": provenance,
        "entries": master2_entries,
    }
    master_page3 = {
        "screen": "master_page3",
        "scenario": "master",
        "theme": "default",
        "chrome": {"title": "Global Mixer 3/3", "rightLabel": ""},
        "provenance": provenance,
        "entries": master3_entries,
    }
    master_eq = {
        "screen": "master_eq",
        "scenario": "master",
        "theme": "default",
        "chrome": {"title": "Master EQ", "rightLabel": ""},
        "provenance": provenance,
        "entries": master_eq_entries,
    }
    config = {
        "screen": "config",
        "scenario": "config",
        "theme": "default",
        "chrome": {"title": "Config", "rightLabel": ""},
        "provenance": provenance,
        "entries": config_entries,
    }
    sample_slice = {
        "screen": "sample_slice",
        "scenario": "sample",
        "theme": "default",
        "chrome": {"title": "Sample Playback", "rightLabel": "@instrumentName"},
        "provenance": provenance,
        "entries": sample_slice_entries,
    }
    config_edit = {
        "screen": "config_edit",
        "scenario": "config",
        "theme": "default",
        "chrome": {"title": "Config", "rightLabel": ""},
        "provenance": provenance,
        "entries": config_edit_entries,
    }
    perform = {
        "screen": "perform",
        "scenario": "perform",
        "theme": "default",
        "chrome": {"title": "Performance FX", "rightLabel": ""},
        "provenance": provenance,
        "entries": perform_entries,
    }
    pattern = {
        "screen": "pattern",
        "scenario": "pattern",
        "theme": "default",
        "chrome": {"title": "Pattern 1", "rightLabel": "@pattern.rightLabel"},
        "provenance": provenance,
        "entries": pattern_entries,
    }
    song = {
        "screen": "song",
        "scenario": "song",
        "theme": "default",
        "chrome": {"title": "Song", "rightLabel": ""},
        "chrome_on_top": True,
        "provenance": provenance,
        "entries": song_entries,
    }
    loader = {
        "screen": "loader",
        "scenario": "loader",
        "theme": "default",
        "chrome": {"title": "Sample Loader", "rightLabel": ""},
        "chrome_on_top": True,
        "provenance": provenance,
        "entries": loader_entries,
    }
    sample = {
        "screen": "sample",
        "scenario": "sample",
        "theme": "default",
        "chrome": {"title": "Sample Playback", "rightLabel": "@sample.rightLabel"},
        "chrome_on_top": True,
        "provenance": provenance,
        "entries": sample_entries,
    }
    file = {
        "screen": "file",
        "scenario": "file",
        "theme": "default",
        "chrome": {"title": "File", "rightLabel": "@file.projectName"},
        "provenance": provenance,
        "entries": file_entries,
    }
    return {
        "chrome.json": chrome,
        "instrument_general.json": instrument,
        "master_page1.json": master,
        "master_page2.json": master_page2,
        "master_page3.json": master_page3,
        "master_eq.json": master_eq,
        "config.json": config,
        "config_edit.json": config_edit,
        "perform.json": perform,
        "pattern.json": pattern,
        "song.json": song,
        "loader.json": loader,
        "sample.json": sample,
        "sample_slice.json": sample_slice,
        "file.json": file,
    }

def main():
    args = sys.argv[1:]
    check = "--check" in args
    spec_dir = SPEC_DIR
    if "--spec" in args:
        spec_dir = args[args.index("--spec") + 1]
        if not os.path.isabs(spec_dir):
            spec_dir = os.path.join(ROOT, spec_dir)
    specs = build()
    stale = []
    os.makedirs(spec_dir, exist_ok=True)
    for name, obj in specs.items():
        text = json.dumps(obj, indent=1) + "\n"
        path = os.path.join(spec_dir, name)
        if check:
            if not os.path.exists(path):
                stale.append(name + " (missing)")
                continue
            with open(path, "r", encoding="utf-8") as f:
                if f.read() != text:
                    stale.append(name)
        else:
            with open(path, "w", encoding="utf-8") as f:
                f.write(text)
            print("wrote", os.path.relpath(path, ROOT))
    if check:
        if stale:
            print("specs are stale: " + ", ".join(stale), file=sys.stderr)
            return 1
        print("specs up to date")
    return 0

if __name__ == "__main__":
    sys.exit(main())

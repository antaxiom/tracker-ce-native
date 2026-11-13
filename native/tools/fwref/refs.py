#!/usr/bin/env python3
"""Shared fwref reference lists.

Single source of truth for the spec files hashed by make_manifest.py and the
PNGs compared/checked by check_reference.py. Spec files are derived from the
specs directory so a new spec is covered by the manifest automatically.
"""

import glob
import os

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))
SPEC_DIR = os.path.join(ROOT, "native", "tools", "fwref", "specs")


def spec_names():
    return sorted(os.path.basename(p) for p in glob.glob(os.path.join(SPEC_DIR, "*.json")))


NON_RENDER_SPECS = {"chrome.json"}
PNG_OVERRIDES = {"instrument_general": "instrument_params.png"}
SAMPLE_STATE_PNGS = {
    "slice": "sample_slice.png",
    "granular": "sample_granular.png",
}


def rendered():
    screens = []
    for name in spec_names():
        if name in NON_RENDER_SPECS:
            continue
        stem = name[:-len(".json")]
        screens.append((stem, PNG_OVERRIDES.get(stem, stem + ".png")))
    return screens


RENDERED = rendered()


def rendered_pngs():
    return [png for _, png in RENDERED]


def all_pngs():
    return rendered_pngs() + list(SAMPLE_STATE_PNGS.values())

#!/usr/bin/env python3
"""Write/verify render/fwref/reference_manifest.json + provenance.json.

The manifest locks together:
  * scenario.json hash
  * spec JSON hashes
  * per-source-file hashes recorded by extract_spec.py
  * the generated PNG hashes

Usage:
  make_manifest.py --out render/fwref          write both files
  make_manifest.py --out render/fwref --check  fail if they are stale
"""

import hashlib
import json
import os
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))
sys.path.insert(0, os.path.dirname(__file__))
import refs  # noqa: E402
SPEC_DIR = refs.SPEC_DIR
SCREENS = refs.RENDERED

def sha_file(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(65536), b""):
            h.update(chunk)
    return h.hexdigest()

def build(out_dir):
    manifest = {"scenario": {}, "specs": {}, "sources": {}, "pngs": {}}
    scenario = os.path.join(os.path.dirname(SPEC_DIR), "scenario.json")
    manifest["scenario"]["sha256"] = sha_file(scenario)

    for name in refs.spec_names():
        manifest["specs"][name] = sha_file(os.path.join(SPEC_DIR, name))

    with open(os.path.join(SPEC_DIR, "instrument_general.json")) as f:
        spec = json.load(f)
    for rel, digest in spec["provenance"]["files"].items():
        manifest["sources"][rel] = digest
    manifest["sources"]["core/source/Themes.cpp@theme"] = spec["provenance"]["theme"]["sha256"]

    for png in refs.all_pngs():
        path = os.path.join(out_dir, png)
        if not os.path.exists(path):
            raise SystemExit(f"make_manifest: missing {path}; run fwref_gen first")
        manifest["pngs"][png] = sha_file(path)

    provenance = {}
    for name, _ in SCREENS:
        with open(os.path.join(SPEC_DIR, name + ".json")) as f:
            s = json.load(f)
        provenance[name] = {
            "screen": s.get("screen"),
            "entries": [
                {
                    "order": e.get("order"),
                    "control": e.get("control"),
                    "file": e.get("source", {}).get("file"),
                    "lines": e.get("source", {}).get("lines"),
                    "sha256": e.get("source", {}).get("sha256"),
                }
                for e in s.get("entries", [])
            ],
        }
    return manifest, provenance

def main():
    out_dir = os.path.join(ROOT, "render", "fwref")
    args = sys.argv[1:]
    check = "--check" in args
    if "--out" in args:
        out_dir = os.path.join(ROOT, args[args.index("--out") + 1])

    manifest, provenance = build(out_dir)
    mpath = os.path.join(out_dir, "reference_manifest.json")
    ppath = os.path.join(out_dir, "provenance.json")
    mtext = json.dumps(manifest, indent=1, sort_keys=True) + "\n"
    ptext = json.dumps(provenance, indent=1, sort_keys=True) + "\n"

    if check:
        stale = []
        for path, text in ((mpath, mtext), (ppath, ptext)):
            if not os.path.exists(path) or open(path).read() != text:
                stale.append(os.path.relpath(path, ROOT))
        if stale:
            print("manifest stale: " + ", ".join(stale), file=sys.stderr)
            return 1
        print("manifest up to date")
        return 0

    os.makedirs(out_dir, exist_ok=True)
    with open(mpath, "w") as f:
        f.write(mtext)
    with open(ppath, "w") as f:
        f.write(ptext)
    print("wrote", os.path.relpath(mpath, ROOT))
    print("wrote", os.path.relpath(ppath, ROOT))
    return 0

if __name__ == "__main__":
    sys.exit(main())

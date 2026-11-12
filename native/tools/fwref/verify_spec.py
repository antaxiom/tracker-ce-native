#!/usr/bin/env python3
"""Re-extract the firmware specs and fail on any source drift."""

import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))

def main():
    return subprocess.call([sys.executable, os.path.join(HERE, "extract_spec.py"), "--check",
                            *sys.argv[1:]])

if __name__ == "__main__":
    sys.exit(main())

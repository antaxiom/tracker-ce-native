#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"
EXT_DIR="${ROOT_DIR}/native/external"

mkdir -p "${EXT_DIR}/miniaudio" "${EXT_DIR}/dr_libs" "${EXT_DIR}/nlohmann"

echo "==> Fetching Dear ImGui (docking branch)..."
if [ ! -d "${EXT_DIR}/imgui" ]; then
    git clone --depth 1 -b docking https://github.com/ocornut/imgui.git "${EXT_DIR}/imgui"
else
    echo "    Dear ImGui already present."
fi

echo "==> Fetching miniaudio.h..."
if [ ! -f "${EXT_DIR}/miniaudio/miniaudio.h" ]; then
    curl -fsSL https://raw.githubusercontent.com/mackron/miniaudio/master/miniaudio.h -o "${EXT_DIR}/miniaudio/miniaudio.h"
else
    echo "    miniaudio.h already present."
fi

echo "==> Fetching dr_libs (dr_wav, dr_flac, dr_mp3)..."
for f in dr_wav.h dr_flac.h dr_mp3.h; do
    if [ ! -f "${EXT_DIR}/dr_libs/${f}" ]; then
        curl -fsSL "https://raw.githubusercontent.com/mackron/dr_libs/master/${f}" -o "${EXT_DIR}/dr_libs/${f}"
    else
        echo "    ${f} already present."
    fi
done

echo "==> Fetching nlohmann/json.hpp..."
if [ ! -f "${EXT_DIR}/nlohmann/json.hpp" ]; then
    curl -fsSL https://raw.githubusercontent.com/nlohmann/json/develop/single_include/nlohmann/json.hpp -o "${EXT_DIR}/nlohmann/json.hpp"
else
    echo "    nlohmann/json.hpp already present."
fi

echo "==> Fetching RtMidi..."
if [ ! -d "${EXT_DIR}/rtmidi" ]; then
    git clone --depth 1 https://github.com/thestk/rtmidi.git "${EXT_DIR}/rtmidi"
else
    echo "    RtMidi already present."
fi

echo "==> All external dependencies fetched successfully into native/external/"


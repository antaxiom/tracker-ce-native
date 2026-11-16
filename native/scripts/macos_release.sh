#!/usr/bin/env bash
# On-demand macOS build + release upload. Not a CI runner and not always-on:
# run this only when you want a macOS build (e.g. after a version/tag push).
# It builds the native app in a fresh Release build dir, runs the unit/ctest
# suite as a sanity gate, packages the .app, and (optionally) creates a GitHub
# release + uploads the zip. Then it exits — nothing stays running.
#
# Usage:
#   bash native/scripts/macos_release.sh            # build+package only (prints zip path)
#   bash native/scripts/macos_release.sh v1.2.3      # also gh release create + upload
#
# Prereqs: cmake, sdl3 (brew), git, gh authenticated to the origin remote.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
NATIVE_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
ROOT_DIR="$(cd "${NATIVE_DIR}/.." && pwd)"
TAG="${1:-}"

# 1. externals (idempotent; only fetches what is missing).
"${SCRIPT_DIR}/fetch_deps.sh"

# 2. Fresh Release build dir so we never disturb the dev build.
BUILD_DIR="${NATIVE_DIR}/build-release-macos"
echo "==> Configuring Release + macOS .app bundle in ${BUILD_DIR}"
cmake -S "${NATIVE_DIR}" -B "${BUILD_DIR}" \
  -DCMAKE_BUILD_TYPE=Release \
  -DTRAKR_BUILD_MACOS_BUNDLE=ON

echo "==> Building trakr_app"
cmake --build "${BUILD_DIR}" --config Release -j

echo "==> Running ctest sanity gate"
(cd "${BUILD_DIR}" && ctest --output-on-failure)

# 3. Locate the .app bundle.
APP="$(find "${BUILD_DIR}" -maxdepth 4 -name "Trakr.app" -print -quit 2>/dev/null || true)"
if [ -z "${APP}" ]; then
  APP="$(find "${BUILD_DIR}" -maxdepth 4 -name "trakr_app.app" -print -quit 2>/dev/null || true)"
fi
if [ -z "${APP}" ] || [ ! -d "${APP}" ]; then
  echo "!! Could not find the .app bundle (expected Trakr.app under ${BUILD_DIR})." >&2
  exit 1
fi
echo "==> Found bundle: ${APP}"

# 4. Package a zip.
STAMP="$(date -u +%Y%m%d-%H%M%S)"
ZIP_BASE="${TAG:-trakr-desktop-${STAMP}}"
ZIP_PATH="${ROOT_DIR}/${ZIP_BASE}-macos.zip"
echo "==> Packing ${ZIP_PATH}"
( cd "${APP}/.." && ditto -c -k --sequesterRsrc --keepParent "$(basename "${APP}")" "${ZIP_PATH}" )

echo "==> macOS build ready: ${ZIP_PATH}"

# 5. Optional release + upload.
if [ -n "${TAG}" ]; then
  echo "==> Publishing ${TAG} (gh release create + upload)"
  gh release create "${TAG}" "${ZIP_PATH}" --notes "Trakr desktop macOS build ${TAG}" 2>/dev/null \
    || gh release upload "${TAG}" "${ZIP_PATH}" --clobber
  echo "==> Published ${TAG}: $(gh release view "${TAG}" --json url -q .url)"
fi

echo "Done. Run: open ${ZIP_PATH}  # (or install the .app inside)"
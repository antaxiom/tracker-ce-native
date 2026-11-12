# Trakr Packaging & Deployment Guide

This document describes how to package, code-sign, and notarize Trakr across platforms.

---

## 1. macOS Application Bundle (.app)

### Building the Bundle
To configure CMake to output a standalone `Trakr.app` bundle:
```bash
cmake -B native/build -S native -DCMAKE_BUILD_TYPE=Release -DTRAKR_BUILD_MACOS_BUNDLE=ON
cmake --build native/build --config Release
```
This generates `native/build/Trakr.app` with `Info.plist` identifying `com.trakr.trakr`.

### Code Signing
To sign the bundle with a Developer ID Application certificate:
```bash
codesign --force --deep --options runtime \
  --sign "Developer ID Application: Your Name (TEAM_ID)" \
  --entitlements native/packaging/entitlements.plist \
  native/build/Trakr.app
```

### Notarization
Submit the signed zip or dmg to Apple's notarization service:
```bash
ditto -c -k --keepParent native/build/Trakr.app Trakr.zip
xcrun notarytool submit Trakr.zip \
  --keychain-profile "notary-profile" \
  --wait
xcrun stapler staple native/build/Trakr.app
```

---

## 2. Linux AppImage / Tarball

### Requirements
- GCC 12+ or Clang 15+
- SDL3 (`libsdl3-dev` or vendored)
- ALSA / PulseAudio development headers

### Build
```bash
cmake -B native/build -S native -DCMAKE_BUILD_TYPE=Release
cmake --build native/build -j
```

---

## 3. Windows Executable

### Requirements
- MSVC v143+ (Visual Studio 2022)
- Windows SDK 10.0.19041+
- Direct3D 12 / Vulkan support

### Build
```cmd
cmake -B native/build -S native -DCMAKE_BUILD_TYPE=Release
cmake --build native/build --config Release
```


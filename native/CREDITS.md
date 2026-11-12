# Credits, Licenses & Project Provenance

This document provides formal credit, attribution, and licensing details for all open-source projects, algorithms, and specifications referenced in **Trakr Native** (`native/`).

---

## 1. Upstream Hardware & Firmware Architecture

### Polyend Tracker Community Edition (CE)
* **Repository**: [`https://github.com/polyend/tracker-ce`](https://github.com/polyend/tracker-ce)
* **Copyright**: © Polyend sp. z o.o. & Tracker CE Community Contributors
* **License**: Polyend Source Available License (MIT License with Part 2 Commercial Terms)
* **Referenced Specifications**:
  * Step effect numbering and dispatch semantics (`core/AudioEngine/mtAudioSequencer.cpp`).
  * `CHIP` polyphonic chiptune synthesizer engine (`core/synth/engines/ChiptuneSynthVoice.cpp`, MIT License).
  * Sample play memory modes: Single Shot, Forward Loop, Backward Loop, Ping-Pong Loop, Slice, Beat Slice, Wavetable, and Granular (`core/libs/Audio/`).
  * State variable filter (Chamberlin SVF), bitcrusher, overdrive, and envelope curves (`core/libs/Audio/`).
  * Hardware layout, soft keys paradigm (`F1`–`F8`), and color palette tokens.

### Polyend Tracker Reverse Engineering (`polyend-tracker-re` / OpenTracker)
* **Repository**: [`https://github.com/sandlbn/polyend-tracker-re`](https://github.com/sandlbn/polyend-tracker-re)
* **Author**: `sandlbn` and OpenTracker Contributors
* **License**: **Apache License, Version 2.0**
  * *Licensed under the Apache License, Version 2.0 (the "License"); you may not use this work except in compliance with the License. You may obtain a copy of the License at [http://www.apache.org/licenses/LICENSE-2.0](http://www.apache.org/licenses/LICENSE-2.0).*
* **Referenced Specifications**:
  * Clean-room reverse engineering documentation for Polyend Tracker hardware behavior.
  * Test song structures and pattern queuing boundary rules incorporated into `native/tests/test_polyend_re_songs.cpp`.
  * Micro-step roll subdivision timing ($2, 3, 4, 6, 8, 12, 16$) and loop wrapping bounds.

### Polyend Tracker Library (`polyend/tracker-lib`)
* **Repository**: [`https://github.com/polyend/tracker-lib`](https://github.com/polyend/tracker-lib)
* **Copyright**: © Polyend sp. z o.o.
* **License**: **MIT License**
* **Referenced Specifications**:
  * Binary file format layouts for Polyend SD projects: `project.mt` (version 17), `pattern_*.mtp` (version 6), and `instrument_*.pti` (version 9).
  * Used to guide clean-room export and import serialization in `native/io/PolyendExport.cpp` and `native/io/PolyendImport.cpp`.

---

## 2. Core Libraries & Third-Party Dependencies

### Dear ImGui
* **Author**: Omar Cornut & Dear ImGui Contributors
* **License**: **MIT License**
* **Usage**: Flat hardware UI rendering, docking, tables, and widget primitives.

### SDL3 & SDL_GPU
* **Author**: Sam Lantinga & Simple DirectMedia Layer Contributors
* **License**: **zlib License**
* **Usage**: Cross-platform desktop windowing, event polling, and GPU rendering backends (Metal on macOS, Vulkan on Linux, Direct3D12 on Windows).

### miniaudio
* **Author**: David Reid
* **License**: **Public Domain (Unlicense) / MIT-0**
* **Usage**: Low-latency multi-channel audio output backend.

### dr_libs (`dr_wav.h`, `dr_mp3.h`, `dr_flac.h`)
* **Author**: David Reid
* **License**: **Public Domain (Unlicense) / MIT-0**
* **Usage**: Audio decoding and WAV encoding for rendering, stems, and sample loading.

### RtMidi
* **Author**: Gary P. Scavone & McGill University
* **License**: **MIT-style License**
* **Usage**: Real-time MIDI hardware input and output communication.

### Catch2
* **Author**: Catch2 Authors & Contributors
* **License**: **Boost Software License 1.0 (BSL-1.0)**
* **Usage**: Automated test suite, performance benchmarks, and regression testing.

---

## 3. Compliance and Clean-Room Architecture
All implementation in `native/` consists of standalone C++20 code authored specifically for the Trakr native environment. No proprietary binaries or closed-source libraries (such as `libPolySynth.a`) are linked or redistributed.


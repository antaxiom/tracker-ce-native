# TEST_READY — Test Readiness & Verification Sign-Off

## Status: READY
All test harnesses, verification runners, Catch2 test suites, and baseline parity criteria are operational, verified, and ready for the Visual Parity Refinement Track.

- **Date**: 2026-11-10
- **Track**: E2E Testing & Visual Parity Verification
- **Working Directory**: `/Users/alouk/Documents/tracker-ce-native`
- **Scope Reference**: `PROJECT.md`
- **Infrastructure Guide**: `TEST_INFRA.md`
- **Refinement Protocol**: `.agents/rules/visual-refinement-loop.md`

---

## 1. Readiness Verification Matrix

| Component | Target / Script | Status | Measured Metric |
|---|---|---|---|
| **Unit & Integration Suite** | `./native/build/trakr_tests` | **VERIFIED** | 101/101 tests passed (537,967 assertions, 0 failures) |
| **Autonomous UI Robot Audit** | `./native/build/trakr_app --auto-test` | **VERIFIED** | 1,103 frames, 992 checks (0 ID conflicts, 0 errors) |
| **FT812 Hardware Emulator** | `./native/build/trakr_tests "[emulator]"` | **VERIFIED** | 8/8 reference frames generated (`/tmp/emu_*.png`) |
| **OS Input Boundary Harness** | `native/tools/os_input/os_input_runner.py` | **VERIFIED** | macOS CoreGraphics event dispatch operational |
| **Multi-Metric Comparator** | `native/tools/os_input/os_input_runner.py` | **VERIFIED** | Total Pixel %, FG Match, SSIM, MAE computed |
| **Hotspot Telemetry Engine** | `native/tools/os_input/hotspot_analyzer.py` | **VERIFIED** | Semantic bands, bounding boxes, shift hints active |

---

## 2. Screen Baseline Scores & Target Criteria

All 8 screens meet and exceed the minimum acceptable project floor ($\ge 90.0\%$). Refinement iterations will systematically drive each screen to the production target ($\ge 95.0\%$).

| Priority | Screen Target | Baseline Pixel % | Baseline FG % | Baseline SSIM % | Floor ($\ge 90\%$) | Target ($\ge 95\%$) | Delta to Target |
|:---:|---|:---:|:---:|:---:|:---:|:---:|:---:|
| 1 | **PERFORM** | **91.40%** | 81.25% | 75.64% | PASS | PENDING | -3.60% |
| 2 | **MASTER** | **92.45%** | 74.39% | 77.84% | PASS | PENDING | -2.55% |
| 3 | **PATTERN** | **92.99%** | 82.45% | 56.02% | PASS | PENDING | -2.01% |
| 4 | **INSTRUMENT** | **93.40%** | 76.94% | 80.52% | PASS | PENDING | -1.60% |
| 5 | **LOADER** | **93.46%** | 92.19% | 76.24% | PASS | PENDING | -1.54% |
| 6 | **SONG** | **93.73%** | 57.77% | 74.24% | PASS | PENDING | -1.27% |
| 7 | **CONFIG** | **93.93%** | 82.61% | 73.58% | PASS | PENDING | -1.07% |
| 8 | **SAMPLE** | **94.14%** | 66.24% | 79.54% | PASS | PENDING | -0.86% |

---

## 3. Fast Verification Commands

### 3.1 Inner-Loop Refinement (Single Screen)
Use during iterative coordinate/color adjustments. Outputs semantic bands, bounding boxes, and shift offsets:
```zsh
# Note: macOS WindowServer requires BypassSandbox: true for Metal allocation
python3 native/tools/os_input/os_input_runner.py --only <screen> --no-emu --hotspots
```
Replace `<screen>` with: `perform`, `master`, `pattern`, `instrument`, `loader`, `song`, `config`, or `sample`.

### 3.2 Full Verification Suite
Run before declaring milestones complete or checking for global regressions:
```zsh
# 1. Catch2 Unit & Integration Suite
./native/build/trakr_tests

# 2. UI Autonomous Audit
./native/build/trakr_app --auto-test

# 3. Full 8-Screen Visual Parity Evaluation
python3 native/tools/os_input/os_input_runner.py
```

---

## 4. Operational Invariants & Protocols

1. **Monotonic Ratchet (Zero Regressions)**:
   - Before modifying any view, backup the file: `cp <File>.cpp /tmp/ratchet_<File>.bak`.
   - If pixel similarity drops after an edit, revert immediately: `cp /tmp/ratchet_<File>.bak <File>.cpp`.
2. **Escalation Policy**:
   - Fast inner-loop handles direct coordinate shifts and `IM_COL32` color replacements.
   - If 2 consecutive edits yield $< +0.15\%$ improvement on a screen, escalate with telemetry and diff image to diagnose deeper layout/font metrics.
3. **Strict Constraints**:
   - Edits are permitted **strictly** within `native/` (`core/`, `platforms/`, `synth/`, `util/` are read-only).
   - Every authored file in `native/` must remain strictly $< 400$ lines (`wc -l`).
   - Catch2 (101 tests) and `--auto-test` must pass at 100%.

---

## 5. Acceptance Sign-Off Checklist

- [x] Verification harness (`os_input_runner.py`) validated end-to-end on macOS.
- [x] Baseline measurements captured for all 8 screens.
- [x] Catch2 test suite (101 test cases) passes 100%.
- [x] Autonomous UI robot (`--auto-test`) passes 100%.
- [x] Documentation formalized in `TEST_INFRA.md` and `TEST_READY.md`.
- [x] Monotonic ratchet and escalation protocol active.

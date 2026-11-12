# ADR-0003: Fidelity definition and truth hierarchy

- Status: accepted
- Date: 2026-11-12

## Context

`fwref` is a source-derived approximation. Its gates prove provenance and
determinism, not FT812 equivalence. It has never been compared to hardware.
Pixel parity was treated as an architectural constraint and drove the UI into a
per-screen tuning loop.

## Decision

Fidelity means: structural fidelity, palette, typography, spacing and
interaction. Pixel identity is a regression metric for the `Classic` profile,
not the definition of correctness and not an architectural driver.

Truth hierarchy:

1. Firmware source (`core/interface/*/g*.cpp`, `core/libs/NewDisplay`,
   `core/source/Themes.cpp`).
2. Calibrated hardware captures (`native/docs/reference/videos`,
   `reference/screens/video_*.png`) rectified and per-channel calibrated.
3. `fwref` as an audited advisory oracle only.
4. Hand-drawn references: retired.

fwref status is "approximate, advisory only" until:

- strict mode passes on every used spec,
- primitive known-answer tests pass,
- multi-state specs exist for all screens,
- calibrated hardware structural checks pass.

## Consequences

- The app does not depend on fwref.
- Classic verification uses calibrated captures plus layout invariants.
- The ≥95% parity gate is no longer an architecture constraint; it remains an
  advisory metric.

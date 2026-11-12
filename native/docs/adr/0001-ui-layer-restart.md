# ADR-0001: Restart the UI layer, keep the application core

- Status: accepted
- Date: 2026-11-12
- Supersedes: the per-screen `views/*` rendering approach for migrated screens

## Context

The current GUI is a set of firmware-mirroring draw functions rendered into ImGui
draw lists. It achieved 95-98% parity by tuning geometry and fonts, but every
screen still re-rasterizes firmware output instead of replaying firmware draw
semantics. The residual gap is structural: fractional `avail/800` scaling,
ImDrawList stroke/AA rules, TTF/L4 glyph differences, and per-screen chrome.

The product direction is a Renoise-like studio. A fixed 800x480 framebuffer
renderer would cap that direction, and the source-derived fwref rasterizer has
never been validated against hardware.

## Decision

Restart the UI layer, not the application:

- Keep `model/`, `engine/`, `audio/`, `io/`, tests, demo song, fwref tooling.
- Replace `views/*` drawing code with a semantic control layer (`app/ui/`).
- Keep `InstrumentView` and the pattern helpers as edit/command controllers
  during migration; views become rendering-free.
- Delete each legacy view only after its replacement passes the screen gate.

## Consequences

- Short-term dual path for screens not yet migrated; gated per screen.
- No data-model or audio changes; history behavior stays as-is (instrument edits do not record history today; a command model is required before Phase 4).
- The restart is reversible per screen and the `pre-gui-restart` tag is the
  fallback baseline.

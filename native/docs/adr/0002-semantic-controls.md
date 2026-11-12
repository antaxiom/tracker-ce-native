# ADR-0002: Semantic controls, Canvas, and two layout profiles

- Status: accepted
- Date: 2026-11-12

## Context

Screens must preserve the original look while growing into a larger studio UI.
Pixel replay is not compatible with resizable panes, zoom, or more tracks.

## Decision

- Port firmware controls once as semantic controls (`Frame`, `Bar`, `List`,
  `BlankCover`, `BgLabel`, `Label`, ...): they own geometry in device pixels,
  value binding, painting and hit-testing.
- Paint through `Canvas`, which maps device pixels into a host rectangle with
  snapped edges and L4 bitmap text. Canvas is not a framebuffer.
- `Display` keeps controls in creation order, composites in reverse order, and
  hit-tests front-to-back, mirroring `cDisplay`.
- Screens mirror `g*.cpp` creation order and visibility.
- Layout is a profile:
  - `Classic`: fixed firmware rects at 800x480.
  - `Studio`: the same controls with host-sized constraints (later phase).
- All LCD geometry is integer device pixels. No `avail/800` scaling in screens.

## Consequences

- Parity becomes structural and testable per control, not tuned per screen.
- Studio expansion reuses the same controls and input semantics.
- Controls must be ported before their screens; no screen may keep two pixel
  paths once migrated.

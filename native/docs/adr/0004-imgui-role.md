# ADR-0004: ImGui is the host, not the LCD compositor

- Status: accepted
- Date: 2026-11-12

## Context

The app needs desktop behavior (windows, docks, menus, dialogs, tooltips,
keyboard/mouse routing) and an LCD that preserves the original design. ImGui is
excellent for the former and structurally wrong for the latter.

## Decision

- ImGui owns: SDL3 window/event loop, menu bar, docks, modal dialogs, desktop
  button deck, overlays, tooltips, dev panels.
- ImGui does not own LCD pixels, LCD layout, or LCD text.
- LCD content paints through `Canvas` into an ImGui draw list inside a
  decoration-free workspace window (`--gui-next` during migration).
- Editable text uses an anchored ImGui `InputText` overlay; firmware-style
  editing stays in the controllers.

## Consequences

- One windowing/input stack, no second toolkit.
- LCD pixels are independent of ImGui style and theme inside the workspace.
- The host window z-order must not cover LCD rows: the 28px header window has
  its minimum height clamped so it cannot paint over the workspace.
- The new path is opt-in until each screen passes its gate, then it becomes the
  default.

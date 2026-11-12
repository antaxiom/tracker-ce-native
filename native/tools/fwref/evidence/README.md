# Hardware evidence

The plan's low-frequency structural check uses user-supplied photos or video
frames of the Tracker LCD. No hardware captures were supplied with this task,
so this step is recorded as **not applicable** rather than fabricated.

When captures are available:

1. Rectify the LCD to an exact 800x480 frame (four corner homography).
2. Save as `hw_instrument.png` / `hw_master.png` in this directory.
3. Compare region masks against `render/fwref/*.png`:
   * bar presence and white/green classification per column,
   * filter/saturation row band positions,
   * bottom-bar separator positions and label rows.
4. Record pass/fail per region here. Do not use hardware as a pixel metric.

The source-true oracle gates (`check_reference.py`) remain the pixel metric.

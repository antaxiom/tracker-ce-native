#pragma once

// Animation primitives for the semantic UI.
//
// The firmware animates by timer polls inside its update passes (e.g. the
// 200 ms pattern-number blink in gPatternEdit.cpp:383 and the 500 ms playhead
// blink in fPatternEdit.cpp:278). To match that behaviour the host renderer
// must obey two rules:
//
//   1. Draw code samples animation state from the clock; it never accumulates
//      per-frame deltas. Rendering the same frame twice, or at any frame rate,
//      yields the same pixels.
//   2. Capture modes freeze the clock, so reference/screenshot output stays
//      deterministic.
//
// Controls that animate report it through Control::animatingAt(); AppLayout then
// keeps the 60 Hz redraw loop running (RenderScheduler::requestAnimation()).

#include <cmath>
#include <cstdint>

namespace trakr::ui {

// Firmware blink timings (milliseconds).
inline constexpr double kLabelBlinkMs = 200.0;    // gPatternEdit.cpp:383
inline constexpr double kPlayheadBlinkMs = 500.0; // fPatternEdit.cpp:278

// Process-wide animation clock in milliseconds. AppLayout sets it once per
// frame (or freezes it in --lcd-shot / --screenshot captures).
double animationNowMs();
void setAnimationNowMs(double nowMs);

// Increments whenever the clock advances; lets controls cache work per frame.
std::uint64_t animationFrame();

// Time-only helpers ---------------------------------------------------------

inline float lerp(float a, float b, float t) { return a + (b - a) * t; }

// Normalised 0..1 progress of a span that started at `startMs`.
inline float progress01(double nowMs, double startMs, double durationMs) {
    if (durationMs <= 0.0) return 1.0f;
    const double t = (nowMs - startMs) / durationMs;
    return static_cast<float>(t < 0.0 ? 0.0 : (t > 1.0 ? 1.0 : t));
}

enum class Easing { Linear, InQuad, OutQuad, InOutQuad, OutCubic };

inline float ease(Easing easing, float t) {
    t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
    switch (easing) {
        case Easing::Linear: return t;
        case Easing::InQuad: return t * t;
        case Easing::OutQuad: return 1.0f - (1.0f - t) * (1.0f - t);
        case Easing::InOutQuad: return (t < 0.5f) ? 2.0f * t * t
                                                  : 1.0f - 2.0f * (1.0f - t) * (1.0f - t);
        case Easing::OutCubic: {
            const float u = 1.0f - t;
            return 1.0f - u * u * u;
        }
    }
    return t;
}

// One-shot interpolation between two values. `retarget` starts a new span from
// the current value, so interrupted animations never jump.
struct Tween {
    float from{0.0f};
    float to{0.0f};
    double startMs{0.0};
    double durationMs{0.0};
    Easing easing{Easing::Linear};

    static Tween at(float value, double nowMs) {
        Tween t;
        t.from = t.to = value;
        t.startMs = nowMs;
        return t;
    }

    void retarget(float target, double nowMs, double duration, Easing e = Easing::OutCubic) {
        from = valueAt(nowMs);
        to = target;
        startMs = nowMs;
        durationMs = duration;
        easing = e;
    }

    float valueAt(double nowMs) const {
        return lerp(from, to, ease(easing, progress01(nowMs, startMs, durationMs)));
    }

    bool activeAt(double nowMs) const {
        return durationMs > 0.0 && nowMs < startMs + durationMs;
    }
};

// Square-wave blink with a duty cycle. `periodMs` is the full on+off cycle, so
// the firmware's "toggle every 500 ms" is a 1000 ms period (or use
// Blinker::toggling()).
struct Blinker {
    double periodMs{2.0 * kPlayheadBlinkMs};
    double duty{0.5};
    double originMs{0.0};

    // Firmware-style toggle: changes state every `toggleMs`.
    static Blinker toggling(double toggleMs, double nowMs = 0.0) {
        Blinker b;
        b.periodMs = 2.0 * toggleMs;
        b.originMs = nowMs;
        return b;
    }

    void restart(double nowMs) { originMs = nowMs; }

    bool stateAt(double nowMs) const {
        if (periodMs <= 0.0) return true;
        double phase = std::fmod(nowMs - originMs, periodMs);
        if (phase < 0.0) phase += periodMs;
        return phase < periodMs * duty;
    }
};

} // namespace trakr::ui

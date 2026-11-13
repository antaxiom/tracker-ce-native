#pragma once

#include <SDL3/SDL.h>
#include <cstdint>
#include <atomic>

namespace trakr {

class RenderScheduler {
public:
    RenderScheduler();

    // Call when user input occurs or view needs immediate redraw
    void requestRedraw(int burstFrames = 0);

    // Set playing state to govern refresh rate (up to 60Hz when playing, idle timeout when stopped)
    void setPlaying(bool playing);

    // Keep the 60 Hz loop running while semantic controls animate
    // (ui/animation.h). Cleared automatically when no control is animating.
    void requestAnimation(bool animating);

    // Call at start of event loop. Waits up to timeoutMs for next event.
    // Returns true if event was popped, false if timed out.
    bool waitEvent(SDL_Event* event);

    // Call after frame is rendered to reset frame timer
    void onFrameRendered();

    // Check if a redraw is needed right now
    bool shouldRedraw() const;

private:
    std::atomic<bool> redrawRequested_{true};
    std::atomic<int> burstFrames_{0};
    std::atomic<bool> playing_{false};
    std::atomic<bool> animating_{false};
    uint64_t lastRenderTimeMs_{0};
    uint32_t targetFrameIntervalMs_{16}; // ~60 Hz when active
};

} // namespace trakr


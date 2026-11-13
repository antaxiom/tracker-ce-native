#include "RenderScheduler.h"
#include <algorithm>

namespace trakr {

RenderScheduler::RenderScheduler()
    : lastRenderTimeMs_(SDL_GetTicks()) {}

void RenderScheduler::requestRedraw(int burstFrames) {
    redrawRequested_.store(true, std::memory_order_release);
    int current = burstFrames_.load(std::memory_order_relaxed);
    if (burstFrames > current) {
        burstFrames_.store(burstFrames, std::memory_order_relaxed);
    }
}

void RenderScheduler::setPlaying(bool playing) {
    playing_.store(playing, std::memory_order_release);
}

void RenderScheduler::requestAnimation(bool animating) {
    animating_.store(animating, std::memory_order_release);
    if (animating) requestRedraw(0);
}

bool RenderScheduler::waitEvent(SDL_Event* event) {
    const uint64_t now = SDL_GetTicks();
    const bool isPlaying = playing_.load(std::memory_order_relaxed);
    const int burst = burstFrames_.load(std::memory_order_relaxed);
    const bool animating = animating_.load(std::memory_order_relaxed);
    const bool needsRedraw =
        isPlaying || animating || burst > 0 || redrawRequested_.load(std::memory_order_relaxed);

    int timeoutMs = 16; // Up to 60 FPS responsiveness when idle
    if (needsRedraw) {
        const uint64_t elapsed = now - lastRenderTimeMs_;
        if (elapsed < targetFrameIntervalMs_) {
            timeoutMs = static_cast<int>(targetFrameIntervalMs_ - elapsed);
        } else {
            timeoutMs = 0; // Frame deadline reached
        }
    }

    if (timeoutMs > 0) {
        return SDL_WaitEventTimeout(event, timeoutMs);
    }
    return SDL_PollEvent(event);
}

void RenderScheduler::onFrameRendered() {
    lastRenderTimeMs_ = SDL_GetTicks();
    redrawRequested_.store(false, std::memory_order_release);
    int current = burstFrames_.load(std::memory_order_relaxed);
    if (current > 0) {
        burstFrames_.store(current - 1, std::memory_order_relaxed);
    }
}

bool RenderScheduler::shouldRedraw() const {
    if (burstFrames_.load(std::memory_order_relaxed) > 0) return true;
    if (redrawRequested_.load(std::memory_order_relaxed)) return true;
    if (playing_.load(std::memory_order_relaxed) || animating_.load(std::memory_order_relaxed)) {
        const uint64_t now = SDL_GetTicks();
        return (now - lastRenderTimeMs_) >= targetFrameIntervalMs_;
    }
    return false;
}

} // namespace trakr


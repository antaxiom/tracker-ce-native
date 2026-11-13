#include "animation.h"

namespace trakr::ui {
namespace {

double gNowMs = 0.0;
std::uint64_t gFrame = 0;

} // namespace

double animationNowMs() { return gNowMs; }

void setAnimationNowMs(double nowMs) {
    if (nowMs > gNowMs) ++gFrame;
    gNowMs = nowMs;
}

std::uint64_t animationFrame() { return gFrame; }

} // namespace trakr::ui

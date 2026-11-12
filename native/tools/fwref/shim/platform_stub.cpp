#include "Tracker.h"

namespace
{
Timer gTimer;
Platform gPlatform;
Tracker gTracker;
} // namespace

Timer* Platform::getTimer()
{
    return &gTimer;
}

Tracker* Tracker::getInstance()
{
    return &gTracker;
}

Platform* Tracker::getPlatform()
{
    return &gPlatform;
}

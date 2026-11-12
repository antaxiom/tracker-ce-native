#ifndef FWREF_TRACKER_H_
#define FWREF_TRACKER_H_

// Host shim for core/libs/NewDisplay/list.cpp:
//   Tracker::getInstance()->getPlatform()->getTimer()->millis()
//
// Text scrolling is disabled in the fwref scenarios (list->lineWidthMax == 0),
// so a fixed timestamp is sufficient. No layout or drawing logic is replaced.

#include <stdint.h>

class Timer
{
  public:
    uint32_t millis() const { return 0; }
};

class Platform
{
  public:
    Timer* getTimer();
};

class Tracker
{
  public:
    static Tracker* getInstance();
    Platform* getPlatform();
};

#endif // FWREF_TRACKER_H_

#ifndef MT_STRUCTS_H
#define MT_STRUCTS_H

// Host shim for the only mtStructs.h symbols reached transitively by the
// unmodified NewDisplay control sources: the real core/libs/NewDisplay/
// display.h includes sliceControl.h, which includes the real mtStructs.h.
// cSongPlayer uses kAllTracks for its block matrix (songPlayer.cpp:17, 792).
//
// These values are verbatim from core/source/mtStructs.h:87-90. No control,
// layout, style or colour logic lives here.

#include <stdint.h>

static constexpr int kSampleTracks = 8;
static constexpr int kMidiTracks = 8;
static constexpr int kAllTracks = kSampleTracks + kMidiTracks;

// sliceControl.h array bound (core/source/mtStructs.h:339).
static constexpr int MAX_SLICE_NUMBER = 48;
// spectrum.cpp y-normalization denominator (core/source/mtStructs.h:53).
static const int MAX_16BIT = 65535;

#endif // MT_STRUCTS_H

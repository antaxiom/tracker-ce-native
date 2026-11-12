#ifndef FWREF_SPEC_CONTROLS_H_
#define FWREF_SPEC_CONTROLS_H_

#include "display.h"
#include "spec.h"

namespace fwref
{

struct Scenario;

// Creates the firmware control described by `e` and returns its slot. The
// caller has already populated prop.x/y/w/h/style/colors. Returns hControl(-1)
// when the control is unknown. `ok` is cleared on a spec evaluation failure.
hControl createSpecControl(const SpecEntry& e, strControlProperties& prop,
                           const Scenario& scenario, bool& ok);

// cSongPlayer creation (songPlayer.cpp) lives in spec_controls_song.cpp to keep
// the shared control factory under the source-size budget.
hControl createSongPlayerControl(const SpecEntry& e, strControlProperties& prop,
                                 const Scenario& scenario, bool& ok);
void resetSongSpecArenas();

// Loader/Sample waveform control family (spec_controls_sample.cpp): cPoints,
// cSpectrum, cProgressCursor, cLineIndicator, cSlicePoints, cWtProgress.
hControl createSampleControl(const SpecEntry& e, strControlProperties& prop,
                             const Scenario& scenario, bool& ok);
void resetSampleSpecArenas();

// Frees the backing arenas. Only safe after display.destroyAll().
void resetSpecArenas();

// Stable storage for dynamically generated text owned by a control spec.
const std::string& storeSpecString(const std::string& text);

} // namespace fwref

#endif // FWREF_SPEC_CONTROLS_H_

#ifndef FWREF_RENDER_SPEC_H_
#define FWREF_RENDER_SPEC_H_

#include "fwref.h"
#include "spec.h"

namespace trakr::emulator
{
class FT812DisplayEmulator;
}

namespace fwref
{

// Creates the controls described by `spec` (merged with `chrome`) and renders
// them through cDisplay::renderAll(), which reproduces firmware draw order.
bool renderSpec(trakr::emulator::FT812DisplayEmulator& emu,
                const ScreenSpec& spec,
                const std::vector<SpecEntry>& chrome,
                const Scenario& scenario);

} // namespace fwref

#endif // FWREF_RENDER_SPEC_H_

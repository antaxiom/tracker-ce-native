#include "fwref.h"
#include "render_spec.h"
#include "scenario_util.h"
#include "spec.h"
#include "spec_paths.h"

#include <string>

namespace fwref
{

bool renderLoader(trakr::emulator::FT812DisplayEmulator& emu, const LoaderScenario& scenario)
{
    const std::string dir = specDirectory();
    ScreenSpec spec;
    std::string error;
    if (!loadScreenSpecJson(dir + "/loader.json", spec, error))
        return false;

    std::vector<SpecEntry> chrome;
    if (!loadChromeSpecJson(dir + "/chrome.json", chrome, error))
        return false;

    Scenario s;
    s.loader = scenario;
    ensureLoaderSlots(s.loader);
    return renderSpec(emu, spec, chrome, s);
}

} // namespace fwref

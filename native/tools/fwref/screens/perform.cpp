#include "fwref.h"
#include "render_spec.h"
#include "spec.h"
#include "spec_paths.h"

#include <string>

namespace fwref
{

bool renderPerform(trakr::emulator::FT812DisplayEmulator& emu, const PerformScenario& scenario)
{
    const std::string dir = specDirectory();
    ScreenSpec spec;
    std::string error;
    if (!loadScreenSpecJson(dir + "/perform.json", spec, error))
        return false;

    std::vector<SpecEntry> chrome;
    if (!loadChromeSpecJson(dir + "/chrome.json", chrome, error))
        return false;

    Scenario s;
    s.perform = scenario;
    return renderSpec(emu, spec, chrome, s);
}

} // namespace fwref

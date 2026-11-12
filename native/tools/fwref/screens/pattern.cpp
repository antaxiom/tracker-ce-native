#include "fwref.h"
#include "render_spec.h"
#include "spec.h"
#include "spec_paths.h"

#include <string>

namespace fwref
{

bool renderPattern(trakr::emulator::FT812DisplayEmulator& emu, const PatternScenario& scenario)
{
    const std::string dir = specDirectory();
    ScreenSpec spec;
    std::string error;
    if (!loadScreenSpecJson(dir + "/pattern.json", spec, error))
        return false;

    std::vector<SpecEntry> chrome;
    if (!loadChromeSpecJson(dir + "/chrome.json", chrome, error))
        return false;

    Scenario s;
    s.pattern = scenario;
    return renderSpec(emu, spec, chrome, s);
}

} // namespace fwref

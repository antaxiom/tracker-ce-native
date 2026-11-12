#include "fwref.h"
#include "render_spec.h"
#include "spec.h"
#include "spec_paths.h"

#include <string>

namespace fwref
{

bool renderMasterPage1(trakr::emulator::FT812DisplayEmulator& emu,
                       const MasterScenario& scenario)
{
    const std::string dir = specDirectory();
    ScreenSpec spec;
    std::string error;
    if (!loadScreenSpecJson(dir + "/master_page1.json", spec, error))
        return false;

    std::vector<SpecEntry> chrome;
    if (!loadChromeSpecJson(dir + "/chrome.json", chrome, error))
        return false;

    Scenario s;
    s.master = scenario;
    s.instrument.rightLabel = "";
    return renderSpec(emu, spec, chrome, s);
}

bool renderMasterEq(trakr::emulator::FT812DisplayEmulator& emu,
                    const MasterScenario& scenario)
{
    const std::string dir = specDirectory();
    ScreenSpec spec;
    std::string error;
    if (!loadScreenSpecJson(dir + "/master_eq.json", spec, error))
        return false;

    std::vector<SpecEntry> chrome;
    if (!loadChromeSpecJson(dir + "/chrome.json", chrome, error))
        return false;

    Scenario s;
    s.master = scenario;
    s.instrument.rightLabel = "";
    return renderSpec(emu, spec, chrome, s);
}

} // namespace fwref

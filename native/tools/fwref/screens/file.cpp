#include "fwref.h"
#include "render_spec.h"
#include "scenario_util.h"
#include "spec.h"
#include "spec_paths.h"

#include <string>

namespace fwref
{

bool renderFile(trakr::emulator::FT812DisplayEmulator& emu, const FileScenario& scenario)
{
    const std::string dir = specDirectory();
    ScreenSpec spec;
    std::string error;
    if (!loadScreenSpecJson(dir + "/file.json", spec, error))
        return false;

    std::vector<SpecEntry> chrome;
    if (!loadChromeSpecJson(dir + "/chrome.json", chrome, error))
        return false;

    Scenario s;
    s.file = scenario;
    return renderSpec(emu, spec, chrome, s);
}

} // namespace fwref

#include "fwref.h"
#include "render_spec.h"
#include "scenario_util.h"
#include "spec.h"
#include "spec_paths.h"

#include <string>

namespace fwref
{

bool renderSampleState(trakr::emulator::FT812DisplayEmulator& emu, const SampleScenario& scenario,
                       const std::string& specName)
{
    const std::string dir = specDirectory();
    ScreenSpec spec;
    std::string error;
    if (!loadScreenSpecJson(dir + "/" + specName, spec, error))
        return false;

    std::vector<SpecEntry> chrome;
    if (!loadChromeSpecJson(dir + "/chrome.json", chrome, error))
        return false;

    Scenario s;
    s.sample = scenario;
    if (s.sample.waveform.empty())
        buildSampleWaveform(s.sample);
    // The spec only addresses sample.* keys, but chrome right-label resolution
    // reads the same scenario.
    return renderSpec(emu, spec, chrome, s);
}

bool renderSample(trakr::emulator::FT812DisplayEmulator& emu, const SampleScenario& scenario)
{
    return renderSampleState(emu, scenario, "sample.json");
}

} // namespace fwref

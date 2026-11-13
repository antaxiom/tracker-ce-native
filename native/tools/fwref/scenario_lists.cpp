#include "scenario_util.h"

#include <string>
#include <utility>
#include <vector>

namespace fwref
{

namespace
{

float mapf(float v, float inMin, float inMax, float outMin, float outMax)
{
    return outMin + (v - inMin) * (outMax - outMin) / (inMax - inMin);
}

int auxVolumeAt(const Scenario& scenario, int idx)
{
    switch (idx)
    {
        case 0: return scenario.master.delayVolume;
        case 1: return scenario.master.reverbVolume;
        case 2: return scenario.master.dryMixVolume;
        default: return scenario.master.lineInVolume;
    }
}

int auxLevelAt(const Scenario& scenario, int idx)
{
    switch (idx)
    {
        case 0: return scenario.master.delayLevel;
        case 1: return scenario.master.reverbLevel;
        case 2: return scenario.master.dryMixLevel;
        default: return scenario.master.lineInLevel;
    }
}

} // namespace

bool resolveStringList(const std::string& key, const Scenario& scenario,
                       std::vector<std::string>& out)
{
    out.clear();
    if (key == "config.baseMenu")
    {
        out = scenario.config.baseMenu;
        return true;
    }
    if (key == "config.editItems")
    {
        out = scenario.config.editItems;
        return true;
    }
    return resolveLoaderSampleList(key, scenario, out);
}

bool resolveRows(const std::string& key, const Scenario& scenario,
                 std::vector<std::pair<std::string, std::string>>& out)
{
    out.clear();
    if (key == "config.items")
    {
        out = scenario.config.items;
        return true;
    }
    return false;
}

int barValueFor(const std::string& id, int arg, const Scenario& scenario)
{
    if (id == "track_volume")
        return (arg >= 0 && arg < 8) ? scenario.master.trackVolume[arg] : 0;
    if (id == "track_level")
        return (arg >= 0 && arg < 8) ? scenario.master.trackLevel[arg] : 0;
    if (id == "aux_volume")
        return auxVolumeAt(scenario, arg);
    if (id == "aux_level")
        return auxLevelAt(scenario, arg);
    if (id == "eq_band")
        return (arg >= 0 && arg < 5) ? scenario.master.eqBand[arg] : 0;
    if (id == "volume")
        return scenario.instrument.volume;
    if (id == "pan_leftright")
        return scenario.instrument.panning * 2 - 100; // gInstrEdit.cpp:1587
    if (id == "tune_leftright")
        return (scenario.instrument.tune * 100) / 24; // gInstrEdit.cpp:1598
    if (id == "fine_leftright")
        return scenario.instrument.fineTune; // gInstrEdit.cpp:1608
    if (id == "cutoff")
        return static_cast<int>(scenario.instrument.cutOff * 100.0f); // gInstrEdit.cpp:1665
    if (id == "resonance")
    {
        constexpr float kResMin = 0.0f;
        constexpr float kResMax = 4.3f;
        return static_cast<int>(((scenario.instrument.resonance - kResMin) /
                                 (kResMax - kResMin)) *
                                100.0f); // gInstrEdit.cpp:1690
    }
    if (id == "master_volume")
        return scenario.master.volume; // gMasterParams.cpp:1182
    if (id == "bass_boost")
        return scenario.master.bassBoost; // gMasterParams.cpp:1202
    if (id == "stereo_boost_bar")
        return static_cast<int>(mapf(static_cast<float>(scenario.master.stereoBoost), 0.0f, 8.0f,
                                     0.0f, 100.0f)); // gMasterParams.cpp:1211
    if (id == "song_tempo")
        return static_cast<int>((scenario.song.globalTempo * 100.0f) /
                                800.0f); // gSongEdit.cpp:407-408 (Sequencer::MAX_TEMPO)
    if (id == "loader_memory")
        return scenario.loader.memoryUsage; // gSampleImport.cpp:283
    if (id == "loader_preview_volume")
        return scenario.loader.previewVolume; // gSampleImport.cpp:266
    return 0;
}

} // namespace fwref

#include "scenario_util.h"
#include "spec.h"
#include "spec_paths.h"

#include "ampLogarythmicValues.h"

#include <nlohmann/json.hpp>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>

using nlohmann::json;

namespace fwref
{
namespace
{

// firmware map() linear interpolation (util_math.h)
float mapf(float v, float inMin, float inMax, float outMin, float outMax)
{
    return outMin + (v - inMin) * (outMax - outMin) / (inMax - inMin);
}

const char* filterLabel(int listPos)
{
    // gInstrEdit.cpp:48-53 filterModeFunctLabels
    static const char* const kLabels[4] = { "Disabled", "Low-pass", "High-pass", "Band-pass" };
    if (listPos < 0 || listPos > 3)
        listPos = 0;
    return kLabels[listPos];
}

bool parseSuffix(const std::string& key, const std::string& prefix, int& idx)
{
    if (key.rfind(prefix, 0) != 0)
        return false;
    idx = std::atoi(key.c_str() + prefix.size());
    return true;
}

std::string levelDbString(int value)
{
    // gMasterParams.cpp:1638-1641 calcLevelDecybelValue()
    if (value < 0)
        value = 0;
    if (value >= static_cast<int>(AMP_LOG_VALUES))
        value = AMP_LOG_VALUES - 1;
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%0.2f dB", 20.0f * std::log10(mixAmpLogValues[value]));
    return buf;
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

bool auxMuteAt(const Scenario& scenario, int idx)
{
    switch (idx)
    {
        case 0: return scenario.master.delayMute;
        case 1: return scenario.master.reverbMute;
        case 2: return scenario.master.dryMixMute;
        default: return scenario.master.lineInMute;
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

Scenario defaultComparisonScenario()
{
    Scenario s;
    // Single source of truth: the checked-in scenario.json.
    std::string error;
    if (loadScenarioJson(scenarioFile(), s, error))
        return s;

    s.instrument.name = "909 Kick";
    s.instrument.rightLabel = "1. 909 Kick";
    s.instrument.volume = 50;
    s.instrument.panning = 50;
    s.instrument.tune = 0;
    s.instrument.fineTune = 0;
    s.instrument.filterEnable = 1;
    s.instrument.filterType = "lowPass";
    s.instrument.cutOff = 1.0f;
    s.instrument.resonance = 0.0f;
    s.instrument.selectedPlace = 0;
    s.master.volume = 81;
    s.master.saturationType = 0;
    s.master.bassBoost = 0;
    s.master.stereoBoost = 0;
    s.master.selectedPlace = 0;
    return s;
}

int filterListPosOf(const Scenario& scenario)
{
    // gInstrEdit.cpp:1612-1626 showFilterType()
    if (!scenario.instrument.filterEnable)
        return 0;
    if (scenario.instrument.filterType == "lowPass")
        return 1;
    if (scenario.instrument.filterType == "highPass")
        return 2;
    return 3;
}

int saturationTypeOf(const Scenario& scenario)
{
    return scenario.master.saturationType;
}

int resolveInt(const std::string& exprOrKey, const Scenario& scenario, bool& ok)
{
    ok = true;
    if (exprOrKey.empty())
        return 0;
    if (exprOrKey[0] == '@')
    {
        const std::string key = exprOrKey.substr(1);
        if (key == "instrument.volume")
            return scenario.instrument.volume;
        if (key == "instrument.selectedPlace")
            return scenario.instrument.selectedPlace;
        if (key == "master.volume")
            return scenario.master.volume;
        if (key == "master.saturationType")
            return scenario.master.saturationType;
        if (key == "master.selectedPlace")
            return scenario.master.selectedPlace;
        if (key == "instrument.filterListPos")
            return filterListPosOf(scenario);
        if (key == "pattern.selectedPlace")
            return scenario.pattern.selectedPlace;
        if (key == "pattern.viewMode")
            return scenario.pattern.viewMode;
        if (key == "config.selectedBase")
            return scenario.config.selectedBase;
        if (key == "config.selectedItem")
            return scenario.config.selectedItem;
        if (key == "config.selectedPlace")
            return scenario.config.selectedPlace;
        if (key == "config.gridSelected")
            return scenario.config.gridSelected;
        if (key == "song.selectedSlot")
            return scenario.song.selectedSlot;
        if (key == "song.length")
            return static_cast<int>(scenario.song.playlist.size());
        if (key.rfind("perform.selectedRow.", 0) == 0)
        {
            int idx = std::atoi(key.c_str() + 20);
            if (idx >= 0 && idx < static_cast<int>(scenario.perform.selectedRows.size()))
                return scenario.perform.selectedRows[idx];
        }
        int loaderSample = 0;
        if (resolveLoaderSampleInt(key, scenario, loaderSample))
            return loaderSample;
        ok = false;
        return 0;
    }
    int v = 0;
    ok = evaluateExpression(exprOrKey, v);
    return v;
}

std::string resolveText(const std::string& literalOrKey, const Scenario& scenario)
{
    if (literalOrKey.empty())
        return "";
    if (literalOrKey[0] != '@')
        return literalOrKey;
    const std::string key = literalOrKey.substr(1);
    if (key == "instrument.rightLabel")
        return scenario.instrument.rightLabel;
    if (key == "instrument.name")
        return scenario.instrument.name;
    if (key == "pattern.rightLabel")
        return scenario.pattern.rightLabel;
    if (key == "pattern.patternNumber")
        return std::to_string(scenario.pattern.actualPattern);
    if (key == "pattern.length")
        return std::to_string(scenario.pattern.patternLength);
    if (key == "pattern.editStep")
        return std::to_string(scenario.pattern.editStep);

    int idx = 0;
    if (parseSuffix(key, "master.trackName.", idx) && idx >= 0 && idx < 8)
        return scenario.master.trackNames[idx];
    if (parseSuffix(key, "master.trackDb.", idx) && idx >= 0 && idx < 8)
        return levelDbString(scenario.master.trackVolume[idx]);
    if (parseSuffix(key, "master.mixMute.", idx) && idx >= 0 && idx < 8)
        return scenario.master.trackMute[idx] ? "Unmute" : "Mute";

    static const char* const kAuxNames[4] = { "Delay", "Reverb", "Dry Mix", "Line In" };
    if (parseSuffix(key, "master.auxName.", idx) && idx >= 0 && idx < 4)
        return kAuxNames[idx];
    if (parseSuffix(key, "master.auxDb.", idx) && idx >= 0 && idx < 4)
        return levelDbString(auxVolumeAt(scenario, idx));
    if (parseSuffix(key, "master.auxMute.", idx) && idx >= 0 && idx < 4)
        return auxMuteAt(scenario, idx) ? "Unmute" : "Mute";

    if (key == "config.credits")
        return scenario.config.credits;
    if (key == "config.description")
        return scenario.config.description;

    if (parseSuffix(key, "perform.fxName.", idx) && idx >= 0 && idx < 12)
        return scenario.perform.fxNames[idx];
    if (parseSuffix(key, "perform.trackName.", idx) && idx >= 0 && idx < 8)
        return scenario.perform.trackNames[idx];
    if (parseSuffix(key, "perform.trackPattern.", idx) && idx >= 0 && idx < 8)
        return scenario.perform.trackPatterns[idx];
    if (parseSuffix(key, "perform.trackLabel.", idx) && idx >= 0 && idx < 8)
        return scenario.perform.activeTracks[idx] ? scenario.perform.trackNames[idx] : "";
    if (parseSuffix(key, "perform.trackPatternLabel.", idx) && idx >= 0 && idx < 8)
        return scenario.perform.activeTracks[idx] ? scenario.perform.trackPatterns[idx] : "";
    if (parseSuffix(key, "perform.trackMute.", idx) && idx >= 0 && idx < 8)
        return scenario.perform.trackMuted[idx] ? "Unmute" : "Mute";
    if (key.rfind("perform.fxValue.", 0) == 0)
    {
        int slot = 0, row = 0;
        if (std::sscanf(key.c_str() + 16, "%d.%d", &slot, &row) == 2 && slot >= 0 && slot < 12 &&
            row >= 0 && row < 5)
            return scenario.perform.fxValues[slot][row];
    }
    return resolveLoaderSampleText(key, scenario);
}

std::string formatLabelValue(const std::string& formatId, const Scenario& scenario)
{
    char buf[64];

    if (formatId == "volume_db")
    {
        // gInstrEdit.cpp:1567-1578 -> fIinstrEdit.cpp:3472 calcInstrumentVolumeDecybelValue
        float db = 20.0f * std::log10(ampLogValues[scenario.instrument.volume]);
        std::snprintf(buf, sizeof(buf), "%0.02f dB", db);
        return buf;
    }
    if (formatId == "panning")
    {
        // gInstrEdit.cpp:1580-1589
        std::snprintf(buf, sizeof(buf), "%d", scenario.instrument.panning - 50);
        return buf;
    }
    if (formatId == "tune")
    {
        // gInstrEdit.cpp:1591-1600
        std::snprintf(buf, sizeof(buf), "%d", scenario.instrument.tune);
        return buf;
    }
    if (formatId == "finetune")
    {
        // gInstrEdit.cpp:1602-1610
        std::snprintf(buf, sizeof(buf), "%d", scenario.instrument.fineTune);
        return buf;
    }
    if (formatId == "filter")
    {
        // gInstrEdit.cpp:1633
        return filterLabel(filterListPosOf(scenario));
    }
    if (formatId == "cutoff")
    {
        // gInstrEdit.cpp:1645-1667: uint8_t temp_cutoff = cutOff * 100
        int cutoff = static_cast<int>(scenario.instrument.cutOff * 100.0f);
        std::snprintf(buf, sizeof(buf), "%d", cutoff);
        return buf;
    }
    if (formatId == "resonance")
    {
        // gInstrEdit.cpp:1669-1692: (resonance - MIN) / (MAX - MIN) * 100
        constexpr float kResMin = 0.0f; // mtStructs.h:186
        constexpr float kResMax = 4.3f; // mtStructs.h:187
        int res = static_cast<int>(((scenario.instrument.resonance - kResMin) /
                                    (kResMax - kResMin)) *
                                   100.0f);
        std::snprintf(buf, sizeof(buf), "%d", res);
        return buf;
    }
    if (formatId == "master_db")
    {
        // gMasterParams.cpp:1157-1187 showVolume()
        constexpr int kMasterVolumeDefault = 81; // mtStructs.h:254
        constexpr float kMinDb = -51.5f;
        constexpr float kMaxDb = 12.0f;
        float db = 0.0f;
        if (scenario.master.volume == kMasterVolumeDefault)
        {
            db = 0.0f;
        }
        else
        {
            db = mapf(scenario.master.volume / 100.0f, 0.0f, 1.0f, kMinDb, kMaxDb);
        }
        std::snprintf(buf, sizeof(buf), "%0.2f dB", db);
        return buf;
    }
    if (formatId == "saturation")
    {
        // gMasterParams.cpp:21-27 saturationListTexts / 1189-1196
        static const char* const kTexts[5] = { "Hard Clip", "Soft Clip", "Saturate", "Overdrive",
                                               "Drive+" };
        int idx = scenario.master.saturationType;
        if (idx < 0 || idx > 4)
            idx = 0;
        return kTexts[idx];
    }
    if (formatId == "bass_boost")
    {
        // gMasterParams.cpp:1198-1207
        std::snprintf(buf, sizeof(buf), "%d", scenario.master.bassBoost);
        return buf;
    }
    if (formatId == "stereo_boost")
    {
        // gMasterParams.cpp:1209-1220
        std::snprintf(buf, sizeof(buf), "%d", scenario.master.stereoBoost);
        return buf;
    }
    if (formatId == "song_timer")
    {
        // songTimer.cpp:113-135: "%3im:%02is %2s P%i.%03i%2s" with the
        // stopped-state sequence glyphs 0x18/0x19 from fonts[3].
        // numberingMode defaults to kDecimalWithStepsFrom1 (mtStructs.h:1011).
        const int stepNum = scenario.pattern.actualStep + 1;
        std::snprintf(buf, sizeof(buf), "%3im:%02is %2s P%i.%03i%2s",
                      scenario.pattern.timerMinutes, scenario.pattern.timerSeconds, "\x18\x19",
                      scenario.pattern.actualPattern, stepNum, "");
        return buf;
    }
    if (formatId == "int")
    {
        return "0";
    }
    if (formatId == "song_tempo_text")
    {
        // gSongEdit.cpp:387 showTempoValue(): "%.1f BPM"
        std::snprintf(buf, sizeof(buf), "%.1f BPM", scenario.song.globalTempo);
        return buf;
    }
    return formatLoaderSampleValue(formatId, scenario);
}

} // namespace fwref

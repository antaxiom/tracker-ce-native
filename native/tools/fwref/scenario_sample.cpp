#include "scenario_util.h"

#include "ampLogarythmicValues.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace fwref
{
namespace
{

// gSamplePlay.cpp:806-819 formatTimeSeconds(): "%.3f" + "s" with no separator.
std::string timeSeconds(float value)
{
    char valueBuffer[16];
    std::snprintf(valueBuffer, sizeof(valueBuffer), "%.3f", value);
    return std::string(valueBuffer) + "s";
}

std::string twoDecimals(float value)
{
    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), "%.2f", value);
    return buffer;
}

} // namespace

bool resolveLoaderSampleInt(const std::string& key, const Scenario& scenario, int& out)
{
    if (key == "file.selectedPlace")
    {
        out = scenario.file.selectedPlace;
        return true;
    }
    if (key == "file.selectedProject")
    {
        out = scenario.file.selectedProject;
        return true;
    }
    if (key == "loader.selectedFile")
    {
        out = scenario.loader.selectedFile;
        return true;
    }
    if (key == "loader.selectedSlot")
    {
        out = scenario.loader.selectedSlot;
        return true;
    }
    if (key == "loader.selectedPlace")
    {
        out = scenario.loader.selectedPlace;
        return true;
    }
    if (key == "loader.memoryUsage")
    {
        out = scenario.loader.memoryUsage;
        return true;
    }
    if (key == "loader.memoryUsageAdd")
    {
        out = scenario.loader.memoryUsageAdd;
        return true;
    }
    if (key == "loader.previewVolume")
    {
        out = scenario.loader.previewVolume;
        return true;
    }

    if (key == "sample.playMode")
    {
        out = scenario.sample.playMode;
        return true;
    }
    if (key == "sample.selectedPlace")
    {
        out = scenario.sample.selectedPlace;
        return true;
    }
    if (key == "sample.progress")
    {
        out = scenario.sample.progress;
        return true;
    }
    if (key == "sample.startPoint")
    {
        out = scenario.sample.startPoint;
        return true;
    }
    if (key == "sample.endPoint")
    {
        out = scenario.sample.endPoint;
        return true;
    }
    if (key == "sample.loop1")
    {
        out = scenario.sample.loop1;
        return true;
    }
    if (key == "sample.loop2")
    {
        out = scenario.sample.loop2;
        return true;
    }
    if (key == "sample.selected")
    {
        out = scenario.sample.selected;
        return true;
    }
    if (key == "sample.zerocrossed")
    {
        out = scenario.sample.zerocrossed;
        return true;
    }
    if (key == "sample.regionStart")
    {
        out = scenario.sample.regionStart;
        return true;
    }
    if (key == "sample.regionEnd")
    {
        out = scenario.sample.regionEnd;
        return true;
    }
    if (key == "sample.sliceSelected")
    {
        out = scenario.sample.sliceSelected;
        return true;
    }
    if (key == "sample.wtPosition")
    {
        out = scenario.sample.wtPosition;
        return true;
    }
    if (key == "sample.wtWindow")
    {
        out = scenario.sample.wtWindow;
        return true;
    }
    if (key == "sample.wtPositionMax")
    {
        out = scenario.sample.wtPositionMax;
        return true;
    }
    if (key == "sample.wavePoints")
    {
        out = scenario.sample.wavePoints;
        return true;
    }
    return false;
}

std::string resolveLoaderSampleText(const std::string& key, const Scenario& scenario)
{
    if (key == "file.projectName")
    {
        return scenario.file.projectName;
    }
    if (key == "loader.previewDb")
    {
        return formatLoaderSampleValue("loader_preview_db", scenario);
    }
    if (key == "sample.rightLabel")
    {
        return scenario.sample.rightLabel;
    }
    return "";
}

std::string formatLoaderSampleValue(const std::string& formatId, const Scenario& scenario)
{
    if (formatId == "loader_preview_db")
    {
        // gSampleImport.cpp:264-276 showPreviewVolume():
        // sprintf(previewVolumeText, "%0.2f dB", 20 * log10f(mixAmpLogValues[volume]))
        int volume = std::clamp(scenario.loader.previewVolume, 0,
                                static_cast<int>(AMP_LOG_VALUES) - 1);
        float db = (volume == 0) ? -96.0f : 20.0f * std::log10(mixAmpLogValues[volume]);
        char buffer[24];
        std::snprintf(buffer, sizeof(buffer), "%0.2f dB", db);
        return buffer;
    }
    if (formatId == "loader_memory")
    {
        // gSampleImport.cpp:288 sprintf(memoryUsageValue, "%d%c", memoryUsage, '%')
        char buffer[16];
        std::snprintf(buffer, sizeof(buffer), "%d%%", scenario.loader.memoryUsage);
        return buffer;
    }
    if (formatId == "sample_start_time")
        return timeSeconds(scenario.sample.startTime);
    if (formatId == "sample_end_time")
        return timeSeconds(scenario.sample.endTime);
    if (formatId == "sample_loop1_time")
        return timeSeconds(scenario.sample.loop1Time);
    if (formatId == "sample_loop2_time")
        return timeSeconds(scenario.sample.loop2Time);
    if (formatId == "sample_zoom")
        return twoDecimals(scenario.sample.zoom); // gSamplePlay.cpp:432-447 showZoomValue()
    if (formatId == "sample_preview")
        return "-"; // hidePreviewValue(): display.setControlText2(label[0], "-")
    return "";
}

bool resolveLoaderSampleList(const std::string& key, const Scenario& scenario,
                             std::vector<std::string>& out)
{
    out.clear();
    if (key == "file.projects")
    {
        out = scenario.file.projects;
        return true;
    }
    if (key == "file.recent")
    {
        out = scenario.file.recent;
        return true;
    }
    if (key == "loader.files")
    {
        out = scenario.loader.files;
        return true;
    }
    if (key == "loader.slots")
    {
        out = scenario.loader.slots;
        return true;
    }
    if (key == "sample.playModes")
    {
        out = scenario.sample.playModes;
        return true;
    }
    return false;
}

bool resolveLoaderSampleIntList(const std::string& key, const Scenario& scenario,
                                std::vector<int>& out)
{
    out.clear();
    if (key == "sample.slices")
    {
        out = scenario.sample.slices;
        return true;
    }
    if (key == "sample.waveform")
    {
        out = scenario.sample.waveform;
        return true;
    }
    return false;
}

void buildSampleWaveform(SampleScenario& sample)
{
    // Deterministic representative waveform so the Sample reference is never
    // empty. Upper/lower data are int8 peaks (graphicProcessing.cpp:478).
    const int points = std::clamp(sample.wavePoints, 1, 600);
    sample.wavePoints = points;
    sample.waveform.resize(static_cast<size_t>(points));
    const double amplitude = static_cast<double>(std::clamp(sample.waveAmplitude, 0, 127));
    for (int i = 0; i < points; ++i)
    {
        const double envelope = std::exp(-static_cast<double>(sample.waveDecay) * i);
        const double wave = std::sin(static_cast<double>(sample.waveFrequency) * i);
        int amp = static_cast<int>(std::lround(amplitude * std::fabs(wave) * envelope));
        if (amp > 127)
            amp = 127;
        if (amp < 0)
            amp = 0;
        sample.waveform[static_cast<size_t>(i)] = amp;
    }
}

void ensureLoaderSlots(LoaderScenario& loader)
{
    // listInstrumentSlots(): "%d." then the sample file name (empty slot -> "N.")
    const int count = std::clamp(loader.slotCount, 1, 48);
    loader.slotCount = count;
    if (static_cast<int>(loader.slots.size()) == count)
        return;
    std::vector<std::string> slots = loader.slots;
    loader.slots.clear();
    loader.slots.reserve(static_cast<size_t>(count));
    for (int i = 0; i < count; ++i)
    {
        if (i < static_cast<int>(slots.size()))
            loader.slots.push_back(slots[static_cast<size_t>(i)]);
        else
            loader.slots.push_back(std::to_string(i + 1) + ".");
    }
}

} // namespace fwref

#include "scenario_util.h"

#include <nlohmann/json.hpp>

#include <string>
#include <vector>

using nlohmann::json;

namespace fwref
{

void loadLoaderScenarioJson(const json& j, Scenario& out)
{
    LoaderScenario& loader = out.loader;
    loader.selectedFile = j.value("selectedFile", loader.selectedFile);
    loader.selectedSlot = j.value("selectedSlot", loader.selectedSlot);
    loader.selectedPlace = j.value("selectedPlace", loader.selectedPlace);
    loader.memoryUsage = j.value("memoryUsage", loader.memoryUsage);
    loader.memoryUsageAdd = j.value("memoryUsageAdd", loader.memoryUsageAdd);
    loader.previewVolume = j.value("previewVolume", loader.previewVolume);
    loader.slotCount = j.value("slotCount", loader.slotCount);

    if (j.contains("files") && j["files"].is_array())
    {
        loader.files.clear();
        for (const auto& f : j["files"])
            loader.files.push_back(f.get<std::string>());
    }
    if (j.contains("slots") && j["slots"].is_array())
    {
        loader.slots.clear();
        for (const auto& s : j["slots"])
            loader.slots.push_back(s.get<std::string>());
    }
}

void loadFileScenarioJson(const json& j, Scenario& out)
{
    FileScenario& file = out.file;
    file.selectedPlace = j.value("selectedPlace", file.selectedPlace);
    file.projectName = j.value("projectName", file.projectName);
    file.selectedProject = j.value("selectedProject", file.selectedProject);
    if (j.contains("projects") && j["projects"].is_array())
    {
        file.projects.clear();
        for (const auto& p : j["projects"])
            file.projects.push_back(p.get<std::string>());
    }
    if (j.contains("recent") && j["recent"].is_array())
    {
        file.recent.clear();
        for (const auto& p : j["recent"])
            file.recent.push_back(p.get<std::string>());
    }
}

void loadSampleScenarioJson(const json& j, Scenario& out)
{
    SampleScenario& sample = out.sample;
    sample.rightLabel = j.value("rightLabel", sample.rightLabel);
    sample.playMode = j.value("playMode", sample.playMode);
    sample.selectedPlace = j.value("selectedPlace", sample.selectedPlace);
    sample.progress = j.value("progress", sample.progress);
    sample.startPoint = j.value("startPoint", sample.startPoint);
    sample.endPoint = j.value("endPoint", sample.endPoint);
    sample.loop1 = j.value("loop1", sample.loop1);
    sample.loop2 = j.value("loop2", sample.loop2);
    sample.selected = j.value("selected", sample.selected);
    sample.zerocrossed = j.value("zerocrossed", sample.zerocrossed);
    sample.startTime = j.value("startTime", sample.startTime);
    sample.endTime = j.value("endTime", sample.endTime);
    sample.loop1Time = j.value("loop1Time", sample.loop1Time);
    sample.loop2Time = j.value("loop2Time", sample.loop2Time);
    sample.zoom = j.value("zoom", sample.zoom);
    sample.sliceSelected = j.value("sliceSelected", sample.sliceSelected);

    if (j.contains("waveform") && j["waveform"].is_object())
    {
        const auto& w = j["waveform"];
        sample.wavePoints = w.value("points", sample.wavePoints);
        sample.waveAmplitude = w.value("amplitude", sample.waveAmplitude);
        sample.waveFrequency = w.value("frequency", sample.waveFrequency);
        sample.waveDecay = w.value("decay", sample.waveDecay);
        sample.waveform.clear();
    }
    if (j.contains("region") && j["region"].is_object())
    {
        sample.regionStart = j["region"].value("start", sample.regionStart);
        sample.regionEnd = j["region"].value("end", sample.regionEnd);
    }
    if (j.contains("slices") && j["slices"].is_array())
    {
        sample.slices.clear();
        for (const auto& s : j["slices"])
            sample.slices.push_back(s.get<int>());
    }
    sample.sliceCount = j.value("sliceCount", static_cast<int>(sample.slices.size()));
    if (j.contains("wavetable") && j["wavetable"].is_object())
    {
        const auto& w = j["wavetable"];
        sample.wtPosition = w.value("position", sample.wtPosition);
        sample.wtWindow = w.value("window", sample.wtWindow);
        sample.wtPositionMax = w.value("positionMax", sample.wtPositionMax);
    }
    if (j.contains("playModes") && j["playModes"].is_array())
    {
        sample.playModes.clear();
        for (const auto& m : j["playModes"])
            sample.playModes.push_back(m.get<std::string>());
    }
}

void finalizeLoaderSampleScenario(Scenario& out)
{
    ensureLoaderSlots(out.loader);
    buildSampleWaveform(out.sample);
}

} // namespace fwref

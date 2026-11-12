#include "scenario_util.h"

#include "ampLogarythmicValues.h"

#include <nlohmann/json.hpp>

#include <fstream>
#include <string>

using nlohmann::json;

namespace fwref
{

bool loadScenarioJson(const std::string& path, Scenario& out, std::string& error)
{
    std::ifstream f(path);
    if (!f)
    {
        error = "cannot open scenario: " + path;
        return false;
    }
    json j;
    try
    {
        f >> j;
    }
    catch (const std::exception& e)
    {
        error = std::string("invalid scenario JSON: ") + e.what();
        return false;
    }

    if (j.contains("instrument"))
    {
        const auto& i = j["instrument"];
        auto& s = out.instrument;
        s.name = i.value("name", s.name);
        s.volume = i.value("volume", s.volume);
        s.panning = i.value("panning", s.panning);
        s.tune = i.value("tune", s.tune);
        s.fineTune = i.value("fineTune", s.fineTune);
        s.filterEnable = i.value("filterEnable", s.filterEnable);
        s.filterType = i.value("filterType", s.filterType);
        s.cutOff = i.value("cutOff", s.cutOff);
        s.resonance = i.value("resonance", s.resonance);
        s.selectedPlace = i.value("selectedPlace", s.selectedPlace);
        s.rightLabel = i.value("rightLabel", s.rightLabel);
    }

    if (j.contains("master"))
    {
        const auto& m = j["master"];
        auto& s = out.master;
        s.volume = m.value("volume", s.volume);
        s.saturationType = m.value("saturationType", s.saturationType);
        s.bassBoost = m.value("bassBoost", s.bassBoost);
        s.stereoBoost = m.value("stereoBoost", s.stereoBoost);
        s.selectedPlace = m.value("selectedPlace", s.selectedPlace);
        s.delayVolume = m.value("delayVolume", s.delayVolume);
        s.reverbVolume = m.value("reverbVolume", s.reverbVolume);
        s.dryMixVolume = m.value("dryMixVolume", s.dryMixVolume);
        s.lineInVolume = m.value("lineInVolume", s.lineInVolume);
        s.delayMute = m.value("delayMute", s.delayMute);
        s.reverbMute = m.value("reverbMute", s.reverbMute);
        s.dryMixMute = m.value("dryMixMute", s.dryMixMute);
        s.lineInMute = m.value("lineInMute", s.lineInMute);
        s.delayLevel = m.value("delayLevel", s.delayLevel);
        s.reverbLevel = m.value("reverbLevel", s.reverbLevel);
        s.dryMixLevel = m.value("dryMixLevel", s.dryMixLevel);
        s.lineInLevel = m.value("lineInLevel", s.lineInLevel);
        if (m.contains("trackNames") && m["trackNames"].is_array())
        {
            size_t idx = 0;
            for (const auto& n : m["trackNames"])
            {
                if (idx >= s.trackNames.size())
                    break;
                s.trackNames[idx++] = n.get<std::string>();
            }
        }
        if (m.contains("trackVolume") && m["trackVolume"].is_array())
        {
            size_t idx = 0;
            for (const auto& v : m["trackVolume"])
            {
                if (idx >= s.trackVolume.size())
                    break;
                s.trackVolume[idx++] = v.get<int>();
            }
        }
        if (m.contains("trackMute") && m["trackMute"].is_array())
        {
            size_t idx = 0;
            for (const auto& v : m["trackMute"])
            {
                if (idx >= s.trackMute.size())
                    break;
                s.trackMute[idx++] = v.get<bool>();
            }
        }
        if (m.contains("trackLevel") && m["trackLevel"].is_array())
        {
            size_t idx = 0;
            for (const auto& v : m["trackLevel"])
            {
                if (idx >= s.trackLevel.size())
                    break;
                s.trackLevel[idx++] = v.get<int>();
            }
        }
    }

    if (j.contains("config"))
    {
        const auto& c = j["config"];
        auto& s = out.config;
        s.selectedBase = c.value("selectedBase", s.selectedBase);
        s.selectedItem = c.value("selectedItem", s.selectedItem);
        s.selectedPlace = c.value("selectedPlace", s.selectedPlace);
        s.gridSelected = c.value("gridSelected", s.gridSelected);
        s.credits = c.value("credits", s.credits);
        s.description = c.value("description", s.description);
        if (c.contains("baseMenu") && c["baseMenu"].is_array())
            for (const auto& n : c["baseMenu"])
                s.baseMenu.push_back(n.get<std::string>());
        if (c.contains("items") && c["items"].is_array())
            for (const auto& row : c["items"])
                if (row.is_array() && row.size() == 2)
                    s.items.emplace_back(row[0].get<std::string>(), row[1].get<std::string>());
    }

    if (j.contains("perform"))
    {
        const auto& p = j["perform"];
        auto& s = out.perform;
        s.editPlace = p.value("editPlace", s.editPlace);
        if (p.contains("fxNames") && p["fxNames"].is_array())
        {
            size_t idx = 0;
            for (const auto& n : p["fxNames"])
            {
                if (idx >= s.fxNames.size())
                    break;
                s.fxNames[idx++] = n.get<std::string>();
            }
        }
        if (p.contains("fxValues") && p["fxValues"].is_array())
        {
            size_t slot = 0;
            for (const auto& rows : p["fxValues"])
            {
                if (slot >= s.fxValues.size() || !rows.is_array())
                    break;
                size_t row = 0;
                for (const auto& v : rows)
                {
                    if (row >= s.fxValues[slot].size())
                        break;
                    s.fxValues[slot][row++] = v.get<std::string>();
                }
                ++slot;
            }
        }
        if (p.contains("selectedRows") && p["selectedRows"].is_array())
        {
            size_t idx = 0;
            for (const auto& v : p["selectedRows"])
            {
                if (idx >= s.selectedRows.size())
                    break;
                s.selectedRows[idx++] = v.get<int>();
            }
        }
        if (p.contains("trackNames") && p["trackNames"].is_array())
        {
            size_t idx = 0;
            for (const auto& n : p["trackNames"])
            {
                if (idx >= s.trackNames.size())
                    break;
                s.trackNames[idx++] = n.get<std::string>();
            }
        }
        if (p.contains("activeTracks") && p["activeTracks"].is_array())
        {
            size_t idx = 0;
            for (const auto& v : p["activeTracks"])
            {
                if (idx >= s.activeTracks.size())
                    break;
                s.activeTracks[idx++] = v.get<bool>();
            }
        }
        if (p.contains("trackMuted") && p["trackMuted"].is_array())
        {
            size_t idx = 0;
            for (const auto& v : p["trackMuted"])
            {
                if (idx >= s.trackMuted.size())
                    break;
                s.trackMuted[idx++] = v.get<bool>();
            }
        }
        if (p.contains("trackPatterns") && p["trackPatterns"].is_array())
        {
            size_t idx = 0;
            for (const auto& n : p["trackPatterns"])
            {
                if (idx >= s.trackPatterns.size())
                    break;
                s.trackPatterns[idx++] = n.get<std::string>();
            }
        }
    }

    if (j.contains("pattern"))
    {
        const auto& p = j["pattern"];
        auto& s = out.pattern;
        s.actualPattern = p.value("actualPattern", s.actualPattern);
        s.rightLabel = p.value("rightLabel", s.rightLabel);
        s.patternLength = p.value("patternLength", s.patternLength);
        s.actualStep = p.value("actualStep", s.actualStep);
        s.actualTrack = p.value("actualTrack", s.actualTrack);
        s.firstVisibleTrack = p.value("firstVisibleTrack", s.firstVisibleTrack);
        s.stepDevider = p.value("stepDevider", s.stepDevider);
        s.selectState = p.value("selectState", s.selectState);
        s.selectColor = p.value("selectColor", s.selectColor);
        s.selectedParam = p.value("selectedParam", s.selectedParam);
        s.viewMode = p.value("viewMode", s.viewMode);
        s.showPlayhead = p.value("showPlayhead", s.showPlayhead);
        s.playheadPosition = p.value("playheadPosition", s.playheadPosition);
        s.selectedPlace = p.value("selectedPlace", s.selectedPlace);
        s.editStep = p.value("editStep", s.editStep);
        s.timerMinutes = p.value("timerMinutes", s.timerMinutes);
        s.timerSeconds = p.value("timerSeconds", s.timerSeconds);
        if (p.contains("trackNames") && p["trackNames"].is_array())
        {
            size_t idx = 0;
            for (const auto& n : p["trackNames"])
            {
                if (idx >= s.trackNames.size())
                    break;
                s.trackNames[idx++] = n.get<std::string>();
            }
        }
    }
    if (j.contains("song"))
    {
        const auto& p = j["song"];
        auto& s = out.song;
        s.selectedSlot = p.value("selectedSlot", s.selectedSlot);
        s.linesCount = p.value("linesCount", s.linesCount);
        s.normalTracksCount = p.value("normalTracksCount", s.normalTracksCount);
        s.midiTracksCount = p.value("midiTracksCount", s.midiTracksCount);
        s.isListActive = p.value("isListActive", s.isListActive);
        s.editMode = p.value("editMode", s.editMode);
        s.globalTempo = p.value("globalTempo", s.globalTempo);
        s.hexNumbers = p.value("hexNumbers", s.hexNumbers);
        if (p.contains("playlist") && p["playlist"].is_array())
        {
            s.playlist.clear();
            for (const auto& v : p["playlist"])
                s.playlist.push_back(v.get<int>());
        }
        if (p.contains("patternNames") && p["patternNames"].is_array())
        {
            s.patternNames.clear();
            for (const auto& n : p["patternNames"])
                s.patternNames.push_back(n.get<std::string>());
        }
        if (p.contains("trackNames") && p["trackNames"].is_array())
        {
            size_t idx = 0;
            for (const auto& n : p["trackNames"])
            {
                if (idx >= s.trackNames.size())
                    break;
                s.trackNames[idx++] = n.get<std::string>();
            }
        }
        if (p.contains("normalMuted") && p["normalMuted"].is_array())
        {
            size_t idx = 0;
            for (const auto& v : p["normalMuted"])
            {
                if (idx >= s.normalMuted.size())
                    break;
                s.normalMuted[idx++] = v.get<bool>();
            }
        }
        if (p.contains("midiMuted") && p["midiMuted"].is_array())
        {
            size_t idx = 0;
            for (const auto& v : p["midiMuted"])
            {
                if (idx >= s.midiMuted.size())
                    break;
                s.midiMuted[idx++] = v.get<bool>();
            }
        }
        if (p.contains("normalPatternMask") && p["normalPatternMask"].is_array())
        {
            size_t idx = 0;
            for (const auto& v : p["normalPatternMask"])
            {
                if (idx >= s.normalPatternMask.size())
                    break;
                s.normalPatternMask[idx++] = v.get<int>();
            }
        }
        if (p.contains("midiPatternMask") && p["midiPatternMask"].is_array())
        {
            size_t idx = 0;
            for (const auto& v : p["midiPatternMask"])
            {
                if (idx >= s.midiPatternMask.size())
                    break;
                s.midiPatternMask[idx++] = v.get<int>();
            }
        }
        if (p.contains("selection") && p["selection"].is_object())
        {
            const auto& sel = p["selection"];
            s.selectionIsActive = sel.value("isActive", s.selectionIsActive);
            s.selectionStartSongPosition =
              sel.value("startSongPosition", s.selectionStartSongPosition);
            s.selectionPatternLength =
              sel.value("patternSelectionLength", s.selectionPatternLength);
            s.selectionStartTrack = sel.value("startTrack", s.selectionStartTrack);
            s.selectionTrackLength =
              sel.value("trackSelectionLength", s.selectionTrackLength);
        }
        if (p.contains("progress") && p["progress"].is_object())
        {
            const auto& pr = p["progress"];
            s.progressIsPlaying = pr.value("isPlaying", s.progressIsPlaying);
            s.progressTrackPreview = pr.value("trackPreview", s.progressTrackPreview);
            s.progressCurrentSongPosition =
              pr.value("currentSongPosition", s.progressCurrentSongPosition);
            s.progressPatternLength = pr.value("patternLength", s.progressPatternLength);
            s.progressPositionInPattern =
              pr.value("positionInPattern", s.progressPositionInPattern);
        }
    }
    if (j.contains("loader") && j["loader"].is_object())
        loadLoaderScenarioJson(j["loader"], out);
    if (j.contains("sample") && j["sample"].is_object())
        loadSampleScenarioJson(j["sample"], out);
    if (j.contains("file") && j["file"].is_object())
        loadFileScenarioJson(j["file"], out);

    finalizeLoaderSampleScenario(out);
    return true;
}

} // namespace fwref

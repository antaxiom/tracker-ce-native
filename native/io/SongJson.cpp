#include "SongJson.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <vector>

namespace trakr {

using json = nlohmann::json;

bool saveSongJson(const Song& song, const std::string& filepath) {
    json j;
    j["bpm"] = song.bpm;
    j["lpb"] = song.lpb;

    // 1. Tracks
    j["tracks"] = json::array();
    for (const auto& trk : song.tracks) {
        json t;
        t["name"] = trk.name;
        t["gainDb"] = trk.gainDb;
        t["pan"] = trk.pan;
        t["mute"] = trk.mute;
        t["solo"] = trk.solo;
        t["fxColumns"] = trk.fxColumns;
        t["delaySend"] = trk.delaySend;
        t["reverbSend"] = trk.reverbSend;
        j["tracks"].push_back(t);
    }

    // 2. Patterns
    j["patterns"] = json::array();
    for (const auto& pat : song.patterns) {
        json p;
        p["rows"] = pat.rows;
        p["tracks"] = json::array();
        for (const auto& trkCells : pat.tracks) {
            json tCells = json::array();
            for (const auto& cell : trkCells) {
                json c;
                c["note"] = cell.note;
                c["inst"] = cell.instrument;
                c["vol"] = cell.volume;
                json fxArr = json::array();
                for (const auto& fx : cell.fx) {
                    if (fx.type != 0 || fx.value != 0) {
                        json f;
                        f["t"] = fx.type;
                        f["v"] = fx.value;
                        fxArr.push_back(f);
                    }
                }
                if (!fxArr.empty()) c["fx"] = fxArr;
                tCells.push_back(c);
            }
            p["tracks"].push_back(tCells);
        }
        j["patterns"].push_back(p);
    }

    // 3. Order
    j["order"] = song.order;

    // 4. Instruments
    j["instruments"] = json::array();
    for (const auto& inst : song.instruments) {
        json i;
        i["name"] = inst.name;
        i["rootNote"] = inst.rootNote;
        i["finetune"] = inst.finetune;
        i["gainDb"] = inst.gainDb;
        i["tune"] = inst.tune;
        i["panning"] = inst.panning;
        i["volume"] = inst.volume;
        i["loop"] = static_cast<int>(inst.loop);
        i["startFrame"] = inst.startFrame;
        i["loopStart"] = inst.loopStart;
        i["loopEnd"] = inst.loopEnd;
        i["endFrame"] = inst.endFrame;

        // DSP parameters
        i["attackMs"] = inst.attackMs;
        i["decayMs"] = inst.decayMs;
        i["sustain"] = inst.sustain;
        i["releaseMs"] = inst.releaseMs;
        i["filterType"] = static_cast<int>(inst.filterType);
        i["cutoff"] = inst.cutoff;
        i["resonance"] = inst.resonance;
        i["overdrive"] = inst.overdrive;
        i["bitDepth"] = inst.bitDepth;
        i["delaySend"] = inst.delaySend;
        i["reverbSend"] = inst.reverbSend;
        i["filterCutoffHz"] = inst.filterCutoffHz;
        i["filterResonance"] = inst.filterResonance;
        i["lfoWave"] = static_cast<int>(inst.lfoWave);
        i["lfoTarget"] = static_cast<int>(inst.lfoTarget);
        i["lfoRateHz"] = inst.lfoRateHz;
        i["lfoDepth"] = inst.lfoDepth;

        j["instruments"].push_back(i);
    }

    std::ofstream out(filepath);
    if (!out.is_open()) return false;
    out << j.dump(2);
    return true;
}

bool loadSongJson(Song& outSong, const std::string& filepath) {
    std::ifstream in(filepath);
    if (!in.is_open()) return false;

    json j;
    try {
        in >> j;
    } catch (...) {
        return false;
    }

    if (j.contains("bpm")) outSong.bpm = j["bpm"].get<float>();
    if (j.contains("lpb")) outSong.lpb = j["lpb"].get<int>();

    // 1. Tracks
    if (j.contains("tracks") && j["tracks"].is_array()) {
        outSong.tracks.clear();
        for (const auto& t : j["tracks"]) {
            TrackInfo trk;
            if (t.contains("name")) trk.name = t["name"].get<std::string>();
            if (t.contains("gainDb")) trk.gainDb = t["gainDb"].get<float>();
            if (t.contains("pan")) trk.pan = t["pan"].get<float>();
            if (t.contains("mute")) trk.mute = t["mute"].get<bool>();
            if (t.contains("solo")) trk.solo = t["solo"].get<bool>();
            if (t.contains("fxColumns")) trk.fxColumns = t["fxColumns"].get<int>();
            if (t.contains("delaySend")) trk.delaySend = t["delaySend"].get<float>();
            if (t.contains("reverbSend")) trk.reverbSend = t["reverbSend"].get<float>();
            outSong.tracks.push_back(trk);
        }
    }

    // 2. Patterns
    if (j.contains("patterns") && j["patterns"].is_array()) {
        outSong.patterns.clear();
        for (const auto& p : j["patterns"]) {
            Pattern pat;
            pat.rows = p.value("rows", 64);
            if (p.contains("tracks") && p["tracks"].is_array()) {
                for (const auto& tCells : p["tracks"]) {
                    std::vector<Cell> trkVec;
                    if (tCells.is_array()) {
                        for (const auto& c : tCells) {
                            Cell cell;
                            cell.note = c.value("note", kNoNote);
                            cell.instrument = c.value("inst", -1);
                            cell.volume = c.value("vol", -1);
                            if (c.contains("fx") && c["fx"].is_array()) {
                                size_t fxIdx = 0;
                                for (const auto& f : c["fx"]) {
                                    if (fxIdx < cell.fx.size()) {
                                        cell.fx[fxIdx].type = f.value("t", uint8_t(0));
                                        cell.fx[fxIdx].value = f.value("v", uint8_t(0));
                                        fxIdx++;
                                    }
                                }
                            }
                            trkVec.push_back(cell);
                        }
                    }
                    pat.tracks.push_back(trkVec);
                }
            }
            outSong.patterns.push_back(pat);
        }
    }

    // 3. Order
    if (j.contains("order") && j["order"].is_array()) {
        outSong.order = j["order"].get<std::vector<int>>();
    }

    // 4. Instruments
    if (j.contains("instruments") && j["instruments"].is_array()) {
        outSong.instruments.clear();
        for (const auto& i : j["instruments"]) {
            Instrument inst;
            inst.name = i.value("name", "Instrument");
            inst.rootNote = i.value("rootNote", 60);
            inst.tune = i.value("tune", 0);
            inst.finetune = i.value("finetune", 0);
            inst.panning = i.value("panning", 0);
            inst.volume = i.value("volume", 50);
            inst.gainDb = i.value("gainDb", 0.0f);
            inst.loop = static_cast<Instrument::Loop>(i.value("loop", 0));
            inst.startFrame = i.value("startFrame", 0);
            inst.loopStart = i.value("loopStart", 0);
            inst.loopEnd = i.value("loopEnd", 0);
            inst.endFrame = i.value("endFrame", -1);

            inst.attackMs = i.value("attackMs", 1.0f);
            inst.decayMs = i.value("decayMs", 100.0f);
            inst.sustain = i.value("sustain", 1.0f);
            inst.releaseMs = i.value("releaseMs", 50.0f);
            inst.filterType = static_cast<Instrument::FilterType>(i.value("filterType", 0));
            inst.cutoff = i.value("cutoff", 100);
            inst.resonance = i.value("resonance", 0);
            inst.overdrive = i.value("overdrive", 0);
            inst.bitDepth = i.value("bitDepth", 16);
            inst.delaySend = i.value("delaySend", 0);
            inst.reverbSend = i.value("reverbSend", 0);
            inst.filterCutoffHz = i.value("filterCutoffHz", 20000.0f);
            inst.filterResonance = i.value("filterResonance", 0.707f);
            inst.lfoWave = static_cast<Instrument::LfoWave>(i.value("lfoWave", 0));
            inst.lfoTarget = static_cast<Instrument::LfoTarget>(i.value("lfoTarget", 0));
            inst.lfoRateHz = i.value("lfoRateHz", 1.0f);
            inst.lfoDepth = i.value("lfoDepth", 0.0f);

            outSong.instruments.push_back(inst);
        }
    }

    return true;
}

} // namespace trakr

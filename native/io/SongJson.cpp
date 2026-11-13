#include "SongJson.h"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <exception>
#include <fstream>
#include <utility>
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

    // 5. Master section (Master 1/3, EQ page and the project headroom flag)
    {
        json m;
        m["gainDb"] = song.masterGainDb;
        m["pan"] = song.masterPan;
        m["delayTimeMs"] = song.delayTimeMs;
        m["delayFeedback"] = song.delayFeedback;
        m["reverbSize"] = song.reverbSize;
        m["reverbDamping"] = song.reverbDamping;
        m["bassBoost"] = song.masterBassBoost;
        m["saturationDrive"] = song.masterSaturationDrive;
        m["saturationType"] = song.masterSaturationType;
        m["space"] = song.masterSpace;
        m["eq"] = json::array();
        for (float band : song.masterEq) m["eq"].push_back(band);
        m["headroom"] = song.headroom;
        m["aux"] = json::array();
        m["auxMute"] = json::array();
        for (int i = 0; i < 4; ++i) {
            m["aux"].push_back(song.auxVolume[i]);
            m["auxMute"].push_back(song.auxMute[i]);
        }
        j["master"] = m;
    }

    std::ofstream out(filepath);
    if (!out.is_open()) return false;
    out << j.dump(2);
    return true;
}

static bool loadSongJsonFile(Song& outSong, const json& j) {
    if (j.contains("bpm")) outSong.bpm = j["bpm"].get<float>();
    if (j.contains("lpb")) outSong.lpb = j["lpb"].get<int>();

    if (j.contains("master") && j["master"].is_object()) {
        const json& m = j["master"];
        outSong.masterGainDb = m.value("gainDb", outSong.masterGainDb);
        outSong.masterPan = m.value("pan", outSong.masterPan);
        outSong.delayTimeMs = m.value("delayTimeMs", outSong.delayTimeMs);
        outSong.delayFeedback = m.value("delayFeedback", outSong.delayFeedback);
        outSong.reverbSize = m.value("reverbSize", outSong.reverbSize);
        outSong.reverbDamping = m.value("reverbDamping", outSong.reverbDamping);
        outSong.masterBassBoost = m.value("bassBoost", outSong.masterBassBoost);
        outSong.masterSaturationDrive = m.value("saturationDrive", outSong.masterSaturationDrive);
        outSong.masterSaturationType = m.value("saturationType", outSong.masterSaturationType);
        outSong.masterSpace = m.value("space", outSong.masterSpace);
        outSong.headroom = m.value("headroom", outSong.headroom);
        if (m.contains("aux") && m["aux"].is_array()) {
            size_t idx = 0;
            for (const auto& v : m["aux"]) {
                if (idx >= 4) break;
                outSong.auxVolume[idx++] = std::clamp(v.get<int>(), 0, 100);
            }
        }
        if (m.contains("auxMute") && m["auxMute"].is_array()) {
            size_t idx = 0;
            for (const auto& v : m["auxMute"]) {
                if (idx >= 4) break;
                outSong.auxMute[idx++] = v.get<bool>();
            }
        }
        if (m.contains("eq") && m["eq"].is_array()) {
            size_t idx = 0;
            for (const auto& band : m["eq"]) {
                if (idx >= 5) break;
                outSong.masterEq[idx++] = band.get<float>();
            }
        }
    }

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

// Validate the parts every screen indexes without re-checking: at least one
// pattern/slot, a legal row count, one cell vector per track and legal slots.
static void normalizeLoadedSong(Song& song) {
    if (song.tracks.empty()) {
        TrackInfo t;
        t.name = "Track 1";
        song.tracks.push_back(t);
    }
    if (song.patterns.empty()) song.patterns.push_back(Pattern{});

    for (Pattern& pat : song.patterns) {
        pat.rows = std::clamp(pat.rows, 1, 1024);
        pat.tracks.resize(song.tracks.size());
        for (std::vector<Cell>& trk : pat.tracks) trk.resize(pat.rows);
    }

    const int patternCount = static_cast<int>(song.patterns.size());
    for (int& slot : song.order) slot = std::clamp(slot, 0, patternCount - 1);
    if (song.order.empty()) song.order.push_back(0);
}

bool loadSongJson(Song& outSong, const std::string& filepath) {
    std::ifstream in(filepath);
    if (!in.is_open()) return false;

    try {
        json j;
        in >> j;
        if (!j.is_object()) return false;

        Song song;
        if (!loadSongJsonFile(song, j)) return false;
        normalizeLoadedSong(song);
        outSong = std::move(song);
        return true;
    } catch (const std::exception&) {
        // Type mismatches and malformed documents are rejected, never fatal.
        return false;
    }
}

} // namespace trakr

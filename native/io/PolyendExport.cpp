#include "PolyendExport.h"
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <cstring>
#include <cmath>

namespace trakr::io {

namespace {

static inline void writeU16LE(uint8_t* p, uint16_t val) {
    p[0] = static_cast<uint8_t>(val & 0xFF);
    p[1] = static_cast<uint8_t>((val >> 8) & 0xFF);
}

#pragma pack(push, 1)
struct MtpStepExport {
    int8_t note{-1};
    uint8_t instrument{255};
    struct {
        uint8_t type{0};
        uint8_t value{0};
    } fx[2];
    int8_t volume{-1};
    int8_t micromove{0};
};

struct MtpTrackExport {
    uint8_t length{63};
    MtpStepExport step[128];
};
#pragma pack(pop)

} // namespace

bool exportPtiInstrument(const std::string& path, const Instrument& inst) {
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) return false;

    // 1. Initial 16-byte header
    uint8_t header[16] = {0};
    header[0] = 'T'; header[1] = 'I'; header[2] = '0'; header[3] = '1';
    uint16_t instSize = 100;
    writeU16LE(header + 12, instSize);
    file.write(reinterpret_cast<const char*>(header), sizeof(header));

    // 2. Metadata payload (100 bytes)
    uint8_t instData[100] = {0};
    std::string safeName = inst.name.substr(0, 31);
    std::memcpy(instData + 2, safeName.c_str(), safeName.size());

    uint8_t playMode = 0;
    if (inst.loop == Instrument::Loop::Forward) playMode = 1;
    else if (inst.loop == Instrument::Loop::Backward) playMode = 2;
    else if (inst.loop == Instrument::Loop::PingPong) playMode = 3;
    instData[56] = playMode;

    size_t numSamples = (inst.sample && !inst.sample->ch[0].empty()) ? inst.sample->ch[0].size() : 0;
    if (numSamples > 0) {
        uint16_t startPt = static_cast<uint16_t>((static_cast<uint64_t>(inst.startFrame) * 10000) / numSamples);
        uint16_t loopPt1 = static_cast<uint16_t>((static_cast<uint64_t>(inst.loopStart) * 10000) / numSamples);
        uint16_t loopPt2 = static_cast<uint16_t>((static_cast<uint64_t>(inst.loopEnd > 0 ? inst.loopEnd : numSamples) * 10000) / numSamples);
        uint16_t endPt = static_cast<uint16_t>((static_cast<uint64_t>(inst.endFrame > 0 ? inst.endFrame : numSamples) * 10000) / numSamples);

        writeU16LE(instData + 58, startPt);
        writeU16LE(instData + 60, loopPt1);
        writeU16LE(instData + 62, loopPt2);
        writeU16LE(instData + 64, endPt);
    }
    file.write(reinterpret_cast<const char*>(instData), sizeof(instData));

    // 3. 4-byte padding before PCM payload
    uint8_t pad[4] = {0};
    file.write(reinterpret_cast<const char*>(pad), sizeof(pad));

    // 4. PCM audio samples (16-bit mono)
    if (numSamples > 0) {
        const auto& ch = inst.sample->ch[0];
        std::vector<int16_t> pcm(numSamples);
        for (size_t i = 0; i < numSamples; ++i) {
            float s = std::clamp(ch[i], -1.0f, 1.0f);
            pcm[i] = static_cast<int16_t>(s * 32767.0f);
        }
        file.write(reinterpret_cast<const char*>(pcm.data()), numSamples * sizeof(int16_t));
    }

    return true;
}

bool exportMtpPattern(const std::string& path, const Pattern& pat) {
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) return false;

    // 28-byte header
    uint8_t header[28] = {0};
    header[0] = 'M'; header[1] = 'T'; header[2] = 'P'; header[3] = '1';
    file.write(reinterpret_cast<const char*>(header), sizeof(header));

    // 8 Tracks
    std::vector<MtpTrackExport> tracks(8);
    int numRows = std::clamp(pat.rows, 1, 128);

    for (int t = 0; t < 8; ++t) {
        tracks[t].length = static_cast<uint8_t>(numRows - 1);
        if (t < static_cast<int>(pat.tracks.size())) {
            const auto& srcTrack = pat.tracks[t];
            for (int r = 0; r < numRows && r < static_cast<int>(srcTrack.size()); ++r) {
                const auto& src = srcTrack[r];
                auto& dst = tracks[t].step[r];

                if (src.note == kNoNote) dst.note = -1;
                else if (src.note == kNoteOff) dst.note = -2;
                else dst.note = static_cast<int8_t>(std::clamp<int>(src.note, 0, 119));

                dst.instrument = (src.instrument >= 0 && src.instrument < 48)
                    ? static_cast<uint8_t>(src.instrument)
                    : 255;

                dst.volume = (src.volume >= 0) ? static_cast<int8_t>(std::clamp<int>(src.volume, 0, 64)) : -1;
                dst.fx[0].type = src.fx[0].type;
                dst.fx[0].value = src.fx[0].value;
                dst.fx[1].type = src.fx[1].type;
                dst.fx[1].value = src.fx[1].value;
            }
        }
    }

    file.write(reinterpret_cast<const char*>(tracks.data()), sizeof(MtpTrackExport) * 8);
    return true;
}

bool exportPolyendProject(const std::string& projectDir, const Song& song) {
    namespace fs = std::filesystem;
    std::error_code ec;
    fs::create_directories(projectDir, ec);
    fs::create_directories(fs::path(projectDir) / "patterns", ec);
    fs::create_directories(fs::path(projectDir) / "instruments", ec);

    // 1. project.mt
    std::string mtPath = (fs::path(projectDir) / "project.mt").string();
    std::ofstream mtFile(mtPath, std::ios::binary);
    if (!mtFile.is_open()) return false;

    uint8_t mtHeader[16] = {0};
    mtHeader[0] = 'M';
    mtHeader[1] = 'T';
    mtHeader[2] = static_cast<uint8_t>(std::clamp<int>(static_cast<int>(song.bpm), 10, 255));
    mtFile.write(reinterpret_cast<const char*>(mtHeader), sizeof(mtHeader));

    std::vector<uint8_t> orderBytes;
    orderBytes.reserve(song.order.size());
    for (int p : song.order) {
        orderBytes.push_back(static_cast<uint8_t>(std::clamp(p + 1, 1, 254)));
    }
    if (!orderBytes.empty()) {
        mtFile.write(reinterpret_cast<const char*>(orderBytes.data()), orderBytes.size());
    }
    mtFile.close();

    // 2. Patterns
    for (size_t i = 0; i < song.patterns.size(); ++i) {
        char buf[32];
        snprintf(buf, sizeof(buf), "pattern_%02zu.mtp", i + 1);
        std::string patPath = (fs::path(projectDir) / "patterns" / buf).string();
        exportMtpPattern(patPath, song.patterns[i]);
    }

    // 3. Instruments
    for (size_t i = 0; i < song.instruments.size() && i < 48; ++i) {
        char buf[32];
        snprintf(buf, sizeof(buf), "instrument_%02zu.pti", i + 1);
        std::string instPath = (fs::path(projectDir) / "instruments" / buf).string();
        exportPtiInstrument(instPath, song.instruments[i]);
    }

    return true;
}

} // namespace trakr::io

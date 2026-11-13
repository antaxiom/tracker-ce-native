#include "PolyendImport.h"
#include <dr_wav.h>
#include <filesystem>
#include <fstream>
#include <vector>
#include <cstring>
#include <algorithm>

namespace trakr::io {

bool loadAudioFile(const std::string& path, SampleData& outSample) {
    drwav wav;
    if (!drwav_init_file(&wav, path.c_str(), nullptr)) {
        return false;
    }

    const uint64_t totalFrames = wav.totalPCMFrameCount;
    if (totalFrames == 0) {
        drwav_uninit(&wav);
        return false;
    }

    std::vector<float> interleaved(totalFrames * wav.channels);
    drwav_read_pcm_frames_f32(&wav, totalFrames, interleaved.data());

    outSample.channels = static_cast<int>(wav.channels);
    outSample.sampleRate = static_cast<int>(wav.sampleRate);
    outSample.ch[0].resize(totalFrames);

    if (wav.channels > 1) {
        outSample.ch[1].resize(totalFrames);
        for (uint64_t i = 0; i < totalFrames; ++i) {
            outSample.ch[0][i] = interleaved[i * 2];
            outSample.ch[1][i] = interleaved[i * 2 + 1];
        }
    } else {
        outSample.ch[1].clear();
        for (uint64_t i = 0; i < totalFrames; ++i) {
            outSample.ch[0][i] = interleaved[i];
        }
    }

    drwav_uninit(&wav);
    return true;
}

static inline uint16_t readU16LE(const uint8_t* p) {
    return static_cast<uint16_t>(p[0]) | (static_cast<uint16_t>(p[1]) << 8);
}

bool loadPtiInstrument(const std::string& path, Instrument& outInst) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return false;

    file.seekg(0, std::ios::end);
    std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    if (fileSize < 16) return false;

    std::vector<uint8_t> buf(static_cast<size_t>(fileSize));
    file.read(reinterpret_cast<char*>(buf.data()), fileSize);

    if (buf[0] != 'T' || buf[1] != 'I') return false;

    uint16_t instSize = readU16LE(buf.data() + 12);
    size_t dataOffset = 392; // default legacy fallback

    std::string name;
    uint8_t playMode = 0;
    uint16_t startPoint = 0, loopPoint1 = 0, loopPoint2 = 0, endPoint = 0;

    if (instSize >= 100 && (16 + instSize) <= static_cast<size_t>(fileSize)) {
        // Real firmware .pti format
        size_t metadataSize = 16 + instSize;
        dataOffset = (fileSize >= static_cast<std::streamsize>(metadataSize + 4))
            ? (metadataSize + 4)
            : metadataSize;

        const uint8_t* instData = buf.data() + 16;
        // fileName is at instData + 2 (32 chars)
        const char* fileNamePtr = reinterpret_cast<const char*>(instData + 2);
        size_t fnLen = strnlen(fileNamePtr, 32);
        if (fnLen > 0) {
            name.assign(fileNamePtr, fnLen);
        }

        playMode = instData[56];
        startPoint = readU16LE(instData + 58);
        loopPoint1 = readU16LE(instData + 60);
        loopPoint2 = readU16LE(instData + 62);
        endPoint = readU16LE(instData + 64);
    } else {
        // Fallback / mock format
        dataOffset = std::min<size_t>(392, buf.size());
        if (buf.size() > 16) {
            const char* nPtr = reinterpret_cast<const char*>(buf.data() + 16);
            size_t nLen = strnlen(nPtr, std::min<size_t>(32, buf.size() - 16));
            if (nLen > 0) name.assign(nPtr, nLen);
        }
    }

    if (name.empty()) {
        name = std::filesystem::path(path).stem().string();
    }
    outInst.name = name;

    switch (playMode) {
        case 1: outInst.loop = Instrument::Loop::Forward; break;
        case 2: outInst.loop = Instrument::Loop::Backward; break;
        case 3: outInst.loop = Instrument::Loop::PingPong; break;
        default: outInst.loop = Instrument::Loop::None; break;
    }

    if (dataOffset < buf.size()) {
        size_t pcmBytes = buf.size() - dataOffset;
        size_t numSamples = pcmBytes / sizeof(int16_t);
        if (numSamples > 0) {
            const int16_t* pcm = reinterpret_cast<const int16_t*>(buf.data() + dataOffset);
            auto sample = std::make_shared<SampleData>();
            sample->channels = 1;
            sample->sampleRate = 44100;
            sample->ch[0].resize(numSamples);
            for (size_t i = 0; i < numSamples; ++i) {
                sample->ch[0][i] = static_cast<float>(pcm[i]) / 32768.0f;
            }
            outInst.sample = sample;

            if (endPoint > 0) {
                outInst.startFrame = static_cast<int>((static_cast<uint64_t>(startPoint) * numSamples) / 10000);
                outInst.loopStart = static_cast<int>((static_cast<uint64_t>(loopPoint1) * numSamples) / 10000);
                outInst.loopEnd = static_cast<int>((static_cast<uint64_t>(loopPoint2) * numSamples) / 10000);
                outInst.endFrame = static_cast<int>((static_cast<uint64_t>(endPoint) * numSamples) / 10000);
            }
        }
    }

    return true;
}

#pragma pack(push, 1)
struct MtpStep {
    int8_t note;
    uint8_t instrument;
    struct {
        uint8_t type;
        uint8_t value;
    } fx[2];
    int8_t volume;
    int8_t micromove;
};

struct MtpTrack {
    uint8_t length;
    MtpStep step[128];
};
#pragma pack(pop)

bool loadMtpPattern(const std::string& path, Pattern& outPat) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return false;

    file.seekg(0, std::ios::end);
    std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    constexpr size_t kTrackData8 = sizeof(MtpTrack) * 8;
    if (fileSize < static_cast<std::streamsize>(24 + kTrackData8)) {
        return false;
    }

    // Polyend Tracker CE firmware uses 28-byte header; mock/legacy uses 24-byte
    size_t headerSize = (fileSize >= static_cast<std::streamsize>(28 + kTrackData8)) ? 28 : 24;
    file.seekg(headerSize, std::ios::beg);

    std::vector<MtpTrack> tracks(8);
    file.read(reinterpret_cast<char*>(tracks.data()), kTrackData8);

    outPat.rows = 64;
    outPat.tracks.resize(8);
    for (int t = 0; t < 8; ++t) {
        int trackLen = std::clamp<int>(tracks[t].length + 1, 1, 128);
        outPat.rows = std::max(outPat.rows, trackLen);
        outPat.tracks[t].resize(outPat.rows);

        for (int r = 0; r < trackLen && r < 128; ++r) {
            const auto& src = tracks[t].step[r];
            auto& dst = outPat.tracks[t][r];

            if (src.note == -1) dst.note = kNoNote;
            else if (src.note <= -2) dst.note = kNoteOff;
            else dst.note = src.note;

            // In Polyend Tracker, instrument is 0-indexed (0 = Instrument 1)
            // When there is a note, instrument slot is src.instrument
            if (src.note >= 0 && src.instrument < 48) {
                dst.instrument = static_cast<int>(src.instrument);
            } else {
                dst.instrument = -1;
            }

            dst.volume = (src.volume >= 0) ? src.volume : -1;
            dst.fx[0].type = src.fx[0].type;
            dst.fx[0].value = src.fx[0].value;
            dst.fx[1].type = src.fx[1].type;
            dst.fx[1].value = src.fx[1].value;
        }
    }

    return true;
}

static bool loadProjectMt(const std::string& path, Song& outSong) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return false;

    std::vector<uint8_t> data(16 + 256);
    file.read(reinterpret_cast<char*>(data.data()), data.size());
    if (file.gcount() < 16 + 1) return false;

    if (data[0] != 'M' || data[1] != 'T') return false;
    if (data[2] >= 10) {
        outSong.bpm = static_cast<float>(data[2]);
    }

    // playlist starts at offset 16 (uint8_t playlist[255])
    const uint8_t* playlist = data.data() + 16;
    outSong.order.clear();
    for (int i = 0; i < 255; ++i) {
        uint8_t pat1Based = playlist[i];
        if (pat1Based == 0 || pat1Based == 255) break;
        outSong.order.push_back(static_cast<int>(pat1Based - 1));
    }
    return !outSong.order.empty();
}

bool loadPolyendProject(const std::string& projectDirOrFile, Song& outSong) {
    std::filesystem::path root(projectDirOrFile);
    if (std::filesystem::is_regular_file(root)) {
        root = root.parent_path();
    }

    if (!std::filesystem::exists(root) || !std::filesystem::is_directory(root)) {
        return false;
    }

    outSong = makeDefaultSong();

    // 1. Try reading project.mt if present
    std::filesystem::path projectMt = root / "project.mt";
    bool hasProjectMt = false;
    if (std::filesystem::exists(projectMt)) {
        hasProjectMt = loadProjectMt(projectMt.string(), outSong);
    }

    // 2. Scan for pattern files (.mtp)
    std::vector<std::filesystem::path> patFiles;
    for (const auto& entry : std::filesystem::directory_iterator(root)) {
        if (entry.path().extension() == ".mtp") {
            patFiles.push_back(entry.path());
        }
    }

    std::filesystem::path patternsDir = root / "patterns";
    if (std::filesystem::exists(patternsDir) && std::filesystem::is_directory(patternsDir)) {
        for (const auto& entry : std::filesystem::directory_iterator(patternsDir)) {
            if (entry.path().extension() == ".mtp") {
                patFiles.push_back(entry.path());
            }
        }
    }

    std::sort(patFiles.begin(), patFiles.end());
    if (!patFiles.empty()) {
        outSong.patterns.clear();
        for (const auto& pf : patFiles) {
            Pattern pat;
            if (loadMtpPattern(pf.string(), pat)) {
                outSong.patterns.push_back(std::move(pat));
            }
        }
        if (outSong.patterns.empty()) {
            outSong.patterns.push_back(makeDefaultSong().patterns[0]);
        }
        if (!hasProjectMt) {
            outSong.order.clear();
            for (size_t i = 0; i < outSong.patterns.size(); ++i) {
                outSong.order.push_back(static_cast<int>(i));
            }
        }
    }

    // 3. Scan for instruments (.pti or .wav)
    std::vector<std::filesystem::path> instFiles;
    std::filesystem::path instDir = root / "instruments";
    if (std::filesystem::exists(instDir) && std::filesystem::is_directory(instDir)) {
        for (const auto& entry : std::filesystem::directory_iterator(instDir)) {
            std::string ext = entry.path().extension().string();
            if (ext == ".pti" || ext == ".wav") {
                instFiles.push_back(entry.path());
            }
        }
    }

    for (const auto& entry : std::filesystem::directory_iterator(root)) {
        std::string ext = entry.path().extension().string();
        if (ext == ".pti" || ext == ".wav") {
            instFiles.push_back(entry.path());
        }
    }

    std::sort(instFiles.begin(), instFiles.end());
    if (!instFiles.empty()) {
        outSong.instruments.clear();
        for (const auto& inf : instFiles) {
            Instrument inst;
            if (inf.extension() == ".pti") {
                if (loadPtiInstrument(inf.string(), inst)) {
                    outSong.instruments.push_back(std::move(inst));
                }
            } else if (inf.extension() == ".wav") {
                auto sdata = std::make_shared<SampleData>();
                if (loadAudioFile(inf.string(), *sdata)) {
                    inst.name = inf.stem().string();
                    inst.sample = sdata;
                    outSong.instruments.push_back(std::move(inst));
                }
            }
        }
    }

    return true;
}

} // namespace trakr::io

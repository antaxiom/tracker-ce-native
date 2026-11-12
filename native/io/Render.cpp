#include "Render.h"
#include "Engine.h"
#include <filesystem>

#define DR_WAV_IMPLEMENTATION
#include <dr_wav.h>

#include <vector>
#include <algorithm>
#include <cmath>

namespace trakr {

bool renderToWav(const Song& song,
                 const std::string& filepath,
                 int sampleRate,
                 int bitDepth) {
    if (sampleRate <= 0 || (bitDepth != 16 && bitDepth != 24 && bitDepth != 32)) {
        return false;
    }

    // Calculate total song frames
    int totalRows = 0;
    for (int patIdx : song.order) {
        if (patIdx >= 0 && patIdx < static_cast<int>(song.patterns.size())) {
            totalRows += song.patterns[patIdx].rows;
        }
    }
    if (totalRows <= 0) totalRows = 64;

    const double seconds = (static_cast<double>(totalRows) * 60.0) / (song.bpm * song.lpb);
    const size_t totalFrames = static_cast<size_t>(std::ceil(seconds * sampleRate));

    // Initialize dr_wav writer
    drwav_data_format format{};
    format.container = drwav_container_riff;
    format.format = (bitDepth == 32) ? DR_WAVE_FORMAT_IEEE_FLOAT : DR_WAVE_FORMAT_PCM;
    format.channels = 2;
    format.sampleRate = static_cast<drwav_uint32>(sampleRate);
    format.bitsPerSample = static_cast<drwav_uint32>(bitDepth);

    drwav wav{};
    if (!drwav_init_file_write(&wav, filepath.c_str(), &format, nullptr)) {
        return false;
    }

    // Initialize headless engine
    Engine engine(sampleRate);
    engine.setSongSnapshot(makeSnapshot(song));
    engine.play(0, 0);

    constexpr size_t kBlockFrames = 256;
    std::vector<float> floatStereo(kBlockFrames * 2, 0.0f);
    std::vector<int16_t> pcm16(kBlockFrames * 2, 0);

    size_t framesWritten = 0;
    while (framesWritten < totalFrames) {
        const size_t chunk = std::min(kBlockFrames, totalFrames - framesWritten);
        engine.process(floatStereo.data(), static_cast<int>(chunk));

        if (bitDepth == 16) {
            for (size_t i = 0; i < chunk * 2; ++i) {
                float s = std::clamp(floatStereo[i], -1.0f, 1.0f);
                pcm16[i] = static_cast<int16_t>(s * 32767.0f);
            }
            drwav_write_pcm_frames(&wav, chunk, pcm16.data());
        } else if (bitDepth == 24) {
            std::vector<uint8_t> pcm24(chunk * 2 * 3);
            for (size_t i = 0; i < chunk * 2; ++i) {
                int32_t val = static_cast<int32_t>(std::clamp(floatStereo[i], -1.0f, 1.0f) * 8388607.0f);
                pcm24[i * 3 + 0] = static_cast<uint8_t>(val & 0xFF);
                pcm24[i * 3 + 1] = static_cast<uint8_t>((val >> 8) & 0xFF);
                pcm24[i * 3 + 2] = static_cast<uint8_t>((val >> 16) & 0xFF);
            }
            drwav_write_raw(&wav, pcm24.size(), pcm24.data());
        } else if (bitDepth == 32) {
            drwav_write_pcm_frames(&wav, chunk, floatStereo.data());
        }

        framesWritten += chunk;
    }

    drwav_uninit(&wav);
    return true;
}

bool renderStemsToWav(const Song& song,
                      const std::string& outputDir,
                      int sampleRate,
                      int bitDepth) {
    namespace fs = std::filesystem;
    std::error_code ec;
    fs::create_directories(outputDir, ec);

    // 1. Render Master
    std::string masterPath = (fs::path(outputDir) / "Master.wav").string();
    if (!renderToWav(song, masterPath, sampleRate, bitDepth)) return false;

    // 2. Render each track soloed
    for (size_t t = 0; t < song.tracks.size(); ++t) {
        Song soloSong = song;
        for (size_t other = 0; other < soloSong.tracks.size(); ++other) {
            soloSong.tracks[other].mute = (other != t);
        }
        char filename[64];
        snprintf(filename, sizeof(filename), "Track_%02zu_%s.wav", t + 1, song.tracks[t].name.c_str());
        std::string trackPath = (fs::path(outputDir) / filename).string();
        renderToWav(soloSong, trackPath, sampleRate, bitDepth);
    }
    return true;
}

} // namespace trakr


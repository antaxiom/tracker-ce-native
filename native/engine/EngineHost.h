#pragma once

#include "Engine.h"
#include "AudioBackend.h"
#include <memory>
#include <atomic>
#include <mutex>

namespace trakr {

class EngineHost {
public:
    explicit EngineHost(std::unique_ptr<AudioBackend> backend = nullptr);
    ~EngineHost();

    bool start(int sampleRate = 44100, int bufferFrames = 128);
    void stop();
    bool isRunning() const;

    void publish(const Song& song);
    void play(int orderIndex = 0, int row = 0);
    void stopPlayback();
    void setPosition(int orderIndex, int row);

    void setPatternLoop(bool loop);
    bool isPatternLoop() const;

    void setPerformanceState(const PerformanceState& state);
    PerformanceState getPerformanceState() const;

    void allNotesOff();
    void previewNoteOn(int track, int instrument, int note, float velocity = 1.0f);
    void previewNoteOff(int track = -1);
    bool isPreviewVoiceActive() const { return engine_.isPreviewVoiceActive(); }

    PlayheadPosition getPlayhead() const;
    MeterLevels getTrackMeters(size_t trackIndex) const;
    MeterLevels getMasterMeters() const;
    MeterLevels getAuxMeters(int aux) const;

    Engine& getEngine();

    // Audio input monitoring (Global Mixer Line In). pushInputFrames is called
    // from the capture callback; setAudioInputEnabled opens/closes the capture
    // device (backends without input report false and stay silent).
    void pushInputFrames(const float* interleavedStereo, int frames);
    bool setAudioInputEnabled(bool enabled);

private:
    std::unique_ptr<AudioBackend> backend_;
    Engine engine_;
    int sampleRate_{44100};
    int bufferFrames_{128};
    std::atomic<bool> inputEnabled_{false};
};

} // namespace trakr


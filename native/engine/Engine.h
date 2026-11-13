#pragma once

#include "Song.h"
#include "Voice.h"
#include "WavetableVoice.h"
#include "ChipSynthVoice.h"
#include "Mixer.h"
#include "StepFx.h"

#include <vector>
#include <memory>
#include <atomic>

namespace trakr {

struct PlayheadPosition {
    int orderIndex{0};
    int patternIndex{0};
    int row{0};
    bool playing{false};
    // Song time since transport start, sample-clock derived (cSongTimer).
    double songSeconds{0.0};
};

struct PerformanceState {
    float delaySendBoost{0.0f};
    float reverbSendBoost{0.0f};
    float lowpassCutoffMultiplier{1.0f};
    float highpassCutoffHz{20.0f};
    float pitchShiftSemitones{0.0f};
    int bitCrushBits{16};
    int repeatRoll{0};
    int playMode{0};
};

struct LiveEvent {
    enum class Type {
        None,
        NoteOn,
        NoteOff,
        Play,
        Stop,
        SetPosition,
        SetPerformance
    } type{Type::None};

    int track{0};
    int instrument{0};
    int note{60};
    float velocity{1.0f};
    int orderIndex{0};
    int row{0};
    PerformanceState perfState{};
};

class Engine {
public:
    explicit Engine(int sampleRate = 44100);
    ~Engine();

    void init(int sampleRate);
    void setSongSnapshot(std::shared_ptr<const SongSnapshot> snapshot);

    // Audio thread methods
    void process(float* outStereo, int frames);
    void pushLiveEvent(const LiveEvent& ev);

    // Transport controls
    void play(int startOrderIndex = 0, int startRow = 0);
    void stop();
    void allNotesOff();
    bool isPlaying() const;
    void setPatternLoop(bool loop) { loopPattern_.store(loop, std::memory_order_release); }
    bool isPatternLoop() const { return loopPattern_.load(std::memory_order_acquire); }

    void setPerformanceState(const PerformanceState& state);
    PerformanceState getPerformanceState() const { return perfState_; }

    PlayheadPosition getPlayhead() const;
    // Immediately publishes a transport position for the UI (used when
    // stop-then-rewind must be visible without waiting for the audio thread).
    void publishPosition(int orderIndex, int row);
    MeterLevels getTrackMeters(size_t trackIndex) const;
    MeterLevels getMasterMeters() const;
    MeterLevels getAuxMeters(int aux) const;
    bool isPreviewVoiceActive() const { return previewVoice_ && previewVoice_->isActive(); }

private:
    void triggerRow(const SongSnapshot& snapshot, const Pattern& pattern, int row);
    void handleLiveEvent(const SongSnapshot* snapshot, const LiveEvent& ev);
    void ensureVoiceType(size_t track, const Instrument* inst);
    void ensurePreviewVoiceType(const Instrument* inst);

    int sampleRate_{44100};
    std::shared_ptr<const SongSnapshot> snapshot_{nullptr};

    std::vector<std::unique_ptr<IVoice>> voices_;
    std::unique_ptr<IVoice> previewVoice_{nullptr};
    std::vector<float> previewBufL_;
    std::vector<float> previewBufR_;
    Mixer mixer_;

    // Per-track audio buffers for current block
    std::vector<std::vector<float>> trackBufferL_;
    std::vector<std::vector<float>> trackBufferR_;
    std::vector<float*> trackPtrL_;
    std::vector<float*> trackPtrR_;

    // Playback state
    bool playing_{false};
    std::atomic<bool> loopPattern_{false};
    PerformanceState perfState_{};
    int currentOrderIndex_{0};
    int currentRow_{0};
    double rowProgressFrames_{0.0};

    // Thread-safe published playhead
    std::atomic<int> publishedOrder_{0};
    std::atomic<int> publishedRow_{0};
    std::atomic<bool> publishedPlaying_{false};
    std::atomic<double> publishedSongSeconds_{0.0};
    double songSeconds_{0.0};

    // Live event ring buffer (lock-free single producer / single consumer)
    static constexpr size_t kEventQueueSize = 64;
    std::array<LiveEvent, kEventQueueSize> eventQueue_{};
    std::atomic<size_t> eventWriteIndex_{0};
    std::atomic<size_t> eventReadIndex_{0};
};

} // namespace trakr

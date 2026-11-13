#include "Engine.h"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace trakr {

Engine::Engine(int sampleRate)
    : sampleRate_(sampleRate) {
    init(sampleRate);
}

Engine::~Engine() {
    stop();
}

void Engine::init(int sampleRate) {
    sampleRate_ = std::max(8000, sampleRate);

    constexpr size_t kMaxTracks = 64;
    constexpr size_t kMaxFrames = 4096;

    voices_.clear();
    trackBufferL_.resize(kMaxTracks);
    trackBufferR_.resize(kMaxTracks);
    trackPtrL_.resize(kMaxTracks);
    trackPtrR_.resize(kMaxTracks);

    for (size_t t = 0; t < kMaxTracks; ++t) {
        voices_.push_back(std::make_unique<SampleVoice>(sampleRate_));
        trackBufferL_[t].resize(kMaxFrames, 0.0f);
        trackBufferR_[t].resize(kMaxFrames, 0.0f);
        trackPtrL_[t] = trackBufferL_[t].data();
        trackPtrR_[t] = trackBufferR_[t].data();
    }

    previewVoice_ = std::make_unique<SampleVoice>(sampleRate_);
    previewBufL_.resize(kMaxFrames, 0.0f);
    previewBufR_.resize(kMaxFrames, 0.0f);

    mixer_.init(static_cast<float>(sampleRate_));
    mixer_.resizeTracks(kMaxTracks);
}

void Engine::setSongSnapshot(std::shared_ptr<const SongSnapshot> snapshot) {
    std::atomic_store(&snapshot_, snapshot);
}

void Engine::pushLiveEvent(const LiveEvent& ev) {
    const size_t currentWrite = eventWriteIndex_.load(std::memory_order_relaxed);
    const size_t nextWrite = (currentWrite + 1) % kEventQueueSize;
    if (nextWrite != eventReadIndex_.load(std::memory_order_acquire)) {
        eventQueue_[currentWrite] = ev;
        eventWriteIndex_.store(nextWrite, std::memory_order_release);
    }
}

void Engine::handleLiveEvent(const SongSnapshot* snapshot, const LiveEvent& ev) {
    switch (ev.type) {
        case LiveEvent::Type::Play:
            play(ev.orderIndex, ev.row);
            break;
        case LiveEvent::Type::Stop:
            stop();
            break;
        case LiveEvent::Type::SetPosition:
            currentOrderIndex_ = ev.orderIndex;
            currentRow_ = ev.row;
            rowProgressFrames_ = 0.0;
            // Publish even while stopped so the transport display reflects the
            // new cursor immediately (stop-then-rewind returns to the base row).
            publishedOrder_.store(currentOrderIndex_, std::memory_order_relaxed);
            publishedRow_.store(currentRow_, std::memory_order_relaxed);
            break;
        case LiveEvent::Type::NoteOn: {
            if (ev.track == -1) {
                if (snapshot) {
                    const Instrument* inst = (ev.instrument >= 0 && ev.instrument < static_cast<int>(snapshot->instruments.size()))
                        ? &snapshot->instruments[ev.instrument]
                        : nullptr;
                    ensurePreviewVoiceType(inst);
                    if (previewVoice_) {
                        previewVoice_->noteOn(ev.note, ev.velocity, inst);
                    }
                }
            } else if (ev.track >= 0 && ev.track < static_cast<int>(voices_.size()) && snapshot) {
                const Instrument* inst = (ev.instrument >= 0 && ev.instrument < static_cast<int>(snapshot->instruments.size()))
                    ? &snapshot->instruments[ev.instrument]
                    : nullptr;
                ensureVoiceType(ev.track, inst);
                voices_[ev.track]->noteOn(ev.note, ev.velocity, inst);
            }
            break;
        }
        case LiveEvent::Type::NoteOff: {
            if (ev.track == -1) {
                if (previewVoice_) previewVoice_->noteOff();
            } else if (ev.track >= 0 && ev.track < static_cast<int>(voices_.size())) {
                voices_[ev.track]->noteOff();
            }
            break;
        }
        case LiveEvent::Type::SetPerformance: {
            perfState_ = ev.perfState;
            mixer_.setBitCrush(perfState_.bitCrushBits);
            for (auto& v : voices_) {
                if (!v) continue;
                if (perfState_.pitchShiftSemitones != 0.0f) {
                    v->setPitchRatio(std::pow(2.0f, perfState_.pitchShiftSemitones / 12.0f));
                } else {
                    v->setPitchRatio(1.0f);
                }
                if (perfState_.lowpassCutoffMultiplier < 0.99f) {
                    v->setCutoff(std::clamp(20000.0f * perfState_.lowpassCutoffMultiplier, 100.0f, 20000.0f));
                }
                v->setReverse(perfState_.playMode == 1);
            }
            break;
        }
        case LiveEvent::Type::None:
            break;
    }
}

void Engine::setPerformanceState(const PerformanceState& state) {
    LiveEvent ev;
    ev.type = LiveEvent::Type::SetPerformance;
    ev.perfState = state;
    pushLiveEvent(ev);
}

void Engine::play(int startOrderIndex, int startRow) {
    currentOrderIndex_ = startOrderIndex;
    currentRow_ = startRow;
    rowProgressFrames_ = 0.0;
    playing_ = true;
    publishedPlaying_.store(true, std::memory_order_release);

    auto snap = std::atomic_load(&snapshot_);
    if (snap && currentOrderIndex_ < static_cast<int>(snap->order.size())) {
        int patIdx = snap->order[currentOrderIndex_];
        if (patIdx >= 0 && patIdx < static_cast<int>(snap->patterns.size())) {
            triggerRow(*snap, snap->patterns[patIdx], currentRow_);
        }
    }
}

void Engine::stop() {
    playing_ = false;
    publishedPlaying_.store(false, std::memory_order_release);
    allNotesOff();
}

void Engine::allNotesOff() {
    for (auto& voice : voices_) {
        if (voice) voice->stop();
    }
    if (previewVoice_) previewVoice_->stop();
}

bool Engine::isPlaying() const {
    return publishedPlaying_.load(std::memory_order_acquire);
}

void Engine::publishPosition(int orderIndex, int row) {
    publishedOrder_.store(orderIndex, std::memory_order_relaxed);
    publishedRow_.store(row, std::memory_order_relaxed);
}

PlayheadPosition Engine::getPlayhead() const {
    PlayheadPosition pos;
    pos.orderIndex = publishedOrder_.load(std::memory_order_relaxed);
    pos.row = publishedRow_.load(std::memory_order_relaxed);
    pos.playing = publishedPlaying_.load(std::memory_order_relaxed);
    return pos;
}

MeterLevels Engine::getTrackMeters(size_t trackIndex) const {
    return mixer_.getTrackMeters(trackIndex);
}

MeterLevels Engine::getMasterMeters() const {
    return mixer_.getMasterMeters();
}

void Engine::triggerRow(const SongSnapshot& snapshot, const Pattern& pattern, int row) {
    if (row < 0 || row >= pattern.rows) return;

    publishedOrder_.store(currentOrderIndex_, std::memory_order_relaxed);
    publishedRow_.store(row, std::memory_order_relaxed);

    const double framesPerRow = (sampleRate_ * 60.0) / (snapshot.bpm * snapshot.lpb);

    for (size_t t = 0; t < pattern.tracks.size() && t < voices_.size(); ++t) {
        if (row >= static_cast<int>(pattern.tracks[t].size())) continue;
        const Cell& cell = pattern.tracks[t][row];

        // Chance / Probability evaluation (0..100%)
        bool skipCell = false;
        for (const auto& fx : cell.fx) {
            if (fx.type == FxType::Chance && fx.value < 100) {
                if (fx.value == 0 || (std::rand() % 100) >= fx.value) {
                    skipCell = true;
                    break;
                }
            }
        }
        if (skipCell) continue;

        if (cell.note == kNoteOff) {
            voices_[t]->noteOff();
        } else if (cell.note >= 0) {
            const Instrument* inst = (cell.instrument >= 0 && cell.instrument < static_cast<int>(snapshot.instruments.size()))
                ? &snapshot.instruments[cell.instrument]
                : nullptr;
            ensureVoiceType(t, inst);
            voices_[t]->noteOn(cell.note, 1.0f, inst);
            if (perfState_.pitchShiftSemitones != 0.0f) {
                voices_[t]->setPitchRatio(std::pow(2.0f, perfState_.pitchShiftSemitones / 12.0f));
            }
            if (perfState_.lowpassCutoffMultiplier < 0.99f) {
                voices_[t]->setCutoff(std::clamp(20000.0f * perfState_.lowpassCutoffMultiplier, 100.0f, 20000.0f));
            }
            if (perfState_.repeatRoll > 0) {
                voices_[t]->setRetrigger(static_cast<int>(framesPerRow / perfState_.repeatRoll));
            }
            voices_[t]->setReverse(perfState_.playMode == 1);
        }

        if (cell.volume >= 0) {
            const Instrument* inst = (cell.instrument >= 0 && cell.instrument < static_cast<int>(snapshot.instruments.size()))
                ? &snapshot.instruments[cell.instrument] : nullptr;
            float volDb = inst ? inst->gainDb : 0.0f;
            if (inst && inst->volume != 50) volDb += (inst->volume == 0) ? -100.0f : ((inst->volume - 50) * 0.48f);
            float dbGain = (volDb <= -99.0f) ? 0.0f : std::pow(10.0f, volDb / 20.0f);
            voices_[t]->setVolume((std::clamp<int>(cell.volume, 0, 64) / 64.0f) * dbGain);
        }

        for (const auto& fx : cell.fx) {
            StepFxRegistry::instance().applyFx(fx.type, fx.value, voices_[t].get(), static_cast<int>(framesPerRow));
        }
    }
}

void Engine::process(float* outStereo, int frames) {
    if (!outStereo || frames <= 0) return;

    auto snap = std::atomic_load(&snapshot_);

    // 1. Drain live events
    const size_t targetWrite = eventWriteIndex_.load(std::memory_order_acquire);
    size_t readIdx = eventReadIndex_.load(std::memory_order_relaxed);
    while (readIdx != targetWrite) {
        handleLiveEvent(snap.get(), eventQueue_[readIdx]);
        readIdx = (readIdx + 1) % kEventQueueSize;
        eventReadIndex_.store(readIdx, std::memory_order_release);
    }

    if (!snap) {
        std::memset(outStereo, 0, frames * 2 * sizeof(float));
        return;
    }

    const size_t numTracks = std::min(snap->tracks.size(), voices_.size());
    const double framesPerRow = (sampleRate_ * 60.0) / (snap->bpm * snap->lpb);
    // Project "Extra headroom" (play_memory.cpp lowerVolume3dB).
    const float headroomGain = snap->headroom ? 0.7079458f : 1.0f;
    if (previewVoice_) previewVoice_->setHeadroomGain(headroomGain);

    int framesRendered = 0;
    while (framesRendered < frames) {
        int subFrames = frames - framesRendered;

        if (playing_ && !snap->order.empty()) {
            const double framesToNextRow = framesPerRow - rowProgressFrames_;
            const int framesToNextRowInt = static_cast<int>(std::ceil(framesToNextRow));
            subFrames = std::min(subFrames, std::max(1, framesToNextRowInt));
        }

        // Render voices for subFrames
        for (size_t t = 0; t < numTracks; ++t) {
            std::memset(trackBufferL_[t].data(), 0, subFrames * sizeof(float));
            std::memset(trackBufferR_[t].data(), 0, subFrames * sizeof(float));
            voices_[t]->setHeadroomGain(headroomGain);
            if (voices_[t]->isActive()) {
                voices_[t]->process(trackBufferL_[t].data(), trackBufferR_[t].data(), subFrames);
            }
        }

        // Run mixer on subFrames into target buffer offset
        mixer_.process(snap->tracks,
                       trackPtrL_.data(),
                       trackPtrR_.data(),
                       outStereo + (framesRendered * 2),
                       subFrames,
                       snap->masterGainDb,
                       snap->masterPan,
                       perfState_.delaySendBoost,
                       perfState_.reverbSendBoost,
                       snap->masterEq,
                       snap->masterSaturationDrive,
                       snap->masterBassBoost,
                       snap->masterSpace);

        // Mix isolated preview voice into master output if active
        if (previewVoice_ && previewVoice_->isActive()) {
            std::memset(previewBufL_.data(), 0, subFrames * sizeof(float));
            std::memset(previewBufR_.data(), 0, subFrames * sizeof(float));
            previewVoice_->process(previewBufL_.data(), previewBufR_.data(), subFrames);
            for (int i = 0; i < subFrames; ++i) {
                outStereo[(framesRendered + i) * 2] += previewBufL_[i];
                outStereo[(framesRendered + i) * 2 + 1] += previewBufR_[i];
            }
        }

        framesRendered += subFrames;

        if (playing_ && !snap->order.empty()) {
            rowProgressFrames_ += subFrames;
            if (rowProgressFrames_ >= framesPerRow) {
                rowProgressFrames_ -= framesPerRow;

                int patIdx = snap->order[currentOrderIndex_];
                if (patIdx >= 0 && patIdx < static_cast<int>(snap->patterns.size())) {
                    const Pattern& pat = snap->patterns[patIdx];
                    currentRow_++;
                    if (currentRow_ >= pat.rows) {
                        currentRow_ = 0;
                        if (!loopPattern_.load(std::memory_order_relaxed)) {
                            currentOrderIndex_ = (currentOrderIndex_ + 1) % snap->order.size();
                        }
                    }
                    int nextPatIdx = snap->order[currentOrderIndex_];
                    if (nextPatIdx >= 0 && nextPatIdx < static_cast<int>(snap->patterns.size())) {
                        triggerRow(*snap, snap->patterns[nextPatIdx], currentRow_);
                    }
                }
            }
        }
    }
}

void Engine::ensureVoiceType(size_t track, const Instrument* inst) {
    if (track >= voices_.size()) return;
    if (inst && inst->isSynth) {
        if (!dynamic_cast<ChipSynthVoice*>(voices_[track].get())) {
            voices_[track] = std::make_unique<ChipSynthVoice>(sampleRate_);
        }
    } else if (inst && (inst->loop == Instrument::Loop::Wavetable || inst->loop == Instrument::Loop::Granular)) {
        if (!dynamic_cast<WavetableVoice*>(voices_[track].get())) {
            voices_[track] = std::make_unique<WavetableVoice>(sampleRate_);
        }
    } else {
        if (!dynamic_cast<SampleVoice*>(voices_[track].get())) {
            voices_[track] = std::make_unique<SampleVoice>(sampleRate_);
        }
    }
}

void Engine::ensurePreviewVoiceType(const Instrument* inst) {
    if (inst && inst->isSynth) {
        if (!dynamic_cast<ChipSynthVoice*>(previewVoice_.get())) {
            previewVoice_ = std::make_unique<ChipSynthVoice>(sampleRate_);
        }
    } else if (inst && (inst->loop == Instrument::Loop::Wavetable || inst->loop == Instrument::Loop::Granular)) {
        if (!dynamic_cast<WavetableVoice*>(previewVoice_.get())) {
            previewVoice_ = std::make_unique<WavetableVoice>(sampleRate_);
        }
    } else {
        if (!dynamic_cast<SampleVoice*>(previewVoice_.get())) {
            previewVoice_ = std::make_unique<SampleVoice>(sampleRate_);
        }
    }
}

} // namespace trakr

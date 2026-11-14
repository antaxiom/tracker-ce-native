#include "EngineHost.h"

namespace trakr {

EngineHost::EngineHost(std::unique_ptr<AudioBackend> backend)
    : backend_(std::move(backend)) {
    if (!backend_) {
        backend_ = std::make_unique<NullAudioBackend>();
    }
}

EngineHost::~EngineHost() {
    stop();
}

bool EngineHost::start(int sampleRate, int bufferFrames) {
    sampleRate_ = sampleRate;
    bufferFrames_ = bufferFrames;

    engine_.init(sampleRate_);

    if (!backend_->open(sampleRate_, bufferFrames_, [this](float* outStereo, int frames) {
        engine_.process(outStereo, frames);
    })) {
        return false;
    }

    backend_->setInputCallback([this](const float* interleaved, int frames) {
        engine_.pushInput(interleaved, frames);
    });

    return backend_->start();
}

void EngineHost::stop() {
    if (backend_) {
        backend_->stop();
        backend_->close();
    }
    engine_.stop();
}

bool EngineHost::isRunning() const {
    return backend_ && backend_->isRunning();
}

void EngineHost::pushInputFrames(const float* interleavedStereo, int frames) {
    engine_.pushInput(interleavedStereo, frames);
}

bool EngineHost::setAudioInputEnabled(bool enabled) {
    if (!backend_) return false;
    if (enabled == inputEnabled_.load(std::memory_order_acquire)) return enabled;
    const bool ok = backend_->setInputEnabled(enabled);
    inputEnabled_.store(ok && enabled, std::memory_order_release);
    return ok;
}

void EngineHost::publish(const Song& song) {
    auto snapshot = makeSnapshot(song);
    engine_.setSongSnapshot(snapshot);
    // Global Mixer "Line In": capture runs only while the bus is unmuted, so
    // the OS input device is never opened for a normal (muted) project.
    setAudioInputEnabled(!song.auxMute[3]);
}

void EngineHost::play(int orderIndex, int row) {
    LiveEvent ev;
    ev.type = LiveEvent::Type::Play;
    ev.orderIndex = orderIndex;
    ev.row = row;
    engine_.pushLiveEvent(ev);
}

void EngineHost::stopPlayback() {
    LiveEvent ev;
    ev.type = LiveEvent::Type::Stop;
    engine_.pushLiveEvent(ev);
}

void EngineHost::setPosition(int orderIndex, int row) {
    engine_.publishPosition(orderIndex, row);
    LiveEvent ev;
    ev.type = LiveEvent::Type::SetPosition;
    ev.orderIndex = orderIndex;
    ev.row = row;
    engine_.pushLiveEvent(ev);
}

void EngineHost::setPatternLoop(bool loop) {
    engine_.setPatternLoop(loop);
}

bool EngineHost::isPatternLoop() const {
    return engine_.isPatternLoop();
}

void EngineHost::setPerformanceState(const PerformanceState& state) {
    engine_.setPerformanceState(state);
}

PerformanceState EngineHost::getPerformanceState() const {
    return engine_.getPerformanceState();
}

void EngineHost::allNotesOff() {
    engine_.allNotesOff();
}

void EngineHost::previewNoteOn(int track, int instrument, int note, float velocity) {
    LiveEvent ev;
    ev.type = LiveEvent::Type::NoteOn;
    ev.track = track;
    ev.instrument = instrument;
    ev.note = note;
    ev.velocity = velocity;
    engine_.pushLiveEvent(ev);
}

void EngineHost::previewNoteOff(int track) {
    LiveEvent ev;
    ev.type = LiveEvent::Type::NoteOff;
    ev.track = track;
    engine_.pushLiveEvent(ev);
}

PlayheadPosition EngineHost::getPlayhead() const {
    return engine_.getPlayhead();
}

MeterLevels EngineHost::getTrackMeters(size_t trackIndex) const {
    return engine_.getTrackMeters(trackIndex);
}

MeterLevels EngineHost::getAuxMeters(int aux) const {
    return engine_.getAuxMeters(aux);
}

MeterLevels EngineHost::getMasterMeters() const {
    return engine_.getMasterMeters();
}

Engine& EngineHost::getEngine() {
    return engine_;
}

} // namespace trakr


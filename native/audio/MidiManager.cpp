#include "MidiManager.h"
#include <RtMidi.h>

namespace trakr {

namespace {

static void midiCallback(double timeStamp, std::vector<unsigned char>* message, void* userData) {
    if (!message || message->empty() || !userData) return;
    auto* mgr = static_cast<MidiManager*>(userData);

    uint8_t status = (*message)[0];
    uint8_t type = status & 0xF0;
    uint8_t channel = status & 0x0F;

    MidiEvent ev;
    ev.channel = channel;
    ev.timestamp = timeStamp;

    if (type == 0x90 && message->size() >= 3) {
        ev.note = (*message)[1];
        ev.velocity = (*message)[2];
        ev.type = (ev.velocity > 0) ? MidiEvent::Type::NoteOn : MidiEvent::Type::NoteOff;
        mgr->pushEvent(ev);
    } else if (type == 0x80 && message->size() >= 3) {
        ev.note = (*message)[1];
        ev.velocity = (*message)[2];
        ev.type = MidiEvent::Type::NoteOff;
        mgr->pushEvent(ev);
    } else if (type == 0xB0 && message->size() >= 3) {
        ev.note = (*message)[1];
        ev.velocity = (*message)[2];
        ev.type = MidiEvent::Type::ControlChange;
        mgr->pushEvent(ev);
    }
}

} // namespace

MidiManager& MidiManager::instance() {
    static MidiManager mgr;
    return mgr;
}

MidiManager::MidiManager() {
    init();
}

MidiManager::~MidiManager() {
    shutdown();
}

bool MidiManager::init() {
    if (midiIn_) return true;
    try {
        midiIn_ = std::make_unique<RtMidiIn>();
        midiIn_->setCallback(&midiCallback, this);
        midiIn_->ignoreTypes(true, true, true);
        return true;
    } catch (...) {
        try {
            midiIn_ = std::make_unique<RtMidiIn>(RtMidi::RTMIDI_DUMMY);
            midiIn_->setCallback(&midiCallback, this);
            midiIn_->ignoreTypes(true, true, true);
            return true;
        } catch (...) {
            midiIn_.reset();
            return false;
        }
    }
}

void MidiManager::shutdown() {
    closePort();
    midiIn_.reset();
}

std::vector<std::string> MidiManager::getPortNames() {
    std::vector<std::string> ports;
    if (!midiIn_) return ports;
    try {
        unsigned int n = midiIn_->getPortCount();
        ports.reserve(n);
        for (unsigned int i = 0; i < n; ++i) {
            ports.push_back(midiIn_->getPortName(i));
        }
    } catch (...) {}
    return ports;
}

bool MidiManager::openPort(int portIndex) {
    if (!midiIn_ || portIndex < 0) return false;
    try {
        if (static_cast<unsigned int>(portIndex) >= midiIn_->getPortCount()) return false;
        closePort();
        midiIn_->openPort(static_cast<unsigned int>(portIndex));
        currentPort_ = portIndex;
        return true;
    } catch (...) {
        return false;
    }
}

void MidiManager::closePort() {
    if (midiIn_ && midiIn_->isPortOpen()) {
        try {
            midiIn_->closePort();
        } catch (...) {}
    }
    currentPort_ = -1;
}

bool MidiManager::isPortOpen() const {
    return midiIn_ && midiIn_->isPortOpen();
}

void MidiManager::pushEvent(const MidiEvent& event) {
    size_t w = writeIdx_.load(std::memory_order_relaxed);
    size_t nextW = (w + 1) % kQueueSize;
    if (nextW != readIdx_.load(std::memory_order_acquire)) {
        queue_[w] = event;
        writeIdx_.store(nextW, std::memory_order_release);
    }
}

bool MidiManager::pollEvent(MidiEvent& outEvent) {
    size_t r = readIdx_.load(std::memory_order_relaxed);
    if (r == writeIdx_.load(std::memory_order_acquire)) {
        return false;
    }
    outEvent = queue_[r];
    readIdx_.store((r + 1) % kQueueSize, std::memory_order_release);
    return true;
}

} // namespace trakr


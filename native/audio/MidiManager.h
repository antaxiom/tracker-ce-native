#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <atomic>
#include <memory>
#include <RtMidi.h>

namespace trakr {

struct MidiEvent {
    enum class Type : uint8_t {
        NoteOn = 0x90,
        NoteOff = 0x80,
        ControlChange = 0xB0,
        PitchBend = 0xE0,
        Unknown = 0
    };

    Type type{Type::Unknown};
    uint8_t channel{0};
    uint8_t note{0};
    uint8_t velocity{0};
    double timestamp{0.0};
};

class MidiManager {
public:
    static MidiManager& instance();

    bool init();
    void shutdown();

    std::vector<std::string> getPortNames();
    bool openPort(int portIndex);
    void closePort();
    bool isPortOpen() const;
    int getCurrentPort() const { return currentPort_; }

    // Pops next MIDI event from lock-free ring buffer
    bool pollEvent(MidiEvent& outEvent);

    // Push event (used by MIDI input callback)
    void pushEvent(const MidiEvent& event);

private:
    MidiManager();
    ~MidiManager();

    std::unique_ptr<RtMidiIn> midiIn_;
    int currentPort_{-1};

    static constexpr size_t kQueueSize = 256;
    MidiEvent queue_[kQueueSize];
    std::atomic<size_t> writeIdx_{0};
    std::atomic<size_t> readIdx_{0};
};

} // namespace trakr

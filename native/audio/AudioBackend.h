#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>

namespace trakr {

class AudioBackend {
public:
    using Callback = std::function<void(float* outStereo, int frames)>;

    virtual ~AudioBackend() = default;

    virtual bool open(int sampleRate, int bufferFrames, Callback callback) = 0;
    virtual void close() = 0;
    virtual bool start() = 0;
    virtual void stop() = 0;
    virtual bool isRunning() const = 0;
};

// Silent backend used by headless/screenshot/remote runs. It still pumps the
// engine callback at real-time pace so transport, playhead and meters behave
// like they do with a real device (no sound is produced).
class NullAudioBackend : public AudioBackend {
public:
    ~NullAudioBackend() override;

    bool open(int sampleRate, int bufferFrames, Callback callback) override;
    void close() override;
    bool start() override;
    void stop() override;
    bool isRunning() const override;

private:
    void threadLoop();

    std::atomic<bool> running_{false};
    int sampleRate_{44100};
    int bufferFrames_{256};
    Callback callback_;
    std::thread thread_;
    std::mutex lifecycle_;
    std::mutex cbMutex_;
    std::mutex mutex_;
    std::condition_variable cv_;
};

} // namespace trakr

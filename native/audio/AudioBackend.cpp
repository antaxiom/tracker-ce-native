#include "AudioBackend.h"

#include <chrono>
#include <utility>
#include <vector>

namespace trakr {

NullAudioBackend::~NullAudioBackend() {
    stop();
}

bool NullAudioBackend::open(int sampleRate, int bufferFrames, Callback callback) {
    std::lock_guard<std::mutex> lock(lifecycle_);
    if (running_.load()) return false;
    sampleRate_ = (sampleRate > 0) ? sampleRate : 44100;
    bufferFrames_ = (bufferFrames > 0) ? bufferFrames : 256;
    {
        std::lock_guard<std::mutex> cbLock(cbMutex_);
        callback_ = std::move(callback);
    }
    return true;
}

void NullAudioBackend::close() {
    stop();
    std::lock_guard<std::mutex> cbLock(cbMutex_);
    callback_ = nullptr;
}

bool NullAudioBackend::start() {
    std::lock_guard<std::mutex> lock(lifecycle_);
    if (running_.load()) return true;
    if (thread_.joinable()) return false; // stop() must be called first
    running_.store(true);
    thread_ = std::thread(&NullAudioBackend::threadLoop, this);
    return true;
}

void NullAudioBackend::stop() {
    std::thread worker;
    {
        std::lock_guard<std::mutex> lock(lifecycle_);
        running_.store(false);
        if (thread_.joinable()) worker = std::move(thread_);
    }
    cv_.notify_all();
    if (worker.joinable()) worker.join();
}

bool NullAudioBackend::isRunning() const {
    return running_.load();
}

void NullAudioBackend::threadLoop() {
    using clock = std::chrono::steady_clock;
    const auto period = std::chrono::nanoseconds(
        static_cast<long long>(1'000'000'000.0 * bufferFrames_ / sampleRate_));
    std::vector<float> buffer(static_cast<size_t>(bufferFrames_) * 2, 0.0f);
    auto next = clock::now();

    while (running_.load()) {
        Callback cb;
        {
            std::lock_guard<std::mutex> cbLock(cbMutex_);
            cb = callback_; // copy so close()/open() never races the call
        }
        if (cb) cb(buffer.data(), bufferFrames_);

        next += period;
        auto now = clock::now();
        if (next < now) next = now; // drop missed deadlines instead of spinning
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait_until(lock, next, [this] { return !running_.load(); });
    }
}

} // namespace trakr

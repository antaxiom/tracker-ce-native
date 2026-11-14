#pragma once

#include "AudioBackend.h"
#include <memory>

struct ma_device;

namespace trakr {

class MiniaudioBackend : public AudioBackend {
public:
    MiniaudioBackend();
    ~MiniaudioBackend() override;

    bool open(int sampleRate, int bufferFrames, Callback callback) override;
    void close() override;
    bool start() override;
    void stop() override;
    bool isRunning() const override;

    void setInputCallback(InputCallback callback) override;
    bool setInputEnabled(bool enabled) override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace trakr


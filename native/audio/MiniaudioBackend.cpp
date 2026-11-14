#include "MiniaudioBackend.h"

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>
#include <cstring>

namespace trakr {

struct MiniaudioBackend::Impl {
    ma_device device{};
    bool initialized{false};
    bool running{false};
    Callback callback;

    // Line-in capture device (opened only when monitoring is enabled so the OS
    // never asks for microphone access unless the user unmutes Line In).
    ma_device captureDevice{};
    bool captureInitialized{false};
    bool captureRunning{false};
    int sampleRate{44100};
    InputCallback inputCallback;

    static void dataCallback(ma_device* pDevice, void* pOutput, const void* /*pInput*/, ma_uint32 frameCount) {
        auto* impl = static_cast<Impl*>(pDevice->pUserData);
        if (!impl || !pOutput) return;

        if (impl->callback) {
            impl->callback(static_cast<float*>(pOutput), static_cast<int>(frameCount));
        } else {
            std::memset(pOutput, 0, frameCount * 2 * sizeof(float));
        }
    }

    static void captureCallback(ma_device* pDevice, void* /*pOutput*/, const void* pInput,
                                ma_uint32 frameCount) {
        auto* impl = static_cast<Impl*>(pDevice->pUserData);
        if (!impl || !pInput || !impl->inputCallback) return;
        impl->inputCallback(static_cast<const float*>(pInput), static_cast<int>(frameCount));
    }
};

MiniaudioBackend::MiniaudioBackend()
    : impl_(std::make_unique<Impl>()) {}

MiniaudioBackend::~MiniaudioBackend() {
    close();
}

bool MiniaudioBackend::open(int sampleRate, int bufferFrames, Callback callback) {
    close();

    impl_->callback = std::move(callback);
    impl_->sampleRate = sampleRate;

    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format = ma_format_f32;
    config.playback.channels = 2;
    config.sampleRate = static_cast<ma_uint32>(sampleRate);
    config.periodSizeInFrames = static_cast<ma_uint32>(bufferFrames);
    config.dataCallback = Impl::dataCallback;
    config.pUserData = impl_.get();

    ma_result result = ma_device_init(nullptr, &config, &impl_->device);
    if (result != MA_SUCCESS) {
        return false;
    }

    impl_->initialized = true;
    return true;
}

void MiniaudioBackend::setInputCallback(InputCallback callback) {
    impl_->inputCallback = std::move(callback);
}

bool MiniaudioBackend::setInputEnabled(bool enabled) {
    if (!enabled) {
        if (impl_->captureInitialized) {
            ma_device_uninit(&impl_->captureDevice);
            impl_->captureInitialized = false;
            impl_->captureRunning = false;
        }
        return true;
    }
    if (impl_->captureInitialized) return true;
    if (impl_->inputCallback == nullptr) return false;

    ma_device_config config = ma_device_config_init(ma_device_type_capture);
    config.capture.format = ma_format_f32;
    config.capture.channels = 2;
    config.sampleRate = static_cast<ma_uint32>(impl_->sampleRate);
    config.dataCallback = Impl::captureCallback;
    config.pUserData = impl_.get();

    if (ma_device_init(nullptr, &config, &impl_->captureDevice) != MA_SUCCESS) {
        return false;
    }
    impl_->captureInitialized = true;
    if (ma_device_start(&impl_->captureDevice) != MA_SUCCESS) {
        ma_device_uninit(&impl_->captureDevice);
        impl_->captureInitialized = false;
        return false;
    }
    impl_->captureRunning = true;
    return true;
}

void MiniaudioBackend::close() {
    setInputEnabled(false);
    if (impl_->initialized) {
        stop();
        ma_device_uninit(&impl_->device);
        impl_->initialized = false;
    }
    impl_->callback = nullptr;
}

bool MiniaudioBackend::start() {
    if (!impl_->initialized) return false;
    if (impl_->running) return true;

    ma_result result = ma_device_start(&impl_->device);
    if (result == MA_SUCCESS) {
        impl_->running = true;
        return true;
    }
    return false;
}

void MiniaudioBackend::stop() {
    if (impl_->initialized && impl_->running) {
        ma_device_stop(&impl_->device);
        impl_->running = false;
    }
}

bool MiniaudioBackend::isRunning() const {
    return impl_->running;
}

} // namespace trakr


#include "SampleOps.h"
#include <algorithm>
#include <cmath>

namespace trakr {

static void copyBase(const SampleData& src, SampleData& dst) {
    dst.channels = src.channels;
    dst.sampleRate = src.sampleRate;
    for (int c = 0; c < src.channels && c < 2; ++c) {
        dst.ch[c] = src.ch[c];
    }
}

std::shared_ptr<SampleData> SampleOps::crop(const SampleData& src, int startFrame, int endFrame) {
    if (src.ch[0].empty()) return nullptr;
    int total = static_cast<int>(src.ch[0].size());
    int s = std::clamp(startFrame, 0, total - 1);
    int e = std::clamp(endFrame, s + 1, total);

    auto out = std::make_shared<SampleData>();
    out->channels = src.channels;
    out->sampleRate = src.sampleRate;
    for (int c = 0; c < src.channels && c < 2; ++c) {
        out->ch[c].assign(src.ch[c].begin() + s, src.ch[c].begin() + e);
    }
    return out;
}

std::shared_ptr<SampleData> SampleOps::reverse(const SampleData& src, int startFrame, int endFrame) {
    if (src.ch[0].empty()) return nullptr;
    auto out = std::make_shared<SampleData>();
    copyBase(src, *out);

    int total = static_cast<int>(src.ch[0].size());
    int s = std::clamp(startFrame, 0, total - 1);
    int e = std::clamp(endFrame, s + 1, total);

    for (int c = 0; c < src.channels && c < 2; ++c) {
        std::reverse(out->ch[c].begin() + s, out->ch[c].begin() + e);
    }
    return out;
}

std::shared_ptr<SampleData> SampleOps::normalize(const SampleData& src, int startFrame, int endFrame) {
    if (src.ch[0].empty()) return nullptr;
    auto out = std::make_shared<SampleData>();
    copyBase(src, *out);

    int total = static_cast<int>(src.ch[0].size());
    int s = std::clamp(startFrame, 0, total - 1);
    int e = std::clamp(endFrame, s + 1, total);

    float maxAmp = 0.0f;
    for (int c = 0; c < src.channels && c < 2; ++c) {
        for (int i = s; i < e; ++i) {
            float a = std::abs(out->ch[c][i]);
            if (a > maxAmp) maxAmp = a;
        }
    }

    if (maxAmp > 0.0001f) {
        float gain = 1.0f / maxAmp;
        for (int c = 0; c < src.channels && c < 2; ++c) {
            for (int i = s; i < e; ++i) {
                out->ch[c][i] *= gain;
            }
        }
    }
    return out;
}

std::shared_ptr<SampleData> SampleOps::amplify(const SampleData& src, int startFrame, int endFrame, float gainLinear) {
    if (src.ch[0].empty()) return nullptr;
    auto out = std::make_shared<SampleData>();
    copyBase(src, *out);

    int total = static_cast<int>(src.ch[0].size());
    int s = std::clamp(startFrame, 0, total - 1);
    int e = std::clamp(endFrame, s + 1, total);

    for (int c = 0; c < src.channels && c < 2; ++c) {
        for (int i = s; i < e; ++i) {
            out->ch[c][i] = std::clamp(out->ch[c][i] * gainLinear, -1.0f, 1.0f);
        }
    }
    return out;
}

std::shared_ptr<SampleData> SampleOps::fadeIn(const SampleData& src, int startFrame, int endFrame) {
    if (src.ch[0].empty()) return nullptr;
    auto out = std::make_shared<SampleData>();
    copyBase(src, *out);

    int total = static_cast<int>(src.ch[0].size());
    int s = std::clamp(startFrame, 0, total - 1);
    int e = std::clamp(endFrame, s + 1, total);
    int count = std::max(1, e - s);

    for (int c = 0; c < src.channels && c < 2; ++c) {
        for (int i = s; i < e; ++i) {
            float ramp = static_cast<float>(i - s) / static_cast<float>(count);
            out->ch[c][i] *= ramp;
        }
    }
    return out;
}

std::shared_ptr<SampleData> SampleOps::fadeOut(const SampleData& src, int startFrame, int endFrame) {
    if (src.ch[0].empty()) return nullptr;
    auto out = std::make_shared<SampleData>();
    copyBase(src, *out);

    int total = static_cast<int>(src.ch[0].size());
    int s = std::clamp(startFrame, 0, total - 1);
    int e = std::clamp(endFrame, s + 1, total);
    int count = std::max(1, e - s);

    for (int c = 0; c < src.channels && c < 2; ++c) {
        for (int i = s; i < e; ++i) {
            float ramp = 1.0f - static_cast<float>(i - s) / static_cast<float>(count);
            out->ch[c][i] *= ramp;
        }
    }
    return out;
}

std::shared_ptr<SampleData> SampleOps::silence(const SampleData& src, int startFrame, int endFrame) {
    if (src.ch[0].empty()) return nullptr;
    auto out = std::make_shared<SampleData>();
    copyBase(src, *out);

    int total = static_cast<int>(src.ch[0].size());
    int s = std::clamp(startFrame, 0, total - 1);
    int e = std::clamp(endFrame, s + 1, total);

    for (int c = 0; c < src.channels && c < 2; ++c) {
        std::fill(out->ch[c].begin() + s, out->ch[c].begin() + e, 0.0f);
    }
    return out;
}

} // namespace trakr

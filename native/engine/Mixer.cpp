#include "Mixer.h"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace trakr {

Mixer::Mixer(size_t trackCount, float sampleRate) {
    resizeTracks(trackCount);
    init(sampleRate);
}

void Mixer::init(float sampleRate) {
    delay_.init(sampleRate);
    reverb_.init(sampleRate);
    masterEq_.init(sampleRate);
}

void Mixer::resizeTracks(size_t count) {
    trackMeters_ = std::vector<TrackMeter>(count);
}

float Mixer::softLimit(float x) const {
    constexpr float kThreshold = 0.95f;
    const float absX = std::abs(x);
    if (absX <= kThreshold) {
        return x;
    }
    const float sign = (x >= 0.0f) ? 1.0f : -1.0f;
    const float excess = absX - kThreshold;
    return sign * (kThreshold + 0.05f * std::tanh(excess / 0.05f));
}

void Mixer::process(const std::vector<TrackInfo>& trackInfos,
                    const float* const* trackAudioL,
                    const float* const* trackAudioR,
                    float* masterOutStereo,
                    int frames,
                    float masterGainDb,
                    float masterPan,
                    float delaySendBoost,
                    float reverbSendBoost,
                    const float* masterEq,
                    float masterSaturation,
                    float masterBassBoost,
                    float masterSpace) {
    if (!masterOutStereo || frames <= 0) return;

    if (masterEq) masterEq_.setGains(masterEq);
    if (masterSaturation > 0.0f) masterSaturation_.setDrive(masterSaturation);

    std::memset(masterOutStereo, 0, frames * 2 * sizeof(float));

    if (sendDelayBufL_.size() < static_cast<size_t>(frames)) {
        sendDelayBufL_.assign(frames, 0.0f);
        sendDelayBufR_.assign(frames, 0.0f);
        sendReverbBufL_.assign(frames, 0.0f);
        sendReverbBufR_.assign(frames, 0.0f);
    } else {
        std::fill_n(sendDelayBufL_.begin(), frames, 0.0f);
        std::fill_n(sendDelayBufR_.begin(), frames, 0.0f);
        std::fill_n(sendReverbBufL_.begin(), frames, 0.0f);
        std::fill_n(sendReverbBufR_.begin(), frames, 0.0f);
    }

    const size_t numTracks = std::min(trackInfos.size(), trackMeters_.size());

    // Check if any track is soloed
    bool anySolo = false;
    for (size_t t = 0; t < numTracks; ++t) {
        if (trackInfos[t].solo) {
            anySolo = true;
            break;
        }
    }

    float blockMasterPeakL = 0.0f;
    float blockMasterPeakR = 0.0f;

    for (size_t t = 0; t < numTracks; ++t) {
        const auto& info = trackInfos[t];
        const bool active = anySolo ? info.solo : !info.mute;

        const float* inL = trackAudioL[t];
        const float* inR = trackAudioR[t];

        float trackPeakL = 0.0f;
        float trackPeakR = 0.0f;

        if (active && inL && inR) {
            const float gain = std::pow(10.0f, info.gainDb / 20.0f);
            const float panAngle = (std::clamp(info.pan, -1.0f, 1.0f) + 1.0f) * (static_cast<float>(M_PI) / 4.0f);
            const float panL = std::cos(panAngle) * gain;
            const float panR = std::sin(panAngle) * gain;
            const float dSend = std::clamp(info.delaySend + delaySendBoost, 0.0f, 1.0f);
            const float rSend = std::clamp(info.reverbSend + reverbSendBoost, 0.0f, 1.0f);

            for (int i = 0; i < frames; ++i) {
                const float sL = inL[i] * panL;
                const float sR = inR[i] * panR;

                masterOutStereo[i * 2] += sL;
                masterOutStereo[i * 2 + 1] += sR;

                if (dSend > 0.0f) {
                    sendDelayBufL_[i] += sL * dSend;
                    sendDelayBufR_[i] += sR * dSend;
                }
                if (rSend > 0.0f) {
                    sendReverbBufL_[i] += sL * rSend;
                    sendReverbBufR_[i] += sR * rSend;
                }

                trackPeakL = std::max(trackPeakL, std::abs(sL));
                trackPeakR = std::max(trackPeakR, std::abs(sR));
            }
        }

        // Decay meters
        float prevL = trackMeters_[t].peakL.load(std::memory_order_relaxed);
        float prevR = trackMeters_[t].peakR.load(std::memory_order_relaxed);
        trackMeters_[t].peakL.store(std::max(trackPeakL, prevL * 0.95f), std::memory_order_relaxed);
        trackMeters_[t].peakR.store(std::max(trackPeakR, prevR * 0.95f), std::memory_order_relaxed);
    }

    const float masterGain = std::pow(10.0f, masterGainDb / 20.0f);
    const float mPanL = ((masterPan <= 0.0f) ? 1.0f : (1.0f - masterPan)) * masterGain;
    const float mPanR = ((masterPan >= 0.0f) ? 1.0f : (1.0f + masterPan)) * masterGain;
    const bool bitCrush = (bitCrushBits_ < 16 && bitCrushBits_ >= 2);
    const float bitCrushSteps =
        bitCrush ? std::pow(2.0f, static_cast<float>(bitCrushBits_)) : 0.0f;

    // Process send effects and add return to master bus
    float delayPkL = 0.0f, delayPkR = 0.0f;
    float reverbPkL = 0.0f, reverbPkR = 0.0f;
    float dryPkL = 0.0f, dryPkR = 0.0f;
    for (int i = 0; i < frames; ++i) {
        float delOutL = 0.0f, delOutR = 0.0f;
        delay_.process(sendDelayBufL_[i], sendDelayBufR_[i], delOutL, delOutR);

        float revOutL = 0.0f, revOutR = 0.0f;
        reverb_.process(sendReverbBufL_[i], sendReverbBufR_[i], revOutL, revOutR);

        const float dryL = masterOutStereo[i * 2] * auxDryGain_;
        const float dryR = masterOutStereo[i * 2 + 1] * auxDryGain_;
        const float sendL = delOutL * auxDelayGain_;
        const float sendR = delOutR * auxDelayGain_;
        const float retL = revOutL * auxReverbGain_;
        const float retR = revOutR * auxReverbGain_;

        float l = (dryL + sendL + retL) * mPanL;
        float r = (dryR + sendR + retR) * mPanR;

        delayPkL = std::max(delayPkL, std::fabs(sendL));
        delayPkR = std::max(delayPkR, std::fabs(sendR));
        reverbPkL = std::max(reverbPkL, std::fabs(retL));
        reverbPkR = std::max(reverbPkR, std::fabs(retR));
        dryPkL = std::max(dryPkL, std::fabs(dryL));
        dryPkR = std::max(dryPkR, std::fabs(dryR));

        // Master 5-Band Graphic EQ
        masterEq_.process(l, r);

        // Master Bass Boost (+3 dB low warmth)
        if (masterBassBoost > 0.0f) {
            float boost = masterBassBoost * 0.35f;
            l += (l - softLimit(l * 0.7f)) * boost;
            r += (r - softLimit(r * 0.7f)) * boost;
        }

        // Master Tape Saturation
        if (masterSaturation > 0.0f) {
            l = masterSaturation_.process(l);
            r = masterSaturation_.process(r);
        }

        // Master Stereo Space
        if (masterSpace > 0.0f) {
            float mid = (l + r) * 0.5f;
            float side = (l - r) * 0.5f * (1.0f + masterSpace * 0.85f);
            l = mid + side;
            r = mid - side;
        }

        if (bitCrush) {
            l = std::round(l * bitCrushSteps) / bitCrushSteps;
            r = std::round(r * bitCrushSteps) / bitCrushSteps;
        }

        l = softLimit(l);
        r = softLimit(r);

        masterOutStereo[i * 2] = l;
        masterOutStereo[i * 2 + 1] = r;

        blockMasterPeakL = std::max(blockMasterPeakL, std::abs(l));
        blockMasterPeakR = std::max(blockMasterPeakR, std::abs(r));
    }

    float prevMasterL = masterPeakL_.load(std::memory_order_relaxed);
    float prevMasterR = masterPeakR_.load(std::memory_order_relaxed);
    masterPeakL_.store(std::max(blockMasterPeakL, prevMasterL * 0.95f), std::memory_order_relaxed);
    masterPeakR_.store(std::max(blockMasterPeakR, prevMasterR * 0.95f), std::memory_order_relaxed);

    const float decay = 0.95f;
    auto storeAux = [&](int aux, float pkL, float pkR) {
        const float prevL = auxPeakL_[aux].load(std::memory_order_relaxed);
        const float prevR = auxPeakR_[aux].load(std::memory_order_relaxed);
        auxPeakL_[aux].store(std::max(pkL, prevL * decay), std::memory_order_relaxed);
        auxPeakR_[aux].store(std::max(pkR, prevR * decay), std::memory_order_relaxed);
    };
    storeAux(0, delayPkL, delayPkR);
    storeAux(1, reverbPkL, reverbPkR);
    storeAux(2, dryPkL, dryPkR);
    storeAux(3, 0.0f, 0.0f); // Line In bus is silent in the desktop build
}

MeterLevels Mixer::getAuxMeters(int aux) const {
    if (aux < 0 || aux >= 4) return {};
    return {auxPeakL_[aux].load(std::memory_order_relaxed),
            auxPeakR_[aux].load(std::memory_order_relaxed)};
}

MeterLevels Mixer::getTrackMeters(size_t trackIndex) const {
    if (trackIndex >= trackMeters_.size()) return {};
    return {trackMeters_[trackIndex].peakL.load(std::memory_order_relaxed),
            trackMeters_[trackIndex].peakR.load(std::memory_order_relaxed)};
}

MeterLevels Mixer::getMasterMeters() const {
    return {masterPeakL_.load(std::memory_order_relaxed),
            masterPeakR_.load(std::memory_order_relaxed)};
}

} // namespace trakr

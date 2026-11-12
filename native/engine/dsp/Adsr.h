#pragma once

#include <cmath>
#include <algorithm>

namespace trakr::dsp {

enum class AdsrState {
    Idle,
    Attack,
    Decay,
    Sustain,
    Release
};

class Adsr {
public:
    void init(float sampleRate) {
        sampleRate_ = std::max(1000.0f, sampleRate);
        reset();
    }

    void setParameters(float attackSec, float decaySec, float sustainLevel, float releaseSec) {
        attackSec_ = std::max(0.001f, attackSec);
        decaySec_ = std::max(0.001f, decaySec);
        sustainLevel_ = std::clamp(sustainLevel, 0.0f, 1.0f);
        releaseSec_ = std::max(0.001f, releaseSec);

        attackRate_ = 1.0f / (attackSec_ * sampleRate_);
        decayRate_ = (1.0f - sustainLevel_) / (decaySec_ * sampleRate_);
        releaseRate_ = sustainLevel_ / (releaseSec_ * sampleRate_);
    }

    void noteOn() {
        state_ = AdsrState::Attack;
    }

    void noteOff() {
        if (state_ != AdsrState::Idle) {
            state_ = AdsrState::Release;
            releaseRate_ = currentLevel_ / (releaseSec_ * sampleRate_);
        }
    }

    void reset() {
        state_ = AdsrState::Idle;
        currentLevel_ = 0.0f;
    }

    float process() {
        switch (state_) {
            case AdsrState::Idle:
                currentLevel_ = 0.0f;
                break;
            case AdsrState::Attack:
                currentLevel_ += attackRate_;
                if (currentLevel_ >= 1.0f) {
                    currentLevel_ = 1.0f;
                    state_ = AdsrState::Decay;
                }
                break;
            case AdsrState::Decay:
                currentLevel_ -= decayRate_;
                if (currentLevel_ <= sustainLevel_) {
                    currentLevel_ = sustainLevel_;
                    state_ = AdsrState::Sustain;
                }
                break;
            case AdsrState::Sustain:
                currentLevel_ = sustainLevel_;
                break;
            case AdsrState::Release:
                currentLevel_ -= releaseRate_;
                if (currentLevel_ <= 0.0f) {
                    currentLevel_ = 0.0f;
                    state_ = AdsrState::Idle;
                }
                break;
        }
        return currentLevel_;
    }

    bool isActive() const {
        return state_ != AdsrState::Idle;
    }

    float currentLevel() const { return currentLevel_; }
    AdsrState state() const { return state_; }

private:
    float sampleRate_{44100.0f};
    float attackSec_{0.01f};
    float decaySec_{0.1f};
    float sustainLevel_{0.8f};
    float releaseSec_{0.15f};

    float attackRate_{0.001f};
    float decayRate_{0.001f};
    float releaseRate_{0.001f};

    float currentLevel_{0.0f};
    AdsrState state_{AdsrState::Idle};
};

} // namespace trakr::dsp


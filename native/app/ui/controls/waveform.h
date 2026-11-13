#pragma once

// Waveform controls used by Sample Playback / Sample Loader.
//
// Firmware references:
//   core/libs/NewDisplay/spectrum.cpp       (cSpectrum)
//   core/libs/NewDisplay/points.cpp         (cPoints)
//   core/libs/NewDisplay/slicePoints.cpp    (cSlicePoints)
//   core/libs/NewDisplay/progressCursor.cpp (cProgressCursor)
//   core/libs/NewDisplay/lineIndicator.cpp  (cLineIndicator)
//   core/libs/NewDisplay/wtProgress.cpp     (cWtProgress)
//
// The waveform field is 600 device pixels wide; sample positions are mapped
// through the 16-bit cPoints space and the current zoom window.

#include "ui/control.h"
#include "Song.h"
#include <array>

namespace trakr::ui {

constexpr int kWaveformWidth = 600;
constexpr int kWaveformY = 116;   // 75 + ((300 - (MAX_16BIT / 300)) / 2)
constexpr int kWaveformHeight = 218; // MAX_16BIT / 300
constexpr int kWaveformHalfHeight = kWaveformHeight / 2;

struct WaveformState {
    const Instrument* instrument{nullptr};
    const SampleData* sample{nullptr};
    int length{0};
    float rate{44100.0f};
    bool stereo{false};
    int zoomStart{0};
    int zoomEnd{65535};
    float zoom{1.0f};
    int selectedBits{0};   // cPoints selection: 0x01 start, 0x02 loop1, 0x04 loop2, 0x08 end
    bool auditioning{false};
    float progress{0.0f};  // cProgressCursor 0..600
};

namespace waveform_detail {
int endFrame(const WaveformState& s);
int loopEndFrame(const WaveformState& s);
int pointX(const WaveformState& s, int frame);
bool zeroCrossed(const WaveformState& s, int frame);
ImU32 pointColor(const WaveformState& s, bool selected, bool zero);
bool sliceMode(const WaveformState& s);
} // namespace waveform_detail

class SpectrumControl : public Control {
public:
    SpectrumControl(std::string id, int channel);
    void setState(const WaveformState& state) { state_ = state; }
    void draw(Canvas& canvas) const override;

private:
    int channel_{0};
    WaveformState state_{};
};

class PointsControl : public Control {
public:
    explicit PointsControl(std::string id) : Control(std::move(id)) {}
    void setState(const WaveformState& state) { state_ = state; }
    void draw(Canvas& canvas) const override;
    // cPoints::update draws the range background with additive blending before
    // cSpectrum (layer -1 approximates it by painting underneath); update2
    // draws the markers after the spectrum.
    void drawLayer(Canvas& canvas, int layer) const override {
        if (layer == -1) drawBackground(canvas);
        else if (layer == 1) draw(canvas);
    }

private:
    void drawBackground(Canvas& canvas) const;
    WaveformState state_{};
};

class SlicePointsControl : public Control {
public:
    explicit SlicePointsControl(std::string id) : Control(std::move(id)) {}
    void setState(const WaveformState& state) { state_ = state; }
    void draw(Canvas& canvas) const override;
    void drawLayer(Canvas& canvas, int layer) const override {
        if (layer == 1) draw(canvas);
    }

private:
    WaveformState state_{};
};

class ProgressCursorControl : public Control {
public:
    explicit ProgressCursorControl(std::string id) : Control(std::move(id)) {}
    void setState(const WaveformState& state) { state_ = state; }
    void draw(Canvas& canvas) const override;
    // The audition cursor advances with the sample clock.
    bool animatingAt(double nowMs) const override {
        (void)nowMs;
        return state_.auditioning;
    }
    // The audition cursor is drawn after the marker pass (update3 ordering).
    void drawLayer(Canvas& canvas, int layer) const override {
        if (layer == 2) draw(canvas);
    }

private:
    WaveformState state_{};
};

class LineIndicatorControl : public Control {
public:
    explicit LineIndicatorControl(std::string id) : Control(std::move(id)) {}
    void setState(const WaveformState& state) { state_ = state; }
    void draw(Canvas& canvas) const override;

private:
    WaveformState state_{};
};

class WtProgressControl : public Control {
public:
    explicit WtProgressControl(std::string id) : Control(std::move(id)) {}
    void setState(const WaveformState& state) { state_ = state; }
    void draw(Canvas& canvas) const override;

private:
    WaveformState state_{};
};

} // namespace trakr::ui

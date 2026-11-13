#include "waveform.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace trakr::ui {

namespace waveform_detail {

int endFrame(const WaveformState& s) {
    if (!s.instrument) return s.length;
    return (s.instrument->endFrame > 0) ? s.instrument->endFrame : s.length;
}

int loopEndFrame(const WaveformState& s) {
    if (!s.instrument) return s.length;
    return (s.instrument->loopEnd > 0) ? s.instrument->loopEnd : s.length;
}

int pointX(const WaveformState& s, int frame) {
    if (s.length <= 1) return -1;
    const long p = static_cast<long>(std::clamp(frame, 0, s.length - 1)) * 65535 /
                   (s.length - 1);
    if (p < s.zoomStart || p > s.zoomEnd) return -1;
    return static_cast<int>((p - s.zoomStart) * (kWaveformWidth - 1) /
                            std::max(1, s.zoomEnd - s.zoomStart));
}

bool zeroCrossed(const WaveformState& s, int frame) {
    if (!s.sample || s.sample->ch[0].empty()) return false;
    if (frame <= 0 || frame >= static_cast<int>(s.sample->ch[0].size())) return false;
    return std::abs(s.sample->ch[0][static_cast<size_t>(frame)]) < 0.01f;
}

ImU32 pointColor(const WaveformState& s, bool selected, bool zero) {
    if (selected && zero) return kWaveformPointsColors[3];
    if (selected) return kWaveformPointsColors[2];
    return kWaveformPointsColors[0];
}

bool sliceMode(const WaveformState& s) {
    return s.instrument &&
           (s.instrument->loop == Instrument::Loop::Slice ||
            s.instrument->loop == Instrument::Loop::BeatSlice);
}

namespace {

void processSpectrum(const WaveformState& s, int channel, float* upper, float* lower) {
    for (int i = 0; i < kWaveformWidth; ++i) upper[i] = lower[i] = 0.0f;
    if (!s.sample || s.length <= 0) return;
    const std::vector<float>* src = &s.sample->ch[0];
    if (channel == 1 && s.sample->channels > 1 && !s.sample->ch[1].empty())
        src = &s.sample->ch[1];
    if (src->empty()) return;
    // cGraphicProcessing::processSpectrum(): resolution = length / 600 for the
    // unzoomed view (not (visibleFrames + 1) / 600), with max_resolution
    // capped at 200 samples per column.
    const int first = static_cast<int>(static_cast<long>(s.zoomStart) * s.length / 65535);
    const int last = static_cast<int>(static_cast<long>(s.zoomEnd) * s.length / 65535);
    const int res = std::max(1, (last - first) / kWaveformWidth);
    const int maxRes = std::min(res, 200);
    for (int i = 0; i < kWaveformWidth; ++i) {
        const long s0 = first + static_cast<long>(i) * res;
        if (s0 >= static_cast<long>(src->size())) break;
        float up = 0.0f, low = 0.0f;
        for (int j = 0; j < maxRes && s0 + j < static_cast<long>(src->size()); ++j) {
            const float v = (*src)[static_cast<size_t>(s0 + j)];
            up = std::max(up, v);
            low = std::min(low, v);
        }
        upper[i] = up;
        lower[i] = low;
    }
}

void drawPoint(Canvas& canvas, int x, bool topMark, bool sideMark, ImU32 color) {
    if (x <= 0) return; // marker on the control's left border reads as a stray edge line
    canvas.fillRect(RectI{x, kWaveformY - (topMark ? 8 : 0), 1,
                          kWaveformHeight + (topMark ? 8 : 0)},
                    color);
    const int x1 = x + (sideMark ? 1 : -1), x2 = x + (sideMark ? 5 : -5);
    const int y1 = kWaveformY + (topMark ? -8 : kWaveformHeight + 2);
    const int y2 = kWaveformY + (topMark ? -5 : kWaveformHeight + 5);
    const int y3 = kWaveformY + (topMark ? -2 : kWaveformHeight + 8);
    canvas.line(x1, y1, x2, y2, color);
    canvas.line(x2, y2, x1, y3, color);
    canvas.line(x1, y3, x1, y1, color);
}

} // namespace

} // namespace waveform_detail

using namespace waveform_detail;

SpectrumControl::SpectrumControl(std::string id, int channel)
    : Control(std::move(id)), channel_(channel) {}

void SpectrumControl::draw(Canvas& canvas) const {
    const bool second = state_.stereo && channel_ == 1;
    const float y = static_cast<float>(kWaveformY) +
                    (second ? static_cast<float>(kWaveformHalfHeight) : 0.0f);
    const float h = state_.stereo ? static_cast<float>(kWaveformHalfHeight)
                                  : static_cast<float>(kWaveformHeight);
    const float center = y + h / 2.0f;

    if (state_.length <= 0 || !state_.sample) {
        canvas.line(0, static_cast<int>(center), kWaveformWidth - 1, static_cast<int>(center),
                    kWaveformColors[1]);
        return;
    }

    float upper[kWaveformWidth];
    float lower[kWaveformWidth];
    processSpectrum(state_, channel_, upper, lower);

    const int limit = (state_.stereo && sliceMode(state_)) ? 200 : 300; // resizeWaveforms
    const float mult = static_cast<float>(kWaveformWidth) / static_cast<float>(limit);
    if ((static_cast<float>(state_.length) / state_.zoom) < 3600.0f) {
        canvas.line(0, static_cast<int>(center), kWaveformWidth - 1, static_cast<int>(center),
                    kWaveformColors[1]);
        int prevX = 0;
        float prevY = center;
        for (int i = 0; i < limit; ++i) {
            const int idx = static_cast<int>(static_cast<float>(i) * mult);
            const float v = upper[idx] > 0.0f ? upper[idx] : (lower[idx] < 0.0f ? lower[idx] : 0.0f);
            const float yy = center - v * h / 2.0f;
            if (i > 0) canvas.line(prevX, static_cast<int>(prevY), idx, static_cast<int>(yy),
                                   kWaveformColors[0]);
            prevX = idx;
            prevY = yy;
        }
    } else {
        for (int i = 0; i < limit; ++i) {
            const int idx = static_cast<int>(static_cast<float>(i) * mult);
            // cSpectrum draws a LINES pair with inclusive endpoints; round both
            // ends so the column covers the same rows as EVE's line.
            const int top = static_cast<int>(std::lround(center - upper[idx] * h / 2.0f));
            const int bottom = static_cast<int>(std::lround(center - lower[idx] * h / 2.0f));
            if (bottom > top) {
                canvas.fillRect(RectI{idx, top, 1, bottom - top + 1}, kWaveformColors[0]);
            } else if (bottom == top) {
                // Zero-amplitude column: the firmware's LINES pair collapses to
                // a single pixel on the centre line.
                canvas.fillRect(RectI{idx, top, 1, 1}, kWaveformColors[0]);
            }
        }
    }
}

void PointsControl::drawBackground(Canvas& canvas) const {
    if (!state_.instrument || !state_.sample || state_.length <= 0) return;
    const int s = pointX(state_, state_.instrument->startFrame);
    const int e = pointX(state_, endFrame(state_));
    const int l1 = pointX(state_, state_.instrument->loopStart);
    const int l2 = pointX(state_, loopEndFrame(state_));

    // cPoints::update: selected range filled grey_0A, before the spectrum pass.
    if (state_.instrument->loop == Instrument::Loop::None) {
        if (s >= 0 && e >= 0) {
            canvas.fillRect(RectI{std::min(s, e), kWaveformY, std::abs(e - s), kWaveformHeight},
                            kWaveformPointsColors[1]);
        }
    } else if (l1 >= 0 && l2 >= 0) {
        canvas.fillRect(RectI{std::min(l1, l2), kWaveformY, std::abs(l2 - l1), kWaveformHeight},
                        kWaveformPointsColors[1]);
    }
}

void PointsControl::draw(Canvas& canvas) const {
    if (!state_.instrument || !state_.sample || state_.length <= 0) return;
    const int sel = state_.selectedBits;
    const int s = pointX(state_, state_.instrument->startFrame);
    const int e = pointX(state_, endFrame(state_));
    const int l1 = pointX(state_, state_.instrument->loopStart);
    const int l2 = pointX(state_, loopEndFrame(state_));

    if (state_.instrument->loop != Instrument::Loop::None) {
        if (l1 >= 0)
            drawPoint(canvas, l1, false, true,
                      pointColor(state_, (sel & 0x02) != 0,
                                 zeroCrossed(state_, state_.instrument->loopStart)));
        if (l2 >= 0)
            drawPoint(canvas, l2, false, false,
                      pointColor(state_, (sel & 0x04) != 0, zeroCrossed(state_, loopEndFrame(state_))));
    }
    if (s >= 0)
        drawPoint(canvas, s, true, true,
                  pointColor(state_, (sel & 0x01) != 0,
                             zeroCrossed(state_, state_.instrument->startFrame)));
    if (e >= 0)
        drawPoint(canvas, e, true, false,
                  pointColor(state_, (sel & 0x08) != 0, zeroCrossed(state_, endFrame(state_))));
}

void SlicePointsControl::draw(Canvas& canvas) const {
    if (!state_.instrument) return;
    const auto& slices = state_.instrument->slices;
    for (size_t i = 0; i < slices.size(); ++i) {
        const int x = pointX(state_, slices[i]);
        if (x < 0) continue;
        const bool sel = static_cast<int>(i) == state_.instrument->selectedSlice;
        const ImU32 c = kSliceColors[sel ? 1 : 0];
        const int off = sel ? 9 : 0;
        canvas.fillRect(RectI{x, kWaveformY - (1 + off), 1, kWaveformHeight + (1 + off)}, c);
        const int yy = kWaveformY - (7 + off);
        canvas.line(x - 4, yy, x + 4, yy, c);
        canvas.line(x + 4, yy, x, yy + 5, c);
        canvas.line(x, yy + 5, x - 4, yy, c);
    }
}

void ProgressCursorControl::draw(Canvas& canvas) const {
    if (!state_.auditioning || state_.progress <= 0.0f || state_.progress >= kWaveformWidth) return;
    canvas.fillRect(RectI{static_cast<int>(state_.progress), kWaveformY, 1, kWaveformHeight},
                    kWaveformProgressColors[1]);
}

void LineIndicatorControl::draw(Canvas& canvas) const {
    if (!state_.instrument || state_.length <= 0) return;
    const float pos = std::clamp(state_.instrument->wavetablePos, 0.0f, 1.0f) *
                      static_cast<float>(kWaveformWidth);
    const float grain =
        std::clamp(state_.instrument->granularLengthMs * state_.rate / 1000.0f *
                       static_cast<float>(kWaveformWidth) / static_cast<float>(state_.length),
                   1.0f, static_cast<float>(kWaveformWidth));
    const int a = static_cast<int>(std::clamp(pos - grain / 2.0f, 0.0f,
                                              static_cast<float>(kWaveformWidth)));
    const int b = static_cast<int>(std::clamp(pos + grain / 2.0f, 0.0f,
                                              static_cast<float>(kWaveformWidth)));
    canvas.fillRect(RectI{a, kWaveformY, std::max(1, b - a), kWaveformHeight - 2},
                    kGranularProgressColors[1]);
    canvas.fillRect(RectI{static_cast<int>(pos), kWaveformY, 1, kWaveformHeight},
                    kGranularProgressColors[0]);
}

void WtProgressControl::draw(Canvas& canvas) const {
    if (!state_.instrument) return;
    canvas.line(0, 400, kWaveformWidth, 402, kWaveTablePositionColors[1]);
    const int x = static_cast<int>(std::clamp(state_.instrument->wavetablePos, 0.0f, 1.0f) *
                                   static_cast<float>(kWaveformWidth));
    canvas.fillRect(RectI{x, 400 - 7, 1, 15}, kWaveTablePositionColors[1]);
}

} // namespace trakr::ui

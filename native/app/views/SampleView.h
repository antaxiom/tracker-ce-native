#pragma once
// Trakr Native Sample Playback / Sample Editor view.
// Structural port of core/interface/samplePlayback/gSamplePlay.cpp
// (initDisplayControls) and the NewDisplay controls it creates.
#include "View.h"
#include "Song.h"
#include "History.h"
#include "EngineHost.h"
#include "../hw/ScreenButtons.h"
#include "imgui.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <vector>
#include <string>
namespace trakr {
class SampleView : public View {
public:
    enum class SubPage { Playback = 0, Editor = 1 };
    enum class Param { Start = 0, End = 1, LoopStart = 2, LoopEnd = 3, Zoom = 4, PlayMode = 5, Slices = 6 };
    enum class EditOp { Crop = 0, Reverse = 1, Normalize = 2, Amplify = 3, FadeIn = 4, FadeOut = 5, Silence = 6 };
    SampleView();
    const char* name() const override { return (subPage_ == SubPage::Editor) ? "Sample Editor" : "Sample Playback"; }
    void draw(bool* p_open = nullptr) override;
    void update(Song& song, History& history, EngineHost& engineHost, int selectedInstIdx);
    void triggerAudition();
    bool isAuditioning() const { return auditionStartTime_ > 0.0; }
    void stopAudition(EngineHost& engineHost);
    std::vector<hw::ScreenButton> getScreenButtons(Song& song, History& history, EngineHost& engineHost, int selectedInstIdx);
    // Backwards-compatible overload
    std::vector<hw::ScreenButton> getScreenButtons(Song& song, EngineHost& engineHost, int selectedInstIdx);
    void adjustParam(Song& song, EngineHost& engineHost, int selectedInstIdx, int delta, bool fine);
    void applyEditOp(Song& song, History& history, EngineHost& engineHost, int selectedInstIdx);
    void togglePage() { setSubPage((subPage_ == SubPage::Playback) ? SubPage::Editor : SubPage::Playback); }
    void selectNextParam(int delta) {
        if (subPage_ == SubPage::Playback) {
            activeParam_ = static_cast<Param>(std::clamp(static_cast<int>(activeParam_) + delta, 0, 5));
        } else {
            selectedEditOp_ = static_cast<EditOp>(std::clamp(static_cast<int>(selectedEditOp_) + delta, 0, 6));
        }
    }
    SubPage subPage() const { return subPage_; }
    void setSubPage(SubPage sp) { subPage_ = sp; }
    void setShiftHeld(bool s) { shiftHeld_ = s; }
    Param activeParam() const { return activeParam_; }
    void setActiveParam(Param p) { activeParam_ = p; }
    EditOp selectedEditOp() const { return selectedEditOp_; }
    void setSelectedEditOp(EditOp op) { selectedEditOp_ = op; }
private:
    bool shiftHeld_{false};
    SubPage subPage_{SubPage::Playback};
    int editorButtonPage_{0};
    Param activeParam_{Param::PlayMode};
    EditOp selectedEditOp_{EditOp::Crop};
    float zoomLevel_{1.0f};
    float scrollOffset_{0.0f};
    double auditionStartTime_{-1.0};
};
namespace sample_detail {
// Colors mirrored verbatim from core/source/Themes.cpp (themeDefault); Theme.h/Theme.cpp
// are off-limits to parallel screen ports, so consolidation is deferred.
inline constexpr uint32_t kWhite = 0xFFFFFF, kGrey0A = 0x0A0A0A, kGrey22 = 0x222222;
inline constexpr uint32_t kGrey57 = 0x575757, kGrey77 = 0x777777, kGrey3F = 0x3F3F3F;
inline constexpr uint32_t kRed = 0xCD1818, kGreen = 0x30CB3F, kCyan = 0x3FFFFF;
inline constexpr uint32_t kYellow = 0xFEF749, kBlack = 0x000000;
inline constexpr uint32_t kWaveformColors[2] = {kWhite, kGrey77};
inline constexpr uint32_t kWaveformPointsColors[4] = {kWhite, kGrey0A, kRed, kGreen};
inline constexpr uint32_t kWaveformProgressColors[2] = {kWhite, kRed};
inline constexpr uint32_t kGranularProgressColors[2] = {kCyan, kGrey22};
inline constexpr uint32_t kWaveTablePositionColors[2] = {kWhite, kWhite};
inline constexpr uint32_t kSliceColors[3] = {kYellow, kRed, kGreen};
inline constexpr uint32_t kActiveListColors[7] = {kWhite, kWhite, kGrey57, kGrey0A, kWhite, kWhite, kBlack};
inline constexpr uint32_t kFrameColors[2] = {kRed, kGrey3F};

inline ImU32 c32(uint32_t rgb) { return IM_COL32((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF, 255); }

// playModeFunctLabels from core/interface/samplePlayback/samplePlayback.h.
inline const char* kPlayModeNames[8] = {"1-Shot", "Forward Loop", "Backward Loop", "Pingpong Loop",
                                        "Slice", "Beat Slice", "Wavetable", "Granular"};
inline const char* kEditOps[7] = {"Crop", "Reverse", "Normalize", "Amplify", "Fade In", "Fade Out", "Silence"};

// Firmware geometry: gSamplePlay.cpp framesPlaces + control rects.
inline constexpr int kChromeHeight = 28;
inline constexpr int kMax16 = 65535;
inline constexpr int kSpecY = 116;  // 75 + ((300 - (MAX_16BIT / 300)) / 2)
inline constexpr int kSpecH = 218;  // MAX_16BIT / 300
inline constexpr int kHalfH = 109;  // kSpecH / 2 (stereo halves)
inline constexpr int kRedFrameWidth = 24;
inline constexpr int kListX = (800 / 8) * 6 + 1;  // 601
inline constexpr int kListY = 29;
inline constexpr int kListW = 800 / 4 - 3;  // 197
inline constexpr int kListH = 394;
inline constexpr int kListRow = 27;
inline constexpr int kFramePlace[4] = {kListX, 31, kListW, 391};  // framesPlaces[6]

// One pass of the firmware sample controls. Coordinates stay in firmware device
// pixels; px()/py() only map them into the app workspace.
struct SampleControls {
    ImDrawList* dl{};
    ImVec2 origin{};
    float sx{1.0f}, sy{1.0f};
    Song* song{};
    EngineHost* engine{};
    Instrument* inst{};
    const SampleData* sample{};
    int len{0};
    float rate{44100.0f};
    bool stereo{false};
    int zoomStart{0}, zoomEnd{kMax16};
    float zoom{1.0f};
    int activeParam{5};
    int editOp{0};
    bool editor{false};
    bool auditioning{false};
    float progress{0.0f};
    int clickedMode{-1}, clickedEdit{-1};
    bool applyEdit{false};
    int dragTarget{-1};
    float upper[600]{}, lower[600]{};
    ImFont* font{};
    float fontSize{14.0f};

    float px(float x) const { return origin.x + x * sx; }
    float py(float y) const { return origin.y + (y - kChromeHeight) * sy; }
    float stroke() const { return std::max(1.0f, std::round(kRedFrameWidth / 16.0f * sy)); }
    void fill(float x, float y, float w, float h, ImU32 c) const {
        dl->AddRectFilled(ImVec2(px(x), py(y)), ImVec2(px(x + w), py(y + h)), c);
    }
    void frame(float x, float y, float w, float h, ImU32 c) const {
        dl->AddRect(ImVec2(px(x), py(y)), ImVec2(px(x + w), py(y + h)), c, 0.0f, 0, stroke());
    }
    void hline(float x0, float x1, float y, ImU32 c) const {
        dl->AddLine(ImVec2(px(x0), py(y)), ImVec2(px(x1), py(y)), c, 1.0f);
    }
    void text(float x, float yc, ImU32 c, const char* s) const {
        dl->AddText(font, fontSize, ImVec2(px(x), py(yc) - fontSize * 0.5f), c, s);
    }
    bool hit(const char* id, float x, float y, float w, float h) const {
        ImGui::SetCursorScreenPos(ImVec2(px(x), py(y)));
        return ImGui::InvisibleButton(id, ImVec2(w * sx, h * sy));
    }

    int endFrame() const { return (inst->endFrame > 0) ? inst->endFrame : len; }
    int loopEndFrame() const { return (inst->loopEnd > 0) ? inst->loopEnd : len; }
    int pointPx(int frame) const {
        if (len <= 1) return -1;
        const long p = static_cast<long>(std::clamp(frame, 0, len - 1)) * kMax16 / (len - 1);
        if (p < zoomStart || p > zoomEnd) return -1;
        return static_cast<int>((p - zoomStart) * 599 / std::max(1, zoomEnd - zoomStart));
    }
    bool zeroCrossed(int frame) const {
        if (!sample || sample->ch[0].empty()) return false;
        if (frame <= 0 || frame >= static_cast<int>(sample->ch[0].size())) return false;
        return std::abs(sample->ch[0][frame]) < 0.01f;
    }
    uint8_t selectedBits() const {
        if (editor) return 0;
        switch (activeParam) {
            case 0: return 0x01;  // selectStart
            case 2: return 0x02;  // selectLoop1
            case 3: return 0x04;  // selectLoop2
            case 1: return 0x08;  // selectEnd
            default: return 0;
        }
    }
    ImU32 pointColor(bool selected, bool zero) const {
        if (selected && zero) return c32(kWaveformPointsColors[3]);
        if (selected) return c32(kWaveformPointsColors[2]);
        return c32(kWaveformPointsColors[0]);
    }
    // cPoints::drawPoint (topMark = start/end, sideMark = start/loop1).
    void drawPoint(int x, bool topMark, bool sideMark, ImU32 c) {
        if (x <= 0) return;  // marker on the control's left border reads as a stray edge line
        fill(x, kSpecY - (topMark ? 8 : 0), 1.0f, kSpecH + (topMark ? 8 : 0), c);
        const int x1 = x + (sideMark ? 1 : -1), x2 = x + (sideMark ? 5 : -5);
        const int y1 = kSpecY + (topMark ? -8 : kSpecH + 2);
        const int y2 = kSpecY + (topMark ? -5 : kSpecH + 5);
        const int y3 = kSpecY + (topMark ? -2 : kSpecH + 8);
        dl->AddLine(ImVec2(px(x1), py(y1)), ImVec2(px(x2), py(y2)), c, 1.0f);
        dl->AddLine(ImVec2(px(x2), py(y2)), ImVec2(px(x1), py(y3)), c, 1.0f);
        dl->AddLine(ImVec2(px(x1), py(y3)), ImVec2(px(x1), py(y1)), c, 1.0f);
    }

    // cGraphicProcessing::processSpectrum: per-column min/max of the sample.
    void spectrumProcess(int channel) {
        for (int i = 0; i < 600; ++i) upper[i] = lower[i] = 0.0f;
        if (!sample || len <= 0) return;
        const std::vector<float>* src = &sample->ch[0];
        if (channel == 1 && sample->channels > 1 && !sample->ch[1].empty()) src = &sample->ch[1];
        if (src->empty()) return;
        const int first = static_cast<int>(static_cast<long>(zoomStart) * (len - 1) / kMax16);
        const int last = static_cast<int>(static_cast<long>(zoomEnd) * (len - 1) / kMax16);
        const int res = std::max(1, (last - first + 1) / 600);
        const int maxRes = std::min(res, 200);
        for (int i = 0; i < 600; ++i) {
            const long s0 = first + static_cast<long>(i) * res;
            if (s0 >= static_cast<long>(src->size())) break;
            float up = 0.0f, low = 0.0f;
            for (int j = 0; j < maxRes && s0 + j < static_cast<long>(src->size()); ++j) {
                const float v = (*src)[s0 + j];
                up = std::max(up, v); low = std::min(low, v);
            }
            upper[i] = up; lower[i] = low;
        }
    }
    // cSpectrum::update: spectrumType 1 is a LINE_STRIP, 0 draws vertical LINES.
    void spectrumControl(int channel) {
        spectrumProcess(channel);
        const bool second = stereo && channel == 1;
        const float y = kSpecY + (second ? kHalfH : 0), h = stereo ? kHalfH : kSpecH;
        const float center = y + h / 2.0f;
        const ImU32 wave = c32(kWaveformColors[0]);
        if (len <= 0 || !sample) { hline(0, 599, center, c32(kWaveformColors[1])); return; }
        const bool sliceMode = inst && (inst->loop == Instrument::Loop::Slice || inst->loop == Instrument::Loop::BeatSlice);
        const int limit = (stereo && sliceMode) ? 200 : 300;  // resizeWaveforms pointsLimit
        const float mult = 600.0f / static_cast<float>(limit);
        if ((static_cast<float>(len) / zoom) < 3600.0f) {
            hline(0, 599, center, c32(kWaveformColors[1]));
            float prevY = center, prevX = 0.0f;
            for (int i = 0; i < limit; ++i) {
                const int idx = static_cast<int>(i * mult);
                const float v = upper[idx] > 0.0f ? upper[idx] : (lower[idx] < 0.0f ? lower[idx] : 0.0f);
                const float yy = center - v * h / 2.0f;
                if (i > 0) dl->AddLine(ImVec2(px(prevX), py(prevY)), ImVec2(px(idx), py(yy)), wave, 1.0f);
                prevX = idx; prevY = yy;
            }
        } else {
            for (int i = 0; i < limit; ++i) {
                const int idx = static_cast<int>(i * mult);
                const float y0 = center - upper[idx] * h / 2.0f, y1 = center - lower[idx] * h / 2.0f;
                if (y1 > y0) fill(idx, y0, 1.0f, y1 - y0, wave);
            }
        }
    }
    // cPoints::update (background rect is composited before cSpectrum here
    // because ImGui cannot reproduce the firmware's additive blend mode).
    void pointsBackground() const {
        if (!inst || !sample || len <= 0) return;
        const ImU32 c = c32(kWaveformPointsColors[1]);
        const int s = pointPx(inst->startFrame), e = pointPx(endFrame());
        const int l1 = pointPx(inst->loopStart), l2 = pointPx(loopEndFrame());
        if (inst->loop == Instrument::Loop::None) {
            if (s >= 0 && e >= 0) fill(std::min(s, e), kSpecY, std::abs(e - s), kSpecH, c);
        } else if (l1 >= 0 && l2 >= 0) {
            fill(std::min(l1, l2), kSpecY, std::abs(l2 - l1), kSpecH, c);
        }
    }
    void pointsControl() {
        if (!inst || !sample || len <= 0) return;
        const uint8_t sel = selectedBits();
        const int s = pointPx(inst->startFrame), e = pointPx(endFrame());
        const int l1 = pointPx(inst->loopStart), l2 = pointPx(loopEndFrame());
        if (inst->loop != Instrument::Loop::None) {
            if (l1 >= 0) drawPoint(l1, false, true, pointColor(sel & 0x02, zeroCrossed(inst->loopStart)));
            if (l2 >= 0) drawPoint(l2, false, false, pointColor(sel & 0x04, zeroCrossed(loopEndFrame())));
        }
        if (s >= 0) drawPoint(s, true, true, pointColor(sel & 0x01, zeroCrossed(inst->startFrame)));
        if (e >= 0) drawPoint(e, true, false, pointColor(sel & 0x08, zeroCrossed(endFrame())));
    }
    // cSlicePoints::update.
    void slicePointsControl() {
        if (!inst) return;
        for (size_t i = 0; i < inst->slices.size(); ++i) {
            const int x = pointPx(inst->slices[i]);
            if (x < 0) continue;
            const bool sel = static_cast<int>(i) == inst->selectedSlice;
            const ImU32 c = c32(kSliceColors[sel ? 1 : 0]);
            const int off = sel ? 9 : 0;
            fill(x, kSpecY - (1 + off), 1.0f, kSpecH + (1 + off), c);
            const int yy = kSpecY - (7 + off);
            dl->AddLine(ImVec2(px(x - 4), py(yy)), ImVec2(px(x + 4), py(yy)), c, 1.0f);
            dl->AddLine(ImVec2(px(x + 4), py(yy)), ImVec2(px(x), py(yy + 5)), c, 1.0f);
            dl->AddLine(ImVec2(px(x), py(yy + 5)), ImVec2(px(x - 4), py(yy)), c, 1.0f);
        }
    }

    // cProgressCursor::update (playProgressInSpectrum 0..600).
    void progressCursorUpdate() const {
        if (!auditioning || progress <= 0.0f || progress >= 600.0f) return;
        fill(progress, kSpecY, 1.0f, kSpecH, c32(kWaveformProgressColors[1]));
    }
    // cLineIndicator::update (granular selection + cursor).
    void granularCursorUpdate() const {
        if (!inst || len <= 0) return;
        const float pos = std::clamp(inst->wavetablePos, 0.0f, 1.0f) * 600.0f;
        const float grain = std::clamp(inst->granularLengthMs * rate / 1000.0f * 600.0f / len, 1.0f, 600.0f);
        const int a = static_cast<int>(std::clamp(pos - grain / 2.0f, 0.0f, 600.0f));
        const int b = static_cast<int>(std::clamp(pos + grain / 2.0f, 0.0f, 600.0f));
        fill(a, kSpecY, std::max(1, b - a), kSpecH - 2, c32(kGranularProgressColors[1]));
        fill(pos, kSpecY, 1.0f, kSpecH, c32(kGranularProgressColors[0]));
    }
    // cWtProgress::update.
    void wtPositionCursorUpdate() const {
        if (!inst) return;
        dl->AddLine(ImVec2(px(0), py(400)), ImVec2(px(600), py(402)), c32(kWaveTablePositionColors[1]), 1.0f);
        const int x = static_cast<int>(std::clamp(inst->wavetablePos, 0.0f, 1.0f) * 600.0f);
        fill(x, 400 - 7, 1, 15, c32(kWaveTablePositionColors[1]));
    }
    // cList::update with controlStyleBackground | controlStyleBottomShadow.
    void listBackground() const {
        const ImU32 bg = c32(kActiveListColors[3]);
        fill(kListX, kListY, kListW, kListH, bg);
        const ImU32 clear = bg & 0x00FFFFFFu;
        dl->AddRectFilledMultiColor(ImVec2(px(kListX), py(kListY + kListH - 10)),
                                    ImVec2(px(kListX + kListW), py(kListY + kListH + 2)),
                                    bg, bg, clear, clear);
    }
    void listFrame(int row) const {
        frame(kListX + 3, kListY + 4 + row * kListRow, kListW - 6, kListRow, c32(kActiveListColors[0]));
    }
    void playModeListControl() {
        listBackground();
        const int sel = std::clamp(inst ? static_cast<int>(inst->loop) : 0, 0, 7);
        listFrame(sel);
        for (int i = 0; i < 8; ++i)
            text(kListX + 13, kListY + 4 + kListRow / 2 + i * kListRow,
                 c32(kActiveListColors[i == sel ? 5 : 4]), kPlayModeNames[i]);
        for (int i = 0; i < 8; ++i) {
            char id[32];
            std::snprintf(id, sizeof(id), "##sample_mode_%d", i);
            if (hit(id, kListX + 3, kListY + 4 + i * kListRow, kListW - 6, kListRow)) clickedMode = i;
        }
    }
    // App-only Sample Editor page reuses the firmware list geometry.
    void editorListControl() {
        listBackground();
        const int sel = std::clamp(editOp, 0, 6);
        listFrame(sel);
        for (int i = 0; i < 7; ++i)
            text(kListX + 13, kListY + 4 + kListRow / 2 + i * kListRow,
                 c32(kActiveListColors[i == sel ? 5 : 4]), kEditOps[i]);
        for (int i = 0; i < 7; ++i) {
            char id[32];
            std::snprintf(id, sizeof(id), "##sample_edit_%d", i);
            if (hit(id, kListX + 3, kListY + 4 + i * kListRow, kListW - 6, kListRow)) {
                clickedEdit = i;
                applyEdit = ImGui::IsMouseDoubleClicked(0);
            }
        }
    }
    // cFrame::update: framesPlaces[selectedPlace], frameColors[0].
    void frameControl() const {
        if (editor || activeParam < 5) return;  // places 1..5 are bottom labels (app chrome)
        frame(kFramePlace[0], kFramePlace[1], kFramePlace[2], kFramePlace[3], c32(kFrameColors[0]));
    }

    // Pointer dragging for start/loop/end markers mapped through 16-bit point space.
    void dragMarkers() {
        if (!inst || !sample || len <= 1 || editor || !ImGui::IsWindowHovered()) return;
        int* targets[4] = {&inst->startFrame, &inst->loopStart, &inst->loopEnd, &inst->endFrame};
        const int frames[4] = {inst->startFrame, inst->loopStart, loopEndFrame(), endFrame()};
        for (int k = 0; k < 4; ++k) {
            const int x = pointPx(frames[k]);
            if (x < 0) continue;
            const ImVec2 a(px(x) - 4.0f * sx, py(kSpecY));
            const ImVec2 b(px(x) + 4.0f * sx, py(kSpecY + kSpecH));
            if (ImGui::IsMouseHoveringRect(a, b, false) && ImGui::IsMouseClicked(0)) dragTarget = k;
        }
        if (dragTarget < 0) return;
        if (!ImGui::IsMouseDown(0)) { dragTarget = -1; return; }
        const float local = (ImGui::GetIO().MousePos.x - px(0.0f)) / sx;
        if (local < 0.0f || local > 600.0f) return;
        const long p = zoomStart + static_cast<long>(local * std::max(1, zoomEnd - zoomStart) / 599.0f);
        int frame = std::clamp(static_cast<int>(p * (len - 1) / kMax16), 0, len - 1);
        if (dragTarget == 0) frame = std::min(frame, endFrame() - 1);
        else if (dragTarget == 1) frame = std::min(frame, loopEndFrame() - 1);
        else if (dragTarget == 2) frame = std::max(frame, inst->loopStart + 1);
        else frame = std::max(frame, inst->startFrame + 1);
        if (*targets[dragTarget] != frame) {
            *targets[dragTarget] = frame;
            if (engine) engine->publish(*song);
        }
    }

    // Reverse creation order (see file header).
    void draw() {
        const bool wavetable = inst && inst->loop == Instrument::Loop::Wavetable;
        const bool granular = inst && inst->loop == Instrument::Loop::Granular;
        const bool slices = inst && (inst->loop == Instrument::Loop::Slice || inst->loop == Instrument::Loop::BeatSlice);
        if (wavetable) wtPositionCursorUpdate();
        if (!slices && !granular) pointsBackground();
        if (stereo) spectrumControl(1);
        spectrumControl(0);
        if (slices) slicePointsControl();
        else if (granular) granularCursorUpdate();
        else pointsControl();
        progressCursorUpdate();
        if (editor) editorListControl();
        else playModeListControl();
        frameControl();
        dragMarkers();
    }
};

}  // namespace sample_detail

}  // namespace trakr

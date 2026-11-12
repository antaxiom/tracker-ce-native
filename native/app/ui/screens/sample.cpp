#include "sample.h"

#include "Song.h"
#include "EngineHost.h"
#include "views/SampleView.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace trakr {
using namespace ::trakr::ui;

namespace {

constexpr int kMax16 = 65535;
constexpr int kListX = (800 / 8) * 6 + 1; // 601
constexpr int kListW = 800 / 4 - 3;       // 197
constexpr int kFramePlace[4] = {kListX, 31, kListW, 391}; // framesPlaces[6]
constexpr int kWaveX = 0;
constexpr int kWaveW = 600;

const char* const kPlayModeNames[8] = {"1-Shot", "Forward Loop", "Backward Loop",
                                       "Pingpong Loop", "Slice", "Beat Slice",
                                       "Wavetable", "Granular"};

size_t waveLength(const Instrument& inst) {
    return (inst.sample && !inst.sample->ch[0].empty()) ? inst.sample->ch[0].size() : 0;
}

uint8_t selectedBitsFor(SampleView::Param param) {
    switch (param) {
        case SampleView::Param::Start: return 0x01;
        case SampleView::Param::LoopStart: return 0x02;
        case SampleView::Param::LoopEnd: return 0x04;
        case SampleView::Param::End: return 0x08;
        default: return 0;
    }
}

} // namespace

SampleScreen::SampleScreen() = default;

void SampleScreen::build() {
    if (built_) return;

    std::vector<RectI> places;
    places.reserve(7);
    // framesPlaces[0..5] sit on the bottom labels (app chrome); only the
    // play-mode list place is drawn in the workspace.
    for (int i = 0; i < 6; ++i)
        places.push_back(RectI{(800 / 8) * i + 1, 424, 800 / 8 - 2, 55});
    places.push_back(RectI{kFramePlace[0], kFramePlace[1], kFramePlace[2], kFramePlace[3]});
    frame_ = &display_.add<FrameControl>("frame", std::move(places), 6);
    frame_->setVisible(false);

    std::vector<std::string> modes(kPlayModeNames, kPlayModeNames + 8);
    playModeList_ = &display_.add<ListControl>("playModeList", std::move(modes), 8);
    playModeList_->setRect(RectI{kListX, 29, kListW, 394});
    playModeList_->setStart(0);

    progressCursor_ = &display_.add<ProgressCursorControl>("progressCursor");
    progressCursor_->setRect(RectI{kWaveX, kWaveformY, kWaveW, kWaveformHeight});

    granularCursor_ = &display_.add<LineIndicatorControl>("granularCursor");
    granularCursor_->setRect(RectI{kWaveX, kWaveformY, kWaveW, kWaveformHeight});

    points_ = &display_.add<PointsControl>("points");
    points_->setRect(RectI{kWaveX, kWaveformY, kWaveW, kWaveformHeight});

    slicePoints_ = &display_.add<SlicePointsControl>("slicePoints");
    slicePoints_->setRect(RectI{kWaveX, kWaveformY, kWaveW, kWaveformHeight});

    spectrum_[0] = &display_.add<SpectrumControl>("spectrum0", 0);
    spectrum_[0]->setRect(RectI{kWaveX, kWaveformY, kWaveW, kWaveformHalfHeight});
    spectrum_[1] = &display_.add<SpectrumControl>("spectrum1", 1);
    spectrum_[1]->setRect(RectI{kWaveX, kWaveformY + kWaveformHalfHeight, kWaveW,
                                kWaveformHalfHeight});

    wtPosition_ = &display_.add<WtProgressControl>("wtPosition");
    wtPosition_->setRect(RectI{kWaveX, 400, kWaveW, 2});

    playModeList_->setOnSelect([this](int index) {
        if (!song_ || !engineHost_ || !view_) return;
        if (selectedInst_ < 0 || selectedInst_ >= static_cast<int>(song_->instruments.size()))
            return;
        song_->instruments[static_cast<size_t>(selectedInst_)].loop =
            static_cast<Instrument::Loop>(index);
        view_->setActiveParam(SampleView::Param::PlayMode);
        engineHost_->publish(*song_);
    });

    built_ = true;
}

void SampleScreen::syncFromModel(Song& song, EngineHost& engineHost, int selectedInstIdx,
                                 SampleView& view) {
    if (selectedInstIdx < 0 || selectedInstIdx >= static_cast<int>(song.instruments.size())) {
        for (auto* c : {static_cast<ui::Control*>(progressCursor_),
                        static_cast<ui::Control*>(granularCursor_),
                        static_cast<ui::Control*>(points_),
                        static_cast<ui::Control*>(slicePoints_),
                        static_cast<ui::Control*>(spectrum_[0]),
                        static_cast<ui::Control*>(spectrum_[1]),
                        static_cast<ui::Control*>(wtPosition_),
                        static_cast<ui::Control*>(frame_)}) {
            c->setVisible(false);
        }
        return;
    }

    Instrument& inst = song.instruments[static_cast<size_t>(selectedInstIdx)];
    const int len = static_cast<int>(waveLength(inst));
    const float rate =
        (inst.sample && inst.sample->sampleRate > 0) ? static_cast<float>(inst.sample->sampleRate)
                                                     : 44100.0f;
    const bool hasWave = len > 0 && inst.sample;
    const bool slices = inst.loop == Instrument::Loop::Slice ||
                        inst.loop == Instrument::Loop::BeatSlice;
    const bool granular = inst.loop == Instrument::Loop::Granular;

    WaveformState state;
    state.instrument = &inst;
    state.sample = inst.sample.get();
    state.length = len;
    state.rate = rate;
    state.stereo = inst.sample && inst.sample->channels > 1 && !inst.sample->ch[1].empty();
    state.zoom = view.zoom();
    if (state.zoom > 1.0f) {
        const int width = static_cast<int>(static_cast<float>(kMax16) / state.zoom);
        state.zoomStart = std::clamp(kMax16 / 2 - width / 2, 0, kMax16);
        state.zoomEnd = std::min(kMax16, state.zoomStart + width);
    }
    state.selectedBits = selectedBitsFor(view.activeParam());
    state.auditioning = view.isAuditioning();
    if (state.auditioning) {
        const double elapsed = ImGui::GetTime() - view.auditionStartTime();
        const double dur = len > 0 ? static_cast<double>(len) / rate : 1.0;
        const float frac = dur > 0.0 ? static_cast<float>(elapsed / dur) : 1.0f;
        state.progress = std::clamp(frac, 0.0f, 1.0f) * static_cast<float>(kWaveformWidth);
    }

    progressCursor_->setState(state);
    granularCursor_->setState(state);
    points_->setState(state);
    slicePoints_->setState(state);
    spectrum_[0]->setState(state);
    spectrum_[1]->setState(state);
    wtPosition_->setState(state);

    progressCursor_->setVisible(hasWave);
    wtPosition_->setVisible(!hasWave);
    if (!hasWave) {
        granularCursor_->setVisible(false);
        points_->setVisible(false);
        slicePoints_->setVisible(false);
        spectrum_[0]->setVisible(false);
        spectrum_[1]->setVisible(false);
    } else {
        granularCursor_->setVisible(granular);
        points_->setVisible(!slices && !granular);
        slicePoints_->setVisible(slices);
        spectrum_[0]->setVisible(true);
        spectrum_[1]->setVisible(state.stereo);
    }

    playModeList_->setSelected(static_cast<int>(inst.loop));
    playModeList_->setStart(0);
    const bool showFrame = view.activeParam() == SampleView::Param::PlayMode;
    frame_->setSelected(6);
    frame_->setVisible(showFrame);
}

void SampleScreen::update(Song& song,
                          History& history,
                          EngineHost& engineHost,
                          int selectedInstIdx,
                          SampleView& view,
                          const ImRect& workspace,
                          ui::LayoutProfile profile,
                          bool shiftHeld) {
    build();
    song_ = &song;
    engineHost_ = &engineHost;
    view_ = &view;
    selectedInst_ = selectedInstIdx;

    ImGui::SetCursorScreenPos(workspace.Min);
    ImGui::InvisibleButton("##gui_next_sample_workspace", workspace.GetSize());
    const bool hovered = ImGui::IsItemHovered();
    const bool keyTarget =
        ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) || !ImGui::IsAnyItemActive();

    const bool valid = selectedInstIdx >= 0 &&
                       selectedInstIdx < static_cast<int>(song.instruments.size());
    Instrument* inst = valid ? &song.instruments[static_cast<size_t>(selectedInstIdx)] : nullptr;

    if (valid) {
        if (ImGui::IsKeyPressed(ImGuiKey_Space, false)) {
            view.triggerAudition();
            engineHost.previewNoteOn(-1, selectedInstIdx, inst->rootNote, 0.9f);
        }
        if (ImGui::IsKeyReleased(ImGuiKey_Space)) view.stopAudition(engineHost);
    }

    const bool fine = shiftHeld || ImGui::GetIO().KeyShift;
    if (keyTarget && !ImGui::GetIO().WantTextInput && valid) {
        if (ImGui::IsKeyPressed(ImGuiKey_UpArrow))
            view.adjustParam(song, engineHost, selectedInstIdx, +1, fine);
        if (ImGui::IsKeyPressed(ImGuiKey_DownArrow))
            view.adjustParam(song, engineHost, selectedInstIdx, -1, fine);
        if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) view.selectNextParam(-1);
        if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) view.selectNextParam(+1);
    }

    if (hovered && valid) {
        const float wheel = ImGui::GetIO().MouseWheel;
        if (std::abs(wheel) > 0.01f)
            view.adjustParam(song, engineHost, selectedInstIdx, (wheel > 0.0f) ? +1 : -1, fine);
    }

    if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
        const LayoutContext lc = LayoutContext::fromWorkspace(workspace, profile);
        const ImVec2 device = lc.screenToDevice(ImGui::GetIO().MousePos);
        const int dx = static_cast<int>(std::floor(device.x));
        const int dy = static_cast<int>(std::floor(device.y));
        if (ui::Control* hit = display_.hitTest(dx, dy)) hit->onPress(dx, dy);
    }

    // Marker dragging maps the pointer through the 16-bit cPoints space.
    if (valid && ImGui::IsWindowHovered() && !view.isAuditioning()) {
        const LayoutContext lc = LayoutContext::fromWorkspace(workspace, profile);
        const ImVec2 device = lc.screenToDevice(ImGui::GetIO().MousePos);
        const int len = static_cast<int>(waveLength(*inst));
        const float zoom = view.zoom();
        int zoomStart = 0;
        int zoomEnd = kMax16;
        if (zoom > 1.0f) {
            const int width = static_cast<int>(static_cast<float>(kMax16) / zoom);
            zoomStart = std::clamp(kMax16 / 2 - width / 2, 0, kMax16);
            zoomEnd = std::min(kMax16, zoomStart + width);
        }
        auto pointX = [&](int frame) {
            if (len <= 1) return -1;
            const long p = static_cast<long>(std::clamp(frame, 0, len - 1)) * kMax16 / (len - 1);
            if (p < zoomStart || p > zoomEnd) return -1;
            return static_cast<int>((p - zoomStart) * (kWaveformWidth - 1) /
                                    std::max(1, zoomEnd - zoomStart));
        };
        const int frames[4] = {inst->startFrame, inst->loopStart,
                               (inst->loopEnd > 0) ? inst->loopEnd : len,
                               (inst->endFrame > 0) ? inst->endFrame : len};
        if (dragTarget_ < 0) {
            for (int k = 0; k < 4; ++k) {
                const int x = pointX(frames[k]);
                if (x < 0) continue;
                if (device.x >= x - 4.0f && device.x <= x + 4.0f && device.y >= kWaveformY &&
                    device.y <= kWaveformY + kWaveformHeight) {
                    dragTarget_ = k;
                }
            }
        }
        if (dragTarget_ >= 0) {
            if (!ImGui::IsMouseDown(0)) {
                dragTarget_ = -1;
            } else if (device.x >= 0.0f && device.x <= 600.0f) {
                const long p = zoomStart + static_cast<long>(device.x *
                                                            std::max(1, zoomEnd - zoomStart) / 599.0f);
                int frame = std::clamp(static_cast<int>(p * (len - 1) / kMax16), 0, len - 1);
                int* targets[4] = {&inst->startFrame, &inst->loopStart, &inst->loopEnd,
                                   &inst->endFrame};
                const int loopEnd = (inst->loopEnd > 0) ? inst->loopEnd : len;
                const int end = (inst->endFrame > 0) ? inst->endFrame : len;
                if (dragTarget_ == 0) frame = std::min(frame, end - 1);
                else if (dragTarget_ == 1) frame = std::min(frame, loopEnd - 1);
                else if (dragTarget_ == 2) frame = std::max(frame, inst->loopStart + 1);
                else frame = std::max(frame, inst->startFrame + 1);
                if (*targets[dragTarget_] != frame) {
                    *targets[dragTarget_] = frame;
                    engineHost.publish(song);
                }
            }
        }
    }

    syncFromModel(song, engineHost, selectedInstIdx, view);
}

void SampleScreen::render(ImDrawList* drawList,
                          const ImRect& workspace,
                          ui::LayoutProfile profile) const {
    if (!built_ || drawList == nullptr) return;
    const LayoutContext lc = LayoutContext::fromWorkspace(workspace, profile);
    Canvas canvas(drawList, lc);
    canvas.pushClip(RectI{0, kWorkspaceTop, kDeviceWidth, kWorkspaceSpanY});
    display_.renderAll(canvas);
    canvas.popClip();
}

} // namespace trakr

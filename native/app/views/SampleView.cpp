#include "SampleView.h"
#include "FontLoader.h"
#include "SampleOps.h"
#include "Command.h"
#include "imgui.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

// Trakr Native Sample Playback screen.
// Structural port of core/interface/samplePlayback/gSamplePlay.cpp
// (initDisplayControls) and the NewDisplay controls it instantiates:
//   cFrame frameControl, cLabel label[8], cBgLabel bgLabel, cList playModeListControl,
//   cProgressCursor progressCursor, cLineIndicator granularCursor, cPoints pointsControl,
//   cSlicePoints slicePointsControl, cSpectrum spectrumControl[2], cWtProgress wtPositionCursor.
// NewDisplay composites controls in REVERSE creation order, so the body draws
// wtPositionCursor -> spectrum[1] -> spectrum[0] -> slicePoints -> points ->
// granularCursor -> progressCursor -> playModeList -> bgLabel -> labels -> frame.
// label[]/bgLabel are the app ButtonDeck chrome; frameControl is drawn here for
// the play-mode list place (index 6).

namespace trakr {
using namespace sample_detail;

namespace {

size_t waveLength(const Instrument& inst) {
    return (inst.sample && !inst.sample->ch[0].empty()) ? inst.sample->ch[0].size() : 0;
}

}  // namespace

SampleView::SampleView() = default;

void SampleView::draw(bool* p_open) { (void)p_open; }

void SampleView::triggerAudition() { auditionStartTime_ = ImGui::GetTime(); }

void SampleView::stopAudition(EngineHost& engineHost) {
    if (auditionStartTime_ > 0.0) {
        auditionStartTime_ = -1.0;
        engineHost.previewNoteOff(-1);
    }
}

void SampleView::adjustParam(Song& song, EngineHost& engineHost, int selectedInstIdx, int delta, bool fine) {
    if (selectedInstIdx < 0 || selectedInstIdx >= static_cast<int>(song.instruments.size())) return;
    auto& inst = song.instruments[selectedInstIdx];
    size_t waveCount = waveLength(inst);
    if (waveCount == 0) return;
    int step = fine ? 64 : 1024;
    switch (activeParam_) {
        case Param::Start:
            inst.startFrame = std::clamp(inst.startFrame + delta * step, 0, (inst.endFrame > 0 ? inst.endFrame : static_cast<int>(waveCount)) - 1);
            break;
        case Param::End: {
            int curEnd = (inst.endFrame > 0) ? inst.endFrame : static_cast<int>(waveCount);
            inst.endFrame = std::clamp(curEnd + delta * step, inst.startFrame + 1, static_cast<int>(waveCount));
            break;
        }
        case Param::LoopStart:
            inst.loopStart = std::clamp(inst.loopStart + delta * step, 0, (inst.loopEnd > 0 ? inst.loopEnd : static_cast<int>(waveCount)) - 1);
            break;
        case Param::LoopEnd: {
            int curLE = (inst.loopEnd > 0) ? inst.loopEnd : static_cast<int>(waveCount);
            inst.loopEnd = std::clamp(curLE + delta * step, inst.loopStart + 1, static_cast<int>(waveCount));
            break;
        }
        case Param::Zoom:
            zoomLevel_ = std::clamp(zoomLevel_ + delta * (fine ? 0.25f : 1.0f), 1.0f, 16.0f);
            break;
        case Param::PlayMode:
            inst.loop = static_cast<Instrument::Loop>((static_cast<int>(inst.loop) + delta + 8) % 8);
            break;
        case Param::Slices:
            break;
    }
    engineHost.publish(song);
}

void SampleView::applyEditOp(Song& song, History& history, EngineHost& engineHost, int selectedInstIdx) {
    if (selectedInstIdx < 0 || selectedInstIdx >= static_cast<int>(song.instruments.size())) return;
    auto& inst = song.instruments[selectedInstIdx];
    if (!inst.sample || inst.sample->ch[0].empty()) return;
    int s = inst.startFrame;
    int e = (inst.endFrame > 0) ? inst.endFrame : static_cast<int>(inst.sample->ch[0].size());
    if (s >= e) return;
    std::shared_ptr<SampleData> newSample;
    std::string opLabel = "Edit Sample";
    switch (selectedEditOp_) {
        case EditOp::Crop: newSample = SampleOps::crop(*inst.sample, s, e); opLabel = "Crop Sample"; inst.startFrame = 0; inst.endFrame = static_cast<int>(newSample->ch[0].size()); break;
        case EditOp::Reverse: newSample = SampleOps::reverse(*inst.sample, s, e); opLabel = "Reverse Sample"; break;
        case EditOp::Normalize: newSample = SampleOps::normalize(*inst.sample, s, e); opLabel = "Normalize Sample"; break;
        case EditOp::Amplify: newSample = SampleOps::amplify(*inst.sample, s, e, 1.5f); opLabel = "Amplify Sample"; break;
        case EditOp::FadeIn: newSample = SampleOps::fadeIn(*inst.sample, s, e); opLabel = "Fade In Sample"; break;
        case EditOp::FadeOut: newSample = SampleOps::fadeOut(*inst.sample, s, e); opLabel = "Fade Out Sample"; break;
        case EditOp::Silence: newSample = SampleOps::silence(*inst.sample, s, e); opLabel = "Silence Sample"; break;
    }
    if (newSample) {
        history.execute(std::make_unique<SetSampleDataCommand>(selectedInstIdx, newSample, inst.sample, opLabel), song);
        inst.sample = newSample;
        engineHost.publish(song);
    }
}

std::vector<hw::ScreenButton> SampleView::getScreenButtons(Song& song, EngineHost& engineHost, int selectedInstIdx) {
    static History s_dummyHistory;
    return getScreenButtons(song, s_dummyHistory, engineHost, selectedInstIdx);
}

std::vector<hw::ScreenButton> SampleView::getScreenButtons(Song& song, History& history, EngineHost& engineHost, int selectedInstIdx) {
    std::vector<hw::ScreenButton> btns(8);
    if (selectedInstIdx < 0 || selectedInstIdx >= static_cast<int>(song.instruments.size())) {
        for (int i = 0; i < 8; ++i) btns[i] = {"", "", "", "", hw::UnderlineColor::None, false, false, nullptr};
        return btns;
    }
    auto& inst = song.instruments[selectedInstIdx];
    size_t waveCount = waveLength(inst);
    float sRate = (inst.sample && inst.sample->sampleRate > 0) ? static_cast<float>(inst.sample->sampleRate) : 44100.0f;
    char startBuf[16], endBuf[16], lsBuf[16], leBuf[16], zBuf[16];
    snprintf(startBuf, sizeof(startBuf), "%.3fs", static_cast<float>(inst.startFrame) / sRate);
    int curEnd = (inst.endFrame > 0) ? inst.endFrame : static_cast<int>(waveCount);
    snprintf(endBuf, sizeof(endBuf), "%.3fs", static_cast<float>(curEnd) / sRate);
    snprintf(lsBuf, sizeof(lsBuf), "%.3fs", static_cast<float>(inst.loopStart) / sRate);
    int curLE = (inst.loopEnd > 0) ? inst.loopEnd : static_cast<int>(waveCount);
    snprintf(leBuf, sizeof(leBuf), "%.3fs", static_cast<float>(curLE) / sRate);
    snprintf(zBuf, sizeof(zBuf), "%.2f", zoomLevel_);

    if (subPage_ == SubPage::Editor) {
        if (editorButtonPage_ == 0) {
            btns[0] = {"Apply", kEditOps[static_cast<int>(selectedEditOp_)], "", "", hw::UnderlineColor::Green, true, false, [&]() { applyEditOp(song, history, engineHost, selectedInstIdx); }};
            btns[1] = {"Start", startBuf, "", "", hw::UnderlineColor::Green, true, (activeParam_ == Param::Start), [&]() { activeParam_ = Param::Start; }};
            btns[2] = {"End", endBuf, "", "", hw::UnderlineColor::Green, true, (activeParam_ == Param::End), [&]() { activeParam_ = Param::End; }};
            btns[3] = {"Zoom", zBuf, "", "", hw::UnderlineColor::Green, true, (activeParam_ == Param::Zoom),
                       [&]() { activeParam_ = Param::Zoom; zoomLevel_ = (zoomLevel_ == 1.0f) ? 2.0f : (zoomLevel_ == 2.0f) ? 4.0f : 1.0f; }};
            btns[4] = {"Undo", "", "", "", history.canUndo() ? hw::UnderlineColor::Green : hw::UnderlineColor::Dimmed, history.canUndo(), false,
                       [&]() { history.undo(song); engineHost.publish(song); }};
            btns[5] = {"Preview", "", "", "", hw::UnderlineColor::Green, true, isAuditioning(), [&]() {
                if (isAuditioning()) stopAudition(engineHost);
                else { triggerAudition(); engineHost.previewNoteOn(-1, selectedInstIdx, inst.rootNote, 0.9f); }
            }};
            btns[6] = {"Playback", "Page", "", "", hw::UnderlineColor::Orange, true, false, [&]() { subPage_ = SubPage::Playback; }};
            btns[7] = {"More", "...", "", "", hw::UnderlineColor::None, true, false, [this]() { editorButtonPage_ = 1; }};
        } else {
            btns[0] = {"Crop", "", "", "", hw::UnderlineColor::Green, true, (selectedEditOp_ == EditOp::Crop), [&]() { selectedEditOp_ = EditOp::Crop; applyEditOp(song, history, engineHost, selectedInstIdx); }};
            btns[1] = {"Reverse", "", "", "", hw::UnderlineColor::Green, true, (selectedEditOp_ == EditOp::Reverse), [&]() { selectedEditOp_ = EditOp::Reverse; applyEditOp(song, history, engineHost, selectedInstIdx); }};
            btns[2] = {"Normalize", "", "", "", hw::UnderlineColor::Green, true, (selectedEditOp_ == EditOp::Normalize), [&]() { selectedEditOp_ = EditOp::Normalize; applyEditOp(song, history, engineHost, selectedInstIdx); }};
            btns[3] = {"Fade In", "", "", "", hw::UnderlineColor::Green, true, (selectedEditOp_ == EditOp::FadeIn), [&]() { selectedEditOp_ = EditOp::FadeIn; applyEditOp(song, history, engineHost, selectedInstIdx); }};
            btns[4] = {"Fade Out", "", "", "", hw::UnderlineColor::Green, true, (selectedEditOp_ == EditOp::FadeOut), [&]() { selectedEditOp_ = EditOp::FadeOut; applyEditOp(song, history, engineHost, selectedInstIdx); }};
            btns[5] = {"Silence", "", "", "", hw::UnderlineColor::Red, true, (selectedEditOp_ == EditOp::Silence), [&]() { selectedEditOp_ = EditOp::Silence; applyEditOp(song, history, engineHost, selectedInstIdx); }};
            btns[6] = {"Playback", "Page", "", "", hw::UnderlineColor::Orange, true, false, [&]() { subPage_ = SubPage::Playback; }};
            btns[7] = {"Back", "", "", "", hw::UnderlineColor::None, true, false, [this]() { editorButtonPage_ = 0; }};
        }
        return btns;
    }

    // gSamplePlay.cpp bottom cLabels (specs/sample.json): Preview/- , Start,
    // Loop Start, Loop End, End, Zoom, Play Mode (dim), (blank). The desktop
    // Sample Editor remains reachable from the menu / R0 screen button.
    btns[0] = {"Preview", "-", "", "", hw::UnderlineColor::Green, true, isAuditioning(), [&]() {
        if (isAuditioning()) stopAudition(engineHost);
        else { triggerAudition(); engineHost.previewNoteOn(-1, selectedInstIdx, inst.rootNote, 0.9f); }
    }};
    btns[1] = {"Start", startBuf, "", "", hw::UnderlineColor::Green, true, (activeParam_ == Param::Start), [&]() { activeParam_ = Param::Start; }};
    btns[2] = {"Loop Start", lsBuf, "", "", hw::UnderlineColor::Green, true, (activeParam_ == Param::LoopStart), [&]() { activeParam_ = Param::LoopStart; }};
    btns[3] = {"Loop End", leBuf, "", "", hw::UnderlineColor::Green, true, (activeParam_ == Param::LoopEnd), [&]() { activeParam_ = Param::LoopEnd; }};
    btns[4] = {"End", endBuf, "", "", hw::UnderlineColor::Green, true, (activeParam_ == Param::End), [&]() { activeParam_ = Param::End; }};
    btns[5] = {"Zoom", zBuf, "", "", hw::UnderlineColor::Green, true, (activeParam_ == Param::Zoom), [&]() {
        activeParam_ = Param::Zoom; zoomLevel_ = (zoomLevel_ == 1.0f) ? 2.0f : (zoomLevel_ == 2.0f) ? 4.0f : 1.0f;
    }};
    btns[6] = {"Play Mode", "", "", "", hw::UnderlineColor::Dimmed, true, (activeParam_ == Param::PlayMode), [&]() {
        activeParam_ = Param::PlayMode; inst.loop = static_cast<Instrument::Loop>((static_cast<int>(inst.loop) + 1) % 8); engineHost.publish(song);
    }};
    btns[7] = {"", "", "", "", hw::UnderlineColor::None, false, false, nullptr};
    return btns;
}

void SampleView::update(Song& song, History& history, EngineHost& engineHost, int selectedInstIdx) {
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar;
    if (!ImGui::Begin("Sample Playback", nullptr, flags)) { ImGui::End(); return; }
    if (selectedInstIdx < 0 || selectedInstIdx >= static_cast<int>(song.instruments.size())) { ImGui::End(); return; }
    auto& inst = song.instruments[selectedInstIdx];

    if (ImGui::IsKeyPressed(ImGuiKey_Space, false)) {
        triggerAudition();
        engineHost.previewNoteOn(-1, selectedInstIdx, inst.rootNote, 0.9f);
    }
    if (ImGui::IsKeyReleased(ImGuiKey_Space)) stopAudition(engineHost);

    ImGuiIO& io = ImGui::GetIO();
    const bool isShift = io.KeyShift || shiftHeld_;
    if (ImGui::IsWindowHovered() || !ImGui::IsAnyItemActive()) {
        if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) adjustParam(song, engineHost, selectedInstIdx, 1, isShift);
        if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) adjustParam(song, engineHost, selectedInstIdx, -1, isShift);
        if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) activeParam_ = static_cast<Param>((static_cast<int>(activeParam_) + 6) % 7);
        if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) activeParam_ = static_cast<Param>((static_cast<int>(activeParam_) + 1) % 7);
        if (io.MouseWheel != 0.0f) adjustParam(song, engineHost, selectedInstIdx, (io.MouseWheel > 0) ? 1 : -1, isShift);
    }

    const ImVec2 avail = ImGui::GetContentRegionAvail();
    ImFont* font = getMonoLightFont();
    if (avail.x < 100.0f || avail.y < 100.0f || !font) { ImGui::End(); return; }
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    sample_detail::SampleControls c;
    c.dl = ImGui::GetWindowDrawList();
    c.origin = origin;
    c.sx = avail.x / 800.0f;
    c.sy = avail.y / 397.0f;
    c.song = &song;
    c.engine = &engineHost;
    c.inst = &inst;
    c.sample = inst.sample.get();
    c.len = static_cast<int>(waveLength(inst));
    c.rate = (inst.sample && inst.sample->sampleRate > 0) ? static_cast<float>(inst.sample->sampleRate) : 44100.0f;
    c.stereo = inst.sample && inst.sample->channels > 1 && !inst.sample->ch[1].empty();
    c.zoom = zoomLevel_;
    if (zoomLevel_ > 1.0f) {
        const int width = static_cast<int>(kMax16 / zoomLevel_);
        c.zoomStart = std::clamp(kMax16 / 2 - width / 2, 0, kMax16);
        c.zoomEnd = std::min(kMax16, c.zoomStart + width);
    }
    c.activeParam = static_cast<int>(activeParam_);
    c.editOp = static_cast<int>(selectedEditOp_);
    c.editor = subPage_ == SubPage::Editor;
    c.auditioning = isAuditioning();
    if (c.auditioning) {
        const double elapsed = ImGui::GetTime() - auditionStartTime_;
        const double dur = c.len > 0 ? static_cast<double>(c.len) / c.rate : 1.0;
        const float frac = dur > 0.0 ? static_cast<float>(elapsed / dur) : 1.0f;
        c.progress = std::clamp(frac, 0.0f, 1.0f) * 600.0f;
        if (elapsed > dur) stopAudition(engineHost);
    }
    c.font = font;
    c.fontSize = 14.0f * c.sy;  // list style font0: RobotoMono Light 14
    c.draw();

    if (c.clickedMode >= 0) {
        inst.loop = static_cast<Instrument::Loop>(c.clickedMode);
        engineHost.publish(song);
    }
    if (c.clickedEdit >= 0) {
        selectedEditOp_ = static_cast<EditOp>(c.clickedEdit);
        if (c.applyEdit) applyEditOp(song, history, engineHost, selectedInstIdx);
    }
    ImGui::SetCursorScreenPos(origin);
    ImGui::End();
}

}  // namespace trakr

#include "AppLayout.h"
#include "views/PatternEditHelper.h"
#include "Command.h"
#include <algorithm>
#include <cmath>
#include <string>

namespace trakr {

AppLayout::AppLayout() = default;

void AppLayout::render(ScreenContext& scrCtx,
                       Screen& activeScreen,
                       AppLayoutState& layoutState,
                       bool& showFillModal,
                       bool& showExportDeck,
                       bool& showKitGallery,
                       bool& showShortcutsModal,
                       const std::string& currentFilePath,
                       RenderScheduler& scheduler) {
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    if (!vp) return;

    // 1. Compute geometry for display canvas and button deck
    display_.computeLayout(vp->WorkPos, vp->WorkSize, layoutState.showButtonDeck, layoutState.wideMode);
    display_.renderBezel();

    // 2. Determine title string
    char titleBuf[48];
    const char* pageTitle = "Pattern";
    int curPatIdx = (scrCtx.currentOrderIdx >= 0 && scrCtx.currentOrderIdx < static_cast<int>(scrCtx.song.order.size())) ? scrCtx.song.order[scrCtx.currentOrderIdx] : 0;
    switch (activeScreen) {
        case Screen::Pattern:
            snprintf(titleBuf, sizeof(titleBuf), "Pattern %d", curPatIdx + 1);
            pageTitle = titleBuf;
            break;
        case Screen::Song:       pageTitle = "Song"; break;
        case Screen::Sample:     pageTitle = (scrCtx.sampleView.subPage() == SampleView::SubPage::Editor) ? "Sample Editor" : "Sample Playback"; break;
        case Screen::Instrument: {
            auto sub = scrCtx.instrumentView.subPage();
            if (sub == InstrumentView::SubPage::Effects) pageTitle = "Instrument Effects";
            else if (sub == InstrumentView::SubPage::Automation) pageTitle = "Instrument Automation 2/2";
            else pageTitle = "Instrument Parameters 1/2";
            break;
        }
        case Screen::Master:
            pageTitle = (scrCtx.mixerView.subPage() == MixerView::SubPage::TrackMixer) ? "Track Mixer 2/3"
                      : (scrCtx.mixerView.subPage() == MixerView::SubPage::Master) ? "Master 1/3" : "Global Mixer 3/3";
            break;
        case Screen::Perform:    pageTitle = "Performance FX"; break;
        case Screen::SampleLoader: pageTitle = "Sample Loader"; break;
        case Screen::Config: {
            static bool s_cfgInit = false;
            // Firmware opens Config on the first base menu page (Project
            // Settings -> "Performance presets"), matching selectedBase=0.
            if (!s_cfgInit) { s_cfgInit = true; scrCtx.configView.setActiveGroup(ConfigView::Group::ProjectSettings); }
            pageTitle = "Config"; break;
        }
        case Screen::File: pageTitle = "File"; break;
    }

    bool effectiveShift = layoutState.shiftHeld || ImGui::GetIO().KeyShift;
    scrCtx.patternView.setShiftHeld(effectiveShift);
    scrCtx.instrumentView.setShiftHeld(effectiveShift);
    scrCtx.sampleView.setShiftHeld(effectiveShift);

    // 3. Render Title / Header Bar (y=0..28 logical)
    const bool firmwareChrome = (activeScreen == Screen::Instrument || activeScreen == Screen::Master);
    const char* rightOverride = (activeScreen == Screen::File)
                                  ? scrCtx.fileView.projectName().c_str()
                                  : ((activeScreen == Screen::Master) ? "" : nullptr);
    headerBar_.render(display_.headerRect(),
                      pageTitle,
                      scrCtx.song,
                      scrCtx.engineHost,
                      scrCtx.currentOrderIdx,
                      scrCtx.selectedInstIdx,
                      scrCtx.recordMode,
                      scrCtx.theme,
                      display_.scale(),
                      scrCtx.patternView.cursorRow(),
                      rightOverride,
                      firmwareChrome,
                      // rightLabel (@rightLabel) exists only for instrument/pattern/sample.
                      (activeScreen == Screen::Instrument || activeScreen == Screen::Pattern ||
                       activeScreen == Screen::Sample),
                      activeScreen != Screen::Pattern,
                      // Transport (songTimer) is only composed on the Pattern screen.
                      activeScreen == Screen::Pattern);

    // 4. Render Central Workspace
    const bool guiNextInstrument =
        layoutState.guiNext && activeScreen == Screen::Instrument &&
        scrCtx.instrumentView.subPage() == InstrumentView::SubPage::General;
    const bool guiNextMaster =
        layoutState.guiNext && activeScreen == Screen::Master &&
        scrCtx.mixerView.subPage() == MixerView::SubPage::Master;
    if (guiNextInstrument || guiNextMaster) {
        const ImRect workspace = display_.workspaceRect();
        ImGui::SetNextWindowPos(workspace.Min);
        ImGui::SetNextWindowSize(workspace.GetSize());
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                                       ImGuiWindowFlags_NoSavedSettings |
                                       ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoBringToFrontOnFocus;
        const char* windowName = guiNextInstrument ? "##gui_next_instrument" : "##gui_next_master";
        if (ImGui::Begin(windowName, nullptr, flags)) {
            if (guiNextInstrument) {
                guiInstrument_.update(scrCtx.song, scrCtx.history, scrCtx.engineHost,
                                      scrCtx.selectedInstIdx, scrCtx.instrumentView, workspace,
                                      ui::LayoutProfile::Classic, layoutState.shiftHeld);
                guiInstrument_.render(ImGui::GetWindowDrawList(), workspace,
                                      ui::LayoutProfile::Classic);
            } else {
                guiMaster_.update(scrCtx.song, scrCtx.engineHost, scrCtx.mixerView, workspace,
                                  ui::LayoutProfile::Classic, layoutState.shiftHeld);
                guiMaster_.render(ImGui::GetWindowDrawList(), workspace,
                                  ui::LayoutProfile::Classic);
            }
        }
        ImGui::End();
        ImGui::PopStyleVar(3);
    } else {
        renderActiveScreen(activeScreen, scrCtx, display_.workspaceRect());
    }

    // 5. Update and render 8 Contextual Screen Buttons (y=424..480 logical)
    updateScreenButtons(activeScreen, scrCtx, effectiveShift, showFillModal, showExportDeck, scheduler);
    ImRect btnRect = display_.buttonsRect();
    if (activeScreen == Screen::Perform) {
        // Firmware performance bar background ends at y=478 (row 479 stays black)
        btnRect.Max.y -= 1.0f * display_.scale();
    }
    screenButtons_.setPatternChrome(activeScreen == Screen::Pattern);
    screenButtons_.render(btnRect, effectiveShift, display_.scale());
    screenButtons_.handleKeyShortcuts(effectiveShift);

    // 6. Render On-Screen Hardware Button Deck (if visible)
    if (layoutState.showButtonDeck && display_.deckRect().GetHeight() > 20.0f) {
        buttonDeck_.render(display_.deckRect(),
                           scrCtx.song,
                           scrCtx.engineHost,
                           scrCtx.history,
                           scrCtx.patternView,
                           activeScreen,
                           scrCtx.recordMode,
                           scrCtx.octave,
                           scrCtx.editStep,
                           layoutState.shiftHeld,
                           scrCtx.selectedInstIdx,
                           scrCtx.currentOrderIdx,
                           display_.scale(),
                           &scrCtx.instrumentView,
                           &layoutState.showMenuPopup,
                           &scrCtx.performView,
                           &scrCtx.sampleView,
                           &scrCtx.mixerView,
                           &showExportDeck);
    }

    // 7. Render Authentic Module Menu Popup Overlay if active
    if (layoutState.showMenuPopup) {
        menuPopup_.render(display_.workspaceRect(), activeScreen, layoutState.showMenuPopup,
                          showExportDeck, scrCtx, display_.scale());
    }
}

void AppLayout::updateScreenButtons(Screen activeScreen,
                                    ScreenContext& scrCtx,
                                    bool shiftHeld,
                                    bool& showFillModal,
                                    bool& showExportDeck,
                                    RenderScheduler& scheduler) {
    std::vector<hw::ScreenButton> btns(8);

    int curPat = (scrCtx.currentOrderIdx >= 0 && scrCtx.currentOrderIdx < static_cast<int>(scrCtx.song.order.size())) ? scrCtx.song.order[scrCtx.currentOrderIdx] : 0;
    int patLen = (!scrCtx.song.patterns.empty() && curPat < static_cast<int>(scrCtx.song.patterns.size())) ? scrCtx.song.patterns[curPat].rows : 64;

    switch (activeScreen) {
        case Screen::Pattern: {
            if (shiftHeld) {
                // Tracker Firmware Parity: Shift held displays Track 1..8 Mute/Unmute buttons (gPatternEdit.cpp line 1167)
                for (int i = 0; i < 8; ++i) {
                    bool isMuted = (i < static_cast<int>(scrCtx.song.tracks.size())) ? scrCtx.song.tracks[i].mute : false;
                    std::string trkName = (i < static_cast<int>(scrCtx.song.tracks.size()) && !scrCtx.song.tracks[i].name.empty()) ? scrCtx.song.tracks[i].name : ("Track " + std::to_string(i + 1));
                    btns[i] = {trkName, isMuted ? "Unmute" : "Mute", "", "",
                               isMuted ? hw::UnderlineColor::Dimmed : hw::UnderlineColor::Green,
                               true, isMuted,
                               [&scrCtx, &scheduler, i, isMuted]() {
                                   if (i < static_cast<int>(scrCtx.song.tracks.size())) {
                                       scrCtx.song.tracks[i].mute = !isMuted;
                                       scrCtx.engineHost.publish(scrCtx.song);
                                       scheduler.requestRedraw();
                                   }
                               }};
                }
            } else if (patternButtonPage_ == 0) {
                btns[0] = {"Pattern", std::to_string(curPat + 1), "", "", hw::UnderlineColor::Green, true, true, [&]() {
                    if (scrCtx.song.patterns.empty()) return;
                    scrCtx.song.order[scrCtx.currentOrderIdx] = (curPat + 1) % static_cast<int>(scrCtx.song.patterns.size());
                    scrCtx.engineHost.publish(scrCtx.song); scheduler.requestRedraw();
                }};
                btns[1] = {"Length", std::to_string(patLen), "", "", hw::UnderlineColor::Green, true, false, [&]() {
                    if (scrCtx.song.patterns.empty()) return;
                    int newRows = (patLen == 16) ? 32 : (patLen == 32) ? 64 : (patLen == 64) ? 128 : (patLen == 128) ? 256 : (patLen == 256) ? 512 : (patLen == 512) ? 1024 : 16;
                    scrCtx.history.execute(std::make_unique<SetPatternLengthCommand>(curPat, newRows, patLen), scrCtx.song);
                    scrCtx.engineHost.publish(scrCtx.song); scheduler.requestRedraw();
                }};
                btns[2] = {"Step jump", std::to_string(scrCtx.editStep), "", "", hw::UnderlineColor::Green, true, false, [&]() {
                    int steps[] = {0, 1, 2, 3, 4, 8, 16};
                    int curIdx = 1; for (int i = 0; i < 7; ++i) if (steps[i] == scrCtx.editStep) curIdx = i;
                    scrCtx.editStep = steps[(curIdx + 1) % 7]; scheduler.requestRedraw();
                }};
                btns[3] = {"Fill", "", "", "", hw::UnderlineColor::Red, true, showFillModal, [&]() {
                    showFillModal = !showFillModal; if (showFillModal) showExportDeck = false; scheduler.requestRedraw();
                }};
                btns[4] = {"Preview", "", "", "", hw::UnderlineColor::Green, true, false, [&]() {
                    scrCtx.engineHost.play(scrCtx.currentOrderIdx, scrCtx.patternView.cursorRow()); scheduler.requestRedraw();
                }};
                btns[5] = {"Undo", "", "", "", hw::UnderlineColor::Red, scrCtx.history.canUndo(), false, [&]() {
                    scrCtx.history.undo(scrCtx.song); scrCtx.engineHost.publish(scrCtx.song); scheduler.requestRedraw();
                }};
                btns[6] = {"Redo", "", "", "", hw::UnderlineColor::Red, scrCtx.history.canRedo(), false, [&]() {
                    scrCtx.history.redo(scrCtx.song); scrCtx.engineHost.publish(scrCtx.song); scheduler.requestRedraw();
                }};
                btns[7] = {"More", "...", "", "", hw::UnderlineColor::None, true, false, [&]() { patternButtonPage_ = 1; scheduler.requestRedraw(); }};
            } else if (patternButtonPage_ == 1) {
                auto& pat = scrCtx.song.patterns[curPat];
                btns[0] = {"Render", "Selection", "", "", hw::UnderlineColor::Green, true, showExportDeck, [&]() {
                    showExportDeck = !showExportDeck; if (showExportDeck) showFillModal = false; scheduler.requestRedraw();
                }};
                btns[1] = {"Invert", "Pattern", "", "", hw::UnderlineColor::Green, true, false, [&]() {
                    invertSelectionOrPattern(pat, curPat, scrCtx.patternView.hasSelection(), scrCtx.patternView.getSelection(), scrCtx.song, scrCtx.history, scrCtx.engineHost);
                    scheduler.requestRedraw();
                }};
                btns[2] = {"Duplicate", "Pattern", "", "", hw::UnderlineColor::Green, true, false, [&]() {
                    duplicatePattern(pat, curPat, scrCtx.song, scrCtx.history, scrCtx.engineHost); scheduler.requestRedraw();
                }};
                btns[3] = {"Expand", "Pattern", "", "", hw::UnderlineColor::Green, true, false, [&]() {
                    expandPattern(pat, curPat, scrCtx.song, scrCtx.history, scrCtx.engineHost); scheduler.requestRedraw();
                }};
                btns[4] = {"Shrink", "Pattern", "", "", hw::UnderlineColor::Green, true, false, [&]() {
                    shrinkPattern(pat, curPat, scrCtx.song, scrCtx.history, scrCtx.engineHost); scheduler.requestRedraw();
                }};
                btns[5] = {"Copy", "Pattern", "", "", hw::UnderlineColor::Green, true, false, [&]() {
                    copyPattern(pat, patternClipboard_); scheduler.requestRedraw();
                }};
                btns[6] = {"Paste", "Pattern", "", "", patternClipboard_.tracks.empty() ? hw::UnderlineColor::Dimmed : hw::UnderlineColor::Green, !patternClipboard_.tracks.empty(), false, [&]() {
                    pastePattern(pat, curPat, patternClipboard_, scrCtx.song, scrCtx.history, scrCtx.engineHost); scheduler.requestRedraw();
                }};
                btns[7] = {"More", "...", "", "", hw::UnderlineColor::None, true, false, [&]() { patternButtonPage_ = 2; scheduler.requestRedraw(); }};
            } else {
                // Page 3: Pattern Name, Track Name, Fill Options, Clear Track, Clear Pattern, More
                btns[0] = {"Pattern", "Name", "", "", hw::UnderlineColor::Green, true, false, [&]() { scheduler.requestRedraw(); }};
                btns[1] = {"Track", "Name", "", "", hw::UnderlineColor::Green, true, false, [&]() { scheduler.requestRedraw(); }};
                btns[2] = {"Fill", "Options", "", "", hw::UnderlineColor::Green, true, showFillModal, [&]() { showFillModal = !showFillModal; scheduler.requestRedraw(); }};
                btns[3] = {"Clear", "Track", "", "", hw::UnderlineColor::Red, true, false, [&]() {
                    int ct = scrCtx.patternView.cursorTrack();
                    if (curPat >= 0 && curPat < static_cast<int>(scrCtx.song.patterns.size()) && ct >= 0 && ct < static_cast<int>(scrCtx.song.patterns[curPat].tracks.size())) {
                        for (auto& cl : scrCtx.song.patterns[curPat].tracks[ct]) cl = Cell{};
                        scrCtx.engineHost.publish(scrCtx.song); scheduler.requestRedraw();
                    }
                }};
                btns[4] = {"Clear", "Pattern", "", "", hw::UnderlineColor::Red, true, false, [&]() {
                    if (curPat >= 0 && curPat < static_cast<int>(scrCtx.song.patterns.size())) {
                        for (auto& trk : scrCtx.song.patterns[curPat].tracks) for (auto& cl : trk) cl = Cell{};
                        scrCtx.engineHost.publish(scrCtx.song); scheduler.requestRedraw();
                    }
                }};
                btns[5] = {"", "", "", "", hw::UnderlineColor::None, false, false, nullptr};
                btns[6] = {"", "", "", "", hw::UnderlineColor::None, false, false, nullptr};
                btns[7] = {"More", "...", "", "", hw::UnderlineColor::None, true, false,
                           [&]() { patternButtonPage_ = 0; scheduler.requestRedraw(); }};
            }
            break;
        }
        case Screen::Song: {
            bool isPlaying = scrCtx.engineHost.getPlayhead().playing;
            int curSlotPat = (scrCtx.currentOrderIdx >= 0 && scrCtx.currentOrderIdx < static_cast<int>(scrCtx.song.order.size()))
                             ? scrCtx.song.order[scrCtx.currentOrderIdx] : 0;
            // gSongEdit.cpp bottom cLabels (specs/song.json): Play/Pattern,
            // Play/Song, Undo, Add/Slot, Delete/Slot, Pattern, (blank),
            // Tempo/<bpm>. Firmware labels; actions keep the same behaviour.
            btns[0] = {"Play", "Pattern", "", "", hw::UnderlineColor::Green, true, isPlaying, [&]() {
                if (isPlaying) scrCtx.engineHost.stopPlayback();
                else { scrCtx.engineHost.setPatternLoop(true); scrCtx.engineHost.setPosition(scrCtx.currentOrderIdx, 0); scrCtx.engineHost.play(); }
                scheduler.requestRedraw();
            }};
            btns[1] = {"Play", "Song", "", "", hw::UnderlineColor::Green, true, false, [&]() {
                if (isPlaying) scrCtx.engineHost.stopPlayback();
                else { scrCtx.engineHost.setPatternLoop(false); scrCtx.engineHost.play(scrCtx.currentOrderIdx, 0); }
                scheduler.requestRedraw();
            }};
            btns[2] = {"Undo", "", "", "", scrCtx.history.canUndo() ? hw::UnderlineColor::Red : hw::UnderlineColor::Dimmed,
                       scrCtx.history.canUndo(), false, [&]() {
                scrCtx.history.undo(scrCtx.song); scrCtx.engineHost.publish(scrCtx.song); scheduler.requestRedraw();
            }};
            btns[3] = {shiftHeld ? "Add Empty" : "Add", "Slot", "", "", hw::UnderlineColor::Red, true, false, [&]() {
                int newPat = shiftHeld ? static_cast<int>(scrCtx.song.patterns.size()) : curSlotPat;
                if (shiftHeld) { Pattern np; np.rows = 64; np.tracks.resize(8); scrCtx.song.patterns.push_back(np); }
                scrCtx.song.order.insert(scrCtx.song.order.begin() + scrCtx.currentOrderIdx + 1, newPat);
                scrCtx.currentOrderIdx++; scrCtx.engineHost.publish(scrCtx.song); scheduler.requestRedraw();
            }};
            btns[4] = {"Delete", "Slot", "", "", (scrCtx.song.order.size() > 1) ? hw::UnderlineColor::Red : hw::UnderlineColor::Dimmed,
                       scrCtx.song.order.size() > 1, false, [&]() {
                if (scrCtx.song.order.size() > 1) {
                    scrCtx.song.order.erase(scrCtx.song.order.begin() + scrCtx.currentOrderIdx);
                    if (scrCtx.currentOrderIdx >= static_cast<int>(scrCtx.song.order.size()))
                        scrCtx.currentOrderIdx = static_cast<int>(scrCtx.song.order.size()) - 1;
                    scrCtx.engineHost.publish(scrCtx.song); scheduler.requestRedraw();
                }
            }};
            btns[5] = {"Pattern", "", "", "", hw::UnderlineColor::Dimmed, true, false, [&]() {
                if (!scrCtx.song.patterns.empty()) {
                    scrCtx.song.order[scrCtx.currentOrderIdx] = (curSlotPat + 1) % scrCtx.song.patterns.size();
                    scrCtx.engineHost.publish(scrCtx.song); scheduler.requestRedraw();
                }
            }};
            btns[6] = {"", "", "", "", hw::UnderlineColor::None, false, false, nullptr};
            {
                char tempoBuf[24];
                std::snprintf(tempoBuf, sizeof(tempoBuf), "%.1f BPM", scrCtx.song.bpm);
                btns[7] = {"Tempo", tempoBuf, "", "", hw::UnderlineColor::Green, true, false, nullptr};
            }
            break;
        }
        case Screen::Sample: {
            btns = scrCtx.sampleView.getScreenButtons(scrCtx.song, scrCtx.history, scrCtx.engineHost, scrCtx.selectedInstIdx);
            break;
        }
        case Screen::Instrument: {
            btns = scrCtx.instrumentView.getScreenButtons(scrCtx.song, scrCtx.engineHost, scrCtx.selectedInstIdx);
            break;
        }
        case Screen::Master: {
            if (scrCtx.mixerView.subPage() == MixerView::SubPage::TrackMixer) {
                int selTrack = scrCtx.mixerView.selectedTrack();
                for (int i = 0; i < 8; ++i) {
                    if (i < static_cast<int>(scrCtx.song.tracks.size())) {
                        auto& trk = scrCtx.song.tracks[i];
                        char dbStr[16];
                        if (std::abs(trk.gainDb) < 0.005f) snprintf(dbStr, sizeof(dbStr), "0.00dB");
                        else snprintf(dbStr, sizeof(dbStr), "%.2fdB", trk.gainDb);
                        std::string line2 = trk.mute ? "Muted" : (trk.solo ? "Solo" : dbStr);
                        hw::UnderlineColor uCol = trk.mute ? hw::UnderlineColor::Red : (trk.solo ? hw::UnderlineColor::Orange : hw::UnderlineColor::Green);
                        bool isSel = (i == selTrack);
                        btns[i] = {"Track " + std::to_string(i + 1), line2, "", "", uCol, true, isSel, [&scrCtx, &trk, shiftHeld, &scheduler]() {
                            if (shiftHeld) trk.solo = !trk.solo; else trk.mute = !trk.mute;
                            scrCtx.engineHost.publish(scrCtx.song); scheduler.requestRedraw();
                        }};
                    } else {
                        btns[i] = {"Track " + std::to_string(i + 1), "---", "", "", hw::UnderlineColor::Dimmed, false, false, nullptr};
                    }
                }
            } else if (scrCtx.mixerView.subPage() == MixerView::SubPage::Master) {
                // gMasterParams.cpp showMasterScreen: eight parameter labels; the
                // page buttons are not part of the firmware bottom bar.
                char vbuf[32];
                snprintf(vbuf, sizeof(vbuf), "%.2f dB", scrCtx.song.masterGainDb);
                btns[0] = {"Volume", vbuf, "", "", hw::UnderlineColor::None, true, false, [&scrCtx]() { scrCtx.mixerView.setSelectedTrack(0); }};
                btns[1] = {"Reverb", "", "", "", hw::UnderlineColor::None, true, false, [&scrCtx]() { scrCtx.mixerView.setSelectedTrack(1); }};
                btns[2] = {"Delay", "", "", "", hw::UnderlineColor::None, true, false, [&scrCtx]() { scrCtx.mixerView.setSelectedTrack(2); }};
                btns[3] = {"Limiter", "", "", "", hw::UnderlineColor::None, true, false, [&scrCtx]() { scrCtx.mixerView.setSelectedTrack(3); }};
                btns[4] = {"EQ", "", "", "", hw::UnderlineColor::None, true, false, [&scrCtx]() { scrCtx.mixerView.setSelectedTrack(4); }};
                static const char* kSatNames[5] = {"Hard Clip", "Soft Clip", "Saturate", "Overdrive", "Drive+"};
                int sat = std::clamp(scrCtx.song.masterSaturationType, 0, 4);
                btns[5] = {"Saturation", kSatNames[sat], "", "", hw::UnderlineColor::None, true, false, [&scrCtx]() { scrCtx.mixerView.setSelectedTrack(5); }};
                snprintf(vbuf, sizeof(vbuf), "%d", std::clamp(static_cast<int>(std::lround(scrCtx.song.masterBassBoost * 100.0f)), 0, 100));
                btns[6] = {"Bass Boost", vbuf, "", "", hw::UnderlineColor::None, true, false, [&scrCtx]() { scrCtx.mixerView.setSelectedTrack(6); }};
                snprintf(vbuf, sizeof(vbuf), "%d", std::clamp(static_cast<int>(std::lround(scrCtx.song.masterSpace * 8.0f)), 0, 8));
                btns[7] = {"Space", vbuf, "", "", hw::UnderlineColor::None, true, false, [&scrCtx]() { scrCtx.mixerView.setSelectedTrack(7); }};
            } else {
                btns[0] = {"EQ 115", "Bass", "", "", hw::UnderlineColor::Green, true, false, nullptr};
                btns[1] = {"EQ 330", "LowMid", "", "", hw::UnderlineColor::Green, true, false, nullptr};
                btns[2] = {"EQ 990", "Mid", "", "", hw::UnderlineColor::Green, true, false, nullptr};
                btns[3] = {"EQ 3k", "HighMid", "", "", hw::UnderlineColor::Green, true, false, nullptr};
                btns[4] = {"EQ 9.9k", "Treble", "", "", hw::UnderlineColor::Green, true, false, nullptr};
                btns[5] = {"Delay", "Send 1", "", "", hw::UnderlineColor::Green, true, false, nullptr};
                btns[6] = {"Reverb", "Send 2", "", "", hw::UnderlineColor::Green, true, false, nullptr};
                btns[7] = {"Track", "Mixer", "", "", hw::UnderlineColor::Orange, true, false, [&scrCtx, &scheduler]() {
                    scrCtx.mixerView.setSubPage(MixerView::SubPage::TrackMixer); scheduler.requestRedraw();
                }};
            }
            break;
        }
        case Screen::Perform: {
            // gPerformance.cpp: the eight bottom cLabels show the track name in
            // colors[0] (grey) and the assigned pattern in colors[3], only for
            // tracks that have a pattern. Render all names uniformly (no dim /
            // fill); tracks without a pattern keep a blank second line so the
            // names stay on the two-line baseline.
            for (int i = 0; i < 8; ++i) {
                bool en = scrCtx.performView.isTrackEnabled(i);
                btns[i] = {"Track " + std::to_string(i + 1), (i < 3 ? "Patt: 1" : " "), "", "",
                           hw::UnderlineColor::None,
                           true, en, [&scrCtx, i, &scheduler]() {
                               scrCtx.performView.toggleTrack(i);
                               scheduler.requestRedraw();
                           }};
            }
            break;
        }
        case Screen::SampleLoader: {
            btns = scrCtx.sampleLoaderView.getScreenButtons(scrCtx.song, scrCtx.history, scrCtx.engineHost, scrCtx.selectedInstIdx, activeScreen, true);
            break;
        }
        case Screen::Config: {
            btns = scrCtx.configView.getScreenButtons(scrCtx.song, scrCtx.engineHost, scrCtx.theme, activeScreen, true);
            break;
        }
        case Screen::File: {
            btns = scrCtx.fileView.getScreenButtons(scrCtx.song, scrCtx.history, scrCtx.engineHost,
                                                    activeScreen, showExportDeck, true);
            break;
        }
    }

    // Every firmware screen renders its bottom bar as 8 cLabels + cBgLabel
    // (grey name + white value, no underline/frame/orange fill). Pattern is the
    // exception: it additionally needs the gPatternEdit selection-frame chrome,
    // so it keeps the patternChrome path instead of the plain firmware bar.
    screenButtons_.setFirmwareMode(activeScreen != Screen::Pattern);
    screenButtons_.setButtons(btns);
}

} // namespace trakr


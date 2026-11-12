#include "ButtonDeck.h"
#include "Song.h"
#include "EngineHost.h"
#include "History.h"
#include "Command.h"
#include "ScreenManager.h"
#include "views/PatternView.h"
#include "views/InstrumentView.h"
#include "views/PerformView.h"
#include "views/SampleView.h"
#include "views/MixerView.h"
#include "imgui_internal.h"
#include <cmath>
#include <algorithm>

namespace trakr::hw {

ButtonDeck::ButtonDeck() = default;

ImRect ButtonDeck::buttonBounds(const std::string& name) const {
    for (const auto& b : buttonBoundsList_) {
        if (b.name == name) return b.rect;
    }
    return ImRect(ImVec2(0, 0), ImVec2(0, 0));
}

void ButtonDeck::render(const ImRect& rect, trakr::Song& song, trakr::EngineHost& engineHost,
                        trakr::History& history, trakr::PatternView& patternView,
                        trakr::Screen& activeScreen, bool& recordMode, int& octave,
                        int& editStep, bool& shiftHeld, int& selectedInstIdx,
                        int& currentOrderIdx, float scale, trakr::InstrumentView* iv,
                        bool* pShowMenuPopup, trakr::PerformView* performView,
                        trakr::SampleView* sv, trakr::MixerView* mixerView, bool* pShowExportDeck) {
    if (rect.GetWidth() <= 0.0f || rect.GetHeight() <= 0.0f) return;
    (void)pShowExportDeck;

    ImGui::SetNextWindowPos(rect.Min);
    ImGui::SetNextWindowSize(rect.GetSize());
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    if (!ImGui::Begin("##TrackerButtonDeck", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                      ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
        ImGui::End(); ImGui::PopStyleVar(2); return;
    }

    ImDrawList* dl = ImGui::GetWindowDrawList();
    if (!dl) { ImGui::End(); ImGui::PopStyleVar(2); return; }

    buttonBoundsList_.clear();
    dl->AddRectFilled(rect.Min, rect.Max, IM_COL32(17, 19, 23, 255));

    float availW = rect.GetWidth(), availH = rect.GetHeight();
    int patIdx = (currentOrderIdx >= 0 && currentOrderIdx < static_cast<int>(song.order.size())) ? song.order[currentOrderIdx] : 0;
    if (patIdx < 0 || patIdx >= static_cast<int>(song.patterns.size())) patIdx = 0;
    auto& pat = song.patterns[patIdx];

    // 1. Left/Center: 4x12 Silicone Pad Matrix ("Keyboard")
    const float kPadGap = 2.0f * scale;
    float padSize = std::floor(std::min((availW * 0.47f - 11.0f * kPadGap) / 12.0f, (availH - 20.0f * scale) / 4.0f));
    float totalGridW = 12.0f * padSize + 11.0f * kPadGap;
    float padGridH = 4.0f * padSize + 3.0f * kPadGap;
    float padStartY = rect.Min.y + std::max(6.0f * scale, (availH - padGridH) * 0.5f);
    ImRect padArea(ImVec2(rect.Min.x + 8.0f * scale, padStartY),
                   ImVec2(rect.Min.x + 8.0f * scale + totalGridW, padStartY + padGridH));
    bool isShift = shiftHeld || ImGui::GetIO().KeyShift;
    renderPads(dl, padArea, song, engineHost, history, patternView, patIdx, selectedInstIdx, recordMode, octave, editStep, isShift, activeScreen, performView);

    // 2. Center Section: Authentic Polyend Button Matrix (6 cols x 4 rows)
    float rX = padArea.Max.x + 10.0f * scale;
    float btnW = 44.0f * scale, gap = 2.5f * scale;
    float btnH = std::min(25.0f * scale, (availH - 24.0f * scale) / 4.0f);
    float btnStartY = rect.Min.y + std::max(6.0f * scale, (availH - (4.0f * btnH + 3.0f * gap)) * 0.5f);

    auto drawKey = [&](const char* lbl, const ImVec2& pos, ImU32 textCol, ImU32 bgCol, auto onClick) {
        ImRect kRect(pos, ImVec2(pos.x + btnW, pos.y + btnH));
        buttonBoundsList_.push_back({lbl, kRect});
        bool hov = kRect.Contains(ImGui::GetIO().MousePos);
        if (hov && ImGui::GetIO().MouseClicked[0]) onClick();
        dl->AddRectFilled(kRect.Min, kRect.Max, hov ? IM_COL32(50, 52, 60, 255) : bgCol, 2.5f);
        dl->AddRect(kRect.Min, kRect.Max, IM_COL32(55, 58, 66, 255), 2.5f, 0, 1.0f);
        ImVec2 sz = ImGui::CalcTextSize(lbl);
        dl->AddText(ImVec2(pos.x + (btnW - sz.x) * 0.5f, pos.y + (btnH - sz.y) * 0.5f), textCol, lbl);
    };

    // Row 0: Screen Selectors (Pattern, Song, Perform, Master, Params, Sample)
    float y0 = btnStartY;
    const char* r0Lbls[6] = {"Pattern", "Song", "Perform", "Master", "Params", "Sample"};
    Screen r0Screens[6] = {Screen::Pattern, Screen::Song, Screen::Perform, Screen::Master, Screen::Instrument, Screen::Sample};
    for (int i = 0; i < 6; ++i) {
        bool act = (activeScreen == r0Screens[i]);
        drawKey(r0Lbls[i], ImVec2(rX + i * (btnW + gap), y0),
                act ? IM_COL32(0, 0, 0, 255) : IM_COL32(200, 200, 200, 255),
                act ? IM_COL32(230, 96, 16, 255) : IM_COL32(28, 30, 36, 255), [&, i]() {
                    if (r0Screens[i] == Screen::Instrument && activeScreen == Screen::Instrument && iv) iv->togglePage();
                    else if (r0Screens[i] == Screen::Sample && activeScreen == Screen::Sample && sv) sv->togglePage();
                    else activeScreen = r0Screens[i];
                });
    }

    // Row 1: Sub-columns & File/Config (Note, Instr, FX1, FX2, File, Config)
    float y1 = y0 + btnH + gap;
    const char* r1Lbls[6] = {"Note", "Instr", "FX1", "FX2", "File", "Config"};
    const ImU32 r1Cols[4] = {IM_COL32(0, 232, 190, 255), IM_COL32(254, 247, 73, 255), IM_COL32(187, 88, 241, 255), IM_COL32(87, 241, 255, 255)};
    for (int i = 0; i < 4; ++i) {
        drawKey(r1Lbls[i], ImVec2(rX + i * (btnW + gap), y1), IM_COL32(0, 0, 0, 255), r1Cols[i], [&, i]() {
            patternView.setCursor(patternView.cursorTrack(), patternView.cursorRow(), i);
        });
    }
    bool fileAct = (activeScreen == Screen::File);
    drawKey("File", ImVec2(rX + 4 * (btnW + gap), y1), fileAct ? IM_COL32(0, 0, 0, 255) : IM_COL32(200, 200, 200, 255),
            fileAct ? IM_COL32(230, 96, 16, 255) : IM_COL32(28, 30, 36, 255), [&]() { activeScreen = Screen::File; });
    bool cfgAct = (activeScreen == Screen::Config);
    drawKey("Config", ImVec2(rX + 5 * (btnW + gap), y1), cfgAct ? IM_COL32(0, 0, 0, 255) : IM_COL32(200, 200, 200, 255),
            cfgAct ? IM_COL32(230, 96, 16, 255) : IM_COL32(28, 30, 36, 255), [&]() { activeScreen = Screen::Config; });

    // Row 2: Transport & Edit (Play, Rec, Insert, Delete, Copy, Menu)
    float y2 = y1 + btnH + gap;
    bool playing = engineHost.getPlayhead().playing;
    drawKey(playing ? "STOP" : "Play", ImVec2(rX, y2), playing ? IM_COL32(0, 0, 0, 255) : IM_COL32(230, 230, 230, 255),
            playing ? IM_COL32(48, 203, 63, 255) : IM_COL32(32, 34, 40, 255),
            [&]() { if (playing) engineHost.stopPlayback(); else engineHost.play(currentOrderIdx, patternView.cursorRow()); });
    drawKey("Rec", ImVec2(rX + (btnW + gap), y2), recordMode ? IM_COL32(255, 255, 255, 255) : IM_COL32(200, 200, 200, 255),
            recordMode ? IM_COL32(205, 24, 24, 255) : IM_COL32(45, 25, 25, 255), [&]() { recordMode = !recordMode; });
    drawKey(isShift ? "Home" : "Insert", ImVec2(rX + (btnW + gap) * 2, y2), IM_COL32(200, 200, 200, 255), IM_COL32(28, 30, 36, 255), [&]() {
        if (isShift) patternView.setCursor(patternView.cursorTrack(), 0, patternView.cursorSubCol());
        else patternView.insertRowAtCursor(pat, patIdx, song, history, engineHost);
    });
    drawKey(isShift ? "DelRow" : "Delete", ImVec2(rX + (btnW + gap) * 3, y2), IM_COL32(200, 200, 200, 255), IM_COL32(28, 30, 36, 255), [&]() {
        if (activeScreen == Screen::Instrument && iv) iv->resetSelectedParam(song, engineHost, selectedInstIdx);
        else if (activeScreen == Screen::Master && mixerView) mixerView->resetSelected(song, engineHost);
        else patternView.deleteRowOrCellAtCursor(pat, patIdx, song, history, engineHost, isShift);
    });
    drawKey(isShift ? "Paste" : "Copy", ImVec2(rX + (btnW + gap) * 4, y2), IM_COL32(200, 200, 200, 255), IM_COL32(28, 30, 36, 255), [&]() {
        if (isShift) patternView.pasteAtCursor(pat, patIdx, song, history, engineHost); else patternView.copySelectionOrCell(pat);
    });
    bool menuOpen = (pShowMenuPopup && *pShowMenuPopup);
    drawKey("Menu", ImVec2(rX + (btnW + gap) * 5, y2), menuOpen ? IM_COL32(0, 0, 0, 255) : IM_COL32(200, 200, 200, 255),
            menuOpen ? IM_COL32(254, 247, 73, 255) : IM_COL32(28, 30, 36, 255), [&]() { if (pShowMenuPopup) *pShowMenuPopup = !*pShowMenuPopup; });

    // Row 3: Modifiers (Shift, 1, 2, 3, 4, More)
    float y3 = y2 + btnH + gap;
    drawKey("Shift", ImVec2(rX, y3), isShift ? IM_COL32(0, 0, 0, 255) : IM_COL32(220, 220, 220, 255),
            isShift ? IM_COL32(254, 247, 73, 255) : IM_COL32(28, 30, 36, 255),
            [&]() { shiftHeld = !shiftHeld; isShift = shiftHeld || ImGui::GetIO().KeyShift; });
    for (int n = 1; n <= 4; ++n) {
        char nStr[8]; snprintf(nStr, sizeof(nStr), "%d", n);
        bool octAct = (octave == (isShift ? (n + 4) : n));
        drawKey(nStr, ImVec2(rX + n * (btnW + gap), y3), octAct ? IM_COL32(0, 0, 0, 255) : IM_COL32(180, 180, 180, 255),
                octAct ? IM_COL32(254, 247, 73, 255) : IM_COL32(24, 26, 32, 255), [&, n]() { octave = isShift ? (n + 4) : n; });
    }
    drawKey("More", ImVec2(rX + 5 * (btnW + gap), y3), IM_COL32(180, 180, 180, 255), IM_COL32(28, 30, 36, 255), [&]() {
        if (pShowMenuPopup) *pShowMenuPopup = !*pShowMenuPopup;
    });

    // 3. Navigation D-Pad (Top-Right) & Jog Wheel (Bottom-Right) - Proportional, zero overlap
    float rightColW = rect.Max.x - (rX + 6 * (btnW + gap));
    float colCenterX = (rX + 6 * (btnW + gap) + rect.Max.x) * 0.5f;
    float dpadSize = std::min({availH * 0.30f, 62.0f * scale, rightColW * 0.88f});
    ImVec2 dpadCenter(colCenterX, rect.Min.y + availH * 0.28f);
    renderDpad(dl, dpadCenter, dpadSize, patternView, song, engineHost, patIdx, isShift,
               selectedInstIdx, activeScreen, editStep, iv, sv, mixerView, currentOrderIdx);

    float jogRadius = std::min({availH * 0.23f, 48.0f * scale, rightColW * 0.46f});
    ImVec2 jogCenter(colCenterX, rect.Min.y + availH * 0.73f);
    jogCenter_ = jogCenter; jogRadius_ = jogRadius;
    renderJogWheel(dl, jogCenter, jogRadius, patternView, song, history, engineHost, patIdx,
                   recordMode, isShift, iv, activeScreen, selectedInstIdx, sv, mixerView, currentOrderIdx);

    ImGui::End(); ImGui::PopStyleVar(2);
}

void ButtonDeck::renderDpad(ImDrawList* dl, const ImVec2& center, float size,
                            trakr::PatternView& pv, trakr::Song& song,
                            trakr::EngineHost& engineHost, int patIdx, bool isShift,
                            int& selectedInstIdx, trakr::Screen activeScreen, int editStep,
                            trakr::InstrumentView* iv, trakr::SampleView* sv,
                            trakr::MixerView* mv, int& currentOrderIdx) {
    float armW = size * 0.32f, armL = size * 0.48f;
    ImVec2 mPos = ImGui::GetIO().MousePos;
    bool clicked = ImGui::GetIO().MouseClicked[0];
    int numRows = song.patterns.empty() ? 64 : song.patterns[patIdx].rows;
    int numTracks = song.patterns.empty() ? 8 : static_cast<int>(song.patterns[patIdx].tracks.size());
    auto& pat = song.patterns[patIdx];

    static float s_dpadHoldTime = 0.0f;
    static const char* s_lastHeldDpad = nullptr;
    auto drawArrow = [&](int dir, const ImRect& r, ImU32 col) {
        ImVec2 c = r.GetCenter();
        float s = armW * 0.22f;
        if (dir == 0) dl->AddTriangleFilled(ImVec2(c.x, c.y - s), ImVec2(c.x - s, c.y + s), ImVec2(c.x + s, c.y + s), col);
        else if (dir == 1) dl->AddTriangleFilled(ImVec2(c.x, c.y + s), ImVec2(c.x - s, c.y - s), ImVec2(c.x + s, c.y - s), col);
        else if (dir == 2) dl->AddTriangleFilled(ImVec2(c.x - s, c.y), ImVec2(c.x + s, c.y - s), ImVec2(c.x + s, c.y + s), col);
        else if (dir == 3) dl->AddTriangleFilled(ImVec2(c.x + s, c.y), ImVec2(c.x - s, c.y - s), ImVec2(c.x - s, c.y + s), col);
        else dl->AddCircleFilled(c, s * 0.70f, col);
    };
    auto dpadBtn = [&](const char* name, int dir, const ImRect& r, auto onClick) {
        buttonBoundsList_.push_back({name, r});
        bool hov = r.Contains(mPos), shouldTrigger = false;
        if (hov && ImGui::GetIO().MouseDown[0]) {
            if (clicked) { shouldTrigger = true; s_dpadHoldTime = 0.0f; s_lastHeldDpad = name; }
            else if (s_lastHeldDpad == name) {
                float dt = ImGui::GetIO().DeltaTime, prev = s_dpadHoldTime;
                s_dpadHoldTime += dt;
                if (s_dpadHoldTime > 0.28f && static_cast<int>((s_dpadHoldTime - 0.28f) / 0.06f) > static_cast<int>((prev - 0.28f) / 0.06f)) shouldTrigger = true;
            }
        } else if (s_lastHeldDpad == name && !ImGui::GetIO().MouseDown[0]) { s_lastHeldDpad = nullptr; s_dpadHoldTime = 0.0f; }
        if (shouldTrigger) onClick();
        bool active = hov && ImGui::GetIO().MouseDown[0];
        dl->AddRectFilled(r.Min, r.Max, active ? IM_COL32(50, 58, 72, 255) : (hov ? IM_COL32(38, 42, 52, 255) : IM_COL32(24, 26, 32, 255)), 2.5f);
        dl->AddRect(r.Min, r.Max, active ? IM_COL32(0, 232, 190, 220) : (hov ? IM_COL32(0, 232, 190, 100) : IM_COL32(45, 48, 58, 255)), 2.5f, 0, 1.0f);
        drawArrow(dir, r, active ? IM_COL32(0, 232, 190, 255) : (hov ? IM_COL32(255, 255, 255, 255) : IM_COL32(190, 195, 205, 255)));
    };

    dpadBtn("DpadUp", 0, ImRect(ImVec2(center.x - armW * 0.5f, center.y - armL), ImVec2(center.x + armW * 0.5f, center.y - armW * 0.5f)), [&]() {
        if (activeScreen == Screen::Instrument && iv) iv->adjustSelectedParam(song, engineHost, selectedInstIdx, 1, isShift);
        else if (activeScreen == Screen::Sample && sv) sv->adjustParam(song, engineHost, selectedInstIdx, 1, isShift);
        else if (activeScreen == Screen::Master && mv) mv->adjustSelectedParam(song, engineHost, 1, isShift);
        else if (activeScreen == Screen::Song) currentOrderIdx = std::max(0, currentOrderIdx - 1);
        else pv.navigate(0, -1, 0, isShift, numTracks, numRows, pat, song, selectedInstIdx);
    });
    dpadBtn("DpadDown", 1, ImRect(ImVec2(center.x - armW * 0.5f, center.y + armW * 0.5f), ImVec2(center.x + armW * 0.5f, center.y + armL)), [&]() {
        if (activeScreen == Screen::Instrument && iv) iv->adjustSelectedParam(song, engineHost, selectedInstIdx, -1, isShift);
        else if (activeScreen == Screen::Sample && sv) sv->adjustParam(song, engineHost, selectedInstIdx, -1, isShift);
        else if (activeScreen == Screen::Master && mv) mv->adjustSelectedParam(song, engineHost, -1, isShift);
        else if (activeScreen == Screen::Song) currentOrderIdx = std::min(static_cast<int>(song.order.size()) - 1, currentOrderIdx + 1);
        else pv.navigate(0, 1, 0, isShift, numTracks, numRows, pat, song, selectedInstIdx);
    });
    dpadBtn("DpadLeft", 2, ImRect(ImVec2(center.x - armL, center.y - armW * 0.5f), ImVec2(center.x - armW * 0.5f, center.y + armW * 0.5f)), [&]() {
        if (activeScreen == Screen::Instrument && iv) iv->setSelectedColumn(std::max(0, iv->selectedColumn() - 1));
        else if (activeScreen == Screen::Sample && sv) sv->selectNextParam(-1);
        else if (activeScreen == Screen::Master && mv) mv->selectNextTrack(-1);
        else if (activeScreen == Screen::Song && !song.order.empty()) {
            int& p = song.order[currentOrderIdx]; p = (p > 0) ? p - 1 : static_cast<int>(song.patterns.size()) - 1; engineHost.publish(song);
        } else pv.navigate(-1, 0, isShift ? 0 : -1, isShift, numTracks, numRows, pat, song, selectedInstIdx);
    });
    dpadBtn("DpadRight", 3, ImRect(ImVec2(center.x + armW * 0.5f, center.y - armW * 0.5f), ImVec2(center.x + armL, center.y + armW * 0.5f)), [&]() {
        if (activeScreen == Screen::Instrument && iv) iv->setSelectedColumn(std::min(7, iv->selectedColumn() + 1));
        else if (activeScreen == Screen::Sample && sv) sv->selectNextParam(1);
        else if (activeScreen == Screen::Master && mv) mv->selectNextTrack(1);
        else if (activeScreen == Screen::Song && !song.order.empty()) {
            int& p = song.order[currentOrderIdx]; p = (p + 1) % song.patterns.size(); engineHost.publish(song);
        } else pv.navigate(1, 0, isShift ? 0 : 1, isShift, numTracks, numRows, pat, song, selectedInstIdx);
    });
    dpadBtn("DpadCenter", 4, ImRect(ImVec2(center.x - armW * 0.5f, center.y - armW * 0.5f), ImVec2(center.x + armW * 0.5f, center.y + armW * 0.5f)), [&]() {
        if (activeScreen == Screen::Pattern) {
            int st = (editStep > 0) ? editStep : 1;
            pv.navigate(0, isShift ? -st : st, 0, false, numTracks, numRows, pat, song, selectedInstIdx);
        } else if (activeScreen == Screen::Instrument && iv) iv->togglePage();
        else if (activeScreen == Screen::Sample && sv) sv->togglePage();
        else if (activeScreen == Screen::Master && mv) mv->togglePage();
    });
}

void ButtonDeck::renderJogWheel(ImDrawList* dl, const ImVec2& center, float radius,
                                trakr::PatternView& pv, trakr::Song& song,
                                trakr::History& history, trakr::EngineHost& engineHost,
                                int patIdx, bool recordMode, bool isShift,
                                trakr::InstrumentView* iv, trakr::Screen activeScreen, int& selectedInstIdx,
                                trakr::SampleView* sv, trakr::MixerView* mv, int& currentOrderIdx) {
    ImVec2 mPos = ImGui::GetIO().MousePos;
    float dist = std::hypot(mPos.x - center.x, mPos.y - center.y);
    bool hovered = dist <= radius;
    int numRows = song.patterns.empty() ? 64 : song.patterns[patIdx].rows;
    int numTracks = song.patterns.empty() ? 8 : static_cast<int>(song.patterns[patIdx].tracks.size());
    auto& pat = song.patterns[patIdx];

    auto triggerStep = [&](int step) {
        jogAngle_ += step * 0.25f;
        if (activeScreen == trakr::Screen::Instrument && iv) iv->adjustSelectedParam(song, engineHost, selectedInstIdx, step, isShift);
        else if (activeScreen == trakr::Screen::Sample && sv) sv->adjustParam(song, engineHost, selectedInstIdx, step, isShift);
        else if (activeScreen == trakr::Screen::Master && mv) mv->adjustSelectedParam(song, engineHost, step, isShift);
        else if (activeScreen == trakr::Screen::Song && isShift && !song.order.empty()) {
            int& p = song.order[currentOrderIdx];
            p = (p + step + static_cast<int>(song.patterns.size())) % song.patterns.size();
            engineHost.publish(song);
        } else if (recordMode && !isShift) {
            pv.adjustValueAtCursor(pat, patIdx, song, history, engineHost, step, false);
        } else {
            pv.navigate(0, step, 0, isShift, numTracks, numRows, pat, song, selectedInstIdx);
        }
    };

    if (hovered && ImGui::IsMouseClicked(0)) { jogDragging_ = true; lastJogDragAngle_ = std::atan2(mPos.y - center.y, mPos.x - center.x); }
    if (jogDragging_) {
        if (ImGui::IsMouseDown(0)) {
            float cur = std::atan2(mPos.y - center.y, mPos.x - center.x), d = cur - lastJogDragAngle_;
            if (d > 3.14159f) d -= 6.28318f; else if (d < -3.14159f) d += 6.28318f;
            if (std::abs(d) > 0.08f) { triggerStep((d > 0) ? 1 : -1); lastJogDragAngle_ = cur; }
        } else { jogDragging_ = false; }
    }
    float wheel = ImGui::GetIO().MouseWheel;
    if (hovered && std::abs(wheel) > 0.01f) triggerStep((wheel > 0) ? 1 : -1);

    dl->AddCircleFilled(center, radius, IM_COL32(32, 35, 44, 255), 48);
    dl->AddCircle(center, radius, hovered ? IM_COL32(0, 232, 190, 220) : IM_COL32(65, 70, 82, 255), 48, 2.0f);
    dl->AddCircleFilled(center, radius * 0.82f, IM_COL32(20, 22, 28, 255), 48);
    dl->AddCircle(center, radius * 0.82f, IM_COL32(40, 44, 54, 255), 48, 1.0f);

    float notchX = center.x + std::cos(jogAngle_) * (radius * 0.55f);
    float notchY = center.y + std::sin(jogAngle_) * (radius * 0.55f);
    dl->AddCircleFilled(ImVec2(notchX, notchY), radius * 0.14f, IM_COL32(245, 245, 245, 255));
    dl->AddCircle(ImVec2(notchX, notchY), radius * 0.14f, IM_COL32(160, 165, 175, 255), 16, 1.0f);
}

void ButtonDeck::renderPads(ImDrawList* dl, const ImRect& padArea,
                            trakr::Song& song, trakr::EngineHost& engineHost,
                            trakr::History& history, trakr::PatternView& pv,
                            int patIdx, int selectedInstIdx, bool recordMode, int octave, int editStep,
                            bool isShift, trakr::Screen activeScreen, trakr::PerformView* performView) {
    if (padArea.GetWidth() <= 20.0f || padArea.GetHeight() <= 20.0f) return;
    const int kCols = 12, kRows = 4;
    const float kGap = 2.5f;
    float padW = std::floor(std::min((padArea.GetWidth() - (kCols - 1) * kGap) / kCols, (padArea.GetHeight() - (kRows - 1) * kGap) / kRows));
    float padH = padW, totalGridW = kCols * padW + (kCols - 1) * kGap, totalGridH = kRows * padH + (kRows - 1) * kGap;
    float startX = padArea.Min.x, startY = padArea.Min.y + (padArea.GetHeight() - totalGridH) * 0.5f;
    padsBounds_ = ImRect(ImVec2(startX, startY), ImVec2(startX + totalGridW, startY + totalGridH));

    ImVec2 mPos = ImGui::GetIO().MousePos;
    bool mouseDown = ImGui::GetIO().MouseDown[0];

    if (activeScreen == Screen::Perform && performView) {
        const auto& pSlots = performView->slots();
        for (int r = 0; r < kRows; ++r) {
            for (int c = 0; c < kCols; ++c) {
                ImRect pRect(ImVec2(startX + c * (padW + kGap), startY + r * (padH + kGap)),
                             ImVec2(startX + c * (padW + kGap) + padW, startY + r * (padH + kGap) + padH));
                buttonBoundsList_.push_back({"Pad_" + std::to_string(c) + "_" + std::to_string(r), pRect});
                bool hov = pRect.Contains(mPos);
                if (hov && mouseDown && (activePerformCol_ != c || activePerformRow_ != r)) {
                    activePerformCol_ = c; activePerformRow_ = r; performView->triggerSlotPad(c, r, engineHost);
                }
                bool isActive = (c < 12 && pSlots[c].activeRow == r);
                ImU32 pBg = isActive ? ((r == 0) ? IM_COL32(40, 40, 40, 255) : IM_COL32(230, 96, 16, 255))
                                     : (hov ? IM_COL32(42, 45, 55, 255) : IM_COL32(24, 26, 32, 255));
                dl->AddRectFilled(pRect.Min, pRect.Max, pBg, 2.0f);
                dl->AddRect(pRect.Min, pRect.Max, isActive ? IM_COL32(255, 255, 255, 255) : IM_COL32(40, 44, 52, 255), 2.0f, 0, 1.0f);
                const char* lbl = (c < 12 && r < 4) ? pSlots[c].labels[r].c_str() : "";
                ImVec2 sz = ImGui::CalcTextSize(lbl);
                dl->AddText(ImVec2(pRect.Min.x + (padW - sz.x) * 0.5f, pRect.Min.y + (padH - sz.y) * 0.5f),
                            isActive ? (r == 0 ? IM_COL32(240, 240, 240, 255) : IM_COL32(255, 255, 255, 255)) : (r == 0 ? IM_COL32(48, 203, 63, 255) : IM_COL32(180, 185, 195, 255)), lbl);
            }
        }
        if (!mouseDown) { activePerformCol_ = -1; activePerformRow_ = -1; }
        return;
    }

    int baseNote = octave * 12;
    static const char* kNoteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    static const bool kIsAccidental[] = {false, true, false, true, false, false, true, false, true, false, true, false};

    for (int r = 0; r < kRows; ++r) {
        for (int c = 0; c < kCols; ++c) {
            ImRect pRect(ImVec2(startX + c * (padW + kGap), startY + r * (padH + kGap)),
                         ImVec2(startX + c * (padW + kGap) + padW, startY + r * (padH + kGap) + padH));
            buttonBoundsList_.push_back({"Pad_" + std::to_string(c) + "_" + std::to_string(r), pRect});
            int note = std::clamp(baseNote + (3 - r) * 12 + c, 0, 119), semi = note % 12;
            bool isRoot = (semi == 0), isAcc = kIsAccidental[semi], hov = pRect.Contains(mPos);

            if (hov && mouseDown && activePadNote_ != note) {
                if (activePadNote_ >= 0) engineHost.previewNoteOff(-1);
                activePadNote_ = note;
                engineHost.previewNoteOn(-1, selectedInstIdx, note, 0.9f);
                if (recordMode && !isShift && patIdx >= 0 && patIdx < static_cast<int>(song.patterns.size())) {
                    auto& p = song.patterns[patIdx];
                    if (pv.cursorTrack() < static_cast<int>(p.tracks.size()) && pv.cursorRow() < p.rows) {
                        Cell oldCell = p.tracks[pv.cursorTrack()][pv.cursorRow()], newCell = oldCell;
                        newCell.note = static_cast<int16_t>(note);
                        newCell.instrument = static_cast<int16_t>(selectedInstIdx);
                        if (newCell.volume < 0) newCell.volume = 40;
                        history.execute(std::make_unique<SetCellCommand>(patIdx, pv.cursorTrack(), pv.cursorRow(), newCell, oldCell), song);
                        engineHost.publish(song);
                        pv.setCursor(pv.cursorTrack(), std::min(p.rows - 1, pv.cursorRow() + editStep), pv.cursorSubCol());
                    }
                }
            }
            ImU32 pBg = (activePadNote_ == note) ? IM_COL32(0, 232, 190, 255)
                      : (hov ? IM_COL32(50, 56, 68, 255) : (isAcc ? IM_COL32(24, 27, 32, 255) : (isRoot ? IM_COL32(40, 48, 58, 255) : IM_COL32(34, 38, 46, 255))));
            dl->AddRectFilled(pRect.Min, pRect.Max, pBg, 2.0f);
            dl->AddRect(pRect.Min, pRect.Max, isRoot ? IM_COL32(0, 232, 190, 120) : IM_COL32(20, 22, 26, 255), 2.0f, 0, 1.0f);
            char lbl[8]; snprintf(lbl, sizeof(lbl), "%s%d", kNoteNames[semi], note / 12);
            ImVec2 sz = ImGui::CalcTextSize(lbl);
            dl->AddText(ImVec2(pRect.Min.x + (padW - sz.x) * 0.5f, pRect.Min.y + (padH - sz.y) * 0.5f),
                        (activePadNote_ == note) ? IM_COL32(0, 0, 0, 255) : (isRoot ? IM_COL32(0, 232, 190, 255) : IM_COL32(180, 185, 195, 255)), lbl);
        }
    }
    if (!mouseDown && activePadNote_ >= 0) { engineHost.previewNoteOff(-1); activePadNote_ = -1; }
}

} // namespace trakr::hw

#include "PatternView.h"
#include "PatternEditHelper.h"
#include "PatternFxHelper.h"
#include "kit/Widgets.h"
#include "StepFx.h"
#include "Command.h"
#include "Theme.h"
#include "FontLoader.h"
#include "imgui.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cmath>
#include <memory>

// Trakr Pattern screen.
// Structural port of:
//   core/interface/patternEditor/gPatternEdit.cpp        (initDisplayControls)
//   core/libs/NewDisplay/tracker.cpp                    (cTracker)
//   core/libs/NewDisplay/horizontalTracker.cpp          (cHorizontalTracker)
//   core/libs/NewDisplay/frame.cpp                      (cFrame)
// Control creation order in initDisplayControls(): frameControl, label[8],
// bgLabel, fill-popup controls (param1/2/3 lists, instrument lists, val bars,
// patternPopupTitleLabel, patternPopupLabel), verticalPatternControl,
// horizontalPatternControl. NewDisplay composites in REVERSE creation order, so
// renderPatternScreen() draws the tracker first, the (hidden) fill popup, then
// bgLabel/labels (app ButtonDeck chrome), then frameControl. Bottom chrome and
// the title bar stay in the app layer.
namespace trakr {
PatternView::PatternView() = default;

void PatternView::draw(bool* p_open) {
    if (p_open && !*p_open) return;
    ImGui::Text("PatternView must be drawn via update() with active song model.");
}
std::string PatternView::formatNoteName(int16_t note) {
    if (note == kNoNote || note < 0) return "---";
    if (note == kNoteOff) return "OFF";
    if (note > 119) return "???";
    static const char* kNotes[] = {"C-", "C#", "D-", "D#", "E-", "F-", "F#", "G-", "G#", "A-", "A#", "B-"};
    return std::string(kNotes[note % 12]) + std::to_string(note / 12);
}

void PatternView::syncInstrumentFromCursor(const Pattern& pat, const Song& song, int& selectedInstIdx) {
    if (song.instruments.empty() || cursorTrack_ < 0 || cursorTrack_ >= static_cast<int>(pat.tracks.size())) return;
    const auto& trk = pat.tracks[cursorTrack_];
    if (cursorRow_ >= 0 && cursorRow_ < static_cast<int>(trk.size())) {
        int inst = trk[cursorRow_].instrument;
        if (inst >= 0 && inst < static_cast<int>(song.instruments.size())) { selectedInstIdx = inst; return; }
    }
}

void PatternView::navigate(int dTrack, int dRow, int dSubCol, bool isShift, int numTracks, int numRows, const Pattern& pat, const Song& song, int& selectedInstIdx) {
    navigateCursor(dTrack, dRow, dSubCol, isShift, numTracks, numRows, cursorTrack_, cursorRow_, cursorSubCol_, hasSelection_, selRange_);
    if (!isShift) syncInstrumentFromCursor(pat, song, selectedInstIdx);
}

// handleKeyboardInput() is defined in PatternEditHelper.cpp.

void PatternView::insertRowAtCursor(Pattern& pat, int patIdx, Song& s, History& h, EngineHost& e) { trakr::insertRowAtCursor(pat, patIdx, cursorTrack_, cursorRow_, s, h, e); }
void PatternView::deleteRowOrCellAtCursor(Pattern& pat, int patIdx, Song& s, History& h, EngineHost& e, bool sh) { trakr::deleteRowOrCellAtCursor(pat, patIdx, cursorTrack_, cursorRow_, cursorSubCol_, s, h, e, sh, 1); }
void PatternView::copySelectionOrCell(Pattern& pat) { trakr::copySelectionOrCell(pat, cursorTrack_, cursorRow_, hasSelection_, selRange_, clipboard_); }
void PatternView::pasteAtCursor(Pattern& pat, int patIdx, Song& s, History& h, EngineHost& e) { trakr::pasteAtCursor(pat, patIdx, cursorTrack_, cursorRow_, clipboard_, s, h, e); }
void PatternView::adjustValueAtCursor(Pattern& pat, int patIdx, Song& s, History& h, EngineHost& e, int d, bool sh) { trakr::adjustValueAtCursor(pat, patIdx, cursorTrack_, cursorRow_, cursorSubCol_, d, sh, s, h, e); }

// Step FX catalogue and the app-only FX inspector / waveform strip. Defined
// here (the sole caller) to keep PatternFxHelper.cpp focused on the firmware
// tracker pass.
static const std::vector<FxDefinition> kFxDefs = {
    { FxType::None,              '-', "None",           "No effect on this step" },
    { FxType::Off,               '!', "Off",            "Toggle off the effect used on previous step" },
    { FxType::Volume,            'V', "Volume",         "Sets the volume of an instrument (0..100%)" },
    { FxType::Panning,           'P', "Panning",        "Sets the panning of an instrument (-50..+50)" },
    { FxType::Microtuning,       'M', "Micro-tune",     "Sets pitch microtuning (-99..+99 cents)" },
    { FxType::Glide,             'G', "Glide",          "Pitch travel speed to corresponding note" },
    { FxType::Tempo,             'T', "Tempo",          "Changes pattern tempo (10..400 BPM)" },
    { FxType::Swing,             'I', "Swing",          "Changes pattern groove swing (25..75%)" },
    { FxType::MicroMove,         'm', "Micro-move",     "Nudges forward step position (0..100%)" },
    { FxType::GateLength,        'q', "Gate Length",    "Sets length of note (0..100% of step)" },
    { FxType::Chance,            'C', "Chance",         "Sets probability or condition for playing" },
    { FxType::Roll,              'R', "Roll",           "Step divider roll (0..16, types R/Rv/RV/Rn/RN/RR)" },
    { FxType::Arp,               'A', "Arp",            "Arpeggiate with chord divider" },
    { FxType::RandomNote,        'n', "Random Note",    "Random note offset (+/- 0..100)" },
    { FxType::RandomInstrument,  'i', "Random Instr",   "Random instrument number offset (+/- 0..47)" },
    { FxType::RandomValue,       'x', "Random Value",   "Randomize FX value on the same step" },
    { FxType::RandomVolume,      'v', "Random Volume",  "Random volume variation range" },
    { FxType::Reverse,           'r', "Reverse",        "Reverse sample playback" },
    { FxType::Position,          'p', "Position",       "Sample playback start position (0..100%)" },
    { FxType::SampleSlice,       'S', "Slice",          "Sample slice selection index" },
    { FxType::VolumeLfo,         'g', "Volume LFO",     "Volume LFO rate speed" },
    { FxType::PanningLfo,        'h', "Panning LFO",    "Panning LFO rate speed" },
    { FxType::FilterLfo,         'j', "Filter LFO",     "Filter LFO rate speed" },
    { FxType::PositionLfo,       'k', "Position LFO",   "Position LFO rate speed" },
    { FxType::FinetuneLfo,       'l', "Finetune LFO",   "Finetune LFO rate speed" },
    { FxType::Overdrive,         'D', "Overdrive",      "Instrument overdrive saturation (0..100%)" },
    { FxType::FilterLowpass,     'L', "Low-pass",       "Low-pass filter cutoff frequency (0..100%)" },
    { FxType::FilterBandpass,    'B', "Band-pass",      "Band-pass filter cutoff frequency (0..100%)" },
    { FxType::FilterHighpass,    'H', "High-pass",      "High-pass filter cutoff frequency (0..100%)" },
    { FxType::DelaySend,         's', "Delay Send",     "Send level to common master delay (0..100%)" },
    { FxType::ReverbSend,        't', "Reverb Send",    "Send level to common master reverb (0..100%)" },
    { FxType::BitDepth,          'E', "Bit Depth",      "Bit depth reduction (4..16 bits)" },
    { FxType::Tune,              'U', "Tune",           "Pitch tuning offset in semitones" },
    { FxType::SlideUp,           'F', "Slide Up",       "Pitch bend sliding upwards" },
    { FxType::SlideDown,         'J', "Slide Down",     "Pitch bend sliding downwards" },
    { FxType::AmpEnv,            'w', "WT Vol Env",     "Wavetable volume envelope" },
    { FxType::FilterEnv,         'y', "WT Filter Env",  "Wavetable filter envelope" },
    { FxType::GranularLength,    'z', "Granular Length","Granular grain window duration" },
    { FxType::SendCcA,           'a', "MIDI CC A",      "MIDI CC A / Macro 1 message" },
    { FxType::SendCcB,           'b', "MIDI CC B",      "MIDI CC B / Macro 2 message" },
    { FxType::SendCcC,           'c', "MIDI CC C",      "MIDI CC C / Macro 3 message" },
    { FxType::SendCcD,           'd', "MIDI CC D",      "MIDI CC D / Macro 4 message" },
    { FxType::SendCcE,           'e', "MIDI CC E",      "MIDI CC E / Macro 5 message" },
    { FxType::SendCcF,           'f', "MIDI CC F",      "MIDI CC F / Macro 6 message" },
    { FxType::MidiChord,         '0', "MIDI Chord",     "MIDI chord inversion / structure" },
    { FxType::BreakPattern,      'K', "Break Pattern",  "Prematurely finish pattern and jump to next" },
};

char fxTypeToChar(uint8_t type) {
    for (const auto& def : kFxDefs) if (def.type == type) return def.ch;
    if (type >= 'A' && type <= 'z') return static_cast<char>(type);
    return '-';
}

uint8_t fxCharToType(char ch) {
    for (const auto& def : kFxDefs) if (def.ch == ch) return def.type;
    for (const auto& def : kFxDefs)
        if (std::toupper(static_cast<unsigned char>(def.ch)) == std::toupper(static_cast<unsigned char>(ch)))
            return def.type;
    return FxType::None;
}

const char* fxTypeName(uint8_t type) {
    for (const auto& def : kFxDefs) if (def.type == type || def.ch == type) return def.name;
    return (type == 0) ? "None" : "FX";
}

const std::vector<FxDefinition>& getAllFxDefinitions() { return kFxDefs; }

void formatFxCell(char* outBuf, size_t bufSize, uint8_t type, uint8_t val) {
    if (!outBuf || bufSize == 0) return;
    if (type == 0) { snprintf(outBuf, bufSize, "----"); return; }
    if (type == FxType::Roll) {
        int rType = val / 17, period = val % 17;
        const char* pfx = "R ";
        if (rType == 1) pfx = "RN";
        else if (rType == 2) pfx = "Rn";
        else if (rType == 3) pfx = "RR";
        else if (rType == 4) pfx = "Rv";
        else if (rType == 5) pfx = "RV";
        snprintf(outBuf, bufSize, "%s%2u", pfx, period);
    } else {
        snprintf(outBuf, bufSize, "%c %2d", fxTypeToChar(type), static_cast<int>(val));
    }
}

void renderFxInspector(Cell& cell, int cursorSubCol, int patIdx, int cursorTrack, int cursorRow,
                       History& history, Song& song, EngineHost& engineHost, float width) {
    ImGui::BeginChild("##fx_inspector", ImVec2(width, 0), true);
    kit::PanelHeader("STEP FX INSPECTOR", "HOTKEYS");
    int activeSlot = (cursorSubCol == 4) ? 1 : 0;
    FxCmd& activeFx = cell.fx[activeSlot];

    char slotTitle[48];
    snprintf(slotTitle, sizeof(slotTitle), "SLOT %d: %c%02X (%s)", activeSlot + 1,
             fxTypeToChar(activeFx.type), activeFx.value, fxTypeName(activeFx.type));
    ImGui::TextColored(ImVec4(0.0f, 0.9f, 0.85f, 1.0f), "%s", slotTitle);
    ImGui::Separator();
    int val = static_cast<int>(activeFx.value);
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Value:");
    ImGui::SameLine();
    if (kit::ValueBox("##fx_val_box", &val, 0, 255, true, ImVec2(48, 20))) {
        Cell oldCell = cell;
        cell.fx[activeSlot].value = static_cast<uint8_t>(val);
        history.execute(std::make_unique<SetCellCommand>(patIdx, cursorTrack, cursorRow, cell, oldCell), song);
        engineHost.publish(song);
    }
    ImGui::SameLine();
    if (kit::FlatButton("Clear", ImVec2(50, 20))) {
        Cell oldCell = cell;
        cell.fx[activeSlot] = FxCmd{};
        history.execute(std::make_unique<SetCellCommand>(patIdx, cursorTrack, cursorRow, cell, oldCell), song);
        engineHost.publish(song);
    }
    ImGui::Spacing();
    ImGui::TextDisabled("Select Step FX Type:");
    ImGui::BeginChild("##fx_list", ImVec2(0, 0), false);
    for (const auto& def : kFxDefs) {
        bool isCurrent = (activeFx.type == def.type || activeFx.type == def.ch);
        char btnLabel[64];
        snprintf(btnLabel, sizeof(btnLabel), "[%c] %-11s", def.ch, def.name);
        if (isCurrent) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.92f, 0.50f, 0.10f, 0.8f));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
        }
        if (ImGui::Button(btnLabel, ImVec2(-1, 20))) {
            Cell oldCell = cell;
            cell.fx[activeSlot].type = def.type;
            if (cell.fx[activeSlot].value == 0 && def.type == FxType::Volume) cell.fx[activeSlot].value = 40;
            if (cell.fx[activeSlot].value == 0 && def.type == FxType::FilterLowpass) cell.fx[activeSlot].value = 180;
            history.execute(std::make_unique<SetCellCommand>(patIdx, cursorTrack, cursorRow, cell, oldCell), song);
            engineHost.publish(song);
        }
        if (isCurrent) ImGui::PopStyleColor(2);
        if (ImGui::IsItemHovered()) kit::Tooltip(def.desc);
    }
    ImGui::EndChild();
    ImGui::EndChild();
}

void renderPatternWaveformStrip(ImDrawList* dl, const ImVec2& pos, const ImVec2& size,
                                const Instrument* inst, const EngineHost& engineHost) {
    (void)engineHost;
    if (!dl || size.x < 10.0f || size.y < 8.0f) return;
    ImVec2 maxPos(pos.x + size.x, pos.y + size.y);
    dl->AddRectFilled(pos, maxPos, IM_COL32(14, 16, 20, 255), 2.0f);
    dl->AddRect(pos, maxPos, IM_COL32(40, 44, 52, 255), 2.0f, 0, 1.0f);
    float midY = pos.y + size.y * 0.5f;
    dl->AddLine(ImVec2(pos.x, midY), ImVec2(maxPos.x, midY), IM_COL32(32, 36, 44, 255));
    if (!inst || !inst->sample || inst->sample->ch[0].empty()) {
        dl->AddText(ImVec2(pos.x + 6.0f, midY - 6.0f), IM_COL32(100, 110, 125, 255),
                    inst ? inst->name.c_str() : "---");
        return;
    }
    const auto& wave = inst->sample->ch[0];
    size_t count = wave.size();
    float step = static_cast<float>(count) / size.x;
    for (float x = 0; x < size.x; x += 1.0f) {
        size_t idx0 = static_cast<size_t>(x * step);
        size_t idx1 = std::min(count, static_cast<size_t>((x + 1.0f) * step));
        float minV = 0.0f, maxV = 0.0f;
        for (size_t i = idx0; i < idx1; ++i) {
            float s = wave[i];
            if (s < minV) minV = s;
            if (s > maxV) maxV = s;
        }
        float y0 = midY - maxV * (size.y * 0.44f);
        float y1 = midY - minV * (size.y * 0.44f);
        if (std::abs(y1 - y0) < 1.0f) y1 = y0 + 1.0f;
        dl->AddLine(ImVec2(pos.x + x, y0), ImVec2(pos.x + x, y1), IM_COL32(0, 232, 190, 220));
    }
}

void PatternView::update(Song& song, History& history, EngineHost& engineHost,
                        int currentOrderIdx, int octave, int editStep, bool recordMode,
                        int& selectedInstIdx, const Theme* theme) {
    (void)theme;
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings
                           | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    if (ImGui::Begin("Pattern Editor", nullptr, flags)) {
        ImGui::SetScrollY(0.0f);
        ImGui::SetScrollX(0.0f);
        if (song.patterns.empty() || song.order.empty()) { ImGui::End(); ImGui::PopStyleVar(); return; }

        int patIdx = song.order[std::clamp(currentOrderIdx, 0, static_cast<int>(song.order.size()) - 1)];
        if (patIdx < 0 || patIdx >= static_cast<int>(song.patterns.size())) patIdx = 0;
        auto& pat = song.patterns[patIdx];
        if (selectedInstIdx < 0 || selectedInstIdx >= static_cast<int>(song.instruments.size())) {
            syncInstrumentFromCursor(pat, song, selectedInstIdx);
        }
        const PlayheadPosition playhead = engineHost.getPlayhead();

        renderPatternHeader(pat, patIdx, recordMode, hasSelection_, selRange_, selectedInstIdx,
                            history, song, engineHost, &hasSelection_);
        handleKeyboardInput(pat, patIdx, song, history, engineHost, octave, editStep, recordMode, selectedInstIdx);

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        const ImVec2 cPos = ImGui::GetCursorScreenPos();
        const ImVec2 canvasSize = ImGui::GetWindowSize();
        if (canvasSize.x < 50 || canvasSize.y < 50) { ImGui::End(); ImGui::PopStyleVar(); return; }

        const bool onFxCol = (cursorSubCol_ >= 2);
        const float inspectorW = (!playhead.playing && (showFxInspector_ || onFxCol) && canvasSize.x >= 960.0f) ? 210.0f : 0.0f;
        const float gridW = canvasSize.x - inspectorW;
        const float sx = gridW / 800.0f;
        const float sy = canvasSize.y / 397.0f;
        const float originX = cPos.x, originY = cPos.y;
        const float leftOff = 27.0f * sx;
        const float tracksW = 186.0f * sx;
        const float rowHeight = 26.0f * sy;
        const int startRow = cursorRow_ - 7;

        if (playhead.playing && followPlayhead_) cursorRow_ = playhead.row;
        const int firstTrack = std::clamp(cursorTrack_ - 3, 0,
                                          std::max(0, static_cast<int>(pat.tracks.size()) - 4));

        ImGuiIO& io = ImGui::GetIO();
        if (io.MouseWheel != 0.0f && ImGui::IsWindowHovered()) cursorRow_ = (cursorRow_ - static_cast<int>(io.MouseWheel * 4.0f) + pat.rows * 100) % pat.rows;
        if (ImGui::IsMouseHoveringRect(ImVec2(originX, originY), ImVec2(originX + gridW, originY + 395.0f * sy))) {
            ImVec2 m = ImGui::GetMousePos();
            float rx = m.x - (originX + leftOff), ry = m.y - originY;
            if (ry >= 0 && rx >= 0) {
                int clickedVi = static_cast<int>(ry / rowHeight);
                int cRow = startRow + clickedVi;
                int cTrk = firstTrack + static_cast<int>(rx / tracksW);
                if (cRow >= 0 && cRow < pat.rows && cTrk >= 0 && cTrk < static_cast<int>(pat.tracks.size())) {
                    float subX = std::fmod(rx, tracksW) / sx;
                    int cSub = (subX < 44.0f) ? 0 : (subX < 78.0f) ? 1 : (subX < 132.0f) ? 2 : 3;
                    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                        if (io.KeyShift || shiftHeld_) {
                            if (!hasSelection_) { selRange_.startTrack = cursorTrack_; selRange_.startRow = cursorRow_; hasSelection_ = true; }
                            selRange_.endTrack = cTrk; selRange_.endRow = cRow;
                        } else {
                            hasSelection_ = false; syncInstrumentFromCursor(pat, song, selectedInstIdx);
                        }
                        cursorTrack_ = cTrk; cursorRow_ = cRow; cursorSubCol_ = cSub;
                    } else if (ImGui::IsMouseDragging(ImGuiMouseButton_Left, 4.0f)) {
                        if (!hasSelection_) { selRange_.startTrack = cursorTrack_; selRange_.startRow = cursorRow_; hasSelection_ = true; }
                        selRange_.endTrack = cTrk; selRange_.endRow = cRow;
                        cursorTrack_ = cTrk; cursorRow_ = cRow; cursorSubCol_ = cSub;
                    }
                }
            }
        }

        PatternRenderParams rp;
        rp.song = &song;
        rp.pat = &pat;
        rp.cursorTrack = cursorTrack_;
        rp.cursorRow = cursorRow_;
        rp.cursorSubCol = cursorSubCol_;
        rp.recording = recordMode;
        rp.hasSelection = hasSelection_;
        rp.selStartTrack = selRange_.startTrack;
        rp.selEndTrack = selRange_.endTrack;
        rp.selStartRow = selRange_.startRow;
        rp.selEndRow = selRange_.endRow;
        rp.playing = playhead.playing;
        rp.playheadRow = playhead.row;

        drawList->PushClipRectFullScreen();
        renderPatternScreen(drawList, ImVec2(originX, originY), sx, sy, rp);
        drawList->PopClipRect();

        if (inspectorW > 0.0f) {
            const Instrument* curInst = (selectedInstIdx >= 0 && selectedInstIdx < static_cast<int>(song.instruments.size())) ? &song.instruments[selectedInstIdx] : nullptr;
            renderPatternWaveformStrip(drawList, ImVec2(originX + gridW + 4, originY + 2.0f), ImVec2(inspectorW - 8, 40.0f), curInst, engineHost);
            ImGui::SetCursorScreenPos(ImVec2(originX + gridW + 4, originY + 44.0f));
            renderFxInspector(pat.tracks[cursorTrack_][cursorRow_], cursorSubCol_, patIdx,
                              cursorTrack_, cursorRow_, history, song, engineHost, inspectorW - 4);
        }
        ImGui::SetCursorScreenPos(ImVec2(originX, originY));
    }
    ImGui::End();
    ImGui::PopStyleVar();
}

} // namespace trakr

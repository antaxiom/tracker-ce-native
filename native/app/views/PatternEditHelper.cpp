#include "PatternEditHelper.h"
#include "PatternFxHelper.h"
#include "PatternView.h"
#include "Command.h"
#include "imgui.h"
#include <algorithm>
#include <memory>

namespace trakr {

// Pattern editor input driver (definition lives here to keep PatternView.cpp
// focused on the firmware-mirroring render pass; behaviour is unchanged).
void PatternView::handleKeyboardInput(Pattern& pat, int patIdx, Song& song, History& history,
                                     EngineHost& engineHost, int octave, int editStep,
                                     bool recordMode, int& selectedInstIdx) {
    ImGuiIO& io = ImGui::GetIO();
    const int numTracks = static_cast<int>(pat.tracks.size()), numRows = pat.rows;
    const bool isCtrl = io.KeyCtrl || io.KeySuper;

    // The cursor survives slot and pattern-length changes (shrinking halves the
    // rows); clamp it before any cell access like the firmware does.
    if (numRows > 0)
        cursorRow_ = std::clamp(cursorRow_, 0, numRows - 1);
    if (numTracks > 0)
        cursorTrack_ = std::clamp(cursorTrack_, 0, numTracks - 1);

    const bool isShift = io.KeyShift || shiftHeld_;
    if (handleSelectionShortcuts(pat, patIdx, song, history, engineHost, octave, editStep,
                                recordMode, selectedInstIdx, cursorTrack_, cursorRow_,
                                cursorSubCol_, hasSelection_, selRange_, clipboard_, isShift)) {
        return;
    }

    bool moved = false;
    if (ImGui::IsKeyPressed(ImGuiKey_UpArrow) && !isCtrl) { navigate(0, -1, 0, isShift, numTracks, numRows, pat, song, selectedInstIdx); moved = true; }
    if (ImGui::IsKeyPressed(ImGuiKey_DownArrow) && !isCtrl) { navigate(0, 1, 0, isShift, numTracks, numRows, pat, song, selectedInstIdx); moved = true; }
    if ((ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter)) && !isCtrl) {
        navigate(0, isShift ? -((editStep > 0) ? editStep : 1) : ((editStep > 0) ? editStep : 1), 0, false, numTracks, numRows, pat, song, selectedInstIdx);
        moved = true;
    }
    if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) { navigate(-1, 0, isShift ? 0 : -1, isShift, numTracks, numRows, pat, song, selectedInstIdx); moved = true; }
    if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) { navigate(1, 0, isShift ? 0 : 1, isShift, numTracks, numRows, pat, song, selectedInstIdx); moved = true; }
    if (ImGui::IsKeyPressed(ImGuiKey_Tab)) { navigate(isShift ? -1 : 1, 0, 0, false, numTracks, numRows, pat, song, selectedInstIdx); moved = true; }
    if (ImGui::IsKeyPressed(ImGuiKey_PageUp)) { navigate(0, -16, 0, isShift, numTracks, numRows, pat, song, selectedInstIdx); moved = true; }
    if (ImGui::IsKeyPressed(ImGuiKey_PageDown)) { navigate(0, 16, 0, isShift, numTracks, numRows, pat, song, selectedInstIdx); moved = true; }
    if (ImGui::IsKeyPressed(ImGuiKey_Home)) { hasSelection_ = false; cursorRow_ = 0; moved = true; }
    if (ImGui::IsKeyPressed(ImGuiKey_End)) { hasSelection_ = false; cursorRow_ = numRows - 1; moved = true; }
    if (moved && !isShift) syncInstrumentFromCursor(pat, song, selectedInstIdx);

    static const struct { ImGuiKey key; int semi; } kPianoKeys[] = {
        {ImGuiKey_Z, 0}, {ImGuiKey_S, 1}, {ImGuiKey_X, 2}, {ImGuiKey_D, 3}, {ImGuiKey_C, 4}, {ImGuiKey_V, 5},
        {ImGuiKey_G, 6}, {ImGuiKey_B, 7}, {ImGuiKey_H, 8}, {ImGuiKey_N, 9}, {ImGuiKey_J, 10}, {ImGuiKey_M, 11},
        {ImGuiKey_Q, 12}, {ImGuiKey_2, 13}, {ImGuiKey_W, 14}, {ImGuiKey_3, 15}, {ImGuiKey_E, 16}, {ImGuiKey_R, 17},
        {ImGuiKey_5, 18}, {ImGuiKey_T, 19}, {ImGuiKey_6, 20}, {ImGuiKey_Y, 21}, {ImGuiKey_7, 22}, {ImGuiKey_U, 23}
    };

    const bool canEdit = recordMode;
    if (!io.KeyCtrl && !io.KeySuper && !io.KeyAlt) {
        if (cursorSubCol_ == 0 || !canEdit) {
            for (const auto& kn : kPianoKeys) {
                if (ImGui::IsKeyPressed(kn.key, false)) {
                    int midi = std::clamp(octave * 12 + kn.semi, 0, 119);
                    engineHost.previewNoteOn(cursorTrack_, selectedInstIdx, midi, 0.9f);
                    if (canEdit && !isShift) {
                        Cell oldCell = pat.tracks[cursorTrack_][cursorRow_], newCell = oldCell;
                        newCell.note = static_cast<int16_t>(midi);
                        newCell.instrument = static_cast<int16_t>(selectedInstIdx);
                        if (newCell.volume < 0) newCell.volume = 40;
                        history.execute(std::make_unique<SetCellCommand>(patIdx, cursorTrack_, cursorRow_, newCell, oldCell), song);
                        engineHost.publish(song);
                        cursorRow_ = std::min(numRows - 1, cursorRow_ + editStep);
                    }
                    return;
                }
                if (ImGui::IsKeyReleased(kn.key)) { engineHost.previewNoteOff(cursorTrack_); }
            }
            if ((ImGui::IsKeyPressed(ImGuiKey_A, false) || ImGui::IsKeyPressed(ImGuiKey_1, false)) && canEdit) {
                Cell oldCell = pat.tracks[cursorTrack_][cursorRow_], newCell = oldCell;
                newCell.note = kNoteOff;
                history.execute(std::make_unique<SetCellCommand>(patIdx, cursorTrack_, cursorRow_, newCell, oldCell), song);
                engineHost.publish(song);
                cursorRow_ = std::min(numRows - 1, cursorRow_ + editStep);
                return;
            }
        } else if (canEdit) {
            int hexDigit = -1;
            for (int d = 0; d <= 9; ++d) if (ImGui::IsKeyPressed(static_cast<ImGuiKey>(ImGuiKey_0 + d), false) || ImGui::IsKeyPressed(static_cast<ImGuiKey>(ImGuiKey_Keypad0 + d), false)) { hexDigit = d; break; }
            if (hexDigit < 0) for (int d = 0; d < 6; ++d) if (ImGui::IsKeyPressed(static_cast<ImGuiKey>(ImGuiKey_A + d), false)) { hexDigit = 10 + d; break; }

            if (hexDigit >= 0) {
                Cell oldCell = pat.tracks[cursorTrack_][cursorRow_], newCell = oldCell;
                if (cursorSubCol_ == 1) {
                    int cur = (newCell.instrument >= 0) ? (newCell.instrument + 1) : 0;
                    int nextVal = (cur % 10) * 10 + hexDigit;
                    if (nextVal > 48) nextVal = hexDigit;
                    newCell.instrument = (nextVal == 0) ? -1 : static_cast<int16_t>(nextVal - 1);
                    if (newCell.instrument >= 0) selectedInstIdx = newCell.instrument;
                } else if (cursorSubCol_ >= 2) {
                    int fxSlot = (cursorSubCol_ == 2) ? 0 : 1;
                    newCell.fx[fxSlot].value = static_cast<uint8_t>(((newCell.fx[fxSlot].value & 0x0F) << 4) | hexDigit);
                    if (newCell.fx[fxSlot].type == 0) newCell.fx[fxSlot].type = 18;
                }
                history.execute(std::make_unique<SetCellCommand>(patIdx, cursorTrack_, cursorRow_, newCell, oldCell), song);
                engineHost.publish(song);
                cursorRow_ = std::min(numRows - 1, cursorRow_ + editStep);
                return;
            } else if (cursorSubCol_ >= 2) {
                int fxSlot = (cursorSubCol_ == 2) ? 0 : 1;
                for (const auto& def : getAllFxDefinitions()) {
                    if (def.ch >= 'A' && def.ch <= 'Z') {
                        ImGuiKey k = static_cast<ImGuiKey>(ImGuiKey_A + (def.ch - 'A'));
                        if (ImGui::IsKeyPressed(k, false)) {
                            Cell oldCell = pat.tracks[cursorTrack_][cursorRow_], newCell = oldCell;
                            newCell.fx[fxSlot].type = def.type;
                            if (newCell.fx[fxSlot].value == 0) newCell.fx[fxSlot].value = (def.type == 18) ? 40 : 128;
                            history.execute(std::make_unique<SetCellCommand>(patIdx, cursorTrack_, cursorRow_, newCell, oldCell), song);
                            engineHost.publish(song);
                            cursorRow_ = std::min(numRows - 1, cursorRow_ + editStep);
                            return;
                        }
                    }
                }
            }
            for (const auto& kn : kPianoKeys) {
                if (ImGui::IsKeyPressed(kn.key, false)) {
                    cursorSubCol_ = 0;
                    int midi = std::clamp(octave * 12 + kn.semi, 0, 119);
                    engineHost.previewNoteOn(cursorTrack_, selectedInstIdx, midi, 0.9f);
                    Cell oldCell = pat.tracks[cursorTrack_][cursorRow_], newCell = oldCell;
                    newCell.note = static_cast<int16_t>(midi);
                    newCell.instrument = static_cast<int16_t>(selectedInstIdx);
                    if (newCell.volume < 0) newCell.volume = 40;
                    history.execute(std::make_unique<SetCellCommand>(patIdx, cursorTrack_, cursorRow_, newCell, oldCell), song);
                    engineHost.publish(song);
                    cursorRow_ = std::min(numRows - 1, cursorRow_ + editStep);
                    return;
                }
            }
        }

        if (ImGui::IsKeyPressed(ImGuiKey_Insert, false)) {
            if (isShift) { cursorRow_ = 0; moved = true; }
            else if (canEdit) { insertRowAtCursor(pat, patIdx, song, history, engineHost); return; }
        }
        if ((ImGui::IsKeyPressed(ImGuiKey_Period, false) || ImGui::IsKeyPressed(ImGuiKey_Delete, false) ||
             ImGui::IsKeyPressed(ImGuiKey_Backspace, false)) && canEdit) {
            deleteRowOrCellAtCursor(pat, patIdx, song, history, engineHost, isShift);
            if (!isShift) cursorRow_ = std::min(numRows - 1, cursorRow_ + editStep);
            return;
        }
    }

    if (isCtrl) {
        if (ImGui::IsKeyPressed(ImGuiKey_C)) copySelectionOrCell(pat);
        if (ImGui::IsKeyPressed(ImGuiKey_X) && recordMode) { copySelectionOrCell(pat); deleteRowOrCellAtCursor(pat, patIdx, song, history, engineHost, false); }
        if (ImGui::IsKeyPressed(ImGuiKey_V) && recordMode) { pasteAtCursor(pat, patIdx, song, history, engineHost); cursorRow_ = std::min(numRows - 1, cursorRow_ + editStep); }
        if (ImGui::IsKeyPressed(ImGuiKey_UpArrow) || ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
            adjustValueAtCursor(pat, patIdx, song, history, engineHost, ImGui::IsKeyPressed(ImGuiKey_UpArrow) ? 1 : -1, isShift);
        }
    }
}

void insertRowAtCursor(Pattern& pat, int patIdx, int track, int row,
                       Song& song, History& history, EngineHost& engineHost) {
    (void)track;
    if (patIdx < 0 || patIdx >= static_cast<int>(song.patterns.size())) return;
    if (row < 0 || row >= pat.rows) return;
    history.execute(std::make_unique<InsertRowsCommand>(patIdx, row, 1), song);
    engineHost.publish(song);
}

void deleteRowOrCellAtCursor(Pattern& pat, int patIdx, int track, int row, int subCol,
                             Song& song, History& history, EngineHost& engineHost,
                             bool shiftHeld, int editStep) {
    (void)editStep;
    if (patIdx < 0 || patIdx >= static_cast<int>(song.patterns.size())) return;
    if (track < 0 || track >= static_cast<int>(pat.tracks.size())) return;
    if (row < 0 || row >= pat.rows) return;

    if (shiftHeld) {
        // Shift+Delete: delete row, shifting lower rows up
        history.execute(std::make_unique<DeleteRowsCommand>(patIdx, row, 1), song);
    } else {
        // Normal Delete: clear active cell or subcolumn
        Cell oldCell = pat.tracks[track][row];
        Cell newCell = oldCell;
        if (subCol == 0) {
            newCell.note = kNoNote;
            newCell.instrument = -1;
        } else if (subCol == 1) {
            newCell.instrument = -1;
        } else if (subCol == 2) {
            newCell.fx[0] = FxCmd{};
        } else if (subCol == 3 || subCol == 4) {
            newCell.fx[1] = FxCmd{};
        }
        history.execute(std::make_unique<SetCellCommand>(patIdx, track, row, newCell, oldCell), song);
    }
    engineHost.publish(song);
}

void copySelectionOrCell(const Pattern& pat, int track, int row,
                         bool hasSelection, const SelectionRange& sel,
                         std::vector<std::vector<Cell>>& clipboard) {
    if (hasSelection) {
        copySelection(pat, sel, clipboard);
    } else {
        if (track >= 0 && track < static_cast<int>(pat.tracks.size()) &&
            row >= 0 && row < pat.rows) {
            clipboard = {{ pat.tracks[track][row] }};
        }
    }
}

void pasteAtCursor(Pattern& pat, int patIdx, int track, int row,
                   const std::vector<std::vector<Cell>>& clipboard,
                   Song& song, History& history, EngineHost& engineHost) {
    if (clipboard.empty() || clipboard[0].empty()) return;
    pasteSelection(pat, patIdx, track, row, clipboard, history, song, engineHost);
}

void adjustValueAtCursor(Pattern& pat, int patIdx, int track, int row, int subCol,
                         int delta, bool shiftHeld,
                         Song& song, History& history, EngineHost& engineHost) {
    if (patIdx < 0 || patIdx >= static_cast<int>(song.patterns.size())) return;
    if (track < 0 || track >= static_cast<int>(pat.tracks.size())) return;
    if (row < 0 || row >= pat.rows) return;

    Cell oldCell = pat.tracks[track][row];
    Cell newCell = oldCell;

    if (subCol == 0) {
        // Note transpose
        int step = shiftHeld ? 12 : 1;
        int curNote = (oldCell.note >= 0 && oldCell.note < 120) ? oldCell.note : 48; // default C-4
        newCell.note = static_cast<int16_t>(std::clamp(curNote + delta * step, 0, 119));
    } else if (subCol == 1) {
        // Instrument index (1..48)
        int step = shiftHeld ? 10 : 1;
        int curInst = (oldCell.instrument >= 0) ? oldCell.instrument : 0;
        newCell.instrument = static_cast<int16_t>(std::clamp(curInst + delta * step, 0, 48));
    } else if (subCol == 2) {
        // FX1 value
        int step = shiftHeld ? 10 : 1;
        newCell.fx[0].value = static_cast<uint8_t>(std::clamp(static_cast<int>(newCell.fx[0].value) + delta * step, 0, 255));
    } else if (subCol == 3 || subCol == 4) {
        // FX2 value
        int step = shiftHeld ? 10 : 1;
        newCell.fx[1].value = static_cast<uint8_t>(std::clamp(static_cast<int>(newCell.fx[1].value) + delta * step, 0, 255));
    }

    history.execute(std::make_unique<SetCellCommand>(patIdx, track, row, newCell, oldCell), song);
    engineHost.publish(song);
}

namespace {
class TransformPatternCommand : public Command {
public:
    TransformPatternCommand(int patternIdx, Pattern newPat, Pattern oldPat, std::string label = "Transform Pattern")
        : patternIdx_(patternIdx), newPat_(std::move(newPat)), oldPat_(std::move(oldPat)), label_(std::move(label)) {}
    void apply(Song& song) override {
        if (patternIdx_ >= 0 && patternIdx_ < static_cast<int>(song.patterns.size())) song.patterns[patternIdx_] = newPat_;
    }
    void revert(Song& song) override {
        if (patternIdx_ >= 0 && patternIdx_ < static_cast<int>(song.patterns.size())) song.patterns[patternIdx_] = oldPat_;
    }
    std::string label() const override { return label_; }
private:
    int patternIdx_;
    Pattern newPat_;
    Pattern oldPat_;
    std::string label_;
};
} // namespace

void duplicatePattern(Pattern& pat, int patIdx, Song& song, History& history, EngineHost& engineHost) {
    if (patIdx < 0 || patIdx >= static_cast<int>(song.patterns.size())) return;
    int oldRows = pat.rows;
    int newRows = std::min(256, oldRows * 2);
    if (newRows <= oldRows) return;
    Pattern newPat = pat;
    newPat.rows = newRows;
    for (auto& trk : newPat.tracks) {
        trk.resize(newRows);
        for (int r = 0; r < oldRows && oldRows + r < newRows; ++r) trk[oldRows + r] = trk[r];
    }
    history.execute(std::make_unique<TransformPatternCommand>(patIdx, newPat, pat, "Duplicate Pattern"), song);
    engineHost.publish(song);
}

void expandPattern(Pattern& pat, int patIdx, Song& song, History& history, EngineHost& engineHost) {
    if (patIdx < 0 || patIdx >= static_cast<int>(song.patterns.size())) return;
    int oldRows = pat.rows;
    int newRows = std::min(256, oldRows * 2);
    if (newRows <= oldRows) return;
    Pattern newPat = pat;
    newPat.rows = newRows;
    for (auto& trk : newPat.tracks) {
        std::vector<Cell> expanded(newRows, Cell{});
        for (int r = 0; r < oldRows && r * 2 < newRows; ++r) expanded[r * 2] = trk[r];
        trk = std::move(expanded);
    }
    history.execute(std::make_unique<TransformPatternCommand>(patIdx, newPat, pat, "Expand Pattern"), song);
    engineHost.publish(song);
}

void shrinkPattern(Pattern& pat, int patIdx, Song& song, History& history, EngineHost& engineHost) {
    if (patIdx < 0 || patIdx >= static_cast<int>(song.patterns.size())) return;
    int oldRows = pat.rows;
    int newRows = std::max(16, oldRows / 2);
    if (newRows >= oldRows) return;
    Pattern newPat = pat;
    newPat.rows = newRows;
    for (auto& trk : newPat.tracks) {
        std::vector<Cell> shrunk(newRows, Cell{});
        for (int r = 0; r < newRows && r * 2 < oldRows; ++r) shrunk[r] = trk[r * 2];
        trk = std::move(shrunk);
    }
    history.execute(std::make_unique<TransformPatternCommand>(patIdx, newPat, pat, "Shrink Pattern"), song);
    engineHost.publish(song);
}

void invertSelectionOrPattern(Pattern& pat, int patIdx, bool hasSelection, const SelectionRange& sel,
                              Song& song, History& history, EngineHost& engineHost) {
    if (patIdx < 0 || patIdx >= static_cast<int>(song.patterns.size())) return;
    int minT = hasSelection ? sel.minTrack() : 0;
    int maxT = hasSelection ? sel.maxTrack() : static_cast<int>(pat.tracks.size()) - 1;
    int minR = hasSelection ? sel.minRow() : 0;
    int maxR = hasSelection ? sel.maxRow() : pat.rows - 1;

    int minNote = 120, maxNote = -1;
    for (int t = minT; t <= maxT && t < static_cast<int>(pat.tracks.size()); ++t) {
        for (int r = minR; r <= maxR && r < pat.rows; ++r) {
            int16_t n = pat.tracks[t][r].note;
            if (n >= 0 && n < 120) {
                if (n < minNote) minNote = n;
                if (n > maxNote) maxNote = n;
            }
        }
    }
    int center = (maxNote >= minNote) ? ((minNote + maxNote) / 2) : 60;

    Pattern newPat = pat;
    for (int t = minT; t <= maxT && t < static_cast<int>(newPat.tracks.size()); ++t) {
        for (int r = minR; r <= maxR && r < newPat.rows; ++r) {
            int16_t n = newPat.tracks[t][r].note;
            if (n >= 0 && n < 120) {
                newPat.tracks[t][r].note = static_cast<int16_t>(std::clamp(2 * center - n, 0, 119));
            }
        }
    }
    history.execute(std::make_unique<TransformPatternCommand>(patIdx, newPat, pat, "Invert Pattern"), song);
    engineHost.publish(song);
}

void copyPattern(const Pattern& pat, Pattern& clipboard) {
    clipboard = pat;
}

void pastePattern(Pattern& pat, int patIdx, const Pattern& clipboard,
                  Song& song, History& history, EngineHost& engineHost) {
    if (patIdx < 0 || patIdx >= static_cast<int>(song.patterns.size()) || clipboard.tracks.empty()) return;
    Pattern newPat = clipboard;
    newPat.rows = pat.rows;
    for (auto& trk : newPat.tracks) trk.resize(pat.rows);
    history.execute(std::make_unique<TransformPatternCommand>(patIdx, newPat, pat, "Paste Pattern"), song);
    engineHost.publish(song);
}

} // namespace trakr


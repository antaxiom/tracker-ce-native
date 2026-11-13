#include "PatternSelectionHelper.h"
#include "Command.h"
#include "../kit/Widgets.h"
#include <algorithm>
#include <cstdio>

namespace trakr {

template <typename F>
static void applyToSelection(Pattern& pat, int patIdx, const SelectionRange& sel, History& history, Song& song, EngineHost& engineHost, F&& fn) {
    int minT = sel.minTrack(), maxT = sel.maxTrack(), minR = sel.minRow(), maxR = sel.maxRow();
    // A selection can outlive a pattern-length change; clamp it to the pattern.
    const int selMaxR = maxR;
    minR = std::clamp(minR, 0, std::max(0, selMaxR));
    for (int t = minT; t <= maxT && t < static_cast<int>(pat.tracks.size()); ++t) {
        const int trackRows = std::min({selMaxR, pat.rows - 1,
                                        static_cast<int>(pat.tracks[t].size()) - 1});
        if (trackRows < minR) continue;
        int count = trackRows - minR + 1;
        std::vector<Cell> oldCells(count), newCells(count);
        for (int i = 0; i < count; ++i) {
            oldCells[i] = pat.tracks[t][minR + i];
            newCells[i] = oldCells[i];
            fn(newCells[i]);
        }
        history.execute(std::make_unique<SetCellRangeCommand>(patIdx, t, minR, newCells, oldCells), song);
    }
    engineHost.publish(song);
}

void transposeSelection(Pattern& pat, int patIdx, const SelectionRange& sel, int semitones,
                        History& history, Song& song, EngineHost& engineHost) {
    if (semitones == 0) return;
    applyToSelection(pat, patIdx, sel, history, song, engineHost, [&](Cell& c) {
        if (c.note >= 0 && c.note < 120) c.note = static_cast<int16_t>(std::clamp(c.note + semitones, 0, 119));
    });
}

void adjustVolumeSelection(Pattern& pat, int patIdx, const SelectionRange& sel, int deltaVol,
                           History& history, Song& song, EngineHost& engineHost) {
    if (deltaVol == 0) return;
    applyToSelection(pat, patIdx, sel, history, song, engineHost, [&](Cell& c) {
        if (c.volume >= 0) c.volume = static_cast<int16_t>(std::clamp(c.volume + deltaVol, 0, 64));
    });
}

void setSelectionToNote(Pattern& pat, int patIdx, const SelectionRange& sel, int note, int inst,
                        History& history, Song& song, EngineHost& engineHost) {
    applyToSelection(pat, patIdx, sel, history, song, engineHost, [&](Cell& c) {
        c.note = static_cast<int16_t>(note);
        if (inst >= 0) c.instrument = static_cast<int16_t>(inst);
        if (c.volume < 0) c.volume = 40;
    });
}

void setSelectionInstrument(Pattern& pat, int patIdx, const SelectionRange& sel, int inst,
                            History& history, Song& song, EngineHost& engineHost) {
    applyToSelection(pat, patIdx, sel, history, song, engineHost, [&](Cell& c) {
        c.instrument = static_cast<int16_t>(inst);
    });
}

void setSelectionVolume(Pattern& pat, int patIdx, const SelectionRange& sel, int vol,
                        History& history, Song& song, EngineHost& engineHost) {
    applyToSelection(pat, patIdx, sel, history, song, engineHost, [&](Cell& c) {
        c.volume = static_cast<int16_t>(std::clamp(vol, 0, 64));
    });
}

void clearSelection(Pattern& pat, int patIdx, const SelectionRange& sel, int subCol,
                    History& history, Song& song, EngineHost& engineHost) {
    applyToSelection(pat, patIdx, sel, history, song, engineHost, [&](Cell& c) {
        if (subCol == 0) { c.note = kNoNote; c.instrument = -1; }
        else if (subCol == 1) c.instrument = -1;
        else if (subCol == 2) c.fx[0] = FxCmd{};
        else if (subCol == 3 || subCol == 4) c.fx[1] = FxCmd{};
        else c = Cell{};
    });
}

void copySelection(const Pattern& pat, const SelectionRange& sel,
                   std::vector<std::vector<Cell>>& clipboard) {
    int minT = sel.minTrack(), maxT = sel.maxTrack();
    int minR = sel.minRow(), maxR = sel.maxRow();
    clipboard.clear();
    for (int t = minT; t <= maxT && t < static_cast<int>(pat.tracks.size()); ++t) {
        std::vector<Cell> trkCells;
        for (int r = minR; r <= maxR && r < pat.rows; ++r) {
            trkCells.push_back(pat.tracks[t][r]);
        }
        clipboard.push_back(trkCells);
    }
}

void pasteSelection(Pattern& pat, int patIdx, int destTrack, int destRow,
                    const std::vector<std::vector<Cell>>& clipboard,
                    History& history, Song& song, EngineHost& engineHost) {
    if (clipboard.empty()) return;
    for (size_t i = 0; i < clipboard.size(); ++i) {
        int t = destTrack + static_cast<int>(i);
        if (t >= static_cast<int>(pat.tracks.size())) break;
        int count = static_cast<int>(clipboard[i].size());
        if (destRow + count > pat.rows) count = pat.rows - destRow;
        if (count <= 0) continue;

        std::vector<Cell> oldCells(count), newCells(count);
        for (int r = 0; r < count; ++r) {
            oldCells[r] = pat.tracks[t][destRow + r];
            newCells[r] = clipboard[i][r];
        }
        history.execute(std::make_unique<SetCellRangeCommand>(patIdx, t, destRow, newCells, oldCells), song);
    }
    engineHost.publish(song);
}

void renderSelectionOverlay(ImDrawList* drawList, const ImVec2& canvasPos, float gutterWidth,
                            float headerHeight, float trackWidth, float rowHeight,
                            int startRow, int visibleRows, const SelectionRange& sel,
                            ImU32 colHighlight, ImU32 colBorder, int startTrack) {
    int minR = sel.minRow(), maxR = sel.maxRow();
    int minT = std::max(sel.minTrack(), startTrack), maxT = sel.maxTrack();
    if (minT > maxT) return;

    int startVis = std::max(minR, startRow);
    int endVis = std::min(maxR, startRow + visibleRows - 1);
    if (startVis > endVis) return;

    float x0 = canvasPos.x + gutterWidth + (minT - startTrack) * trackWidth;
    float x1 = canvasPos.x + gutterWidth + (maxT - startTrack + 1) * trackWidth;
    float y0 = canvasPos.y + headerHeight + (startVis - startRow) * rowHeight;
    float y1 = canvasPos.y + headerHeight + (endVis - startRow + 1) * rowHeight;

    drawList->AddRectFilled(ImVec2(x0, y0), ImVec2(x1, y1), colHighlight);
    drawList->AddRect(ImVec2(x0, y0), ImVec2(x1, y1), colBorder, 1.0f, 0, 1.5f);
}

void renderPatternHeader(Pattern& pat, int patIdx, bool recordMode, bool hasSelection,
                         const SelectionRange& sel, int selectedInstIdx,
                         History& history, Song& song, EngineHost& engineHost,
                         bool* pHasSelection) {
    if (!hasSelection) return;

    const float headerH = 22.0f;
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 barPos = ImGui::GetCursorScreenPos();
    float availW = ImGui::GetContentRegionAvail().x;

    // Header background & subtle border
    drawList->AddRectFilled(barPos, ImVec2(barPos.x + availW, barPos.y + headerH), IM_COL32(20, 22, 26, 255));
    drawList->AddLine(ImVec2(barPos.x, barPos.y + headerH), ImVec2(barPos.x + availW, barPos.y + headerH), IM_COL32(45, 48, 54, 255));

    ImGui::SetCursorScreenPos(ImVec2(barPos.x + 6, barPos.y + 2));

    if (hasSelection) {
        char rangeTitle[64];
        snprintf(rangeTitle, sizeof(rangeTitle), "SEL T%d R%02d..%02d:",
                 sel.minTrack() + 1, sel.minRow(), sel.maxRow());
        ImGui::TextColored(ImVec4(0.95f, 0.55f, 0.12f, 1.0f), "%s", rangeTitle);
        ImGui::SameLine();

        if (kit::FlatButton("-1 Semi", ImVec2(52, 18))) {
            transposeSelection(pat, patIdx, sel, -1, history, song, engineHost);
        }
        ImGui::SameLine();
        if (kit::FlatButton("+1 Semi", ImVec2(52, 18))) {
            transposeSelection(pat, patIdx, sel, 1, history, song, engineHost);
        }
        ImGui::SameLine();
        if (kit::FlatButton("-1 Oct", ImVec2(46, 18))) {
            transposeSelection(pat, patIdx, sel, -12, history, song, engineHost);
        }
        ImGui::SameLine();
        if (kit::FlatButton("+1 Oct", ImVec2(46, 18))) {
            transposeSelection(pat, patIdx, sel, 12, history, song, engineHost);
        }
        ImGui::SameLine();
        if (kit::FlatButton("Vol -4", ImVec2(44, 18))) {
            adjustVolumeSelection(pat, patIdx, sel, -4, history, song, engineHost);
        }
        ImGui::SameLine();
        if (kit::FlatButton("Vol +4", ImVec2(44, 18))) {
            adjustVolumeSelection(pat, patIdx, sel, 4, history, song, engineHost);
        }
        ImGui::SameLine(0, 6);
        ImGui::TextDisabled("|");
        ImGui::SameLine(0, 6);

        char setInstLabel[32];
        snprintf(setInstLabel, sizeof(setInstLabel), "Set I%02X", selectedInstIdx);
        if (kit::FlatButton(setInstLabel, ImVec2(54, 18))) {
            setSelectionInstrument(pat, patIdx, sel, selectedInstIdx, history, song, engineHost);
        }
        ImGui::SameLine();
        if (kit::FlatButton("Set V40", ImVec2(52, 18))) {
            setSelectionVolume(pat, patIdx, sel, 40, history, song, engineHost);
        }
        ImGui::SameLine();
        if (kit::FlatButton("Clear", ImVec2(40, 18))) {
            clearSelection(pat, patIdx, sel, -1, history, song, engineHost);
        }
        ImGui::SameLine();
        if (kit::FlatButton("X", ImVec2(22, 18))) {
            if (pHasSelection) *pHasSelection = false;
        }

        // Fixed spacing advance: headerH + 2px
        ImGui::SetCursorScreenPos(ImVec2(barPos.x, barPos.y + headerH + 2.0f));
        ImGui::Dummy(ImVec2(availW, 0.0f));
    }
}

void navigateCursor(int dTrack, int dRow, int dSubCol, bool isShift,
                    int numTracks, int numRows,
                    int& cursorTrack, int& cursorRow, int& cursorSubCol,
                    bool& hasSelection, SelectionRange& sel) {
    if (isShift) {
        if (!hasSelection) {
            sel.startTrack = cursorTrack;
            sel.startRow = cursorRow;
            sel.endTrack = cursorTrack;
            sel.endRow = cursorRow;
            hasSelection = true;
        }
        if (dRow < 0 && cursorRow == 0) {
            if (sel.startRow != 0 || sel.endRow != numRows - 1) {
                sel.startRow = 0; sel.endRow = numRows - 1;
                sel.startTrack = cursorTrack; sel.endTrack = cursorTrack;
            } else {
                sel.startTrack = 0; sel.endTrack = numTracks - 1;
            }
        } else {
            if (dRow != 0) {
                cursorRow = std::clamp(cursorRow + dRow, 0, numRows - 1);
                sel.endRow = cursorRow;
            }
            if (dTrack != 0) {
                cursorTrack = std::clamp(cursorTrack + dTrack, 0, numTracks - 1);
                sel.endTrack = cursorTrack;
            }
        }
    } else {
        hasSelection = false;
        if (dRow != 0) {
            cursorRow = (dRow > 0) ? ((cursorRow + dRow < numRows) ? cursorRow + dRow : 0)
                                   : ((cursorRow + dRow >= 0) ? cursorRow + dRow : numRows - 1);
        }
        if (dSubCol != 0) {
            int totalSub = cursorTrack * 4 + cursorSubCol + dSubCol;
            int maxSub = numTracks * 4 - 1;
            if (totalSub < 0) totalSub = maxSub;
            else if (totalSub > maxSub) totalSub = 0;
            cursorTrack = totalSub / 4;
            cursorSubCol = totalSub % 4;
        } else if (dTrack != 0) {
            cursorTrack = (dTrack > 0) ? ((cursorTrack + dTrack < numTracks) ? cursorTrack + dTrack : 0)
                                       : ((cursorTrack + dTrack >= 0) ? cursorTrack + dTrack : numTracks - 1);
        }
    }
}

bool handleSelectionShortcuts(Pattern& pat, int patIdx, Song& song, History& history,
                             EngineHost& engineHost, int octave, int editStep,
                             bool recordMode, int selectedInstIdx,
                             int& cursorTrack, int& cursorRow, int& cursorSubCol,
                             bool& hasSelection, SelectionRange& sel,
                             std::vector<std::vector<Cell>>& clipboard,
                             bool shiftHeld) {
    ImGuiIO& io = ImGui::GetIO();
    const bool isCtrl = io.KeyCtrl || io.KeySuper;
    const bool isShift = io.KeyShift || shiftHeld;
    const bool canEdit = recordMode;
    const int numTracks = static_cast<int>(pat.tracks.size());
    const int numRows = pat.rows;

    // 1. Shift+Arrows Selection Expansion (Tracker Manual 1.7.0 lines 964-971)
    if (isShift && !isCtrl) {
        if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
            navigateCursor(0, -1, 0, true, numTracks, numRows, cursorTrack, cursorRow, cursorSubCol, hasSelection, sel);
            return true;
        } else if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
            navigateCursor(0, 1, 0, true, numTracks, numRows, cursorTrack, cursorRow, cursorSubCol, hasSelection, sel);
            return true;
        } else if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) {
            navigateCursor(-1, 0, 0, true, numTracks, numRows, cursorTrack, cursorRow, cursorSubCol, hasSelection, sel);
            return true;
        } else if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) {
            navigateCursor(1, 0, 0, true, numTracks, numRows, cursorTrack, cursorRow, cursorSubCol, hasSelection, sel);
            return true;
        } else if (ImGui::IsKeyPressed(ImGuiKey_PageUp)) {
            navigateCursor(0, -16, 0, true, numTracks, numRows, cursorTrack, cursorRow, cursorSubCol, hasSelection, sel);
            return true;
        } else if (ImGui::IsKeyPressed(ImGuiKey_PageDown)) {
            navigateCursor(0, 16, 0, true, numTracks, numRows, cursorTrack, cursorRow, cursorSubCol, hasSelection, sel);
            return true;
        }
    }

    if (!hasSelection) return false;

    // Deselect on unshifted arrow moves
    if (!isShift && !isCtrl && !io.KeyAlt) {
        if (ImGui::IsKeyPressed(ImGuiKey_UpArrow) || ImGui::IsKeyPressed(ImGuiKey_DownArrow) ||
            ImGui::IsKeyPressed(ImGuiKey_LeftArrow) || ImGui::IsKeyPressed(ImGuiKey_RightArrow) ||
            ImGui::IsKeyPressed(ImGuiKey_PageUp) || ImGui::IsKeyPressed(ImGuiKey_PageDown) ||
            ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            hasSelection = false;
            return false;
        }
    }

    // 2. Relative Transpose: Ctrl + Up/Down (or Shift+Ctrl for octaves)
    if (isCtrl && (ImGui::IsKeyPressed(ImGuiKey_UpArrow) || ImGui::IsKeyPressed(ImGuiKey_DownArrow))) {
        int delta = (ImGui::IsKeyPressed(ImGuiKey_UpArrow) ? 1 : -1) * (isShift ? 12 : 1);
        transposeSelection(pat, patIdx, sel, delta, history, song, engineHost);
        return true;
    }

    // 3. Relative Volume: Alt + Up/Down or '[' / ']'
    if (io.KeyAlt && (ImGui::IsKeyPressed(ImGuiKey_UpArrow) || ImGui::IsKeyPressed(ImGuiKey_DownArrow))) {
        int delta = ImGui::IsKeyPressed(ImGuiKey_UpArrow) ? 4 : -4;
        adjustVolumeSelection(pat, patIdx, sel, delta, history, song, engineHost);
        return true;
    }
    if (ImGui::IsKeyPressed(ImGuiKey_LeftBracket) || ImGui::IsKeyPressed(ImGuiKey_RightBracket)) {
        int delta = ImGui::IsKeyPressed(ImGuiKey_RightBracket) ? 4 : -4;
        adjustVolumeSelection(pat, patIdx, sel, delta, history, song, engineHost);
        return true;
    }

    // 4. Clipboard: Copy / Cut / Paste
    if (isCtrl) {
        if (ImGui::IsKeyPressed(ImGuiKey_C)) {
            copySelection(pat, sel, clipboard);
            return true;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_X) && canEdit) {
            copySelection(pat, sel, clipboard);
            clearSelection(pat, patIdx, sel, -1, history, song, engineHost);
            return true;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_V) && canEdit && !clipboard.empty()) {
            pasteSelection(pat, patIdx, sel.minTrack(), sel.minRow(), clipboard, history, song, engineHost);
            return true;
        }
    }

    // 5. Clear / Delete
    if (canEdit && (ImGui::IsKeyPressed(ImGuiKey_Period) || ImGui::IsKeyPressed(ImGuiKey_Delete) || ImGui::IsKeyPressed(ImGuiKey_Backspace))) {
        clearSelection(pat, patIdx, sel, cursorSubCol, history, song, engineHost);
        return true;
    }

    // 6. Uniform Note Assignment via Piano Keys
    if (canEdit && !isCtrl && !io.KeyAlt && cursorSubCol == 0) {
        struct KeyNote { ImGuiKey key; int semi; };
        const KeyNote pianoKeys[] = {
            {ImGuiKey_Z, 0}, {ImGuiKey_S, 1}, {ImGuiKey_X, 2}, {ImGuiKey_D, 3}, {ImGuiKey_C, 4},
            {ImGuiKey_V, 5}, {ImGuiKey_G, 6}, {ImGuiKey_B, 7}, {ImGuiKey_H, 8}, {ImGuiKey_N, 9},
            {ImGuiKey_J, 10}, {ImGuiKey_M, 11}, {ImGuiKey_Q, 12}, {ImGuiKey_2, 13}, {ImGuiKey_W, 14},
            {ImGuiKey_3, 15}, {ImGuiKey_E, 16}, {ImGuiKey_R, 17}, {ImGuiKey_5, 18}, {ImGuiKey_T, 19},
            {ImGuiKey_6, 20}, {ImGuiKey_Y, 21}, {ImGuiKey_7, 22}, {ImGuiKey_U, 23}
        };
        for (const auto& kn : pianoKeys) {
            if (ImGui::IsKeyPressed(kn.key, false)) {
                int midiNote = std::clamp(octave * 12 + kn.semi, 0, 119);
                engineHost.previewNoteOn(sel.minTrack(), selectedInstIdx, midiNote, 0.9f);
                setSelectionToNote(pat, patIdx, sel, midiNote, selectedInstIdx, history, song, engineHost);
                return true;
            }
        }
    }

    return false;
}

} // namespace trakr

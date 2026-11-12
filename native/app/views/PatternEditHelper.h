#pragma once

#include "Song.h"
#include "History.h"
#include "EngineHost.h"
#include "PatternSelectionHelper.h"
#include <vector>

namespace trakr {

// Inserts an empty cell at (track, row), shifting lower cells down
void insertRowAtCursor(Pattern& pat, int patIdx, int track, int row,
                       Song& song, History& history, EngineHost& engineHost);

// Clears cell or deletes row (shifting lower cells up)
void deleteRowOrCellAtCursor(Pattern& pat, int patIdx, int track, int row, int subCol,
                             Song& song, History& history, EngineHost& engineHost,
                             bool shiftHeld, int editStep);

// Copies selection or cell at cursor to clipboard
void copySelectionOrCell(const Pattern& pat, int track, int row,
                         bool hasSelection, const SelectionRange& sel,
                         std::vector<std::vector<Cell>>& clipboard);

// Pastes clipboard at cursor
void pasteAtCursor(Pattern& pat, int patIdx, int track, int row,
                   const std::vector<std::vector<Cell>>& clipboard,
                   Song& song, History& history, EngineHost& engineHost);

// Adjusts value at cursor (note transpose, inst idx, volume, fx value)
void adjustValueAtCursor(Pattern& pat, int patIdx, int track, int row, int subCol,
                         int delta, bool shiftHeld,
                         Song& song, History& history, EngineHost& engineHost);

// Pattern transforms (More page 2 parity: duplicate, expand, shrink, invert, copy, paste)
void duplicatePattern(Pattern& pat, int patIdx, Song& song, History& history, EngineHost& engineHost);
void expandPattern(Pattern& pat, int patIdx, Song& song, History& history, EngineHost& engineHost);
void shrinkPattern(Pattern& pat, int patIdx, Song& song, History& history, EngineHost& engineHost);
void invertSelectionOrPattern(Pattern& pat, int patIdx, bool hasSelection, const SelectionRange& sel,
                              Song& song, History& history, EngineHost& engineHost);
void copyPattern(const Pattern& pat, Pattern& clipboard);
void pastePattern(Pattern& pat, int patIdx, const Pattern& clipboard,
                  Song& song, History& history, EngineHost& engineHost);

} // namespace trakr


#include "Command.h"
#include <algorithm>

namespace trakr {

void InsertRowsCommand::apply(Song& song) {
    if (patternIdx_ < 0 || patternIdx_ >= static_cast<int>(song.patterns.size())) return;
    auto& pat = song.patterns[patternIdx_];
    droppedRows_.clear();
    droppedRows_.resize(pat.tracks.size());

    for (size_t t = 0; t < pat.tracks.size(); ++t) {
        auto& trk = pat.tracks[t];
        if (startRow_ < 0 || startRow_ >= static_cast<int>(trk.size())) continue;

        // Save rows pushed off the end
        int saveCount = std::min(count_, static_cast<int>(trk.size()) - startRow_);
        droppedRows_[t].assign(trk.end() - saveCount, trk.end());

        // Shift down from the bottom
        for (int r = static_cast<int>(trk.size()) - 1; r >= startRow_ + count_; --r) {
            trk[r] = trk[r - count_];
        }
        // Blank newly inserted rows
        for (int r = startRow_; r < startRow_ + count_ && r < static_cast<int>(trk.size()); ++r) {
            trk[r] = Cell{};
        }
    }
}

void InsertRowsCommand::revert(Song& song) {
    if (patternIdx_ < 0 || patternIdx_ >= static_cast<int>(song.patterns.size())) return;
    auto& pat = song.patterns[patternIdx_];

    for (size_t t = 0; t < pat.tracks.size(); ++t) {
        auto& trk = pat.tracks[t];
        if (startRow_ < 0 || startRow_ >= static_cast<int>(trk.size())) continue;

        // Shift up
        for (int r = startRow_; r + count_ < static_cast<int>(trk.size()); ++r) {
            trk[r] = trk[r + count_];
        }
        // Restore dropped rows at the end
        if (t < droppedRows_.size() && !droppedRows_[t].empty()) {
            size_t restoreOffset = trk.size() - droppedRows_[t].size();
            for (size_t i = 0; i < droppedRows_[t].size(); ++i) {
                trk[restoreOffset + i] = droppedRows_[t][i];
            }
        }
    }
}

void DeleteRowsCommand::apply(Song& song) {
    if (patternIdx_ < 0 || patternIdx_ >= static_cast<int>(song.patterns.size())) return;
    auto& pat = song.patterns[patternIdx_];
    deletedRows_.clear();
    deletedRows_.resize(pat.tracks.size());

    for (size_t t = 0; t < pat.tracks.size(); ++t) {
        auto& trk = pat.tracks[t];
        if (startRow_ < 0 || startRow_ >= static_cast<int>(trk.size())) continue;

        // Save rows to be deleted
        int actualCount = std::min(count_, static_cast<int>(trk.size()) - startRow_);
        deletedRows_[t].assign(trk.begin() + startRow_, trk.begin() + startRow_ + actualCount);

        // Shift up
        for (int r = startRow_; r + actualCount < static_cast<int>(trk.size()); ++r) {
            trk[r] = trk[r + actualCount];
        }
        // Blank the tail rows
        for (size_t r = trk.size() - actualCount; r < trk.size(); ++r) {
            trk[r] = Cell{};
        }
    }
}

void DeleteRowsCommand::revert(Song& song) {
    if (patternIdx_ < 0 || patternIdx_ >= static_cast<int>(song.patterns.size())) return;
    auto& pat = song.patterns[patternIdx_];

    for (size_t t = 0; t < pat.tracks.size(); ++t) {
        auto& trk = pat.tracks[t];
        if (startRow_ < 0 || startRow_ >= static_cast<int>(trk.size())) continue;
        if (t >= deletedRows_.size() || deletedRows_[t].empty()) continue;

        int actualCount = static_cast<int>(deletedRows_[t].size());
        // Shift down
        for (int r = static_cast<int>(trk.size()) - 1; r >= startRow_ + actualCount; --r) {
            trk[r] = trk[r - actualCount];
        }
        // Restore deleted rows
        for (int i = 0; i < actualCount; ++i) {
            trk[startRow_ + i] = deletedRows_[t][i];
        }
    }
}

void SetPatternLengthCommand::apply(Song& song) {
    if (patternIdx_ < 0 || patternIdx_ >= static_cast<int>(song.patterns.size())) return;
    auto& pat = song.patterns[patternIdx_];
    truncatedCells_.clear();

    if (newRows_ < oldRows_) {
        truncatedCells_.resize(pat.tracks.size());
        for (size_t t = 0; t < pat.tracks.size(); ++t) {
            truncatedCells_[t].assign(pat.tracks[t].begin() + newRows_, pat.tracks[t].end());
        }
    }

    pat.rows = newRows_;
    for (auto& trk : pat.tracks) {
        trk.resize(newRows_);
    }
}

void SetPatternLengthCommand::revert(Song& song) {
    if (patternIdx_ < 0 || patternIdx_ >= static_cast<int>(song.patterns.size())) return;
    auto& pat = song.patterns[patternIdx_];

    pat.rows = oldRows_;
    for (size_t t = 0; t < pat.tracks.size(); ++t) {
        pat.tracks[t].resize(oldRows_);
        if (t < truncatedCells_.size() && !truncatedCells_[t].empty()) {
            std::copy(truncatedCells_[t].begin(), truncatedCells_[t].end(), pat.tracks[t].begin() + newRows_);
        }
    }
}

} // namespace trakr


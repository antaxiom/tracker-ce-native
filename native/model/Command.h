#pragma once

#include "Song.h"
#include <string>
#include <vector>
#include <memory>
#include <chrono>

namespace trakr {

class Command {
public:
    virtual ~Command() = default;
    virtual void apply(Song& song) = 0;
    virtual void revert(Song& song) = 0;
    virtual std::string label() const = 0;
    virtual bool canMerge(const Command* other) const { (void)other; return false; }
    virtual void merge(std::unique_ptr<Command> other) { (void)other; }

    uint64_t timestampMs() const { return timestampMs_; }
    void setTimestampMs(uint64_t ts) { timestampMs_ = ts; }

protected:
    uint64_t timestampMs_{0};
};

class SetCellCommand : public Command {
public:
    SetCellCommand(int patternIdx, int trackIdx, int rowIdx, const Cell& newCell, const Cell& oldCell)
        : patternIdx_(patternIdx), trackIdx_(trackIdx), rowIdx_(rowIdx),
          newCell_(newCell), oldCell_(oldCell) {}

    void apply(Song& song) override {
        if (isValid(song)) song.patterns[patternIdx_].tracks[trackIdx_][rowIdx_] = newCell_;
    }

    void revert(Song& song) override {
        if (isValid(song)) song.patterns[patternIdx_].tracks[trackIdx_][rowIdx_] = oldCell_;
    }

    std::string label() const override { return "Edit Cell"; }

    bool canMerge(const Command* other) const override {
        const auto* o = dynamic_cast<const SetCellCommand*>(other);
        if (!o) return false;
        return (patternIdx_ == o->patternIdx_ && trackIdx_ == o->trackIdx_ && rowIdx_ == o->rowIdx_);
    }

    void merge(std::unique_ptr<Command> other) override {
        auto* o = static_cast<SetCellCommand*>(other.get());
        newCell_ = o->newCell_;
    }

private:
    bool isValid(const Song& song) const {
        return (patternIdx_ >= 0 && patternIdx_ < static_cast<int>(song.patterns.size()) &&
                trackIdx_ >= 0 && trackIdx_ < static_cast<int>(song.patterns[patternIdx_].tracks.size()) &&
                rowIdx_ >= 0 && rowIdx_ < song.patterns[patternIdx_].rows);
    }

    int patternIdx_;
    int trackIdx_;
    int rowIdx_;
    Cell newCell_;
    Cell oldCell_;
};

class SetCellRangeCommand : public Command {
public:
    SetCellRangeCommand(int patternIdx, int trackIdx, int startRow,
                        std::vector<Cell> newCells, std::vector<Cell> oldCells)
        : patternIdx_(patternIdx), trackIdx_(trackIdx), startRow_(startRow),
          newCells_(std::move(newCells)), oldCells_(std::move(oldCells)) {}

    void apply(Song& song) override {
        applyCells(song, newCells_);
    }

    void revert(Song& song) override {
        applyCells(song, oldCells_);
    }

    std::string label() const override { return "Edit Cell Range"; }

private:
    void applyCells(Song& song, const std::vector<Cell>& cells) {
        if (patternIdx_ < 0 || patternIdx_ >= static_cast<int>(song.patterns.size())) return;
        auto& pat = song.patterns[patternIdx_];
        if (trackIdx_ < 0 || trackIdx_ >= static_cast<int>(pat.tracks.size())) return;
        auto& trk = pat.tracks[trackIdx_];
        for (size_t i = 0; i < cells.size(); ++i) {
            int r = startRow_ + static_cast<int>(i);
            if (r >= 0 && r < pat.rows && r < static_cast<int>(trk.size())) {
                trk[r] = cells[i];
            }
        }
    }

    int patternIdx_;
    int trackIdx_;
    int startRow_;
    std::vector<Cell> newCells_;
    std::vector<Cell> oldCells_;
};

class InsertRowsCommand : public Command {
public:
    InsertRowsCommand(int patternIdx, int startRow, int count)
        : patternIdx_(patternIdx), startRow_(startRow), count_(count) {}

    void apply(Song& song) override;
    void revert(Song& song) override;
    std::string label() const override { return "Insert Rows"; }

private:
    int patternIdx_;
    int startRow_;
    int count_;
    std::vector<std::vector<Cell>> droppedRows_; // Cells pushed off the bottom
};

class DeleteRowsCommand : public Command {
public:
    DeleteRowsCommand(int patternIdx, int startRow, int count)
        : patternIdx_(patternIdx), startRow_(startRow), count_(count) {}

    void apply(Song& song) override;
    void revert(Song& song) override;
    std::string label() const override { return "Delete Rows"; }

private:
    int patternIdx_;
    int startRow_;
    int count_;
    std::vector<std::vector<Cell>> deletedRows_;
};

class SetPatternLengthCommand : public Command {
public:
    SetPatternLengthCommand(int patternIdx, int newRows, int oldRows)
        : patternIdx_(patternIdx), newRows_(newRows), oldRows_(oldRows) {}

    void apply(Song& song) override;
    void revert(Song& song) override;
    std::string label() const override { return "Change Pattern Length"; }

private:
    int patternIdx_;
    int newRows_;
    int oldRows_;
    std::vector<std::vector<Cell>> truncatedCells_;
};

class SetOrderCommand : public Command {
public:
    SetOrderCommand(std::vector<int> newOrder, std::vector<int> oldOrder)
        : newOrder_(std::move(newOrder)), oldOrder_(std::move(oldOrder)) {}

    void apply(Song& song) override { song.order = newOrder_; }
    void revert(Song& song) override { song.order = oldOrder_; }
    std::string label() const override { return "Reorder Sequence"; }

private:
    std::vector<int> newOrder_;
    std::vector<int> oldOrder_;
};

class SetTrackInfoCommand : public Command {
public:
    SetTrackInfoCommand(int trackIdx, TrackInfo newInfo, TrackInfo oldInfo)
        : trackIdx_(trackIdx), newInfo_(std::move(newInfo)), oldInfo_(std::move(oldInfo)) {}

    void apply(Song& song) override {
        if (trackIdx_ >= 0 && trackIdx_ < static_cast<int>(song.tracks.size())) {
            song.tracks[trackIdx_] = newInfo_;
        }
    }
    void revert(Song& song) override {
        if (trackIdx_ >= 0 && trackIdx_ < static_cast<int>(song.tracks.size())) {
            song.tracks[trackIdx_] = oldInfo_;
        }
    }
    std::string label() const override { return "Edit Track Info"; }

private:
    int trackIdx_;
    TrackInfo newInfo_;
    TrackInfo oldInfo_;
};

class SetInstrumentCommand : public Command {
public:
    SetInstrumentCommand(int instIdx, Instrument newInst, Instrument oldInst)
        : instIdx_(instIdx), newInst_(std::move(newInst)), oldInst_(std::move(oldInst)) {}

    void apply(Song& song) override {
        if (instIdx_ >= 0 && instIdx_ < static_cast<int>(song.instruments.size())) {
            song.instruments[instIdx_] = newInst_;
        }
    }
    void revert(Song& song) override {
        if (instIdx_ >= 0 && instIdx_ < static_cast<int>(song.instruments.size())) {
            song.instruments[instIdx_] = oldInst_;
        }
    }
    std::string label() const override { return "Edit Instrument"; }

private:
    int instIdx_;
    Instrument newInst_;
    Instrument oldInst_;
};

class SetSongParamCommand : public Command {
public:
    SetSongParamCommand(float newBpm, float oldBpm, int newLpb, int oldLpb)
        : newBpm_(newBpm), oldBpm_(oldBpm), newLpb_(newLpb), oldLpb_(oldLpb) {}

    void apply(Song& song) override { song.bpm = newBpm_; song.lpb = newLpb_; }
    void revert(Song& song) override { song.bpm = oldBpm_; song.lpb = oldLpb_; }
    std::string label() const override { return "Set Tempo / LPB"; }

private:
    float newBpm_;
    float oldBpm_;
    int newLpb_;
    int oldLpb_;
};

class SetTrackCommand : public Command {
public:
    SetTrackCommand(int patternIdx, int trackIdx, std::vector<Cell> newTrack, std::vector<Cell> oldTrack)
        : patIdx_(patternIdx), trackIdx_(trackIdx), newTrack_(std::move(newTrack)), oldTrack_(std::move(oldTrack)) {}

    void apply(Song& song) override {
        if (patIdx_ >= 0 && patIdx_ < static_cast<int>(song.patterns.size())) {
            auto& pat = song.patterns[patIdx_];
            if (trackIdx_ >= 0 && trackIdx_ < static_cast<int>(pat.tracks.size())) {
                pat.tracks[trackIdx_] = newTrack_;
            }
        }
    }
    void revert(Song& song) override {
        if (patIdx_ >= 0 && patIdx_ < static_cast<int>(song.patterns.size())) {
            auto& pat = song.patterns[patIdx_];
            if (trackIdx_ >= 0 && trackIdx_ < static_cast<int>(pat.tracks.size())) {
                pat.tracks[trackIdx_] = oldTrack_;
            }
        }
    }
    std::string label() const override { return "Fill Track"; }

private:
    int patIdx_;
    int trackIdx_;
    std::vector<Cell> newTrack_;
    std::vector<Cell> oldTrack_;
};

class SetSampleDataCommand : public Command {
public:
    SetSampleDataCommand(int instIdx, std::shared_ptr<const SampleData> newSample,
                         std::shared_ptr<const SampleData> oldSample, std::string label = "Edit Sample")
        : instIdx_(instIdx), newSample_(std::move(newSample)),
          oldSample_(std::move(oldSample)), label_(std::move(label)) {}

    void apply(Song& song) override {
        if (instIdx_ >= 0 && instIdx_ < static_cast<int>(song.instruments.size())) {
            song.instruments[instIdx_].sample = newSample_;
        }
    }
    void revert(Song& song) override {
        if (instIdx_ >= 0 && instIdx_ < static_cast<int>(song.instruments.size())) {
            song.instruments[instIdx_].sample = oldSample_;
        }
    }
    std::string label() const override { return label_; }

private:
    int instIdx_;
    std::shared_ptr<const SampleData> newSample_;
    std::shared_ptr<const SampleData> oldSample_;
    std::string label_;
};

class AddTrackCommand : public Command {
public:
    AddTrackCommand() = default;
    void apply(Song& song) override {
        int newIdx = static_cast<int>(song.tracks.size()) + 1;
        TrackInfo info;
        info.name = "Track " + std::to_string(newIdx);
        song.tracks.push_back(info);
        for (auto& pat : song.patterns) {
            pat.tracks.emplace_back(pat.rows, Cell{});
        }
    }
    void revert(Song& song) override {
        if (!song.tracks.empty()) {
            song.tracks.pop_back();
            for (auto& pat : song.patterns) {
                if (!pat.tracks.empty()) pat.tracks.pop_back();
            }
        }
    }
    std::string label() const override { return "Add Track"; }
};

class RemoveTrackCommand : public Command {
public:
    RemoveTrackCommand() = default;
    void apply(Song& song) override {
        if (song.tracks.size() <= 1) return;
        savedTrackInfo_ = song.tracks.back();
        song.tracks.pop_back();
        savedPatternCells_.clear();
        for (auto& pat : song.patterns) {
            if (!pat.tracks.empty()) {
                savedPatternCells_.push_back(pat.tracks.back());
                pat.tracks.pop_back();
            }
        }
    }
    void revert(Song& song) override {
        song.tracks.push_back(savedTrackInfo_);
        for (size_t p = 0; p < song.patterns.size() && p < savedPatternCells_.size(); ++p) {
            song.patterns[p].tracks.push_back(savedPatternCells_[p]);
        }
    }
    std::string label() const override { return "Remove Track"; }
private:
    TrackInfo savedTrackInfo_{};
    std::vector<std::vector<Cell>> savedPatternCells_{};
};

} // namespace trakr


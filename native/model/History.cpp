#include "History.h"
#include <chrono>

namespace trakr {

History::History(size_t maxDepth)
    : maxDepth_(maxDepth) {}

uint64_t History::nowMs() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();
}

void History::execute(std::unique_ptr<Command> cmd, Song& song) {
    if (!cmd) return;

    const uint64_t now = nowMs();
    cmd->setTimestampMs(now);

    // Check merge with top of undo stack if within 500ms
    if (!undoStack_.empty()) {
        auto& top = undoStack_.back();
        if (top->canMerge(cmd.get()) && (now - top->timestampMs() < 500)) {
            cmd->apply(song);
            top->merge(std::move(cmd));
            top->setTimestampMs(now);
            redoStack_.clear();
            return;
        }
    }

    cmd->apply(song);
    undoStack_.push_back(std::move(cmd));
    redoStack_.clear();

    if (undoStack_.size() > maxDepth_) {
        undoStack_.erase(undoStack_.begin());
    }
}

bool History::undo(Song& song) {
    if (undoStack_.empty()) return false;

    auto cmd = std::move(undoStack_.back());
    undoStack_.pop_back();

    cmd->revert(song);
    redoStack_.push_back(std::move(cmd));
    return true;
}

bool History::redo(Song& song) {
    if (redoStack_.empty()) return false;

    auto cmd = std::move(redoStack_.back());
    redoStack_.pop_back();

    cmd->apply(song);
    undoStack_.push_back(std::move(cmd));
    return true;
}

std::string History::undoLabel() const {
    if (undoStack_.empty()) return "";
    return undoStack_.back()->label();
}

std::string History::redoLabel() const {
    if (redoStack_.empty()) return "";
    return redoStack_.back()->label();
}

void History::clear() {
    undoStack_.clear();
    redoStack_.clear();
}

} // namespace trakr


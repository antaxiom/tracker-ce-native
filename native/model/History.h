#pragma once

#include "Command.h"
#include <vector>
#include <memory>
#include <string>

namespace trakr {

class History {
public:
    History(size_t maxDepth = 100);

    void execute(std::unique_ptr<Command> cmd, Song& song);
    bool undo(Song& song);
    bool redo(Song& song);

    bool canUndo() const { return !undoStack_.empty(); }
    bool canRedo() const { return !redoStack_.empty(); }

    std::string undoLabel() const;
    std::string redoLabel() const;

    void clear();

private:
    uint64_t nowMs() const;

    size_t maxDepth_;
    std::vector<std::unique_ptr<Command>> undoStack_;
    std::vector<std::unique_ptr<Command>> redoStack_;
};

} // namespace trakr


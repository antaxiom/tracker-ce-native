#pragma once

#include "imgui.h"
#include <string>
#include <unordered_map>

namespace trakr {

enum class ActionId {
    PlayStop,
    NewProject,
    OpenProject,
    Fill,
    ExportDeck,
    PlayPattern,
    RecordToggle,
    Stop,
    OctaveUp,
    OctaveDown,
    Undo,
    Redo,
    Cut,
    Copy,
    Paste,
    Save,
    SaveAs,
    ToggleKitGallery,
    ResetLayout
};

struct KeyChord {
    ImGuiKey key{ImGuiKey_None};
    bool ctrl{false};
    bool shift{false};
    bool alt{false};

    bool matches(const ImGuiIO& io) const;
    std::string toString() const;
};

class ShortcutMap {
public:
    ShortcutMap();

    void bind(ActionId action, KeyChord chord);
    bool isTriggered(ActionId action) const;
    std::string getShortcutText(ActionId action) const;

    void resetToDefaults();

private:
    std::unordered_map<ActionId, KeyChord> bindings_;
};

} // namespace trakr


#include "ShortcutMap.h"

namespace trakr {

bool KeyChord::matches(const ImGuiIO& io) const {
    if (key == ImGuiKey_None) return false;
    if (!ImGui::IsKeyPressed(key, false)) return false;

    const bool hasCtrl = io.KeyCtrl;
    const bool hasShift = io.KeyShift;
    const bool hasAlt = io.KeyAlt;

    return (hasCtrl == ctrl) && (hasShift == shift) && (hasAlt == alt);
}

std::string KeyChord::toString() const {
    std::string s;
    // On macOS io.KeyCtrl is driven by Cmd (ImGui swaps them), so label it Cmd.
    const bool mac = ImGui::GetCurrentContext() && ImGui::GetIO().ConfigMacOSXBehaviors;
    if (ctrl) s += mac ? "Cmd+" : "Ctrl+";
    if (alt) s += "Alt+";
    if (shift) s += "Shift+";

    const char* keyName = ImGui::GetKeyName(key);
    s += (keyName ? keyName : "");
    return s;
}

ShortcutMap::ShortcutMap() {
    resetToDefaults();
}

void ShortcutMap::resetToDefaults() {
    bindings_.clear();
    bindings_[ActionId::PlayStop] = {ImGuiKey_Space, false, false, false};
    bindings_[ActionId::NewProject] = {ImGuiKey_N, true, false, false};
    bindings_[ActionId::OpenProject] = {ImGuiKey_O, true, false, false};
    bindings_[ActionId::Fill] = {ImGuiKey_F, true, false, false};
    // F1..F8 are the eight on-screen soft buttons; do not shadow F8 here.
    bindings_[ActionId::ExportDeck] = {ImGuiKey_E, true, false, false};
    bindings_[ActionId::PlayPattern] = {ImGuiKey_RightShift, false, false, false};
    bindings_[ActionId::RecordToggle] = {ImGuiKey_R, true, false, false}; // Ctrl+R
    bindings_[ActionId::Stop] = {ImGuiKey_Escape, false, false, false};

    bindings_[ActionId::OctaveUp] = {ImGuiKey_KeypadDivide, false, false, false};
    bindings_[ActionId::OctaveDown] = {ImGuiKey_KeypadMultiply, false, false, false};

    bindings_[ActionId::Undo] = {ImGuiKey_Z, true, false, false};
    bindings_[ActionId::Redo] = {ImGuiKey_Y, true, false, false};
    bindings_[ActionId::Cut] = {ImGuiKey_X, true, false, false};
    bindings_[ActionId::Copy] = {ImGuiKey_C, true, false, false};
    bindings_[ActionId::Paste] = {ImGuiKey_V, true, false, false};
    bindings_[ActionId::Save] = {ImGuiKey_S, true, false, false};
    bindings_[ActionId::SaveAs] = {ImGuiKey_S, true, true, false};

    bindings_[ActionId::ToggleKitGallery] = {ImGuiKey_F11, false, false, false};
    bindings_[ActionId::ResetLayout] = {ImGuiKey_R, true, true, false}; // Ctrl+Shift+R
}

void ShortcutMap::bind(ActionId action, KeyChord chord) {
    bindings_[action] = chord;
}

bool ShortcutMap::isTriggered(ActionId action) const {
    auto it = bindings_.find(action);
    if (it == bindings_.end()) return false;
    return it->second.matches(ImGui::GetIO());
}

std::string ShortcutMap::getShortcutText(ActionId action) const {
    auto it = bindings_.find(action);
    if (it == bindings_.end()) return "";
    return it->second.toString();
}

} // namespace trakr


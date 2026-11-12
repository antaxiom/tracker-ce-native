#include "FileDialogs.h"
#include "RenderScheduler.h"
#include "imgui.h"
#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <vector>

namespace trakr::dialogs {

namespace {

enum class Mode { None, OpenSong, SaveSong, SaveWav, ChooseFolder };

struct DialogState {
    bool open{false};
    bool popupOpened{false};
    bool focusSet{false};
    Mode mode{Mode::None};
    std::string* target{nullptr};
    RenderScheduler* scheduler{nullptr};
    std::filesystem::path dir;
    std::string name;
    int selected{0};
    std::vector<std::string> entries;
    std::vector<bool> isDir;
};

static DialogState s_dlg;
static const char* kPopupId = "##trakr_file_dialog";

static bool matchesMode(const std::filesystem::path& p, Mode mode) {
    std::string ext = p.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    if (mode == Mode::OpenSong) return ext == ".json" || ext == ".trakr" || ext == ".mtp";
    if (mode == Mode::SaveWav) return ext == ".wav";
    return true;
}

static void refresh() {
    s_dlg.entries.clear();
    s_dlg.isDir.clear();
    std::error_code ec;
    if (s_dlg.dir.has_parent_path()) {
        s_dlg.entries.push_back("..");
        s_dlg.isDir.push_back(true);
    }
    std::vector<std::pair<std::string, bool>> dirs, files;
    for (const auto& e : std::filesystem::directory_iterator(s_dlg.dir, ec)) {
        std::string n = e.path().filename().string();
        if (n.empty() || n[0] == '.') continue;
        if (e.is_directory(ec)) dirs.push_back({n, true});
        else if (matchesMode(e.path(), s_dlg.mode)) files.push_back({n, false});
    }
    std::sort(dirs.begin(), dirs.end());
    std::sort(files.begin(), files.end());
    for (auto& d : dirs) { s_dlg.entries.push_back(d.first); s_dlg.isDir.push_back(true); }
    for (auto& f : files) { s_dlg.entries.push_back(f.first); s_dlg.isDir.push_back(false); }
    s_dlg.selected = s_dlg.entries.empty() ? -1 : 0;
}

static void begin(Mode mode, std::string* target, RenderScheduler* scheduler,
                  const std::string& defaultName) {
    s_dlg = DialogState{};
    s_dlg.open = true;
    s_dlg.mode = mode;
    s_dlg.target = target;
    s_dlg.scheduler = scheduler;
    std::error_code ec;
    s_dlg.dir = std::filesystem::current_path(ec);
    s_dlg.name = defaultName;
    if (mode != Mode::SaveSong && mode != Mode::SaveWav) refresh();
}

static void finish(const std::string& value) {
    if (s_dlg.target) *(s_dlg.target) = value;
    if (s_dlg.scheduler) s_dlg.scheduler->requestRedraw(6);
    ImGui::CloseCurrentPopup();
    s_dlg = DialogState{};
}

static void cancel() {
    ImGui::CloseCurrentPopup();
    s_dlg = DialogState{};
}

} // namespace

void showOpenSong(SDL_Window*, std::string* targetString, RenderScheduler* scheduler) {
    begin(Mode::OpenSong, targetString, scheduler, "");
}

void showSaveSong(SDL_Window*, std::string* targetString, RenderScheduler* scheduler) {
    begin(Mode::SaveSong, targetString, scheduler, "project.trakr.json");
}

void showExportWav(SDL_Window*, std::string* targetString, RenderScheduler* scheduler) {
    begin(Mode::SaveWav, targetString, scheduler, "export.wav");
}

void showExportFolder(SDL_Window*, std::string* targetString, RenderScheduler* scheduler) {
    begin(Mode::ChooseFolder, targetString, scheduler, "");
}

bool isOpen() { return s_dlg.open; }

void renderDialogs() {
    if (!s_dlg.open) return;
    const char* title = s_dlg.mode == Mode::OpenSong ? "Open Project"
                      : s_dlg.mode == Mode::SaveSong ? "Save Project"
                      : s_dlg.mode == Mode::SaveWav ? "Export WAV"
                      : "Choose Folder";
    if (!s_dlg.popupOpened) {
        ImGui::OpenPopup(kPopupId);
        s_dlg.popupOpened = true;
    }
    ImGui::SetNextWindowSize(ImVec2(620, 420), ImGuiCond_Appearing);
    bool keep = true;
    if (!ImGui::BeginPopupModal(kPopupId, &keep, ImGuiWindowFlags_NoSavedSettings)) {
        s_dlg = DialogState{};
        return;
    }
    ImGui::TextUnformatted(title);

    if (s_dlg.mode == Mode::SaveSong || s_dlg.mode == Mode::SaveWav) {
        ImGui::TextDisabled("Save to: %s", s_dlg.dir.string().c_str());
        if (s_dlg.popupOpened && !s_dlg.focusSet) { ImGui::SetKeyboardFocusHere(); s_dlg.focusSet = true; }
        char buf[512];
        std::snprintf(buf, sizeof(buf), "%s", s_dlg.name.c_str());
        ImGui::SetNextItemWidth(-1);
        if (ImGui::InputText("##name", buf, sizeof(buf))) s_dlg.name = buf;
        ImGui::Spacing();
        bool ok = !s_dlg.name.empty();
        if (ImGui::Button("Save", ImVec2(140, 0)) && ok) {
            finish((s_dlg.dir / s_dlg.name).string());
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) cancel();
        if (ok && ImGui::IsKeyPressed(ImGuiKey_Enter, false)) {
            finish((s_dlg.dir / s_dlg.name).string());
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Escape, false)) cancel();
        ImGui::EndPopup();
        return;
    }

    ImGui::TextDisabled("%s", s_dlg.dir.string().c_str());
    if (ImGui::Button("Up")) {
        if (s_dlg.dir.has_parent_path()) { s_dlg.dir = s_dlg.dir.parent_path(); refresh(); }
    }
    ImGui::SameLine();
    if (ImGui::Button("Refresh")) refresh();

    ImGui::BeginChild("##entries", ImVec2(0, -36), ImGuiChildFlags_Borders);
    for (int i = 0; i < static_cast<int>(s_dlg.entries.size()); ++i) {
        std::string label = s_dlg.entries[i] + (s_dlg.isDir[i] ? "/" : "");
        if (ImGui::Selectable(label.c_str(), s_dlg.selected == i)) s_dlg.selected = i;
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
            if (s_dlg.isDir[i]) {
                if (s_dlg.entries[i] == "..") s_dlg.dir = s_dlg.dir.parent_path();
                else s_dlg.dir /= s_dlg.entries[i];
                refresh();
            } else if (s_dlg.mode == Mode::OpenSong) {
                finish((s_dlg.dir / s_dlg.entries[i]).string());
                ImGui::EndChild();
                ImGui::EndPopup();
                return;
            }
        }
    }
    ImGui::EndChild();

    if (ImGui::IsKeyPressed(ImGuiKey_Enter, false)) {
        if (s_dlg.selected >= 0 && s_dlg.selected < static_cast<int>(s_dlg.entries.size())) {
            if (s_dlg.isDir[s_dlg.selected]) {
                if (s_dlg.entries[s_dlg.selected] == "..") s_dlg.dir = s_dlg.dir.parent_path();
                else s_dlg.dir /= s_dlg.entries[s_dlg.selected];
                refresh();
            } else if (s_dlg.mode == Mode::OpenSong) {
                finish((s_dlg.dir / s_dlg.entries[s_dlg.selected]).string());
                ImGui::EndChild();
                ImGui::EndPopup();
                return;
            }
        }
    }
    const bool haveSel = s_dlg.selected >= 0 && s_dlg.selected < static_cast<int>(s_dlg.entries.size());
    const bool selIsFile = haveSel && !s_dlg.isDir[s_dlg.selected];
    const char* confirmLabel = (s_dlg.mode == Mode::ChooseFolder) ? "Choose Folder" : "Open";
    const bool canConfirm = (s_dlg.mode == Mode::ChooseFolder) || selIsFile;
    ImGui::BeginDisabled(!canConfirm);
    if (ImGui::Button(confirmLabel, ImVec2(140, 0))) {
        if (s_dlg.mode == Mode::ChooseFolder) finish(s_dlg.dir.string());
        else if (selIsFile) finish((s_dlg.dir / s_dlg.entries[s_dlg.selected]).string());
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(120, 0))) cancel();
    ImGui::SameLine();
    if (haveSel && s_dlg.isDir[s_dlg.selected]) {
        if (ImGui::Button("Enter", ImVec2(120, 0))) {
            if (s_dlg.entries[s_dlg.selected] == "..") s_dlg.dir = s_dlg.dir.parent_path();
            else s_dlg.dir /= s_dlg.entries[s_dlg.selected];
            refresh();
        }
    }
    if (ImGui::IsKeyPressed(ImGuiKey_Escape, false)) cancel();
    ImGui::EndPopup();
}

} // namespace trakr::dialogs

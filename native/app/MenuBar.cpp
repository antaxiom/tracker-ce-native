#include "MenuBar.h"
#include "imgui.h"
#include "Theme.h"
#include "ShortcutMap.h"
#include "RenderScheduler.h"
#include "FileDialogs.h"
#include "MidiManager.h"
#include "Song.h"
#include "SongJson.h"
#include "History.h"
#include "Command.h"
#include "EngineHost.h"
#include <filesystem>

namespace trakr {

static std::string s_pendingDeletePath;

void renderMainMenuBar(MenuBarContext& ctx) {
    if (!ImGui::BeginMainMenuBar()) return;

    ImGuiIO& io = ImGui::GetIO();

    if (ImGui::BeginMenu("File")) {
        auto sc = [&](ActionId a) { return ctx.shortcuts.getShortcutText(a); };
        if (ImGui::MenuItem("New Project", sc(ActionId::NewProject).c_str())) {
            ctx.song = makeDefaultSong();
            ctx.history.clear();
            ctx.currentFilePath.clear();
            if (ctx.resetCursor) ctx.resetCursor();
            ctx.engineHost.publish(ctx.song);
        }
        if (ImGui::MenuItem("Open Project...", sc(ActionId::OpenProject).c_str())) {
            dialogs::showOpenSong(ctx.window, &ctx.dlgOpen, &ctx.scheduler);
        }
        if (ImGui::MenuItem("Save Project", sc(ActionId::Save).c_str())) {
            if (!ctx.currentFilePath.empty()) {
                saveSongJson(ctx.song, ctx.currentFilePath);
            } else {
                dialogs::showSaveSong(ctx.window, &ctx.dlgSave, &ctx.scheduler);
            }
        }
        if (ImGui::MenuItem("Save Project As...", sc(ActionId::SaveAs).c_str())) {
            dialogs::showSaveSong(ctx.window, &ctx.dlgSave, &ctx.scheduler);
        }
        if (ImGui::MenuItem("Delete Current Project...", nullptr, false, !ctx.currentFilePath.empty())) {
            s_pendingDeletePath = ctx.currentFilePath;
            ImGui::OpenPopup("Delete Project?");
        }
        ImGui::Separator();
        // Firmware File page: Export Song / Song Stems / Pattern / Pattern Stems / Samples.
        if (ImGui::BeginMenu("Export")) {
            if (ImGui::MenuItem("Export Song (WAV)...")) dialogs::showExportWav(ctx.window, &ctx.dlgWav, &ctx.scheduler);
            if (ImGui::MenuItem("Export Song Stems (WAV)...")) dialogs::showExportFolder(ctx.window, &ctx.dlgStems, &ctx.scheduler);
            if (ImGui::MenuItem("Render & Export Pattern (WAV)...")) dialogs::showExportWav(ctx.window, &ctx.dlgPatternWav, &ctx.scheduler);
            if (ImGui::MenuItem("Render & Export Pattern Stems...")) dialogs::showExportFolder(ctx.window, &ctx.dlgPatternStems, &ctx.scheduler);
            if (ImGui::MenuItem("Export Tracker Project (.mtp)...")) dialogs::showExportFolder(ctx.window, &ctx.dlgPolyExport, &ctx.scheduler);
            ImGui::Separator();
            if (ImGui::MenuItem("Export Deck...", sc(ActionId::ExportDeck).c_str())) {
                ctx.showExportDeck = true;
                ctx.scheduler.requestRedraw();
            }
            ImGui::EndMenu();
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Quit", "Cmd+Q / Esc")) ctx.running = false;
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Edit")) {
        auto sc = [&](ActionId a) { return ctx.shortcuts.getShortcutText(a); };
        if (ImGui::MenuItem("Undo", sc(ActionId::Undo).c_str(), false, ctx.history.canUndo())) {
            ctx.history.undo(ctx.song);
            ctx.engineHost.publish(ctx.song);
        }
        if (ImGui::MenuItem("Redo", sc(ActionId::Redo).c_str(), false, ctx.history.canRedo())) {
            ctx.history.redo(ctx.song);
            ctx.engineHost.publish(ctx.song);
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Cut", sc(ActionId::Cut).c_str(), false, ctx.recordMode && ctx.editCut)) ctx.editCut();
        if (ImGui::MenuItem("Copy", sc(ActionId::Copy).c_str(), false, (bool)ctx.editCopy)) ctx.editCopy();
        if (ImGui::MenuItem("Paste", sc(ActionId::Paste).c_str(), false, ctx.recordMode && ctx.canPaste && ctx.editPaste)) ctx.editPaste();
        ImGui::Separator();
        if (ImGui::MenuItem("Fill Generator...", sc(ActionId::Fill).c_str())) {
            ctx.showFillModal = true;
            ctx.scheduler.requestRedraw();
        }
        ImGui::Separator();
        if (ImGui::BeginMenu("Desktop")) {
            if (ImGui::MenuItem("Add Track", nullptr, false, ctx.song.tracks.size() < 64)) {
                ctx.history.execute(std::make_unique<AddTrackCommand>(), ctx.song);
                ctx.engineHost.publish(ctx.song);
                ctx.scheduler.requestRedraw();
            }
            if (ImGui::MenuItem("Remove Track", nullptr, false, ctx.song.tracks.size() > 1)) {
                ctx.history.execute(std::make_unique<RemoveTrackCommand>(), ctx.song);
                ctx.engineHost.publish(ctx.song);
                ctx.scheduler.requestRedraw();
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("View")) {
        ImGui::MenuItem("Button Deck", "F9", &ctx.showButtonDeck);
        ImGui::MenuItem("Desktop Wide Mode", "F10", &ctx.wideMode);
        ImGui::Separator();
        ImGui::MenuItem("Kit Gallery", "F11", &ctx.showKitGallery);
        ImGui::Separator();
        if (ImGui::BeginMenu("Theme")) {
            if (ImGui::MenuItem("Renoise Dark")) { ctx.theme = makeRenoiseDarkTheme(); applyTheme(ctx.theme, ImGui::GetStyle()); ctx.scheduler.requestRedraw(6); }
            if (ImGui::MenuItem("Tracker Classic")) { ctx.theme = makeTrackerClassicTheme(); applyTheme(ctx.theme, ImGui::GetStyle()); ctx.scheduler.requestRedraw(6); }
            if (ImGui::MenuItem("High Contrast / Accessible")) { ctx.theme = makeColorblindTheme(); applyTheme(ctx.theme, ImGui::GetStyle()); ctx.scheduler.requestRedraw(6); }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("UI Scale")) {
            if (ImGui::MenuItem("100%")) io.FontGlobalScale = 1.0f;
            if (ImGui::MenuItem("125%")) io.FontGlobalScale = 1.25f;
            if (ImGui::MenuItem("150%")) io.FontGlobalScale = 1.5f;
            if (ImGui::MenuItem("200%")) io.FontGlobalScale = 2.0f;
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("MIDI Input")) {
            auto ports = MidiManager::instance().getPortNames();
            int curPort = MidiManager::instance().getCurrentPort();
            if (ports.empty()) ImGui::MenuItem("No Devices", nullptr, false, false);
            for (size_t p = 0; p < ports.size(); ++p) {
                if (ImGui::MenuItem(ports[p].c_str(), nullptr, (static_cast<int>(p) == curPort))) {
                    MidiManager::instance().openPort(static_cast<int>(p));
                }
            }
            ImGui::EndMenu();
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Reset Layout", "Ctrl+Shift+R")) { ctx.resetLayoutRequested = true; }
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Help")) {
        if (ImGui::MenuItem("Keyboard Shortcuts")) { ctx.showShortcutsModal = true; }
        ImGui::EndMenu();
    }

    if (ImGui::BeginPopupModal("Delete Project?", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Delete this project file from disk?");
        ImGui::TextUnformatted(s_pendingDeletePath.c_str());
        ImGui::Spacing();
        if (ImGui::Button("Delete", ImVec2(120, 0))) {
            std::error_code ec;
            std::filesystem::remove(s_pendingDeletePath, ec);
            if (!ec && ctx.currentFilePath == s_pendingDeletePath) ctx.currentFilePath.clear();
            s_pendingDeletePath.clear();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            s_pendingDeletePath.clear();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::EndMainMenuBar();
    dialogs::renderDialogs();
}

} // namespace trakr


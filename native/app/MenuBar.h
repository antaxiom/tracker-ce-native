#pragma once

#include <functional>
#include <string>
#include <SDL3/SDL.h>

namespace trakr {

class Song;
class History;
class EngineHost;
class ShortcutMap;
class RenderScheduler;
struct Theme;

struct MenuBarContext {
    SDL_Window* window{nullptr};
    Song& song;
    History& history;
    EngineHost& engineHost;
    ShortcutMap& shortcuts;
    RenderScheduler& scheduler;
    Theme& theme;
    bool& running;
    bool& showTransport;
    bool& showPattern;
    bool& showInstruments;
    bool& showSample;
    bool& showMixer;
    bool& showOrderList;
    bool& showKitGallery;
    bool& showFillModal;
    bool& showShortcutsModal;
    bool& showExportDeck;
    bool& resetLayoutRequested;
    bool& showButtonDeck;
    bool& wideMode;
    bool& recordMode;
    bool canPaste{false};
    std::string& currentFilePath;
    std::string& dlgOpen;
    std::string& dlgSave;
    std::string& dlgWav;
    std::string& dlgStems;
    std::string& dlgPolyExport;
    std::string& dlgPatternWav;
    std::string& dlgPatternStems;
    std::function<void()> editCut;
    std::function<void()> editCopy;
    std::function<void()> editPaste;
    std::function<void()> resetCursor;
};

void renderMainMenuBar(MenuBarContext& ctx);

} // namespace trakr


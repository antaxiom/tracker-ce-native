#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_dialog.h>
#include <string>
#include <functional>

namespace trakr {

class RenderScheduler;

namespace dialogs {

void showOpenSong(SDL_Window* window, std::string* targetString, RenderScheduler* scheduler);
void showSaveSong(SDL_Window* window, std::string* targetString, RenderScheduler* scheduler);
void showExportWav(SDL_Window* window, std::string* targetString, RenderScheduler* scheduler);
void showExportFolder(SDL_Window* window, std::string* targetString, RenderScheduler* scheduler);

// In-app (ImGui) dialogs: rendered once per frame while open. Native SDL file
// dialogs post SDL_EVENT_QUIT for non-bundled macOS apps, so they are not used.
bool isOpen();
void renderDialogs();

} // namespace dialogs
} // namespace trakr


#pragma once

// Song screen — semantic UI port.
//
// Firmware reference: core/interface/songEditor/gSongEdit.cpp
//   initDisplayControls(): cFrame (3 places, startPlace 2), cBar tempo (col 7),
//   cSongPlayer over cols 0..6. The 8 bottom cLabels and cBgLabel belong to the
//   app chrome (ScreenButtons/ButtonDeck) and are not re-rendered here.
//   cSongPlayer: core/libs/NewDisplay/songPlayer.cpp.

#include "ui/controls/primitives.h"
#include "ui/controls/song_player.h"
#include "ui/display.h"
#include "ui/layout.h"
#include "imgui.h"

namespace trakr {

class Song;
class History;
class EngineHost;

class SongScreen {
public:
    SongScreen();

    void update(Song& song,
                History& history,
                EngineHost& engineHost,
                int& currentOrderIdx,
                const ImRect& workspace,
                ui::LayoutProfile profile = ui::LayoutProfile::Classic);

    void render(ImDrawList* drawList,
                const ImRect& workspace,
                ui::LayoutProfile profile = ui::LayoutProfile::Classic) const;

private:
    void build();
    void syncFromModel(const Song& song, EngineHost& engineHost, int currentOrderIdx);

    ui::Display display_;
    ui::FrameControl* frame_{nullptr};
    ui::BarControl* tempoBar_{nullptr};
    SongPlayerControl* player_{nullptr};
    Song* song_{nullptr};
    History* history_{nullptr};
    EngineHost* engineHost_{nullptr};
    int* currentOrderIdx_{nullptr};
    bool built_{false};
};

} // namespace trakr

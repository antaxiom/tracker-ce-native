#pragma once

// File screen (project browser) — semantic UI port.
//
// Firmware reference: core/interface/projectEditor/gProjEdit.cpp
//   initDisplayControls(): cFrame (2 places), cLabel[8] + cBgLabel (app chrome),
//   browser/recent labels, file/recent cLists, coverImg, blankSpaceCover.
//   showProjectBrowser(): file list + recent list + labels; cover hidden.

#include "ui/controls/primitives.h"
#include "ui/display.h"
#include "ui/layout.h"
#include "imgui.h"

namespace trakr {

class Song;
class EngineHost;
class FileView;

class FileScreen {
public:
    FileScreen();

    void update(Song& song,
                EngineHost& engineHost,
                FileView& view,
                const ImRect& workspace,
                ui::LayoutProfile profile = ui::LayoutProfile::Classic);

    void render(ImDrawList* drawList,
                const ImRect& workspace,
                ui::LayoutProfile profile = ui::LayoutProfile::Classic) const;

private:
    void build();
    void syncFromModel(const FileView& view);

    ui::Display display_;
    ui::FrameControl* frame_{nullptr};
    ui::LabelControl* browserLabel_{nullptr};
    ui::LabelControl* recentLabel_{nullptr};
    ui::ListControl* fileList_{nullptr};
    ui::ListControl* recentList_{nullptr};
    ui::BlankCoverControl* blankCover_{nullptr};
    FileView* view_{nullptr};
    bool built_{false};
};

} // namespace trakr

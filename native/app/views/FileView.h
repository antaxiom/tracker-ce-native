#pragma once

// File screen structural port.
// Firmware reference: core/interface/projectEditor/gProjEdit.cpp
//   initDisplayControls() and showProjectBrowser().
// Controls (creation order): frameControl, label[8], bgLabel, browserLabelControl,
// recentLabelControl, fileListControl, recentListControl, coverImg, blankSpaceCover.
// The app File page is the project browser, so it draws the open-project stage:
// file list + recent list + browser labels + cover background + selection frame.
// Bottom buttons stay in the app ButtonDeck layer.

#include "View.h"
#include "Song.h"
#include "History.h"
#include "EngineHost.h"
#include "../hw/ScreenButtons.h"
#include <vector>
#include <string>
#include <functional>

namespace trakr {

enum class Screen;

class FileView : public View {
public:
    FileView();
    const char* name() const override { return "File"; }
    void draw(bool* p_open = nullptr) override;

    void update(Song& song, EngineHost& engineHost);
    std::vector<hw::ScreenButton> getScreenButtons(Song& song, History& history,
                                                   EngineHost& engineHost,
                                                   Screen& activeScreen,
                                                   bool& showExportDeck,
                                                   bool firmwareParity = false);

    void setParityState();
    void setActions(std::function<void()> onOpen, std::function<void()> onSave,
                    std::function<void()> onSaveAs);
    const std::string& projectName() const { return projectName_; }
    void setProjectName(const std::string& name) { projectName_ = name; }
    int selectedProject() const { return selectedProject_; }
    void setSelectedProject(int idx) { selectedProject_ = idx; }

    // Semantic UI port (FileScreen) hooks.
    const std::vector<std::string>& projects() const { return projects_; }
    int listStart() const { return listStart_; }
    void moveSelection(int delta);
    void selectProject(int index);
    void openSelected() { if (onOpen_) onOpen_(); }

private:
    std::vector<std::string> projects_;
    int selectedProject_{0};
    int listStart_{0}; // cList::list->start (projectList.start)
    std::string projectName_{"New Project"};
    std::function<void()> onOpen_;
    std::function<void()> onSave_;
    std::function<void()> onSaveAs_;
};

} // namespace trakr

#pragma once

// Polyend Tracker Config Page Parity
// Firmware reference: core/interface/configEditor/configMenu.cpp, gConfigEdit.cpp
// Manual reference: tracker-manual-1.7.0.txt Section 3.7 & 13 (lines 2068-2450)

#include "View.h"
#include "Song.h"
#include "Theme.h"
#include "EngineHost.h"
#include "../hw/ScreenButtons.h"
#include <vector>
#include <string>

namespace trakr {

enum class Screen;

class ConfigView : public View {
public:
    enum class Group { ProjectSettings = 0, General = 1, Midi = 2, Metronome = 3, About = 4 };

    ConfigView();
    const char* name() const override { return "Config"; }
    void draw(bool* p_open = nullptr) override;

    void update(Song& song, EngineHost& engineHost, Theme& theme);
    std::vector<hw::ScreenButton> getScreenButtons(Song& song, EngineHost& engineHost,
                                                   Theme& theme, Screen& activeScreen,
                                                   bool firmwareParity = false);

    void adjustSelectedValue(Song& song, EngineHost& engineHost, Theme& theme, int delta);

    Group activeGroup() const { return activeGroup_; }
    void setActiveGroup(Group g) { activeGroup_ = g; selectedItem_ = 0; selectedPlace_ = 0; }
    int selectedItem() const { return selectedItem_; }
    void setSelectedItem(int i) { selectedItem_ = i; }

    // Semantic UI port (ConfigScreen) hooks.
    struct ConfigRow {
        std::string name;
        std::string value;
    };
    static constexpr int kGroupCount = 5;
    static const char* groupName(int index);
    std::vector<ConfigRow> currentRows(const Song& song) const;
    int selectedPlace() const { return selectedPlace_; }
    void setSelectedPlace(int place) { selectedPlace_ = static_cast<int>(place); }
    void moveSelection(int delta);
    void switchGroup(int delta);
    void selectGroup(int index);
    void selectItem(int index);

private:
    Group activeGroup_{Group::ProjectSettings};
    int selectedItem_{0};
    // Firmware selectedPlace: 0 = basemenu list, 1 = submenu param/value list.
    int selectedPlace_{0};
    bool configListShown_{false}; // cConfigEditor::configListShown
    bool hexDisplay_{true};
};

} // namespace trakr

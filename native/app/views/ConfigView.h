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

    // Config item editor (core/interface/configEditor/gConfigEdit.cpp
    // showConfigList4 / showConfigItemLabel; configMenu.cpp itemEditorOpen:
    // the submenu is hidden, the title + description are shown, and the option
    // list is opened with Apply/Cancel on the bottom labels).
    struct ConfigEditor {
        bool open{false};
        int item{0};
        std::string title;
        std::string description;
        std::vector<std::string> options;
        int selected{0};
    };
    bool editing() const { return editor_.open; }
    const ConfigEditor& editor() const { return editor_; }
    // Opens the editor for the selected submenu item; false for label-only
    // rows (firmware menuTypeItemLabel / About rows).
    bool openEditor(Song& song, EngineHost& engineHost);
    // Opens the editor for a row name in the active group (CLI/tests).
    bool openEditorForName(const std::string& name, Song& song, EngineHost& engineHost);
    void moveEditorSelection(int delta);
    void setEditorSelection(int index);
    void applyEditor(Song& song, EngineHost& engineHost);
    void cancelEditor();
    // Deferred --config-edit request: ConfigScreen opens it on the next sync.
    void requestEditor(std::string itemName) { pendingEditor_ = std::move(itemName); }
    const std::string& pendingEditor() const { return pendingEditor_; }
    void clearPendingEditor() { pendingEditor_.clear(); }

private:
    // Option list, description and current option index for a group row
    // (configMenu.cpp menu item setups + configMenuDefs.h option tables).
    struct ItemOptions {
        std::vector<std::string> options;
        std::string description;
        int value{0};
        bool editable{false};
    };
    ItemOptions optionsFor(int group, int item, const Song& song) const;
    void setItemValue(int group, int item, int value, Song& song, EngineHost& engineHost);
    std::vector<ConfigRow> rowsForGroup(int group, const Song& song) const;

    Group activeGroup_{Group::ProjectSettings};
    int selectedItem_{0};
    // Firmware selectedPlace: 0 = basemenu list, 1 = submenu param/value list.
    int selectedPlace_{0};
    bool configListShown_{false}; // cConfigEditor::configListShown
    ConfigEditor editor_;
    std::string pendingEditor_;
    // Firmware config values (mtConfig/mtProject) mirrored as option indices.
    int perfSource_{0};      // performanceSourceTexts
    int notesScale_{0};      // scaleNames
    int rootNote_{0};        // allNotes[12 + index]
    int notesLayout_{0};     // padsLayoutTexts
    int antialiasing_{1};    // onOffStateTexts
    int limiterMode_{0};     // limiterModeTexts
    int snapToZero_{1};      // onOffStateTexts
    int patternTopInfo_{1};  // patternInfoTexts
    int recOptions_{3};      // recOptionsTexts
    int numberingMode_{2};   // numberingModeTexts; 2 = Hexadecimal
    int clockComp_{6};       // slaveClockComp; 6 = "0"
    int metroState_{0};      // onOffStateTexts
    int metroPreRoll_{0};    // onOffStateTexts
    int metroNum_{3};        // metroTimeSig
    int metroDenum_{3};      // metroTimeSig
    int metroVolume_{50};    // 0..100
    int clockIn_{0};         // clockIn
    int clockOut_{0};        // midiOut
    int transportIn_{0};     // clockIn
    int transportOut_{0};    // midiOut
    int notesChannel_{0};    // notesChannel; 0 = ALL
    int middleC_{4};         // octave reported as "C-4" in the General list
};

} // namespace trakr

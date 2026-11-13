#pragma once

// Config screen — semantic UI port.
//
// Firmware reference: core/interface/configEditor/gConfigEdit.cpp
//   initDisplayControls(): popoutWindowLabel, cFrame (4 places), cLabel[8] +
//   cBgLabel (app chrome), basemenu cList, submenu cParamValueList,
//   configList (item editor), configLabel, credits/description cTextBox,
//   blankSpaceCover[0..1].
//   paramValueList: core/libs/NewDisplay/paramValueList.cpp.

#include "ui/controls/param_value_list.h"
#include "ui/controls/primitives.h"
#include "ui/display.h"
#include "ui/layout.h"
#include "imgui.h"

namespace trakr {

class Song;
class EngineHost;
class Theme;
class ConfigView;

class ConfigScreen {
public:
    ConfigScreen();

    void update(Song& song,
                EngineHost& engineHost,
                Theme& theme,
                ConfigView& view,
                const ImRect& workspace,
                ui::LayoutProfile profile = ui::LayoutProfile::Classic);

    void render(ImDrawList* drawList,
                const ImRect& workspace,
                ui::LayoutProfile profile = ui::LayoutProfile::Classic) const;

    // True while a control on this screen samples the animation clock
    // (ui/animation.h); AppLayout then keeps the 60 Hz loop running.
    bool animatingAt(double nowMs) const { return display_.animatingAt(nowMs); }

private:
    void build();
    void syncFromModel(const Song& song, const ConfigView& view);

    ui::Display display_;
    ui::FrameControl* frame_{nullptr};
    ui::ListControl* baseList_{nullptr};
    ui::ParamValueListControl* submenuList_{nullptr};
    ui::BlankCoverControl* blankCoverA_{nullptr};
    ui::BlankCoverControl* blankCoverB_{nullptr};
    ConfigView* view_{nullptr};
    bool built_{false};

};

} // namespace trakr

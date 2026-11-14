#pragma once

// Polyend Tracker Screen Buttons Parity (8 contextual bottom buttons)
// Firmware reference: core/interface/patternEditor/gPatternEdit.cpp lines 95-135
// Manual reference: tracker-manual-1.7.0.txt line 716 (Screen Buttons)

#include "imgui.h"
#include "imgui_internal.h"
#include <string>
#include <vector>
#include <functional>

namespace trakr::hw {

enum class UnderlineColor {
    None,
    Green,   // Available / Navigational (#30CB3F)
    Red,     // Edit / Active / Destructive (#CD1818)
    Orange,  // Selected Track / Channel (#D67022)
    Dimmed   // Inactive (#2A2A2A)
};

struct ScreenButton {
    std::string label;       // Primary label (dim text on top)
    std::string value;       // Primary value (bright text on bottom)
    std::string shiftLabel;  // Displayed when Shift is held
    std::string shiftValue;  // Displayed when Shift is held
    UnderlineColor underline{UnderlineColor::None};
    bool enabled{true};
    bool active{false};      // Selected / latched state
    std::function<void()> onClick;
    // Firmware label colours: grey_33 text while the underline is still drawn
    // (e.g. gSongEdit Add/Delete labels for the current slot).
    bool dimText{false};
};

class ScreenButtons {
public:
    ScreenButtons();

    // Set buttons for the active page (always 8 items)
    void setButtons(const std::vector<ScreenButton>& buttons);

    // Firmware-faithful bottom bar: bgLabel background/separators, Regular14
    // grey names + Light17 white values, no underlines and no button frame.
    void setFirmwareMode(bool on) { firmwareMode_ = on; }

    // Pattern screen (gPatternEdit.cpp): 1px label underline at y=474 and the
    // selection frame at the framesPlaces[0] geometry (y=423..478).
    void setPatternChrome(bool on) { patternChrome_ = on; }

    // cBgLabel (bgLabel.cpp): buttons are separated by a vertical grey line at
    // (width/8)*i only where the bgLabel `value` bit i is set (i in 1..7). The
    // firmware calls setValue(bgLabel, ...) per screen (config=196, sample=127,
    // song=191, others 255). Mirror that mask so separators appear only where
    // the firmware draws them.
    void setSeparatorMask(int mask) { separatorMask_ = mask; }
    int separatorMask() const { return separatorMask_; }

    // Render the 8 buttons inside the given screen rect
    // Returns index of button clicked (0..7) or -1
    int render(const ImRect& rect, bool shiftHeld, float scale = 1.0f);

    // Handle F1..F8 keyboard events (invoking the corresponding button's onClick)
    // Returns true if a key was handled
    bool handleKeyShortcuts(bool shiftHeld);

private:
    int renderFirmware(const ImRect& rect, bool shiftHeld, float scale);

    std::vector<ScreenButton> buttons_;
    bool firmwareMode_{false};
    bool patternChrome_{false};
    int separatorMask_{255};
};

} // namespace trakr::hw

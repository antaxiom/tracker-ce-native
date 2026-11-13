#pragma once

#include "imgui.h"
#include <string>

namespace trakr {

struct Theme {
    // Backgrounds
    ImVec4 bg0{0.075f, 0.082f, 0.094f, 1.0f}; // Main window bg (#131518)
    ImVec4 bg1{0.102f, 0.114f, 0.133f, 1.0f}; // Panel / child bg (#1a1d22)
    ImVec4 bg2{0.141f, 0.157f, 0.188f, 1.0f}; // Control bg (#242830)
    ImVec4 bg3{0.184f, 0.208f, 0.251f, 1.0f}; // Hover / active control (#2f3540)
    ImVec4 border{0.200f, 0.227f, 0.267f, 1.0f}; // Subtle borders (#333a44)

    // Text & Accents
    ImVec4 text{0.839f, 0.867f, 0.894f, 1.0f};    // Bright text (#d6dde4)
    ImVec4 textDim{0.451f, 0.498f, 0.553f, 1.0f}; // Dim text / labels (#737f8d)
    ImVec4 accentAmber{0.910f, 0.600f, 0.133f, 1.0f}; // Primary amber (#e89922)
    ImVec4 accentCyan{0.133f, 0.733f, 0.933f, 1.0f};  // Secondary cyan (#22bbee)
    ImVec4 selection{0.145f, 0.267f, 0.376f, 0.8f};   // Selection box (#254460)
    ImVec4 cursor{1.000f, 0.800f, 0.200f, 1.0f};      // Tracker cursor (#ffcc33)

    // Tracker Grid Column Hues
    ImVec4 colNote{0.400f, 0.867f, 1.000f, 1.0f};       // Cyan
    ImVec4 colInstrument{1.000f, 0.667f, 0.267f, 1.0f}; // Amber
    ImVec4 colVolume{0.333f, 0.867f, 0.533f, 1.0f};     // Green
    ImVec4 colFxType{0.867f, 0.467f, 0.933f, 1.0f};     // Magenta
    ImVec4 colFxValue{0.933f, 0.867f, 0.333f, 1.0f};    // Yellow

    // Tracker Grid Dim / Empty Dash Colors (Themes.cpp patternTrackerColors)
    ImVec4 colNoteDim{0.00f, 0.36f, 0.31f, 1.0f};       // Dim teal Note empty dash
    ImVec4 colInstrDim{0.36f, 0.32f, 0.13f, 1.0f};      // Dim amber Instr empty dash
    ImVec4 colVolDim{0.10f, 0.25f, 0.12f, 1.0f};        // Volume empty dash
    ImVec4 colFx1Dim{0.29f, 0.14f, 0.35f, 1.0f};        // Dim magenta FX1 empty dash
    ImVec4 colFx2Dim{0.13f, 0.30f, 0.34f, 1.0f};        // Dim sky cyan FX2 empty dash
    ImVec4 colPlayheadLine{0.00f, 0.00f, 0.00f, 1.0f};  // 0x000000 Playhead bg
    ImVec4 colStepNum{0.34f, 0.34f, 0.34f, 1.0f};       // 0x575757 Step number
    ImVec4 colTrackInactive{0.09f, 0.09f, 0.09f, 1.0f}; // 0x171717 Inactive track
    ImVec4 colPatternWrap{1.00f, 0.97f, 0.29f, 1.0f};   // 0xFEF749 Pattern end line
    ImVec4 colModePlay{0.19f, 0.80f, 0.25f, 1.0f};      // 0x30CB3F (green navigate box)
    ImVec4 colModeRec{0.80f, 0.09f, 0.09f, 1.0f};       // 0xCD1818 (red edit box)

    // Song screen palette (Themes.cpp: songPlayerColors[10]) used by cSongPlayer.
    // 0 text, 1 active block, 2 scroll thumb, 3 background, 4 muted text,
    // 5 selection/edit frame, 6 scroll track, 7 muted block, 8 empty block,
    // 9 active list frame.
    ImVec4 songPlayerColors[10] = {
        {1.000f, 1.000f, 1.000f, 1.0f}, // 0xFFFFFF white
        {1.000f, 1.000f, 1.000f, 1.0f}, // 0xFFFFFF white (active track block)
        {0.341f, 0.341f, 0.341f, 1.0f}, // 0x575757 grey_57
        {0.039f, 0.039f, 0.039f, 1.0f}, // 0x0A0A0A grey_0A
        {0.200f, 0.200f, 0.200f, 1.0f}, // 0x333333 grey_33
        {0.804f, 0.094f, 0.094f, 1.0f}, // 0xCD1818 one_true_red
        {0.000f, 0.000f, 0.000f, 1.0f}, // 0x000000 black
        {0.267f, 0.263f, 0.267f, 1.0f}, // 0x444344 grey_44
        {0.071f, 0.071f, 0.071f, 1.0f}, // 0x121212 grey_12
        {0.188f, 0.796f, 0.247f, 1.0f}, // 0x30CB3F one_true_green
    };

    // Song tempo column palette (Themes.cpp: activeBarColors[5]) used by cBar.
    // 0 fill, 1 compare up, 2 compare down, 3 container, 4 background.
    ImVec4 activeBarColors[5] = {
        {1.000f, 1.000f, 1.000f, 1.0f}, // white
        {1.000f, 0.000f, 0.000f, 1.0f}, // red
        {0.000f, 1.000f, 0.000f, 1.0f}, // green
        {0.031f, 0.031f, 0.031f, 1.0f}, // 0x080808 grey_08
        {0.039f, 0.039f, 0.039f, 1.0f}, // 0x0A0A0A grey_0A
    };

    // Song place frame palette (Themes.cpp: frameColors[2]) used by cFrame.
    ImVec4 frameColors[2] = {
        {0.804f, 0.094f, 0.094f, 1.0f}, // 0xCD1818 one_true_red
        {0.247f, 0.247f, 0.247f, 1.0f}, // 0x3F3F3F grey_3F
    };

    // Config screen palette (Themes.cpp). list colours are
    // frameNormal, frameSelect, listScrollBar, listBg, fontNormal,
    // fontSelection, scrollParCont; labels are text, bg, frame, text2.
    ImVec4 activeListColors[7] = {
        {1.000f, 1.000f, 1.000f, 1.0f}, // white
        {1.000f, 1.000f, 1.000f, 1.0f}, // white
        {0.341f, 0.341f, 0.341f, 1.0f}, // grey_57
        {0.039f, 0.039f, 0.039f, 1.0f}, // grey_0A
        {1.000f, 1.000f, 1.000f, 1.0f}, // white
        {1.000f, 1.000f, 1.000f, 1.0f}, // white
        {0.000f, 0.000f, 0.000f, 1.0f}, // black
    };
    ImVec4 activeLabelsColors[4] = {
        {0.467f, 0.467f, 0.467f, 1.0f}, // grey_77
        {0.196f, 0.192f, 0.196f, 1.0f}, // grey_32
        {0.804f, 0.094f, 0.094f, 1.0f}, // one_true_red
        {1.000f, 1.000f, 1.000f, 1.0f}, // white
    };
    ImVec4 configItemEditorTextLabelColors[4] = {
        {1.000f, 1.000f, 1.000f, 1.0f}, // white
        {0.039f, 0.039f, 0.039f, 1.0f}, // grey_0A
        {1.000f, 0.000f, 0.000f, 1.0f}, // red
        {0.000f, 0.000f, 0.000f, 1.0f}, // black
    };
    ImVec4 configConfirmPopUpLabelColors[4] = {
        {1.000f, 1.000f, 1.000f, 1.0f}, // white
        {0.133f, 0.133f, 0.133f, 1.0f}, // grey_22
        {0.804f, 0.094f, 0.094f, 1.0f}, // one_true_red
        {1.000f, 1.000f, 1.000f, 1.0f}, // white
    };
    ImVec4 blankSpaceCoverLabelColors[4] = {
        {1.000f, 1.000f, 1.000f, 1.0f}, // white
        {0.039f, 0.039f, 0.039f, 1.0f}, // grey_0A
        {1.000f, 0.000f, 0.000f, 1.0f}, // red
        {0.000f, 0.000f, 0.000f, 1.0f}, // black
    };
    int bottomScreenShadow{1};
    int barShadow{1};

    // Meter colors
    ImVec4 meterGreen{0.2f, 0.8f, 0.3f, 1.0f};
    ImVec4 meterYellow{0.9f, 0.8f, 0.2f, 1.0f};
    ImVec4 meterRed{0.9f, 0.2f, 0.2f, 1.0f};
    ImVec4 meterClip{1.0f, 0.1f, 0.1f, 1.0f};
};

Theme makeRenoiseDarkTheme();
Theme makeTrackerClassicTheme();
inline Theme makePolyendHardwareTheme() { return makeTrackerClassicTheme(); }
Theme makeColorblindTheme();
void applyTheme(const Theme& theme, ImGuiStyle& style);

bool saveThemeJson(const Theme& theme, const std::string& filepath);
bool loadThemeJson(Theme& outTheme, const std::string& filepath);

} // namespace trakr


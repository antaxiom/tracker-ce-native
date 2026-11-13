#include "Theme.h"
#include <nlohmann/json.hpp>
#include <fstream>

namespace trakr {

Theme makeRenoiseDarkTheme() {
    Theme t;
    t.colNoteDim = ImVec4{0.15f, 0.28f, 0.32f, 1.0f};
    t.colInstrDim = ImVec4{0.32f, 0.26f, 0.15f, 1.0f};
    t.colVolDim = ImVec4{0.15f, 0.30f, 0.20f, 1.0f};
    t.colFx1Dim = ImVec4{0.30f, 0.18f, 0.32f, 1.0f};
    t.colFx2Dim = ImVec4{0.15f, 0.28f, 0.32f, 1.0f};
    return t;
}

Theme makeTrackerClassicTheme() {
    Theme t;
    t.bg0 = ImVec4{0.00f, 0.00f, 0.00f, 1.0f}; // Pure black 0x000000
    t.bg1 = ImVec4{0.03f, 0.03f, 0.03f, 1.0f}; // 0x080808 (grey_08)
    t.bg2 = ImVec4{0.07f, 0.07f, 0.07f, 1.0f}; // 0x121212 (grey_12)
    t.bg3 = ImVec4{0.13f, 0.13f, 0.13f, 1.0f}; // 0x222222 (grey_22)
    t.border = ImVec4{0.20f, 0.19f, 0.20f, 1.0f}; // 0x323132 (grey_32)
    t.text = ImVec4{1.0f, 1.0f, 1.0f, 1.0f};       // 0xFFFFFF (pure white)
    t.textDim = ImVec4{0.47f, 0.47f, 0.47f, 1.0f}; // 0x777777 (grey_77)
    t.accentAmber = ImVec4{0.84f, 0.44f, 0.13f, 1.0f}; // 0xD67022 amber/orange
    t.accentCyan = ImVec4{0.25f, 1.00f, 1.00f, 1.0f};  // 0x3FFFFF cyan
    t.selection = ImVec4{0.80f, 0.09f, 0.09f, 0.40f};  // Red highlight
    t.cursor = ImVec4{0.80f, 0.09f, 0.09f, 1.0f};      // 0xCD1818 one_true_red

    // Polyend Tracker Hardware Grid Palette (Themes.cpp)
    t.colNote = ImVec4{0.00f, 0.91f, 0.75f, 1.0f};       // 0x00E8BE Cyan/Mint
    t.colInstrument = ImVec4{1.00f, 0.97f, 0.29f, 1.0f}; // 0xFEF749 instrYellow
    t.colVolume = ImVec4{0.19f, 0.80f, 0.25f, 1.0f};     // 0x30CB3F one_true_green
    t.colFxType = ImVec4{0.73f, 0.35f, 0.95f, 1.0f};     // 0xBB58F1 Electric Violet
    t.colFxValue = ImVec4{0.34f, 0.95f, 1.00f, 1.0f};    // 0x57F1FF Bright Sky Cyan

    // Neutral Empty Dash Matrix: Light gray on Note, darker gray on params (Parity)
    t.colNoteDim = ImVec4{0.45f, 0.45f, 0.45f, 1.0f};       // Light gray #737373 on empty note (---)
    t.colInstrDim = ImVec4{0.27f, 0.27f, 0.27f, 1.0f};      // Darker gray #454545 on empty instr (--)
    t.colVolDim = ImVec4{0.27f, 0.27f, 0.27f, 1.0f};        // Darker gray #454545 on empty volume
    t.colFx1Dim = ImVec4{0.27f, 0.27f, 0.27f, 1.0f};        // Darker gray #454545 on empty FX1 (----)
    t.colFx2Dim = ImVec4{0.27f, 0.27f, 0.27f, 1.0f};        // Darker gray #454545 on empty FX2 (----)
    t.colPlayheadLine = ImVec4{0.00f, 0.00f, 0.00f, 1.0f};  // 0x000000 Playhead bg
    t.colStepNum = ImVec4{0.34f, 0.34f, 0.34f, 1.0f};       // 0x575757 Step number
    t.colTrackInactive = ImVec4{0.09f, 0.09f, 0.09f, 1.0f}; // 0x171717 Inactive track
    t.colPatternWrap = ImVec4{1.00f, 0.97f, 0.29f, 1.0f};   // 0xFEF749 Pattern end line
    t.colModePlay = ImVec4{0.19f, 0.80f, 0.25f, 1.0f};      // 0x30CB3F (green navigate box)
    t.colModeRec = ImVec4{0.80f, 0.09f, 0.09f, 1.0f};       // 0xCD1818 (red edit box)

    // Song screen controls (Themes.cpp songPlayerColors / activeBarColors / frameColors)
    t.songPlayerColors[0] = ImVec4{1.000f, 1.000f, 1.000f, 1.0f}; // white
    t.songPlayerColors[1] = ImVec4{1.000f, 1.000f, 1.000f, 1.0f}; // white active track
    t.songPlayerColors[2] = ImVec4{0.341f, 0.341f, 0.341f, 1.0f}; // grey_57
    t.songPlayerColors[3] = ImVec4{0.039f, 0.039f, 0.039f, 1.0f}; // grey_0A
    t.songPlayerColors[4] = ImVec4{0.200f, 0.200f, 0.200f, 1.0f}; // grey_33
    t.songPlayerColors[5] = ImVec4{0.804f, 0.094f, 0.094f, 1.0f}; // one_true_red
    t.songPlayerColors[6] = ImVec4{0.000f, 0.000f, 0.000f, 1.0f}; // black
    t.songPlayerColors[7] = ImVec4{0.267f, 0.263f, 0.267f, 1.0f}; // grey_44
    t.songPlayerColors[8] = ImVec4{0.071f, 0.071f, 0.071f, 1.0f}; // grey_12
    t.songPlayerColors[9] = ImVec4{0.188f, 0.796f, 0.247f, 1.0f}; // one_true_green
    t.activeBarColors[0] = ImVec4{1.000f, 1.000f, 1.000f, 1.0f};  // white fill
    t.activeBarColors[1] = ImVec4{1.000f, 0.000f, 0.000f, 1.0f};  // red
    t.activeBarColors[2] = ImVec4{0.000f, 1.000f, 0.000f, 1.0f};  // green
    t.activeBarColors[3] = ImVec4{0.031f, 0.031f, 0.031f, 1.0f};  // grey_08 container
    t.activeBarColors[4] = ImVec4{0.039f, 0.039f, 0.039f, 1.0f};  // grey_0A background
    t.frameColors[0] = ImVec4{0.804f, 0.094f, 0.094f, 1.0f};      // one_true_red
    t.frameColors[1] = ImVec4{0.247f, 0.247f, 0.247f, 1.0f};      // grey_3F

    // Config screen controls (Themes.cpp activeListColors / label palettes).
    t.activeListColors[0] = ImVec4{1.000f, 1.000f, 1.000f, 1.0f}; // white
    t.activeListColors[1] = ImVec4{1.000f, 1.000f, 1.000f, 1.0f}; // white
    t.activeListColors[2] = ImVec4{0.341f, 0.341f, 0.341f, 1.0f}; // grey_57
    t.activeListColors[3] = ImVec4{0.039f, 0.039f, 0.039f, 1.0f}; // grey_0A
    t.activeListColors[4] = ImVec4{1.000f, 1.000f, 1.000f, 1.0f}; // white
    t.activeListColors[5] = ImVec4{1.000f, 1.000f, 1.000f, 1.0f}; // white
    t.activeListColors[6] = ImVec4{0.000f, 0.000f, 0.000f, 1.0f}; // black
    t.activeLabelsColors[0] = ImVec4{0.467f, 0.467f, 0.467f, 1.0f};    // grey_77
    t.activeLabelsColors[1] = ImVec4{0.196f, 0.192f, 0.196f, 1.0f};    // grey_32
    t.activeLabelsColors[2] = ImVec4{0.804f, 0.094f, 0.094f, 1.0f};    // one_true_red
    t.activeLabelsColors[3] = ImVec4{1.000f, 1.000f, 1.000f, 1.0f};    // white
    t.configItemEditorTextLabelColors[0] = ImVec4{1.000f, 1.000f, 1.000f, 1.0f}; // white
    t.configItemEditorTextLabelColors[1] = ImVec4{0.039f, 0.039f, 0.039f, 1.0f}; // grey_0A
    t.configItemEditorTextLabelColors[2] = ImVec4{1.000f, 0.000f, 0.000f, 1.0f}; // red
    t.configItemEditorTextLabelColors[3] = ImVec4{0.000f, 0.000f, 0.000f, 1.0f}; // black
    t.configConfirmPopUpLabelColors[0] = ImVec4{1.000f, 1.000f, 1.000f, 1.0f};   // white
    t.configConfirmPopUpLabelColors[1] = ImVec4{0.133f, 0.133f, 0.133f, 1.0f};   // grey_22
    t.configConfirmPopUpLabelColors[2] = ImVec4{0.804f, 0.094f, 0.094f, 1.0f};   // one_true_red
    t.configConfirmPopUpLabelColors[3] = ImVec4{1.000f, 1.000f, 1.000f, 1.0f};   // white
    t.blankSpaceCoverLabelColors[0] = ImVec4{1.000f, 1.000f, 1.000f, 1.0f};      // white
    t.blankSpaceCoverLabelColors[1] = ImVec4{0.039f, 0.039f, 0.039f, 1.0f};      // grey_0A
    t.blankSpaceCoverLabelColors[2] = ImVec4{1.000f, 0.000f, 0.000f, 1.0f};      // red
    t.blankSpaceCoverLabelColors[3] = ImVec4{0.000f, 0.000f, 0.000f, 1.0f};      // black
    t.bottomScreenShadow = 1;
    t.barShadow = 1;
    return t;
}

Theme makeColorblindTheme() {
    Theme t;
    t.bg0 = ImVec4{0.00f, 0.00f, 0.00f, 1.0f}; // Pitch black
    t.bg1 = ImVec4{0.07f, 0.07f, 0.08f, 1.0f}; // High contrast dark
    t.bg2 = ImVec4{0.15f, 0.15f, 0.17f, 1.0f}; // Crisp controls
    t.bg3 = ImVec4{0.25f, 0.26f, 0.28f, 1.0f}; // High visibility hover
    t.border = ImVec4{0.55f, 0.58f, 0.62f, 1.0f}; // Crisp high-contrast borders
    t.text = ImVec4{1.0f, 1.0f, 1.0f, 1.0f};       // 100% white
    t.textDim = ImVec4{0.72f, 0.75f, 0.80f, 1.0f}; // High visibility dim
    t.accentAmber = ImVec4{1.00f, 0.75f, 0.00f, 1.0f}; // Vivid Gold
    t.accentCyan = ImVec4{0.00f, 0.90f, 1.00f, 1.0f};  // Vivid Cyan
    t.selection = ImVec4{0.00f, 0.55f, 1.00f, 0.60f};  // Bright Blue selection
    t.cursor = ImVec4{1.00f, 0.20f, 0.20f, 1.0f};      // Neon Red

    // Accessible high-contrast hues
    t.colNote = ImVec4{0.00f, 1.00f, 1.00f, 1.0f};       // Pure Cyan
    t.colInstrument = ImVec4{1.00f, 1.00f, 0.00f, 1.0f}; // Pure Yellow
    t.colVolume = ImVec4{0.20f, 1.00f, 0.40f, 1.0f};     // Vivid Lime
    t.colFxType = ImVec4{1.00f, 0.40f, 1.00f, 1.0f};     // Pure Magenta
    t.colFxValue = ImVec4{1.00f, 0.80f, 0.20f, 1.0f};    // Vivid Orange
    t.colNoteDim = ImVec4{0.00f, 0.35f, 0.35f, 1.0f};
    t.colInstrDim = ImVec4{0.35f, 0.35f, 0.00f, 1.0f};
    t.colVolDim = ImVec4{0.10f, 0.35f, 0.15f, 1.0f};
    t.colFx1Dim = ImVec4{0.35f, 0.15f, 0.35f, 1.0f};
    t.colFx2Dim = ImVec4{0.35f, 0.28f, 0.10f, 1.0f};
    t.colModePlay = ImVec4{0.20f, 1.00f, 0.40f, 1.0f};
    t.colModeRec = ImVec4{1.00f, 0.20f, 0.20f, 1.0f};
    return t;
}

void applyTheme(const Theme& theme, ImGuiStyle& style) {
    // Sharp, flat, zero-rounding Renoise geometry
    style.WindowRounding = 0.0f;
    style.ChildRounding = 0.0f;
    style.FrameRounding = 0.0f;
    style.PopupRounding = 0.0f;
    style.ScrollbarRounding = 0.0f;
    style.GrabRounding = 0.0f;
    style.TabRounding = 0.0f;

    style.WindowBorderSize = 1.0f;
    style.ChildBorderSize = 1.0f;
    style.PopupBorderSize = 1.0f;
    style.FrameBorderSize = 1.0f;
    style.TabBorderSize = 0.0f;

    style.WindowPadding = ImVec2(6.0f, 6.0f);
    style.FramePadding = ImVec2(5.0f, 2.0f);
    style.ItemSpacing = ImVec2(4.0f, 3.0f);
    style.ItemInnerSpacing = ImVec2(4.0f, 3.0f);
    style.ScrollbarSize = 10.0f;
    style.GrabMinSize = 8.0f;

    // Palette mapping to ImGui colors
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_Text] = theme.text;
    colors[ImGuiCol_TextDisabled] = theme.textDim;
    colors[ImGuiCol_WindowBg] = theme.bg0;
    colors[ImGuiCol_ChildBg] = theme.bg1;
    colors[ImGuiCol_PopupBg] = theme.bg1;
    colors[ImGuiCol_Border] = theme.border;
    colors[ImGuiCol_BorderShadow] = ImVec4(0, 0, 0, 0);

    colors[ImGuiCol_FrameBg] = theme.bg2;
    colors[ImGuiCol_FrameBgHovered] = theme.bg3;
    colors[ImGuiCol_FrameBgActive] = theme.accentAmber;

    colors[ImGuiCol_TitleBg] = theme.bg1;
    colors[ImGuiCol_TitleBgActive] = theme.bg2;
    colors[ImGuiCol_TitleBgCollapsed] = theme.bg0;

    colors[ImGuiCol_MenuBarBg] = theme.bg1;
    colors[ImGuiCol_ScrollbarBg] = theme.bg0;
    colors[ImGuiCol_ScrollbarGrab] = theme.bg3;
    colors[ImGuiCol_ScrollbarGrabHovered] = theme.border;
    colors[ImGuiCol_ScrollbarGrabActive] = theme.accentAmber;

    colors[ImGuiCol_CheckMark] = theme.accentAmber;
    colors[ImGuiCol_SliderGrab] = theme.accentAmber;
    colors[ImGuiCol_SliderGrabActive] = theme.cursor;

    colors[ImGuiCol_Button] = theme.bg2;
    colors[ImGuiCol_ButtonHovered] = theme.bg3;
    colors[ImGuiCol_ButtonActive] = theme.accentAmber;

    colors[ImGuiCol_Header] = theme.bg2;
    colors[ImGuiCol_HeaderHovered] = theme.bg3;
    colors[ImGuiCol_HeaderActive] = theme.selection;

    colors[ImGuiCol_Separator] = theme.border;
    colors[ImGuiCol_SeparatorHovered] = theme.accentAmber;
    colors[ImGuiCol_SeparatorActive] = theme.accentAmber;

    colors[ImGuiCol_ResizeGrip] = ImVec4(0, 0, 0, 0);
    colors[ImGuiCol_ResizeGripHovered] = theme.accentAmber;
    colors[ImGuiCol_ResizeGripActive] = theme.accentAmber;

    colors[ImGuiCol_Tab] = theme.bg1;
    colors[ImGuiCol_TabHovered] = theme.bg3;
    colors[ImGuiCol_TabSelected] = theme.bg2;
    colors[ImGuiCol_TabSelectedOverline] = theme.accentAmber;
    colors[ImGuiCol_TabDimmed] = theme.bg0;
    colors[ImGuiCol_TabDimmedSelected] = theme.bg1;

    colors[ImGuiCol_DockingPreview] = theme.selection;
    colors[ImGuiCol_DockingEmptyBg] = theme.bg0;
}

bool saveThemeJson(const Theme& theme, const std::string& filepath) {
    nlohmann::json j;
    auto to_arr = [](const ImVec4& c) { return std::vector<float>{c.x, c.y, c.z, c.w}; };
    j["bg0"] = to_arr(theme.bg0);
    j["bg1"] = to_arr(theme.bg1);
    j["bg2"] = to_arr(theme.bg2);
    j["bg3"] = to_arr(theme.bg3);
    j["border"] = to_arr(theme.border);
    j["text"] = to_arr(theme.text);
    j["textDim"] = to_arr(theme.textDim);
    j["accentAmber"] = to_arr(theme.accentAmber);
    j["accentCyan"] = to_arr(theme.accentCyan);

    std::ofstream out(filepath);
    if (!out.is_open()) return false;
    out << j.dump(2);
    return true;
}

bool loadThemeJson(Theme& outTheme, const std::string& filepath) {
    std::ifstream in(filepath);
    if (!in.is_open()) return false;
    nlohmann::json j;
    in >> j;
    auto from_arr = [](const nlohmann::json& node, ImVec4& c) {
        if (node.is_array() && node.size() >= 4) {
            c = ImVec4(node[0], node[1], node[2], node[3]);
        }
    };
    if (j.contains("bg0")) from_arr(j["bg0"], outTheme.bg0);
    if (j.contains("bg1")) from_arr(j["bg1"], outTheme.bg1);
    if (j.contains("bg2")) from_arr(j["bg2"], outTheme.bg2);
    if (j.contains("bg3")) from_arr(j["bg3"], outTheme.bg3);
    if (j.contains("border")) from_arr(j["border"], outTheme.border);
    if (j.contains("text")) from_arr(j["text"], outTheme.text);
    if (j.contains("textDim")) from_arr(j["textDim"], outTheme.textDim);
    if (j.contains("accentAmber")) from_arr(j["accentAmber"], outTheme.accentAmber);
    if (j.contains("accentCyan")) from_arr(j["accentCyan"], outTheme.accentCyan);
    return true;
}

} // namespace trakr


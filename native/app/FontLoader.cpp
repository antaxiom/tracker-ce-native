#include "FontLoader.h"
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <vector>

namespace trakr {

static std::string resolveFont(const std::string& rel) {
    const std::vector<std::string> candidates = {
        rel,
        "native/" + rel,
        "../" + rel,
        "../../" + rel
    };
    for (const auto& c : candidates) {
        if (std::filesystem::exists(c)) return c;
    }
    return "";
}

static AppFonts s_loadedFonts;

AppFonts loadFonts(float dpiScale, bool usePolyendFont, bool firmwareMetrics) {
    AppFonts fonts;
    ImGuiIO& io = ImGui::GetIO();

    const char* env1 = std::getenv("TRAKR_USE_POLYEND_FONT");
    const char* env2 = std::getenv("TRAKR_COMPARE_FONT");
    if ((env1 && env1[0] != '\0' && std::strcmp(env1, "0") != 0) ||
        (env2 && env2[0] != '\0' && std::strcmp(env2, "0") != 0)) {
        usePolyendFont = true;
    }

    float baseMonoSize = 17.5f * dpiScale;
    float baseSansSize = 17.5f * dpiScale;
    float largeMonoSize = 22.0f * dpiScale;

    std::string monoPath = resolveFont("assets/fonts/RobotoMono-Regular.ttf");
    if (monoPath.empty()) monoPath = resolveFont("assets/fonts/monospace.ttf");
    std::string sansPath = monoPath.empty() ? resolveFont("assets/fonts/ui_sans.ttf") : monoPath;
    std::string boldPath = resolveFont("assets/fonts/RobotoMono-Bold.ttf");
    std::string lightPath = resolveFont("assets/fonts/RobotoMono-Light.ttf");

    ImFontConfig fontConfig;
    if (usePolyendFont) {
        baseMonoSize = 17.5f * dpiScale;
        baseSansSize = 17.5f * dpiScale;
        largeMonoSize = 22.0f * dpiScale;
        fontConfig.OversampleH = 1;
        fontConfig.OversampleV = 1;
        fontConfig.PixelSnapH = true;

        std::string reg = resolveFont("assets/fonts/RobotoMono-Regular.ttf");
        if (!reg.empty()) {
            monoPath = reg;
            sansPath = reg;
        }
    }

    if (firmwareMetrics) {
        // 1:1 LCD capture: keep the calibrated logical sizes but snap glyph
        // advances to whole pixels so they match the firmware L4 cells
        // (RobotoMonoBold17 = 10px advance, Regular/Light14 = 8px advance).
        fontConfig.OversampleH = 1;
        fontConfig.OversampleV = 1;
        fontConfig.PixelSnapH = true;
    }

    static const ImWchar kGlyphRanges[] = {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x2000, 0x206F, // General Punctuation
        0x2190, 0x21FF, // Arrows
        0x25A0, 0x25FF, // Geometric Shapes (●, ■, ▲, ▼, ◆, ▷, etc.)
        0x2600, 0x26FF, // Miscellaneous Symbols
        0x2700, 0x27BF, // Dingbats
        0,
    };

    const ImFontConfig* cfgPtr = usePolyendFont ? &fontConfig : nullptr;

    if (!sansPath.empty()) {
        fonts.sans = io.Fonts->AddFontFromFileTTF(sansPath.c_str(), baseSansSize, cfgPtr, kGlyphRanges);
    }
    if (!monoPath.empty()) {
        fonts.mono = io.Fonts->AddFontFromFileTTF(monoPath.c_str(), baseMonoSize, cfgPtr, kGlyphRanges);
    }
    if (!lightPath.empty()) {
        fonts.monoLight = io.Fonts->AddFontFromFileTTF(lightPath.c_str(), baseMonoSize, cfgPtr, kGlyphRanges);
    }
    if (!boldPath.empty()) {
        fonts.monoBold = io.Fonts->AddFontFromFileTTF(boldPath.c_str(), largeMonoSize, cfgPtr, kGlyphRanges);
        fonts.monoLarge = fonts.monoBold;
    } else if (!monoPath.empty()) {
        fonts.monoLarge = io.Fonts->AddFontFromFileTTF(monoPath.c_str(), largeMonoSize, cfgPtr, kGlyphRanges);
        fonts.monoBold = fonts.monoLarge;
    }

    // Use ASCII '?' for unknown codepoints: RobotoMono ships U+FFFD as a hollow
    // box, which ImGui would otherwise pick as the automatic fallback glyph.
    for (ImFont* f : {fonts.sans, fonts.mono, fonts.monoLight, fonts.monoBold}) {
        if (f) f->FallbackChar = (ImWchar)'?';
    }

    if (!fonts.sans) fonts.sans = io.Fonts->AddFontDefault();
    if (!fonts.mono) fonts.mono = fonts.sans;
    if (!fonts.monoLight) fonts.monoLight = fonts.mono;
    if (!fonts.monoLarge) fonts.monoLarge = fonts.mono;
    if (!fonts.monoBold) fonts.monoBold = fonts.monoLarge;

    s_loadedFonts = fonts;
    return fonts;
}

ImFont* getMonoFont() {
    return s_loadedFonts.mono ? s_loadedFonts.mono : ImGui::GetFont();
}

ImFont* getMonoBoldFont() {
    return s_loadedFonts.monoBold ? s_loadedFonts.monoBold : ImGui::GetFont();
}

ImFont* getMonoLightFont() {
    return s_loadedFonts.monoLight ? s_loadedFonts.monoLight : ImGui::GetFont();
}

} // namespace trakr

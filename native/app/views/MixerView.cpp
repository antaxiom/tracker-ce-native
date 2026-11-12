#include "MixerView.h"
#include "FontLoader.h"
#include "imgui.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

// Master screen structural port of:
//   core/interface/masterParams/gMasterParams.cpp
//   core/libs/NewDisplay/{bar,label,list,frame,bgLabel}.cpp
//
// Creation order in initDisplayControls(): frameControl, label[8], bgLabel,
// then per track {trackNameLabel[i], trackVolumeBar[i], barControl[i]},
// blankSpaceCover[0..2], delayPingpongEnableList, delaySyncEnableList,
// delaySyncRateList, limiterSidechainList, saturationList. NewDisplay
// composites controls in REVERSE creation order, so the screen bodies below
// draw saturationList -> blankSpaceCover -> barControl[7..0] ->
// trackVolumeBar[7..0] -> trackNameLabel[7..0] -> bgLabel/labels (app
// ButtonDeck chrome) -> frameControl.

namespace trakr {
namespace {

// Firmware geometry (device pixels, 800x480); gMasterParams.cpp.
constexpr int kChromeHeight = 28;
constexpr int kBarY = 29, kBarH = 394;
constexpr int kBarW = 800 / 8 - 3;       // 97
constexpr int kMixerBarY = 64, kMixerBarH = 359;
constexpr int kMixerBarW = 800 / 16 - 3; // 47
constexpr int kListX = (800 / 8) * 5 + 2; // 502
constexpr int kListW = 800 / 8 - 3;       // 97
constexpr int kRedFrameWidth = 24;        // config.h, 1.5px at 1/16

// framesPlaces[8][4] (one single-column place per track).
constexpr int kFramePlaces[8][4] = {
    {1, 31, kBarW, 391},   {101, 31, kBarW, 391}, {201, 31, kBarW, 391},
    {301, 31, kBarW, 391}, {401, 31, kBarW, 391}, {501, 31, kBarW, 391},
    {601, 31, kBarW, 391}, {701, 31, kBarW, 391},
};

// Saturation list rows, gMasterParams.cpp saturationListTexts[5].
const char* const kSaturationNames[5] = {
    "Hard Clip", "Soft Clip", "Saturate", "Overdrive", "Drive+"
};

// core/source/Themes.cpp (themeDefault), mirrored verbatim. Consolidation into
// native/app/Theme is deferred to avoid cross-screen conflicts.
static const ImVec4 kActiveBarColors[5] = {
    {1.000f, 1.000f, 1.000f, 1.0f}, // white
    {1.000f, 0.000f, 0.000f, 1.0f}, // red
    {0.000f, 1.000f, 0.000f, 1.0f}, // green
    {0.031f, 0.031f, 0.031f, 1.0f}, // grey_08 container
    {0.039f, 0.039f, 0.039f, 1.0f}, // grey_0A background
};
static const ImVec4 kInactiveBarColors[5] = {
    {0.133f, 0.133f, 0.133f, 1.0f}, // grey_22
    {1.000f, 0.000f, 0.000f, 1.0f}, // red
    {0.000f, 1.000f, 0.000f, 1.0f}, // green
    {0.031f, 0.031f, 0.031f, 1.0f}, // grey_08
    {0.039f, 0.039f, 0.039f, 1.0f}, // grey_0A
};
// activeLabelsColors / inactiveLabelsColors.
static const ImVec4 kActiveLabelsColors[4] = {
    {0.467f, 0.467f, 0.467f, 1.0f}, // grey_77
    {0.196f, 0.192f, 0.196f, 1.0f}, // grey_32
    {0.804f, 0.094f, 0.094f, 1.0f}, // one_true_red
    {1.000f, 1.000f, 1.000f, 1.0f}, // white
};
static const ImVec4 kInactiveLabelsColors[4] = {
    {0.200f, 0.200f, 0.200f, 1.0f}, // grey_33
    {0.196f, 0.192f, 0.196f, 1.0f}, // grey_32
    {0.804f, 0.094f, 0.094f, 1.0f}, // one_true_red
    {0.341f, 0.341f, 0.341f, 1.0f}, // grey_57
};
// activeListColors: frameNormal, frameSelect, scrollBar, bg, text, sel, cont.
static const ImVec4 kActiveListColors[7] = {
    {1.000f, 1.000f, 1.000f, 1.0f}, // white
    {1.000f, 1.000f, 1.000f, 1.0f}, // white
    {0.341f, 0.341f, 0.341f, 1.0f}, // grey_57
    {0.039f, 0.039f, 0.039f, 1.0f}, // grey_0A
    {1.000f, 1.000f, 1.000f, 1.0f}, // white
    {1.000f, 1.000f, 1.000f, 1.0f}, // white
    {0.000f, 0.000f, 0.000f, 1.0f}, // black
};
static const ImVec4 kBlankSpaceCoverLabelColors[4] = {
    {1.000f, 1.000f, 1.000f, 1.0f}, // white
    {0.039f, 0.039f, 0.039f, 1.0f}, // grey_0A
    {1.000f, 0.000f, 0.000f, 1.0f}, // red
    {0.000f, 0.000f, 0.000f, 1.0f}, // black
};
static const ImVec4 kFrameColors[2] = {
    {0.804f, 0.094f, 0.094f, 1.0f}, // one_true_red
    {0.247f, 0.247f, 0.247f, 1.0f}, // grey_3F
};
static const ImVec4 kTopBarLabelColors[4] = {
    {1.000f, 1.000f, 1.000f, 1.0f}, // white
    {0.031f, 0.031f, 0.031f, 1.0f}, // grey_08 background
    {1.000f, 1.000f, 1.000f, 1.0f}, // white
    {1.000f, 1.000f, 1.000f, 1.0f}, // white
};

ImU32 rgb(const ImVec4& c) { return ImGui::ColorConvertFloat4ToU32(c); }

// One pass of the firmware master controls. Coordinates stay in firmware
// device pixels; px()/py() only map them into the app workspace.
struct MasterControls {
    ImDrawList* dl{};
    ImVec2 origin{};
    float sx{1.0f}, sy{1.0f};
    Song* song{};
    int selected{0};
    int satSelected{0};
    ImFont* light{};
    ImFont* bold{};
    float lightSize{14.0f};
    float boldSize{17.0f};
    int clickedCol{-1};
    int clickedSat{-1};

    // Control state (position/size/value/colors) mirrors the firmware objects.
    struct Bar {
        int x{}, y{}, w{}, h{}, value{};
        const ImVec4* colors{};
        ImU32 fill{};
        bool reverse{false};
        bool visible{false};
    };
    Bar barCtrl[8];
    Bar trackVol[8];

    float px(float x) const { return origin.x + x * sx; }
    float py(float y) const { return origin.y + (y - kChromeHeight) * sy; }
    float stroke() const { return std::max(1.0f, std::round(kRedFrameWidth / 16.0f * sy)); }

    void fill(float x, float y, float w, float h, ImU32 c) const {
        dl->AddRectFilled(ImVec2(px(x), py(y)), ImVec2(px(x + w), py(y + h)), c);
    }
    void outline(float x, float y, float w, float h, ImU32 c, float t) const {
        dl->AddRect(ImVec2(px(x), py(y)), ImVec2(px(x + w), py(y + h)), c, 0.0f, 0, t);
    }
    void shadow(float x, float y, float w, float h, ImU32 bg) const {
        const ImU32 clear = bg & 0x00FFFFFFu;
        dl->AddRectFilledMultiColor(ImVec2(px(x), py(y + h - 10)),
                                    ImVec2(px(x + w), py(y + h + 2)), bg, bg, clear, clear);
    }
    void text(ImFont* f, float size, float x, float yc, ImU32 c, const char* s) const {
        dl->AddText(f, size, ImVec2(px(x), py(yc) - size * 0.5f), c, s);
    }
    void textCenter(ImFont* f, float size, float xc, float yc, ImU32 c, const char* s) const {
        const ImVec2 ts = f->CalcTextSizeA(size, FLT_MAX, 0.0f, s);
        dl->AddText(f, size, ImVec2(px(xc) - ts.x * 0.5f, py(yc) - size * 0.5f), c, s);
    }
    bool hit(const char* id, float x, float y, float w, float h) const {
        ImGui::SetCursorScreenPos(ImVec2(px(x), py(y)));
        return ImGui::InvisibleButton(id, ImVec2(w * sx, h * sy));
    }

    // cBar::update(): Background | Gradient | BottomShadow, Value_0_100.
    void drawBar(const Bar& b) const {
        const int barWidth = b.w - 20;
        const int barX = b.x + (b.w - barWidth) / 2;
        const int barY = b.y + 10;
        const int barHeight = b.h - 25;
        const ImU32 bg = rgb(b.colors[4]);
        fill(b.x, b.y, b.w, b.h, bg);
        shadow(b.x, b.y, b.w, b.h, bg);
        outline(b.x + 10, b.y + 11, b.w - 20, b.h - 25, rgb(b.colors[3]), 1.0f);
        const ImU32 clear = bg & 0x00FFFFFFu;
        dl->AddRectFilledMultiColor(ImVec2(px(b.x + 10), py(b.y + 11)),
                                    ImVec2(px(b.x + 18), py(b.y + b.h - 14)), clear,
                                    rgb(b.colors[3]), rgb(b.colors[3]), clear);
        if (b.value < 0 || b.value > 100) return;
        const int fillY = b.reverse ? (barHeight * b.value) / 100
                                    : barHeight - (barHeight * b.value) / 100;
        const ImU32 c = b.fill ? b.fill : rgb(b.colors[0]);
        if (b.reverse) fill(barX + 1, barY, barWidth - 2, fillY, c);
        else fill(barX + 1, barY + fillY, barWidth - 2, barHeight - fillY, c);
    }

    // cFrame::update(): selected place outline, frameColors[0].
    void frameControl(int placesCount) const {
        if (selected < 0 || selected >= placesCount) return;
        const int* p = kFramePlaces[selected];
        outline(p[0], p[1], p[2], p[3], rgb(kFrameColors[0]), stroke());
    }
    // label[] and cBgLabel are the app ButtonDeck bottom chrome. label[] uses
    // activeLabelsColors / inactiveLabelsColors (showMixerScreen / showMasterScreen).
    void labels() const {
        (void)kActiveLabelsColors;
        (void)kInactiveLabelsColors;
    }
    void bgLabel() const {}
    // cLabel trackNameLabel: Background | CenterX/Y | Font4 (17 Bold).
    void trackNameLabel(int i) const {
        const float cx = 100.0f * i + 50.0f; // (800/8)*i + 800/16
        fill(cx - 48.0f, 32.0f, 96.0f, 26.0f, rgb(kTopBarLabelColors[1]));
        const char* name =
            (i < static_cast<int>(song->tracks.size())) ? song->tracks[i].name.c_str() : "";
        textCenter(bold, boldSize, cx, 45.0f, rgb(kTopBarLabelColors[0]), name);
    }
    // cBar: trackVolumeBar (mixer gain fader).
    void trackVolumeBar(int i) const {
        if (trackVol[i].visible) drawBar(trackVol[i]);
    }
    // cBar: barControl (volume / level / EQ band).
    void barControl(int i) const {
        if (barCtrl[i].visible) drawBar(barCtrl[i]);
    }
    // cLabel blankSpaceCover: Background | BottomShadow, colors[1].
    void blankSpaceCover(float x, float y, float w, float h) const {
        const ImU32 bg = rgb(kBlankSpaceCoverLabelColors[1]);
        fill(x, y, w, h, bg);
        shadow(x, y, w, h, bg);
    }
    // cList: saturationList (5 rows, 27px each).
    void saturationList() const {
        const ImU32 bg = rgb(kActiveListColors[3]);
        fill(kListX, kBarY, kListW, kBarH, bg);
        shadow(kListX, kBarY, kListW, kBarH, bg);
        const int barPos = std::clamp(satSelected, 0, 4);
        outline(kListX + 3, kBarY + 4 + barPos * 27, kListW - 6, 27, rgb(kActiveListColors[0]),
                stroke());
        for (int i = 0; i < 5; ++i) {
            text(light, lightSize, kListX + 13, kBarY + 4 + 13.5f + i * 27,
                 rgb(kActiveListColors[i == barPos ? 5 : 4]), kSaturationNames[i]);
        }
    }
    // cList: Delay/Limiter lists belong to master pages this app does not expose.
    void delayPingpongEnableList() const {}
    void delaySyncEnableList() const {}
    void delaySyncRateList() const {}
    void limiterSidechainList() const {}

    void hitColumns(const char* prefix, int count) {
        for (int i = 0; i < count; ++i) {
            char id[32];
            std::snprintf(id, sizeof(id), "##%s_col_%d", prefix, i);
            if (hit(id, 100.0f * i + 1.0f, kBarY, kBarW, kBarH)) clickedCol = i;
        }
    }
    void hitSaturationRows() {
        for (int i = 0; i < 5; ++i) {
            char id[32];
            std::snprintf(id, sizeof(id), "##master_sat_%d", i);
            if (hit(id, kListX + 3.0f, kBarY + 4.0f + i * 27.0f, kListW - 6.0f, 27.0f))
                clickedSat = i;
        }
    }
    static int gainValue(float g) {
        const float n =
            g <= 0.0f ? 0.75f * (g + 48.0f) / 48.0f : 0.75f + 0.25f * (g / 6.0f);
        return std::clamp(static_cast<int>(std::lround(n * 100.0f)), 0, 100);
    }

    // cMasterParams::showMasterScreen() (Master 1/3).
    void showMasterScreen() {
        int volume = static_cast<int>(std::lround((song->masterGainDb + 51.5f) / 63.5f * 100.0f));
        volume = std::clamp(volume, 0, 100);
        if (std::abs(song->masterGainDb) < 0.005f) volume = 81; // MASTER_VOLUME_DEFAULT
        const int bass = std::clamp(static_cast<int>(std::lround(song->masterBassBoost * 100.0f)),
                                    0, 100);
        const int space8 = std::clamp(static_cast<int>(std::lround(song->masterSpace * 8.0f)), 0, 8);
        satSelected = std::clamp(song->masterSaturationType, 0, 4);
        for (int i = 0; i < 8; ++i) {
            barCtrl[i] = {100 * i + 2, kBarY, kBarW, kBarH, 0, kActiveBarColors,
                          IM_COL32(255, 255, 255, 255), false, i == 0 || i == 6 || i == 7};
        }
        barCtrl[0].value = volume;
        barCtrl[6].value = bass;
        barCtrl[7].value = space8 * 100 / 8; // map<int>(v, 0, 8, 0, 100)
        // Reverse creation order.
        saturationList();
        blankSpaceCover(102, kBarY, 397, kBarH); // cover[0] from showMasterScreen
        for (int i = 7; i >= 0; --i) barControl(i);
        bgLabel();
        labels();
        frameControl(8);
        hitColumns("master", 8);
        hitSaturationRows();
    }

    // cMasterParams::showMixerScreen() (Track Mixer 2/3).
    void showMixerScreen() {
        for (int i = 0; i < 8; ++i) {
            const bool has = i < static_cast<int>(song->tracks.size());
            const bool mute = has && song->tracks[i].mute;
            const int gain = has ? gainValue(song->tracks[i].gainDb) : 0;
            barCtrl[i] = {100 * i + 2 + 800 / 16, kMixerBarY, kMixerBarW, kMixerBarH, 0,
                          kActiveBarColors, IM_COL32(255, 255, 255, 255), false, true};
            trackVol[i] = {100 * i + 2, kMixerBarY, kMixerBarW, kMixerBarH, gain,
                           mute ? kInactiveBarColors : kActiveBarColors,
                           IM_COL32(255, 255, 255, 255), false, true};
        }
        for (int i = 7; i >= 0; --i) {
            barControl(i);
            trackVolumeBar(i);
            trackNameLabel(i);
        }
        bgLabel();
        labels();
        hitColumns("mixer", 8);
    }

    // cMasterParams::showEqualizerScreen() (Master EQ).
    void showEqualizerScreen() {
        for (int i = 0; i < 8; ++i) {
            const int band = std::clamp(i, 0, 4);
            const int value = std::clamp(
                static_cast<int>(std::lround((song->masterEq[band] + 12.0f) / 24.0f * 100.0f)),
                0, 100);
            barCtrl[i] = {100 * i + 2, kBarY, kBarW, kBarH, value, kActiveBarColors,
                          IM_COL32(255, 255, 255, 255), false, i < 5};
        }
        blankSpaceCover(kListX, kBarY, 800 - kListX, kBarH); // cover[0], x = 502
        for (int i = 7; i >= 0; --i) barControl(i);
        bgLabel();
        labels();
        frameControl(5);
        hitColumns("eq", 5);
    }
};

} // namespace

MixerView::MixerView() = default;

void MixerView::draw(bool* p_open) {
    (void)p_open;
}

void MixerView::update(Song& song, History& history, EngineHost& engineHost) {
    (void)history;
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar;

    if (!ImGui::Begin("Mixer", nullptr, flags)) { ImGui::End(); return; }

    const bool isShift = ImGui::GetIO().KeyShift;
    if (!ImGui::IsAnyItemActive()) {
        if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) selectNextTrack(-1);
        if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) selectNextTrack(1);
        if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) adjustSelectedParam(song, engineHost, 1, isShift);
        if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) adjustSelectedParam(song, engineHost, -1, isShift);
        if (ImGui::IsKeyPressed(ImGuiKey_Delete) || ImGui::IsKeyPressed(ImGuiKey_Backspace))
            resetSelected(song, engineHost);
        if (ImGui::IsKeyPressed(ImGuiKey_Tab)) togglePage();
    }

    const ImVec2 avail = ImGui::GetContentRegionAvail();
    ImFont* light = getMonoLightFont();
    ImFont* bold = getMonoBoldFont();
    if (avail.x < 100.0f || avail.y < 100.0f || !light) { ImGui::End(); return; }

    const ImVec2 origin = ImGui::GetCursorScreenPos();
    MasterControls c;
    c.dl = ImGui::GetWindowDrawList();
    c.origin = origin;
    c.sx = avail.x / 800.0f;
    c.sy = avail.y / 397.0f;
    c.song = &song;
    c.light = light;
    c.bold = bold;
    c.lightSize = 14.0f * c.sy; // fonts[0] Light14 (cList default)
    c.boldSize = 17.0f * c.sy;  // controlStyleFont4: 17 Bold
    c.selected = std::clamp(selectedTrack_, 0, subPage_ == SubPage::MasterFx ? 4 : 7);

    // Reverse creation order is encoded inside each screen body.
    if (subPage_ == SubPage::TrackMixer) {
        c.showMixerScreen();
    } else if (subPage_ == SubPage::MasterFx) {
        c.showEqualizerScreen();
    } else {
        c.showMasterScreen();
    }

    if (c.clickedSat >= 0 && c.clickedSat != song.masterSaturationType) {
        song.masterSaturationType = c.clickedSat;
        engineHost.publish(song);
    }
    if (c.clickedCol >= 0) selectedTrack_ = c.clickedCol;
    ImGui::SetCursorScreenPos(origin);
    ImGui::End();
}

} // namespace trakr

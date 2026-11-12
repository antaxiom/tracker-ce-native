#include "OrderListView.h"
#include "Command.h"
#include "Theme.h"
#include "FontLoader.h"
#include "imgui.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <memory>

// Polyend Tracker Song screen.
// Structural port of:
//   core/interface/songEditor/gSongEdit.cpp       (initDisplayControls)
//   core/libs/NewDisplay/songPlayer.cpp           (cSongPlayer)
//   core/libs/NewDisplay/bar.cpp                  (cBar)
//   core/libs/NewDisplay/frame.cpp                (cFrame)
// Control creation order there is: frameControl, barControl, label[8], bgLabel,
// songPlayerControl. NewDisplay composites controls in REVERSE creation order,
// so this body draws cSongPlayer, then cBar, then cFrame. label[]/bgLabel live
// in the app's ScreenButtons/ButtonDeck layer below the workspace.

namespace trakr {
namespace {

// --- Firmware geometry (device pixels, 800x480) -----------------------------
constexpr int kChromeHeight = 28;        // app header; firmware body starts under it
constexpr int kSongX = 1;
constexpr int kSongY = 29;
constexpr int kSongW = (800 / 8) * 7 - 3; // 697
constexpr int kSongH = 394;
constexpr int kTempoX = (800 / 8) * 7 + 1; // 701
constexpr int kTempoY = 29;
constexpr int kTempoW = 800 / 8 - 3;       // 97
constexpr int kTempoH = 394;

// gSongEdit.cpp framesPlaces[3][4]: 0 tracks matrix, 1 list, 2 tempo.
constexpr int kFramePlaces[3][4] = {
    {1, 31, 500 - 3, 391},
    {(800 / 8) * 5 + 1, 31, 800 / 4 - 3, 391},
    {(800 / 8) * 7 + 1, 31, 800 / 8 - 3, 391},
};

// songPlayer.cpp block/list metrics.
constexpr int kTotalBlocksWidth = 486;
constexpr int kBlocksMargin = 5;
constexpr int kBlockSpacing = 4;
constexpr int kBlockHeight = 23;
constexpr int kMaxPatternsVisible = 12;
constexpr int kListWidth = 210;
constexpr int kListRowHeight = 27;
constexpr int kListLines = 12;
constexpr float kMaxTempo = 800.0f;    // Sequencer::MAX_TEMPO
constexpr float kRedFrameWidth = 24.0f; // config.h; 24/16 device px, rounded below

ImU32 rgb(const ImVec4& c) { return ImGui::ColorConvertFloat4ToU32(c); }

const Theme& songTheme() {
    static const Theme theme = makeTrackerClassicTheme();
    return theme;
}

// One pass of the firmware song controls. Coordinates stay in firmware device
// pixels; px()/py() only map them into the app workspace.
struct SongControls {
    ImDrawList* dl{};
    ImVec2 origin{};
    float sx{1.0f};
    float sy{1.0f};
    Song* song{};
    PlayheadPosition playhead{};
    const Theme* theme{};
    int selected{0};
    int listStart{0};
    int allTracks{0};
    int clickedSlot{-1};
    ImFont* font{};
    ImFont* nameFont{};
    float fontSize{17.0f};
    float nameFontSize{12.0f};

    float px(float x) const { return origin.x + x * sx; }
    float py(float y) const { return origin.y + (y - kChromeHeight) * sy; }
    float stroke() const { return std::max(1.0f, std::round(kRedFrameWidth / 16.0f * sy)); }

    void fill(float x, float y, float w, float h, ImU32 c) const {
        dl->AddRectFilled(ImVec2(px(x), py(y)), ImVec2(px(x + w), py(y + h)), c);
    }
    void line(float x1, float y1, float x2, float y2, ImU32 c) const {
        dl->AddLine(ImVec2(px(x1), py(y1)), ImVec2(px(x2), py(y2)), c, stroke());
    }
    void text(float x, float yCenter, ImU32 c, const char* s) const {
        ImVec2 ts = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, s);
        dl->AddText(font, fontSize, ImVec2(px(x), py(yCenter) - ts.y * 0.5f), c, s);
    }
    void textCenter(float x, float yCenter, ImU32 c, const char* s) const {
        ImVec2 ts = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, s);
        dl->AddText(font, fontSize, ImVec2(px(x) - ts.x * 0.5f, py(yCenter) - ts.y * 0.5f), c, s);
    }
    int patternAt(int slot) const {
        return (slot >= 0 && slot < static_cast<int>(song->order.size())) ? song->order[slot] : -1;
    }
    const Pattern* patternFor(int patIdx) const {
        return (patIdx >= 0 && patIdx < static_cast<int>(song->patterns.size()))
                 ? &song->patterns[patIdx] : nullptr;
    }
    int blockWidth() const {
        const int n = std::max(1, allTracks);
        return ((kTotalBlocksWidth - kBlocksMargin * 2) - ((n - 1) * kBlockSpacing)) / n;
    }

    // cSongPlayer::update()
    void update() {
        const ImU32 bg = rgb(theme->songPlayerColors[3]);
        fill(kSongX, kSongY, kSongW, kSongH, bg);
        if (theme->songPlayerColors[3].w > 0.0f) { // controlStyleBottomShadow
            const ImU32 clear = bg & 0x00FFFFFFu;
            dl->AddRectFilledMultiColor(
                ImVec2(px(kSongX), py(kSongY + kSongH - 10)),
                ImVec2(px(kSongX + kSongW), py(kSongY + kSongH + 2)),
                bg, bg, clear, clear);
        }
        textCenter(600, kSongY + 30, rgb(theme->songPlayerColors[0]), "Pattern");
        showList();
    }

    // cSongPlayer::showList()
    void showList() {
        const int length = static_cast<int>(song->order.size());
        const int posX = (kSongX + kSongW) - kListWidth; // 488
        const int posY = kSongY + 62;
        const int width = kListWidth;
        const int height = kSongH - 30;
        const int hRow = kListRowHeight;
        int wBar = width - 6;
        if (length > kListLines) wBar = width - 13;
        int barPos = std::clamp(selected - listStart, 0, kListLines - 1);
        const int lines = std::clamp(length - listStart, 0, kListLines);

        if (barPos < lines) {
            // Frame color: isListActive -> colors[9], edit -> colors[5],
            // otherwise colors[1]. The app song list is always the active place.
            const ImU32 frame = rgb(theme->songPlayerColors[9]);
            dl->AddRect(ImVec2(px(posX + 3), py(posY + barPos * hRow)),
                        ImVec2(px(posX + 3 + wBar), py(posY + (barPos + 1) * hRow)),
                        frame, 0.0f, 0, stroke());
        }

        dl->PushClipRect(ImVec2(px(posX), py(posY)),
                         ImVec2(px(posX + wBar), py(posY + hRow * kListLines)), true);
        for (int i = 0; i < lines; ++i) {
            const int slot = listStart + i;
            const int pat = patternAt(slot);
            const float rowCenter = static_cast<float>(posY + hRow / 2 + i * hRow);
            char num[16] = "0";
            if (pat >= 0) std::snprintf(num, sizeof(num), "%d", pat + 1);
            // cSongPlayer::showList only draws list->names when set; this song
            // has no pattern names, so the reference shows bare slot numbers.
            text(posX + 7 + 25, rowCenter, rgb(theme->songPlayerColors[0]), num);
            ImGui::SetCursorScreenPos(ImVec2(px(posX), py(posY + i * hRow)));
            char id[32];
            std::snprintf(id, sizeof(id), "##song_slot_%d", slot);
            if (ImGui::InvisibleButton(id, ImVec2(width * sx, hRow * sy))) clickedSlot = slot;
        }
        dl->PopClipRect();
        if (length > kListLines) {
            const int innerH = height - 46;
            const int yBase = length + kListLines - lines;
            const int yLength = innerH * std::min(kListLines, length) / std::max(1, yBase);
            const int thumbY = posY + 6 + (listStart * (innerH - yLength)) / std::max(1, length);
            fill(posX + width - 6, posY + 4, 3, innerH, rgb(theme->songPlayerColors[6]));
            line(posX + width - 5, thumbY, posX + width - 5, thumbY + yLength,
                 rgb(theme->songPlayerColors[2]));
            line(posX + width - 4, thumbY, posX + width - 4, thumbY + yLength,
                 rgb(theme->songPlayerColors[2]));
        }
    }

    // label.cpp controlStyleVerticalText: 270-degree text reading upwards.
    void verticalText(float centerX, float bottomY, ImU32 c, const char* s) const {
        if (!nameFont || !s) return;
        const float step = 8.0f * sy; // fonts[4] RobotoMonoRegular12 advance + 1
        const float xOffset = nameFontSize * 0.5f + 0.5f * sy;
        float curY = bottomY;
        for (const char* p = s; *p; ++p) {
            if (*p != ' ') {
                char ch[2] = {*p, '\0'};
                const int v0 = dl->VtxBuffer.Size;
                dl->PushClipRectFullScreen();
                dl->AddText(nameFont, nameFontSize, ImVec2(0.0f, 0.0f), c, ch);
                dl->PopClipRect();
                for (int i = v0; i < dl->VtxBuffer.Size; ++i) {
                    const float rx = dl->VtxBuffer[i].pos.x;
                    const float ry = dl->VtxBuffer[i].pos.y;
                    dl->VtxBuffer[i].pos.x = (centerX - xOffset) + ry;
                    dl->VtxBuffer[i].pos.y = curY - rx;
                }
            }
            curY -= step;
        }
    }

    // cSongPlayer::update2()
    void update2() {
        const int bw = blockWidth();
        const int y = kSongY + 42;
        for (int track = 0; track < allTracks; ++track) {
            const bool muted =
                track < static_cast<int>(song->tracks.size()) && song->tracks[track].mute;
            const ImU32 c = rgb(theme->songPlayerColors[muted ? 4 : 0]);
            const float centerX =
                px(kSongX + kBlocksMargin + (bw + kBlockSpacing) * track + bw / 2);
            const char* name = (track < static_cast<int>(song->tracks.size()))
                                 ? song->tracks[track].name.c_str() : "";
            verticalText(centerX, py(y), c, name);
        }
    }

    // cSongPlayer::update3() -> fillBlocks()
    void fillBlocks() {
        const int length = static_cast<int>(song->order.size());
        const int bw = blockWidth();
        const int visible = std::min(std::max(0, length - listStart), kMaxPatternsVisible);
        for (int p = 0; p < visible; ++p) {
            const Pattern* pat = patternFor(patternAt(listStart + p));
            if (!pat) continue;
            const int rowY = kSongY + p * (kBlockHeight + kBlockSpacing) + 64;
            for (int track = 0; track < allTracks; ++track) {
                const int x = kSongX + kBlocksMargin + track * (bw + kBlockSpacing);
                bool hasNotes = false;
                if (track < static_cast<int>(pat->tracks.size())) {
                    for (const Cell& cell : pat->tracks[track]) {
                        if (cell.note >= 0 && cell.note < 120) { hasNotes = true; break; }
                    }
                }
                if (!hasNotes) {
                    fill(x, rowY, bw, kBlockHeight, rgb(theme->songPlayerColors[8]));
                } else {
                    const bool muted =
                        track < static_cast<int>(song->tracks.size()) && song->tracks[track].mute;
                    fill(x, rowY + 1, bw, kBlockHeight - 1,
                         rgb(theme->songPlayerColors[muted ? 7 : 1]));
                }
            }
        }
    }

    // cSongPlayer::update3() -> drawProgressLine()
    void drawProgressLine() {
        if (!playhead.playing) return;
        const int patternPosition = playhead.orderIndex - listStart;
        if (patternPosition < 0 || patternPosition >= kMaxPatternsVisible) return;
        const Pattern* pat = patternFor(patternAt(playhead.orderIndex));
        if (!pat) return;
        const int patLen = std::max(1, pat->rows);
        const float inPattern = playhead.row * (kBlockHeight / static_cast<float>(patLen)) + 0.5f;
        const int bw = blockWidth();
        const int localX = kSongX + kBlocksMargin;
        const int localWidth = (bw + kBlockSpacing) * allTracks - kBlockSpacing;
        const int localY = kSongY + patternPosition * (kBlockHeight + kBlockSpacing) + 64;
        line(localX, localY + inPattern, localX + localWidth, localY + inPattern,
             rgb(theme->songPlayerColors[9]));
    }

    // cSongPlayer::update3()
    void update3() { fillBlocks(); drawProgressLine(); }

    // cBar::update(): style Value_0_100 | Background | Gradient | BottomShadow.
    void barUpdate() {
        const int barWidth = kTempoW - 20;
        const int barX = kTempoX + (kTempoW - barWidth) / 2;
        const int barY = kTempoY + 10;
        const int barHeight = kTempoH - 25;
        const ImU32 bg = rgb(theme->activeBarColors[4]);
        fill(kTempoX, kTempoY, kTempoW, kTempoH, bg);
        const ImU32 clear = bg & 0x00FFFFFFu;
        dl->AddRectFilledMultiColor(
            ImVec2(px(kTempoX), py(kTempoY + kTempoH - 10)),
            ImVec2(px(kTempoX + kTempoW), py(kTempoY + kTempoH + 2)), bg, bg, clear, clear);

        const ImU32 container = rgb(theme->activeBarColors[3]);
        dl->AddRect(ImVec2(px(kTempoX + 10), py(kTempoY + 11)),
                    ImVec2(px(kTempoX + kTempoW - 10), py(kTempoY + kTempoH - 14)),
                    container, 0.0f, 0, 1.0f);
        dl->AddRectFilledMultiColor(ImVec2(px(kTempoX + 10), py(kTempoY + 11)),
                                    ImVec2(px(kTempoX + 18), py(kTempoY + kTempoH - 14)),
                                    clear, container, container, clear);

        const int value = std::clamp(static_cast<int>(song->bpm * 100.0f / kMaxTempo), 0, 100);
        const int fillY = barHeight - (barHeight * value) / 100;
        fill(barX + 1, barY + fillY, barWidth - 2, barHeight - fillY,
             rgb(theme->activeBarColors[0]));
    }

    // cFrame::update(): selected place (list) outline, frameColors[0].
    void frameUpdate() {
        const int* place = kFramePlaces[1];
        dl->AddRect(ImVec2(px(place[0]), py(place[1])),
                    ImVec2(px(place[0] + place[2]), py(place[1] + place[3])),
                    rgb(theme->frameColors[0]), 0.0f, 0, stroke());
    }
};

} // namespace

void OrderListView::draw(bool* p_open) {
    (void)p_open;
}

void OrderListView::update(Song& song, History& history, EngineHost& engineHost,
                           int& currentOrderIdx) {
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                                   ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar;
    if (ImGui::Begin("Order List", nullptr, flags)) {
        if (song.order.empty()) song.order.push_back(0);
        currentOrderIdx = std::clamp(currentOrderIdx, 0, static_cast<int>(song.order.size()) - 1);
        const int maxStart = std::max(0, static_cast<int>(song.order.size()) - kListLines);
        listStart_ = std::clamp(listStart_, 0, maxStart);
        if (currentOrderIdx < listStart_) listStart_ = currentOrderIdx;
        else if (currentOrderIdx >= listStart_ + kListLines) listStart_ = currentOrderIdx - kListLines + 1;
        listStart_ = std::clamp(listStart_, 0, maxStart);
        // Keyboard navigation and slot edits (same bindings as before); every
        // order change goes through SetOrderCommand so undo/redo covers it.
        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) || ImGui::IsWindowHovered()) {
            auto commitOrder = [&](std::vector<int> next) {
                if (next == song.order) return;
                history.execute(std::make_unique<SetOrderCommand>(std::move(next), song.order), song);
                engineHost.publish(song);
            };
            if (ImGui::IsKeyPressed(ImGuiKey_UpArrow) && currentOrderIdx > 0) currentOrderIdx--;
            if (ImGui::IsKeyPressed(ImGuiKey_DownArrow) &&
                currentOrderIdx + 1 < static_cast<int>(song.order.size())) currentOrderIdx++;
            if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow) || ImGui::IsKeyPressed(ImGuiKey_Minus) ||
                ImGui::IsKeyPressed(ImGuiKey_KeypadSubtract)) {
                if (song.order[currentOrderIdx] > 0) {
                    std::vector<int> next = song.order;
                    next[currentOrderIdx]--;
                    commitOrder(std::move(next));
                }
            }
            if (ImGui::IsKeyPressed(ImGuiKey_RightArrow) || ImGui::IsKeyPressed(ImGuiKey_Equal) ||
                ImGui::IsKeyPressed(ImGuiKey_KeypadAdd)) {
                if (song.order[currentOrderIdx] + 1 < static_cast<int>(song.patterns.size())) {
                    std::vector<int> next = song.order;
                    next[currentOrderIdx]++;
                    commitOrder(std::move(next));
                }
            }
            if (ImGui::IsKeyPressed(ImGuiKey_Insert)) {
                std::vector<int> next = song.order;
                next.insert(next.begin() + currentOrderIdx + 1, song.order[currentOrderIdx]);
                commitOrder(std::move(next));
                currentOrderIdx++;
            }
            if ((ImGui::IsKeyPressed(ImGuiKey_Delete) || ImGui::IsKeyPressed(ImGuiKey_Backspace)) &&
                song.order.size() > 1) {
                std::vector<int> next = song.order;
                next.erase(next.begin() + currentOrderIdx);
                commitOrder(std::move(next));
                if (currentOrderIdx >= static_cast<int>(song.order.size()))
                    currentOrderIdx = static_cast<int>(song.order.size()) - 1;
            }
        }

        const ImVec2 avail = ImGui::GetContentRegionAvail();
        ImFont* font = getMonoFont();
        if (avail.x < 100.0f || avail.y < 100.0f || !font) { ImGui::End(); return; }
        const ImVec2 origin = ImGui::GetCursorScreenPos();
        SongControls c;
        c.dl = ImGui::GetWindowDrawList();
        c.origin = origin;
        c.sx = avail.x / 800.0f;
        c.sy = avail.y / 397.0f;
        c.song = &song;
        c.playhead = engineHost.getPlayhead();
        c.theme = &songTheme();
        c.selected = currentOrderIdx;
        c.listStart = listStart_;
        c.allTracks = std::max(1, static_cast<int>(song.tracks.size()));
        c.font = font;
        c.nameFont = getMonoFont();
        c.fontSize = font->LegacySize * c.sy;
        c.nameFontSize = 12.0f * c.sy;

        c.update();      // cSongPlayer stage 1: background, header, list
        c.update2();     // cSongPlayer stage 2: track names
        c.update3();     // cSongPlayer stage 3: pattern blocks, progress line
        c.barUpdate();   // cBar (created after cSongPlayer)
        c.frameUpdate(); // cFrame (created first, composited last on top)

        if (c.clickedSlot >= 0) currentOrderIdx = c.clickedSlot;
        ImGui::SetCursorScreenPos(origin);
    }
    ImGui::End();
}

} // namespace trakr

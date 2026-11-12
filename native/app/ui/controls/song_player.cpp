#include "song_player.h"

#include "Song.h"
#include "ui/controls/primitives.h"

#include <algorithm>
#include <cstdio>
#include <string>

namespace trakr {
using namespace ::trakr::ui;

namespace {

// songPlayer.cpp geometry.
constexpr int kTotalBlocksWidth = 486;
constexpr int kBlocksMargin = 5;
constexpr int kBlockSpacing = 4;
constexpr int kBlockHeight = 23;
constexpr int kMaxPatternsVisible = 12;
constexpr int kListWidth = 210;
constexpr int kSongListRowHeight = 27;
constexpr int kListLines = 12;

} // namespace

SongPlayerControl::SongPlayerControl(std::string id) : Control(std::move(id)) {}

void SongPlayerControl::setState(const SongPlayerState& state) {
    state_ = state;
    clampList();
}

int SongPlayerControl::trackCount() const {
    if (state_.trackCount > 0) return state_.trackCount;
    return state_.song ? std::max(1, static_cast<int>(state_.song->tracks.size())) : 1;
}

int SongPlayerControl::listLines() const { return kListLines; }

int SongPlayerControl::blockWidth() const {
    const int n = std::max(1, trackCount());
    return ((kTotalBlocksWidth - kBlocksMargin * 2) - ((n - 1) * kBlockSpacing)) / n;
}

int SongPlayerControl::patternAt(int slot) const {
    if (!state_.song || slot < 0 || slot >= static_cast<int>(state_.song->order.size())) return -1;
    return state_.song->order[static_cast<size_t>(slot)];
}

const Pattern* SongPlayerControl::patternFor(int patternIndex) const {
    if (!state_.song || patternIndex < 0 ||
        patternIndex >= static_cast<int>(state_.song->patterns.size())) {
        return nullptr;
    }
    return &state_.song->patterns[static_cast<size_t>(patternIndex)];
}

bool SongPlayerControl::trackMuted(int track) const {
    if (!state_.song || track < 0 || track >= static_cast<int>(state_.song->tracks.size()))
        return false;
    return state_.song->tracks[static_cast<size_t>(track)].mute;
}

void SongPlayerControl::clampList() const {
    const int length = state_.song ? static_cast<int>(state_.song->order.size()) : 0;
    const int maxStart = std::max(0, length - listLines());
    // Keep the selected slot visible, matching the settled list position.
    if (state_.selectedSlot < listStart_) listStart_ = state_.selectedSlot;
    else if (state_.selectedSlot >= listStart_ + listLines())
        listStart_ = state_.selectedSlot - listLines() + 1;
    listStart_ = std::clamp(listStart_, 0, maxStart);
}

void SongPlayerControl::draw(Canvas& canvas) const {
    clampList();
    drawBackground(canvas);
    drawList(canvas);
    drawTrackNames(canvas);
    drawBlocks(canvas);
    drawProgressLine(canvas);
}

void SongPlayerControl::drawBackground(Canvas& canvas) const {
    canvas.fillRect(rect_, kSongPlayerColors[3]);
    drawBottomShadow(canvas, rect_, kSongPlayerColors[3]);
    // cSongPlayer::update: "Pattern" header, CMD_TEXT(600, posY + 30) centred.
    const int h = canvas.textHeight(trakr::hw::FwFont::Light14);
    canvas.text(trakr::hw::FwFont::Light14, 600 - canvas.textWidth(trakr::hw::FwFont::Light14,
                                                                   "Pattern") / 2,
                30 + rect_.y - h / 2, kSongPlayerColors[0], "Pattern");
}

void SongPlayerControl::drawList(Canvas& canvas) const {
    const int length = state_.song ? static_cast<int>(state_.song->order.size()) : 0;
    if (length <= 0 || listLines() <= 0) return;

    const int posX = rect_.right() - kListWidth;
    const int posY = rect_.y + 62;
    const int width = kListWidth;
    const int height = rect_.h - 30;
    const int hRow = kSongListRowHeight;
    const int lines = std::min(std::max(0, length - listStart_), listLines());
    int wBar = width - 6;
    if (length > listLines()) wBar = width - 13;

    const int barPos = std::clamp(state_.selectedSlot - listStart_, 0, listLines() - 1);
    if (barPos < lines) {
        const ImU32 frame = !state_.listActive ? kSongPlayerColors[1]
                            : state_.editMode  ? kSongPlayerColors[5]
                                               : kSongPlayerColors[9];
        canvas.strokeRectCentered(
            RectI{posX + 3, posY + barPos * hRow, wBar, hRow}, frame,
            kFwRedFrameWidth / 16.0f);
    }

    canvas.pushClip(RectI{posX, posY, wBar, hRow * listLines()});
    for (int i = 0; i < lines; ++i) {
        const int slot = listStart_ + i;
        const int pat = patternAt(slot);
        const int rowCenter = posY + hRow / 2 + i * hRow;
        char num[16] = "0";
        if (pat >= 0) {
            if (state_.startNumbersFrom1) {
                std::snprintf(num, sizeof(num), "%d", pat + 1);
            } else {
                std::snprintf(num, sizeof(num), "%d", pat);
            }
        }
        canvas.textVCentered(trakr::hw::FwFont::Light14, posX + 7 + 25, rowCenter,
                             kSongPlayerColors[0], num);
    }
    canvas.popClip();

    if (length > listLines()) {
        const int innerH = height - 46;
        const int yBase = length + listLines() - lines;
        const int yLength = innerH * std::min(listLines(), length) / std::max(1, yBase);
        const int thumbY = posY + 6 + (listStart_ * (innerH - yLength)) / std::max(1, length);
        canvas.fillRect(RectI{posX + (width - 6), posY + 4, 3, (posY + height - 42) - (posY + 4)},
                        kSongPlayerColors[6]);
        canvas.line(posX + (width - 5), thumbY, posX + (width - 5), thumbY + yLength,
                    kSongPlayerColors[2], kFwRedFrameWidth / 16.0f);
        canvas.line(posX + (width - 4), thumbY, posX + (width - 4), thumbY + yLength,
                    kSongPlayerColors[2], kFwRedFrameWidth / 16.0f);
    }
}

void SongPlayerControl::drawTrackNames(Canvas& canvas) const {
    const int tracks = trackCount();
    const int bw = blockWidth();
    const int y = rect_.y + 42;
    for (int track = 0; track < tracks; ++track) {
        const ImU32 color = trackMuted(track) ? kSongPlayerColors[4] : kSongPlayerColors[0];
        const int centerX = rect_.x + kBlocksMargin + (bw + kBlockSpacing) * track + bw / 2;
        const std::string name = (state_.song &&
                                  track < static_cast<int>(state_.song->tracks.size()))
                                     ? state_.song->tracks[static_cast<size_t>(track)].name
                                     : std::string();
        canvas.textVertical270(trakr::hw::FwFont::Regular12, centerX, y, color, name);
    }
}

void SongPlayerControl::drawBlocks(Canvas& canvas) const {
    const int length = state_.song ? static_cast<int>(state_.song->order.size()) : 0;
    const int tracks = trackCount();
    const int bw = blockWidth();
    const int visible = std::min(std::max(0, length - listStart_), kMaxPatternsVisible);

    for (int p = 0; p < visible; ++p) {
        const Pattern* pat = patternFor(patternAt(listStart_ + p));
        if (!pat) continue;
        const int rowY = rect_.y + p * (kBlockHeight + kBlockSpacing) + 64;
        for (int track = 0; track < tracks; ++track) {
            const int x = rect_.x + kBlocksMargin + track * (bw + kBlockSpacing);
            bool hasNotes = false;
            if (track < static_cast<int>(pat->tracks.size())) {
                for (const Cell& cell : pat->tracks[static_cast<size_t>(track)]) {
                    if (cell.note >= 0 && cell.note < 120) {
                        hasNotes = true;
                        break;
                    }
                }
            }
            if (!hasNotes) {
                canvas.fillRect(RectI{x, rowY, bw, kBlockHeight}, kSongPlayerColors[8]);
            } else {
                canvas.fillRect(RectI{x, rowY + 1, bw - 1, kBlockHeight - 1},
                                kSongPlayerColors[trackMuted(track) ? 7 : 1]);
            }
        }
    }
}

void SongPlayerControl::drawProgressLine(Canvas& canvas) const {
    if (!state_.playing || !state_.song) return;
    const int patternPosition = state_.playhead.orderIndex - listStart_;
    if (patternPosition < 0 || patternPosition >= kMaxPatternsVisible) return;
    const Pattern* pat = patternFor(patternAt(state_.playhead.orderIndex));
    if (!pat) return;
    const int patLen = std::max(1, pat->rows);
    const float inPattern =
        static_cast<float>(state_.playhead.row) * (kBlockHeight / static_cast<float>(patLen)) + 0.5f;

    int localX = rect_.x + kBlocksMargin;
    int localWidth = (blockWidth() + kBlockSpacing) * trackCount() - kBlockSpacing;
    const int localY = rect_.y + patternPosition * (kBlockHeight + kBlockSpacing) + 64;
    canvas.line(localX, localY + static_cast<int>(inPattern), localX + localWidth,
                localY + static_cast<int>(inPattern),
                state_.editMode ? kSongPlayerColors[5] : kSongPlayerColors[9],
                kFwRedFrameWidth / 16.0f);
}

bool SongPlayerControl::onPress(int deviceX, int deviceY) {
    if (!contains(deviceX, deviceY) || !state_.song) return false;
    const int posX = rect_.right() - kListWidth;
    const int posY = rect_.y + 62;
    const int relativeY = deviceY - (posY + kListTopInset);
    if (relativeY < 0) return false;
    const int row = relativeY / kSongListRowHeight;
    if (row >= listLines()) return false;
    const int length = static_cast<int>(state_.song->order.size());
    const int slot = listStart_ + row;
    if (slot < 0 || slot >= length) return false;
    if (deviceX < posX + 3 || deviceX >= posX + kListWidth - 6) return false;
    state_.selectedSlot = slot;
    if (onSelect_) onSelect_(slot);
    return true;
}

} // namespace trakr

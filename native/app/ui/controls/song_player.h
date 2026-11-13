#pragma once

// cSongPlayer semantic port (Song arranger matrix).
//
// Firmware reference: core/libs/NewDisplay/songPlayer.cpp.
// Geometry (block width, spacing, list metrics) is mirrored verbatim; the
// controls draw in device pixels through Canvas.
//
// Not ported: the firmware list animation frames (cSongPlayer interpolates
// listAnimationStep over several refresh passes). The control draws the settled
// state, which is what the reference frame and the app show.

#include "ui/control.h"
#include "EngineHost.h"
#include <functional>
#include <vector>

namespace trakr {

class Song;
class Pattern;

struct SongPlayerState {
    const Song* song{nullptr};
    int selectedSlot{0};
    int trackCount{0};        // 0 means derive from song->tracks
    PlayheadPosition playhead{};
    bool playing{false};
    bool listActive{true};    // frame colour: colors[9] active, colors[1] idle
    bool editMode{false};     // frame colour: colors[5]
    bool startNumbersFrom1{true};
};

class SongPlayerControl : public ui::Control {
public:
    explicit SongPlayerControl(std::string id);

    void setState(const SongPlayerState& state);

    void draw(ui::Canvas& canvas) const override;
    bool acceptsInput() const override { return true; }
    bool onPress(int deviceX, int deviceY) override;
    void setOnSelect(std::function<void(int)> cb) { onSelect_ = std::move(cb); }

    int listStart() const { return listStart_; }

private:
    int trackCount() const;
    int listLines() const;
    int blockWidth() const;
    int patternAt(int slot) const;
    const Pattern* patternFor(int patternIndex) const;
    bool trackMuted(int track) const;
    void clampList() const;
    void drawBackground(ui::Canvas& canvas) const;
    void drawList(ui::Canvas& canvas) const;
    void drawTrackNames(ui::Canvas& canvas) const;
    void drawBlocks(ui::Canvas& canvas) const;
    void drawProgressLine(ui::Canvas& canvas) const;

    SongPlayerState state_{};
    mutable int listStart_{0};
    mutable const Song* cachedSong_{nullptr};
    mutable std::vector<const Pattern*> cachedPatterns_;
    mutable std::vector<std::vector<char>> cachedHasNotes_;
    std::function<void(int)> onSelect_;
};

} // namespace trakr

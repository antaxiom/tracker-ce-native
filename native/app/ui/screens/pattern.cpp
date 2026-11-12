#include "pattern.h"

#include "Song.h"
#include "History.h"
#include "EngineHost.h"
#include "views/PatternView.h"

#include <algorithm>
#include <cmath>

namespace trakr {
using namespace ::trakr::ui;

namespace {
constexpr int kGutter = 27;
constexpr int kTrackWidth = 186;
constexpr int kRowHeight = 26;
constexpr int kRowsVisible = 15;
constexpr int kCursorVisibleRow = 7;
} // namespace

PatternScreen::PatternScreen() = default;

void PatternScreen::build() {
    if (built_) return;
    tracker_ = &display_.add<TrackerControl>("tracker");
    tracker_->setRect(RectI{0, 28, 800, 395});
    built_ = true;
}

void PatternScreen::update(Song& song,
                           History& history,
                           EngineHost& engineHost,
                           int currentOrderIdx,
                           int octave,
                           int editStep,
                           bool recordMode,
                           int& selectedInstIdx,
                           PatternView& view,
                           const ImRect& workspace,
                           ui::LayoutProfile profile,
                           bool shiftHeld) {
    build();
    if (song.patterns.empty() || song.order.empty()) return;

    const int patIdx = std::clamp(song.order[std::clamp(currentOrderIdx, 0,
                                                        static_cast<int>(song.order.size()) - 1)],
                                  0, static_cast<int>(song.patterns.size()) - 1);
    Pattern& pat = song.patterns[patIdx];
    const int rows = std::max(1, pat.rows);
    const int tracks = static_cast<int>(pat.tracks.size());

    const PlayheadPosition playhead = engineHost.getPlayhead();
    if (playhead.playing) {
        view.setCursor(view.cursorTrack(), playhead.row % rows, view.cursorSubCol());
    }

    ImGui::SetCursorScreenPos(workspace.Min);
    ImGui::InvisibleButton("##gui_next_pattern_workspace", workspace.GetSize());
    const bool hovered = ImGui::IsItemHovered();
    const bool keyTarget =
        ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) || !ImGui::IsAnyItemActive();

    if (keyTarget && !ImGui::GetIO().WantTextInput) {
        view.handleKeyboardInput(pat, patIdx, song, history, engineHost, octave, editStep,
                                 recordMode, selectedInstIdx);
    }

    if (hovered) {
        const float wheel = ImGui::GetIO().MouseWheel;
        if (std::abs(wheel) > 0.01f) {
            const int row = (view.cursorRow() - static_cast<int>(wheel * 4.0f) + rows * 100) % rows;
            view.setCursor(view.cursorTrack(), row, view.cursorSubCol());
        }
    }

    if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
        const LayoutContext lc = LayoutContext::fromWorkspace(workspace, profile);
        const ImVec2 device = lc.screenToDevice(ImGui::GetIO().MousePos);
        const int dx = static_cast<int>(std::floor(device.x));
        const int dy = static_cast<int>(std::floor(device.y));
        if (dx >= kGutter && dx < 772 && dy >= 28 && dy < 28 + kRowsVisible * kRowHeight) {
            const int visibleRow = (dy - 28) / kRowHeight;
            const int row = (view.cursorRow() - kCursorVisibleRow + visibleRow + rows * 100) % rows;
            const int first = std::clamp(view.cursorTrack() - 3, 0, std::max(0, tracks - 4));
            const int track = std::clamp(first + (dx - kGutter) / kTrackWidth, 0, tracks - 1);
            const int subX = (dx - kGutter) % kTrackWidth;
            const int subCol = (subX < 44) ? 0 : (subX < 78) ? 1 : (subX < 132) ? 2 : 3;

            if (ImGui::GetIO().KeyShift || shiftHeld) {
                if (!view.hasSelection()) {
                    view.setSelection(view.cursorTrack(), view.cursorRow(), track, row);
                } else {
                    const SelectionRange range = view.getSelection();
                    view.setSelection(range.startTrack, range.startRow, track, row);
                }
            } else {
                view.clearSelection();
            }
            view.setCursor(track, row, subCol);
            view.syncInstrumentFromCursor(pat, song, selectedInstIdx);
        }
    }

    TrackerState state;
    state.song = &song;
    state.pattern = &pat;
    state.cursorTrack = view.cursorTrack();
    state.cursorRow = view.cursorRow();
    state.cursorSubCol = view.cursorSubCol();
    state.recording = recordMode;
    state.hasSelection = view.hasSelection();
    if (state.hasSelection) {
        const SelectionRange range = view.getSelection();
        state.selStartTrack = range.startTrack;
        state.selEndTrack = range.endTrack;
        state.selStartRow = range.startRow;
        state.selEndRow = range.endRow;
    }
    state.playing = playhead.playing;
    state.playheadRow = playhead.row;
    state.stepDev = 4;
    tracker_->setState(state);
}

void PatternScreen::render(ImDrawList* drawList,
                           const ImRect& workspace,
                           ui::LayoutProfile profile) const {
    if (!built_ || drawList == nullptr) return;
    const LayoutContext lc = LayoutContext::fromWorkspace(workspace, profile);
    Canvas canvas(drawList, lc);
    canvas.pushClip(RectI{0, kWorkspaceTop, kDeviceWidth, kWorkspaceSpanY});
    display_.renderAll(canvas);
    canvas.popClip();
}

} // namespace trakr

#include <catch2/catch_test_macros.hpp>

#include "Song.h"
#include "ui/controls/primitives.h"
#include "ui/controls/song_player.h"
#include "ui/controls/waveform.h"
#include "ui/controls/param_value_list.h"
#include "views/MixerView.h"
#include "ui/controls/multi_label.h"
#include "ui/display.h"
#include "ui/layout.h"

#include <string>
#include <vector>

namespace {

class Recorder : public trakr::ui::Control {
public:
    Recorder(std::string id, std::vector<std::string>& out, trakr::ui::RectI rect)
        : Control(std::move(id)), out_(out) {
        setRect(rect);
    }
    void draw(trakr::ui::Canvas&) const override { out_.push_back(id()); }

private:
    std::vector<std::string>& out_;
};

} // namespace

TEST_CASE("Display composites in reverse creation order", "[ui][display]") {
    std::vector<std::string> order;
    trakr::ui::Display display;
    display.add<Recorder>("a", order, trakr::ui::RectI{0, 0, 10, 10});
    display.add<Recorder>("b", order, trakr::ui::RectI{0, 0, 10, 10});
    display.add<Recorder>("c", order, trakr::ui::RectI{0, 0, 10, 10});

    trakr::ui::Canvas canvas(nullptr, trakr::ui::LayoutContext{});
    display.renderAll(canvas);

    CHECK(order == std::vector<std::string>{"c", "b", "a"});
}

TEST_CASE("Display hit-test is front-to-back and skips non-input controls", "[ui][display]") {
    trakr::ui::Display display;
    auto& frame = display.add<trakr::ui::FrameControl>(
        "frame", std::vector<trakr::ui::RectI>{trakr::ui::RectI{0, 0, 100, 100}}, 0);
    auto& bar = display.add<trakr::ui::BarControl>("bar", 1);
    bar.setRect(trakr::ui::RectI{0, 0, 100, 100});

    (void)frame;
    CHECK(display.hitTest(10, 10) == &bar);
    CHECK(display.hitTest(200, 200) == nullptr);
}

TEST_CASE("BarControl routes a press to its column", "[ui][bar]") {
    trakr::ui::BarControl bar("bar", 3);
    bar.setRect(trakr::ui::RectI{301, 29, 97, 394});
    int pressed = -1;
    bar.setOnPress([&pressed](int column) { pressed = column; });

    CHECK(bar.onPress(310, 40));
    CHECK(pressed == 3);
    CHECK_FALSE(bar.onPress(10, 10));
}

TEST_CASE("ListControl selects the pressed row", "[ui][list]") {
    trakr::ui::ListControl list("filter", {"Disabled", "Low-pass", "High-pass", "Band-pass"}, 5);
    list.setRect(trakr::ui::RectI{401, 29, 97, 394});
    int selected = -1;
    list.setOnSelect([&selected](int index) { selected = index; });

    const int rowY = 29 + trakr::ui::kListTopInset + trakr::ui::kListRowHeight + 5;
    CHECK(list.onPress(410, rowY));
    CHECK(list.selected() == 1);
    CHECK(selected == 1);

    CHECK_FALSE(list.onPress(410, 29 + trakr::ui::kListTopInset - 2));
}

TEST_CASE("LayoutContext maps device pixels into the host workspace", "[ui][layout]") {
    trakr::ui::LayoutContext lc;
    lc.origin = ImVec2(0.0f, 28.0f);
    lc.deviceTop = trakr::ui::kWorkspaceTop;
    lc.scaleX = 1.0f;
    lc.scaleY = 1.0f;

    CHECK(lc.screenY(28) == 28.0f);
    CHECK(lc.screenY(424) == 424.0f);

    const ImVec2 device = lc.screenToDevice(ImVec2(100.0f, 128.0f));
    CHECK(device.x == 100.0f);
    CHECK(device.y == 128.0f);

    const ImRect workspace(ImVec2(0.0f, 28.0f), ImVec2(800.0f, 425.0f));
    const trakr::ui::LayoutContext classic =
        trakr::ui::LayoutContext::fromWorkspace(workspace, trakr::ui::LayoutProfile::Classic);
    CHECK(classic.scaleX == 1.0f);
    CHECK(classic.scaleY == 1.0f);
}

TEST_CASE("ListControl keeps the cList anchor window while scrolling", "[ui][list]") {
    std::vector<std::string> items;
    for (int i = 0; i < 20; ++i) items.push_back("row " + std::to_string(i));
    trakr::ui::ListControl list("loader", items, 13);
    list.setRect(trakr::ui::RectI{1, 61, 297, 362});

    // showFilesTree() anchors at the selection; with 13 visible rows the text
    // window stays at row 0 until the anchor passes the second-to-last row.
    list.setSelected(1);
    list.setStart(1);
    CHECK(list.onPress(10, 61 + trakr::ui::kListTopInset + 2 * trakr::ui::kListRowHeight + 3));
    CHECK(list.selected() == 2);

    list.setSelected(19);
    list.setStart(19);
    CHECK(list.onPress(10, 61 + trakr::ui::kListTopInset + 2 * trakr::ui::kListRowHeight + 3));
    CHECK(list.selected() == 9); // first visible row is anchor - (13 - 2) = 8, pressed row 1
}

TEST_CASE("SongPlayerControl clamps its list window to the selection", "[ui][song]") {
    trakr::Song song;
    song.order.assign(30, 0);
    song.tracks.resize(16);

    trakr::SongPlayerControl player("player");
    player.setRect(trakr::ui::RectI{1, 29, 697, 394});
    trakr::SongPlayerState state;
    state.song = &song;
    state.selectedSlot = 20;
    state.trackCount = 16;
    player.setState(state);
    CHECK(player.listStart() == 9); // 12 visible rows: 20 - 12 + 1

    state.selectedSlot = 0;
    player.setState(state);
    CHECK(player.listStart() == 0);

    // Pressing the fifth list row selects slot 4 at the top of the list.
    int chosen = -1;
    player.setOnSelect([&chosen](int slot) { chosen = slot; });
    const int rowY = 29 + 62 + trakr::ui::kListTopInset + 4 * 27 + 5;
    CHECK(player.onPress(600, rowY));
    CHECK(chosen == 4);
}

TEST_CASE("Waveform point mapping honours the zoom window", "[ui][waveform]") {
    trakr::ui::WaveformState state;
    state.length = 1001;
    CHECK(trakr::ui::waveform_detail::pointX(state, 0) == 0);
    CHECK(trakr::ui::waveform_detail::pointX(state, 500) == 299);
    CHECK(trakr::ui::waveform_detail::pointX(state, 1000) == 599);

    state.zoomStart = 32768;
    state.zoomEnd = 49151;
    CHECK(trakr::ui::waveform_detail::pointX(state, 0) == -1);
    CHECK(trakr::ui::waveform_detail::pointX(state, 1000) == -1);
    const int mid = trakr::ui::waveform_detail::pointX(state, 501);
    CHECK(mid >= 0);
    CHECK(mid <= 599);
}

TEST_CASE("ParamValueListControl selects rows and keeps its anchor", "[ui][paramlist]") {
    trakr::ui::ParamValueListControl list("submenu", 14);
    list.setRect(trakr::ui::RectI{201, 29, 397, 394});
    list.setRows({{"Pattern divider", "4"}, {"Root note", "C"}, {"Notes scale", "Chromatic"}});
    int selected = -1;
    list.setOnSelect([&selected](int index) { selected = index; });

    const int rowY = 29 + trakr::ui::kListTopInset + trakr::ui::kListRowHeight + 6;
    CHECK(list.onPress(300, rowY));
    CHECK(list.selected() == 1);
    CHECK(selected == 1);
    CHECK_FALSE(list.onPress(10, rowY));
}

TEST_CASE("MultiLabelControl routes top and preset row presses", "[ui][multilabel]") {
    trakr::ui::MultiLabelControl cell("value");
    cell.setRect(trakr::ui::RectI{11, 34, 62, 389});
    int top = 0, row = -1;
    cell.setOnTop([&top] { ++top; });
    cell.setOnRow([&row](int r) { row = r; });

    CHECK(cell.onPress(40, 50));   // CC value cell
    CHECK(top == 1);
    CHECK(cell.onPress(40, 34 + 35 + 210 + 2 * 30 + 5)); // fourth preset row
    CHECK(row == 2);
    CHECK_FALSE(cell.onPress(40, 34 + 35 + 4)); // gap between cells
}

TEST_CASE("Mixer gain fader mapping matches the firmware curve", "[ui][mixer]") {
    CHECK(trakr::MixerView::gainBarValue(0.0f) == 75);
    CHECK(trakr::MixerView::gainBarValue(6.0f) == 100);
    CHECK(trakr::MixerView::gainBarValue(-48.0f) == 0);
    CHECK(trakr::MixerView::gainBarValue(-3.0f) < 75);
}

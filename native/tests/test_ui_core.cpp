#include <catch2/catch_test_macros.hpp>

#include "ui/controls/primitives.h"
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

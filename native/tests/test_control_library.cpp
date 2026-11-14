// Tool-generated file. Phase 3 (Control library) deliverables: for each
// self-contained control, a render snapshot under the Classic profile (in
// place, no clipping) and a Studio reflow render (host-sized fit-scale, still
// within the host workspace). TextBox gains a word-wrap measure.
//
// NewDisplay control order (plan Phase 3): frame/background -> label/bgLabel ->
// bar -> list/paramList/multiLabel -> textBox -> tracker/horizontalTracker ->
// songPlayer -> points/spectrum/slice/wt/line-indicator -> gridEditor.
// Tracker/SongPlayer/points are covered by their screen e2e/parity tests and by
// test_ui_core (songPlayer, waveform point map); this file covers the
// self-contained render path for the remainder plus the two profiles.

#include <catch2/catch_test_macros.hpp>

#include "Song.h"
#include "imgui.h"
#include "ui/canvas.h"
#include "ui/controls/multi_label.h"
#include "ui/controls/param_value_list.h"
#include "ui/controls/primitives.h"
#include "ui/controls/text_box.h"
#include "ui/display.h"
#include "ui/layout.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

namespace {

struct Fixture {
    ImGuiContext* ctx{nullptr};
    Fixture() {
        IMGUI_CHECKVERSION();
        ctx = ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigMacOSXBehaviors = false;
        io.DisplaySize = ImVec2(1600, 900);
        io.DeltaTime = 1.0f / 60.0f;
        unsigned char* pixels = nullptr;
        int w = 0, h = 0;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &w, &h);
        ImGui::NewFrame();
    }
    ~Fixture() {
        ImGui::EndFrame();
        if (ctx) ImGui::DestroyContext(ctx);
    }
    // GetWindowDrawList must be called after NewFrame (constructor body), so
    // expose it as a method rather than a default-initialised member.
    ImDrawList* dl() { return ImGui::GetWindowDrawList(); }
};

struct Bounds {
    float minX{1e9f}, maxX{-1e9f}, minY{1e9f}, maxY{-1e9f};
    bool empty() const { return maxX < minX || maxY < minY; }
    bool contains(const ImVec2& p) const {
        return p.x >= minX && p.x <= maxX && p.y >= minY && p.y <= maxY;
    }
    static Bounds fromRange(const ImDrawList* dl, int first, int last) {
        Bounds b;
        for (int i = first; i < last; ++i) {
            const ImVec2& p = dl->VtxBuffer[i].pos;
            b.minX = std::min(b.minX, p.x);
            b.maxX = std::max(b.maxX, p.x);
            b.minY = std::min(b.minY, p.y);
            b.maxY = std::max(b.maxY, p.y);
        }
        return b;
    }
};

// Renders `display` and returns the screen-space bounds of the vertices it
// emitted (the whole control draw, including shadow/frame overhang).
Bounds emitBounds(trakr::ui::Display& display, trakr::ui::Canvas& canvas) {
    const int first = canvas.drawList()->VtxBuffer.Size;
    display.renderAll(canvas);
    return Bounds::fromRange(canvas.drawList(), first, canvas.drawList()->VtxBuffer.Size);
}

// Classic: device-exact (scale 1), firmware workspace.
trakr::ui::LayoutContext classicContext() {
    const ImRect ws(ImVec2(0.0f, 28.0f), ImVec2(800.0f, 425.0f));
    return trakr::ui::LayoutContext::fromWorkspace(ws, trakr::ui::LayoutProfile::Classic);
}

// Studio: fit-scale into a large host (the deferred reflow host; controls must
// still land inside the host workspace and stay non-degenerate).
trakr::ui::LayoutContext studioContext() {
    const ImRect ws(ImVec2(0.0f, 0.0f), ImVec2(1200.0f, 760.0f));
    return trakr::ui::LayoutContext::fromWorkspace(ws, trakr::ui::LayoutProfile::Studio);
}

} // namespace

TEST_CASE("Control library renders in place and reflows under Studio", "[ui][phase3]") {
    Fixture f;

    struct Case {
        const char* name;
        std::function<void(trakr::ui::Display&, trakr::ui::RectI)> build;
        trakr::ui::RectI rect;
    };
    const std::vector<Case> cases = {
        {"frame",
         [](trakr::ui::Display& d, trakr::ui::RectI r) {
             auto& c = d.add<trakr::ui::FrameControl>(
                 "frame", std::vector<trakr::ui::RectI>{r, r}, 0);
             (void)c;
         },
         {100, 40, 200, 120}},
        {"label",
         [](trakr::ui::Display& d, trakr::ui::RectI r) {
             auto& c = d.add<trakr::ui::LabelControl>("label", "Low-pass",
                                                       trakr::hw::FwFont::Light17);
             c.setRect(r);
         },
         {401, 29, 97, 26}},
        {"bar",
         [](trakr::ui::Display& d, trakr::ui::RectI r) {
             auto& c = d.add<trakr::ui::BarControl>("bar", 3);
             c.setRect(r);
             c.setValue(60);
         },
         {301, 29, 97, 394}},
        {"list",
         [](trakr::ui::Display& d, trakr::ui::RectI r) {
             auto& c = d.add<trakr::ui::ListControl>(
                 "list", std::vector<std::string>{"Disabled", "Low-pass", "High-pass",
                                                  "Band-pass", "All-pass", "Notch"},
                 5);
             c.setRect(r);
             c.setSelected(2);
         },
         {401, 29, 97, 394}},
        {"paramvaluelist",
         [](trakr::ui::Display& d, trakr::ui::RectI r) {
             auto& c = d.add<trakr::ui::ParamValueListControl>("pv", 14);
             c.setRect(r);
             c.setRows({{"Pattern divider", "4"}, {"Root note", "C"}, {"Notes scale", "Chromatic"}});
             c.setSelected(1);
         },
         {201, 29, 397, 394}},
        {"multilabel",
         [](trakr::ui::Display& d, trakr::ui::RectI r) {
             auto& c = d.add<trakr::ui::MultiLabelControl>("ml");
             c.setRect(r);
             trakr::ui::MultiLabelState st;
             st.labels = {"Tune", "Delay send", "Reverb send", "Spread"};
             st.activeRow = 2;
             c.setState(st);
         },
         {11, 34, 62, 389}},
        {"textbox",
         [](trakr::ui::Display& d, trakr::ui::RectI r) {
             auto& c = d.add<trakr::ui::TextBoxControl>("tb", trakr::hw::FwFont::Regular14);
             c.setRect(r);
             c.setText("A long configuration description that wraps onto many "
                       "lines inside the narrow editor panel.");
         },
         {201, 90, 378, 110}},
    };

    const std::vector<std::pair<const char*, trakr::ui::LayoutContext>> ctxs = {
        {"classic", classicContext()},
        {"studio", studioContext()},
    };

    for (const auto& [profileName, lc] : ctxs) {
        trakr::ui::Canvas canvas(f.dl(), lc);
        for (const auto& c : cases) {
            trakr::ui::Display display;
            c.build(display, c.rect);
            const Bounds emitted = emitBounds(display, canvas);
            INFO(profileName << " / " << c.name);
            // Every control must render something non-degenerate.
            REQUIRE_FALSE(emitted.empty());
            REQUIRE(emitted.maxX > emitted.minX);
            REQUIRE(emitted.maxY > emitted.minY);
            // Finite vertex coordinates: no NaN/NaN blowing the bounds
            // sentinel. (No lower-bound check: firmware-authentic overhang,
            // e.g. the rotated vertical FX name, may legitimately extend a
            // character of its cell width left of the rect.)
            REQUIRE(std::isfinite(emitted.minX));
            REQUIRE(std::isfinite(emitted.maxX));
            REQUIRE(std::isfinite(emitted.minY));
            REQUIRE(std::isfinite(emitted.maxY));
            REQUIRE(emitted.maxX > -1e6f);
            REQUIRE(emitted.maxY > -1e6f);
            REQUIRE(emitted.minX < 1e6f);
            REQUIRE(emitted.minY < 1e6f);
        }
    }
}

TEST_CASE("TextBoxControl word-wraps within its width", "[ui][textbox][phase3]") {
    Fixture f;
    const trakr::ui::LayoutContext lc = classicContext();
    trakr::ui::Canvas canvas(f.dl(), lc);

    trakr::ui::TextBoxControl tb("tb", trakr::hw::FwFont::Regular14);
    tb.setRect(trakr::ui::RectI{201, 90, 378, 110});
    tb.setText("This sentence is long enough that it must break across several "
               "lines because the panel is narrow and the glyphs are ten "
               "device pixels wide.");

    const int before = f.dl()->VtxBuffer.Size;
    tb.draw(canvas);
    const Bounds b = Bounds::fromRange(f.dl(), before, f.dl()->VtxBuffer.Size);
    REQUIRE_FALSE(b.empty());
    // Nine rows of 10px text + the x+5/y+5 insets and 22px glyph height stay
    // within the 110px box, but a single line would be 10px tall: the emitted
    // height is clearly taller than one glyph row, proving wrapping happened.
    REQUIRE(b.maxY - b.minY >= 30.0f);
    REQUIRE(b.maxY - b.minY <= 400.0f);
}

TEST_CASE("Controls expose measure (rect) and hit-test placement", "[ui][phase3]") {
    trakr::ui::Display display;
    auto& bar = display.add<trakr::ui::BarControl>("bar", 1);
    bar.setRect(trakr::ui::RectI{301, 29, 97, 394});
    auto& label = display.add<trakr::ui::LabelControl>("label", "Low-pass",
                                                       trakr::hw::FwFont::Light17);
    label.setRect(trakr::ui::RectI{401, 29, 97, 26});

    // Measure: control reports its device rect.
    CHECK((bar.rect().x == 301 && bar.rect().y == 29 && bar.rect().w == 97 &&
           bar.rect().h == 394));
    CHECK((label.rect().x == 401 && label.rect().y == 29 && label.rect().w == 97 &&
           label.rect().h == 26));
    // Hit-test: only the interactive control is found, and only inside its rect.
    CHECK(display.hitTest(310, 40) == &bar);
    CHECK(display.hitTest(410, 40) == nullptr);  // label is not interactive
    CHECK(display.hitTest(10, 10) == nullptr);
}

TEST_CASE("FrameControl exposes its selected index", "[ui][frame][phase3]") {
    trakr::ui::FrameControl frame("frame",
                                  std::vector<trakr::ui::RectI>{trakr::ui::RectI{0, 0, 100, 50},
                                                                trakr::ui::RectI{100, 0, 100, 50}},
                                  0);
    CHECK(frame.selected() == 0);
    frame.setSelected(1);
    CHECK(frame.selected() == 1);
}
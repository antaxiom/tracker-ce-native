#include <catch2/catch_test_macros.hpp>

#include "ui/canvas.h"
#include "imgui.h"

#include <algorithm>
#include <cmath>

namespace {

struct CanvasFixture {
    ImGuiContext* ctx{nullptr};
    CanvasFixture() {
        IMGUI_CHECKVERSION();
        ctx = ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigMacOSXBehaviors = false;
        io.DisplaySize = ImVec2(1000, 700);
        io.DeltaTime = 1.0f / 60.0f;
        unsigned char* pixels = nullptr;
        int width = 0, height = 0;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
        ImGui::NewFrame();
    }
    ~CanvasFixture() {
        ImGui::EndFrame();
        if (ctx) ImGui::DestroyContext(ctx);
    }

    static ImDrawList* drawList() { return ImGui::GetWindowDrawList(); }

    // Bounding box of the vertices emitted since `first`.
    struct Bounds {
        float minX{1e9f}, maxX{-1e9f}, minY{1e9f}, maxY{-1e9f};
        int vertices{0};
    };
    static Bounds boundsSince(ImDrawList* dl, int first) {
        Bounds b;
        for (int i = first; i < dl->VtxBuffer.Size; ++i) {
            const ImVec2& p = dl->VtxBuffer[i].pos;
            b.minX = std::min(b.minX, p.x);
            b.maxX = std::max(b.maxX, p.x);
            b.minY = std::min(b.minY, p.y);
            b.maxY = std::max(b.maxY, p.y);
            ++b.vertices;
        }
        return b;
    }
};

} // namespace

TEST_CASE("Canvas line raster is hard-edged and stays inside its box", "[ui][canvas][lines]") {
    CanvasFixture fixture;
    ImDrawList* dl = CanvasFixture::drawList();
    // origin (0,0), scale 1, workspace starts at device y=28.
    const trakr::ui::LayoutContext lc{};
    const trakr::ui::Canvas canvas(dl, lc);

    SECTION("horizontal 1px line covers exactly one device row") {
        const int first = dl->VtxBuffer.Size;
        canvas.line(10, 40, 50, 40, IM_COL32(255, 255, 255, 255));
        const auto b = CanvasFixture::boundsSince(dl, first);
        REQUIRE(b.vertices > 0);
        CHECK(b.minX == 10.0f);
        CHECK(b.maxX == 51.0f); // inclusive end point
        CHECK(b.minY == 12.0f); // 40 - deviceTop(28)
        CHECK(b.maxY == 13.0f); // no AA fringe above or below
    }

    SECTION("vertical 1px line covers exactly one device column") {
        const int first = dl->VtxBuffer.Size;
        canvas.line(100, 60, 100, 90, IM_COL32(255, 255, 255, 255));
        const auto b = CanvasFixture::boundsSince(dl, first);
        REQUIRE(b.vertices > 0);
        CHECK(b.minX == 100.0f);
        CHECK(b.maxX == 101.0f);
        CHECK(b.minY == 32.0f);  // 60 - 28
        CHECK(b.maxY == 63.0f);  // 90 - 28 + 1 (inclusive end)
        CHECK(b.maxY - b.minY == 31.0f);
    }

    SECTION("thick orthogonal line stays within its thickness") {
        const int first = dl->VtxBuffer.Size;
        canvas.line(0, 200, 599, 200, IM_COL32(255, 255, 255, 255), 2.0f);
        const auto b = CanvasFixture::boundsSince(dl, first);
        REQUIRE(b.vertices > 0);
        CHECK(b.minY == 171.0f); // 200 - 28 - 1
        CHECK(b.maxY == 173.0f);
        CHECK(b.minX == 0.0f);
        CHECK(b.maxX == 600.0f);
    }

    SECTION("diagonal line has no anti-aliasing fringe") {
        const int first = dl->VtxBuffer.Size;
        canvas.line(10, 40, 30, 60, IM_COL32(255, 255, 255, 255));
        const auto b = CanvasFixture::boundsSince(dl, first);
        REQUIRE(b.vertices > 0);
        // A 1px AA stroke would reach half a fringe pixel past each end.
        CHECK(b.minX >= 9.5f);
        CHECK(b.maxX <= 30.5f);
        CHECK(b.minY >= 11.5f);
        CHECK(b.maxY <= 32.5f);
    }
}

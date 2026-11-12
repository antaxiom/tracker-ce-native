#include "multi_label.h"

#include "ui/controls/primitives.h"
#include "ui/design.h"

#include <cmath>

namespace trakr::ui {
namespace {

constexpr int kMultiCcHeight = 35;
constexpr int kMultiFxHole = 210;
constexpr int kMultiTextOffset = 16;
constexpr int kMultiTextStep = 30;

const ImU32 kValueNormalColors[3] = {kWhite, kGrey0A, kOneTrueGreen};
const ImU32 kValueEditColors[3] = {kWhite, kGrey0A, kOneTrueRed};

void drawCentered(Canvas& canvas, int centerX, int yTop, ImU32 color, const std::string& s) {
    if (s.empty()) return;
    const int w = canvas.textWidth(trakr::hw::FwFont::Light17, s);
    canvas.text(trakr::hw::FwFont::Light17, centerX - w / 2, yTop, color, s);
}

} // namespace

void MultiLabelControl::draw(Canvas& canvas) const {
    const ImU32* colors = state_.editing ? kValueEditColors : kValueNormalColors;
    const RectI& r = rect_;
    const int ccHeight = kMultiCcHeight;
    const int perfY = r.y + ccHeight + kMultiFxHole;
    const int perfH = r.h - ccHeight - kMultiFxHole;

    canvas.fillRect(RectI{r.x, r.y, r.w, ccHeight}, colors[1]);
    canvas.fillRect(RectI{r.x, perfY, r.w, perfH}, colors[1]);

    // controlStyleBottomShadow: background -> transparent over the last 12px.
    const RectI shadow{r.x, r.y + r.h - 10, r.w + 1, 12};
    canvas.vGradient(shadow, colors[1], IM_COL32(0, 0, 0, 255));

    drawCentered(canvas, r.x + r.w / 2, r.y + kMultiTextOffset / 2, colors[0], state_.topText);
    const int lineY = r.y + ccHeight + kMultiFxHole + kMultiTextOffset / 2;
    for (int i = 0; i < 4; ++i) {
        const ImU32 color = (i == state_.activeRow) ? colors[2] : colors[0];
        drawCentered(canvas, r.x + r.w / 2, lineY + i * kMultiTextStep, color,
                     state_.labels[static_cast<size_t>(i)]);
    }
}

bool MultiLabelControl::onPress(int deviceX, int deviceY) {
    if (!contains(deviceX, deviceY)) return false;
    const int topEnd = rect_.y + kMultiCcHeight;
    if (deviceY < topEnd) {
        if (onTop_) onTop_();
        return true;
    }
    const int rowY = rect_.y + kMultiCcHeight + kMultiFxHole;
    const int row = (deviceY - rowY) / 30;
    if (row >= 0 && row < 4 && deviceY < rowY + 4 * 30) {
        if (onRow_) onRow_(row);
        return true;
    }
    return false;
}

void VerticalLabelControl::draw(Canvas& canvas) const {
    canvas.textVertical270(trakr::hw::FwFont::Light17, rect_.x, bottomY_, color_, text_);
}

} // namespace trakr::ui

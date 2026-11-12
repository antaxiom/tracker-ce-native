#include "param_value_list.h"

#include "ui/controls/primitives.h"
#include "ui/design.h"

#include <algorithm>

namespace trakr::ui {

ParamValueListControl::ParamValueListControl(std::string id, int visibleLines)
    : Control(std::move(id)), visibleLines_(std::max(1, visibleLines)) {}

void ParamValueListControl::setRows(std::vector<Row> rows) {
    rows_ = std::move(rows);
    setSelected(selected_);
}

void ParamValueListControl::setSelected(int index) {
    if (rows_.empty()) {
        selected_ = 0;
        return;
    }
    selected_ = std::clamp(index, 0, static_cast<int>(rows_.size()) - 1);
}

int ParamValueListControl::firstRow() const {
    const int count = static_cast<int>(rows_.size());
    if (count <= visibleLines_) return 0;
    // cParamValueList textListPos: anchor stays at row 0 until the selection
    // passes the second-to-last visible row, then the window scrolls.
    if (start_ >= 0)
        return std::clamp(start_ - std::max(0, visibleLines_ - 2), 0, count - visibleLines_);
    return std::clamp(selected_ - (visibleLines_ - 1), 0, count - visibleLines_);
}

void ParamValueListControl::draw(Canvas& canvas) const {
    const RectI& r = rect_;
    canvas.fillRect(r, kActiveListColors[3]);
    drawBottomShadow(canvas, r, kActiveListColors[3]);

    const int count = static_cast<int>(rows_.size());
    if (count <= 0) return;

    const int top = r.y + kListTopInset;
    const int visible = std::min(visibleLines_, count);
    const int first = firstRow();
    const int wFrame = r.w - 6;
    const int barPos = selected_ - first;
    const int h = canvas.textHeight(trakr::hw::FwFont::Light14);

    if (barPos >= 0 && barPos < visible) {
        canvas.strokeRectCentered(RectI{r.x + 3, top + barPos * kListRowHeight, wFrame,
                                        kListRowHeight},
                                  kActiveListColors[0], kFrameLineWidth / 16.0f);
    }

    canvas.pushClip(RectI{r.x, top, wFrame, kListRowHeight * visibleLines_});
    for (int i = 0; i < visible; ++i) {
        const int index = first + i;
        const ImU32 color = kActiveListColors[index == selected_ ? 5 : 4];
        const int centerY = top + kListRowHeight / 2 + i * kListRowHeight;
        const Row& row = rows_[static_cast<size_t>(index)];
        canvas.text(trakr::hw::FwFont::Light14, r.x + 13, centerY - h / 2, color, row.first);
        const int valueWidth = canvas.textWidth(trakr::hw::FwFont::Light14, row.second);
        canvas.text(trakr::hw::FwFont::Light14, r.right() - 10 - valueWidth, centerY - h / 2,
                    color, row.second);
    }
    canvas.popClip();
}

bool ParamValueListControl::onPress(int deviceX, int deviceY) {
    if (!contains(deviceX, deviceY) || rows_.empty()) return false;
    const int relativeY = deviceY - (rect_.y + kListTopInset);
    if (relativeY < 0) return false;
    const int row = relativeY / kListRowHeight;
    if (row >= std::min(visibleLines_, static_cast<int>(rows_.size()))) return false;
    if (deviceX < rect_.x + 3 || deviceX >= rect_.x + rect_.w - 6) return false;

    const int first = firstRow();
    setSelected(first + row);
    if (start_ >= 0) start_ = selected_;
    if (onSelect_) onSelect_(selected_);
    return true;
}

} // namespace trakr::ui

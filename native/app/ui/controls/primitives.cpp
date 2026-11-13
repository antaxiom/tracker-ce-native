#include "primitives.h"

#include <algorithm>
#include <cmath>

namespace trakr::ui {
namespace {

void drawRowText(Canvas& canvas, int x, int centerY, ImU32 color, const std::string& s) {
    const int h = canvas.textHeight(trakr::hw::FwFont::Light14);
    canvas.text(trakr::hw::FwFont::Light14, x, centerY - h / 2, color, s);
}

} // namespace

void drawBottomShadow(Canvas& canvas, const RectI& rect, ImU32 background) {
    // cBar/cList use BLEND_FUNC(SRC_ALPHA, ZERO): the gradient overwrites the
    // destination and ends in black. One column wider than the control.
    const RectI shadow{rect.x, rect.bottom() - 10, rect.w + 1, 12};
    canvas.vGradient(shadow, background, IM_COL32(0, 0, 0, 255));
}

FrameControl::FrameControl(std::string id, std::vector<RectI> places, int selected)
    : Control(std::move(id)), places_(std::move(places)), selected_(selected) {
    setSelected(selected_);
}

void FrameControl::setSelected(int index) {
    if (places_.empty()) {
        selected_ = 0;
        return;
    }
    selected_ = std::clamp(index, 0, static_cast<int>(places_.size()) - 1);
}

void FrameControl::draw(Canvas& canvas) const {
    if (places_.empty() || selected_ < 0 || selected_ >= static_cast<int>(places_.size())) return;
    canvas.strokeRectCentered(places_[selected_], kFrameColors[0], kFrameLineWidth / 16.0f);
}

BarControl::BarControl(std::string id, int column, Mode mode)
    : Control(std::move(id)), column_(column), mode_(mode) {}

void BarControl::draw(Canvas& canvas) const {
    const ImU32* c = colors_ ? colors_ : (inactive_ ? kInactiveBarColors : kActiveBarColors);
    const RectI& r = rect_;

    const int barWidth = r.w - 20;
    const int barX = r.x + (r.w - barWidth) / 2;
    const int barY = r.y + 10;
    const int barHeight = r.h - 25;

    canvas.fillRect(r, c[4]);
    drawBottomShadow(canvas, r, c[4]);
    if (mode_ == Mode::LeftRight || (mode_ == Mode::Fill && value_ >= 0) ||
        (mode_ == Mode::Compare && value_ >= 0)) {
        canvas.fillRect(RectI{r.x + 10, r.y + 11, r.w - 20, r.h - 25}, c[3]);
        canvas.hGradient(RectI{r.x + 10, r.y + 11, 8, r.h - 25}, IM_COL32(0, 0, 0, 255), c[3]);
    }

    if (mode_ == Mode::Compare) {
        if (value_ < 0 || value_ > 100 || compare_ < 0 || compare_ > 100) return;
        // cBar controlStyleCompareTwoValues: current value filled solid, the
        // delta to the compared value drawn in colours[1] (up) / colours[2] (down).
        const int barFillY = barHeight - (barHeight * value_) / 100;
        const int valueSub = compare_ - value_;
        const int absSub = (valueSub >= 0) ? valueSub : -valueSub;
        const int compareBarHeight = (barHeight * absSub) / 100;
        canvas.fillRect(RectI{barX + 1,
                              (valueSub < 0) ? barY + barFillY + compareBarHeight
                                             : barY + barFillY,
                              barWidth - 2, barHeight - ((valueSub < 0) ? barFillY + compareBarHeight
                                                                        : barFillY)},
                        c[0]);
        if (valueSub > 0) {
            canvas.fillRect(RectI{barX, barY + barFillY - compareBarHeight,
                                  barWidth - 1, compareBarHeight},
                            c[1]);
        } else if (valueSub < 0) {
            canvas.fillRect(RectI{barX + 1, barY + barFillY, barWidth - 2, compareBarHeight},
                            c[2]);
        }
        return;
    }

    if (mode_ == Mode::LeftRight) {
        if (value_ < -100 || value_ > 100) return;
        const int half = barHeight / 2;
        const int top = (barY + 1) + (value_ > 0 ? ((half * (100 - value_)) / 100) : half);
        const int bot = barY + barHeight - (value_ < 0 ? ((half * (100 + value_)) / 100) : half);
        if (bot > top) {
            canvas.fillRect(RectI{barX + 1, top, barWidth - 2, bot - top}, c[0]);
        }
        return;
    }

    if (value_ < 0 || value_ > 100) return;
    const int fillY = barHeight - (barHeight * value_) / 100;
    canvas.fillRect(RectI{barX + 1, barY + fillY, barWidth - 2, barHeight - fillY}, c[0]);
}

bool BarControl::onPress(int deviceX, int deviceY) {
    if (!contains(deviceX, deviceY)) return false;
    if (onPress_) onPress_(column_);
    return true;
}

ListControl::ListControl(std::string id, std::vector<std::string> items, int visibleLines)
    : Control(std::move(id)), items_(std::move(items)), visibleLines_(std::max(1, visibleLines)) {
    setSelected(0);
}

void ListControl::setSelected(int index) {
    if (items_.empty()) {
        selected_ = 0;
        return;
    }
    selected_ = std::clamp(index, 0, static_cast<int>(items_.size()) - 1);
}

void ListControl::setItems(std::vector<std::string> items) {
    items_ = std::move(items);
    setSelected(selected_);
}

int ListControl::firstRow() const {
    const int count = static_cast<int>(items_.size());
    if (count <= visibleLines_) return 0;
    // cList anchor semantics: textListPos stays 0 until the anchor passes the
    // second-to-last row, then the list scrolls to keep the last rows visible.
    if (start_ >= 0)
        return std::clamp(start_ - std::max(0, visibleLines_ - 2), 0, count - visibleLines_);
    return std::clamp(selected_ - (visibleLines_ - 1), 0, count - visibleLines_);
}

void ListControl::draw(Canvas& canvas) const {
    const RectI& r = rect_;
    const ImU32* c = colors_ ? colors_ : kActiveListColors;
    const ImU32 bg = c[3];
    canvas.fillRect(r, bg);
    drawBottomShadow(canvas, r, bg);

    const int count = static_cast<int>(items_.size());
    if (count <= 0) return;

    const int top = r.y + kListTopInset;
    const int visible = std::min(visibleLines_, count);
    const int first = firstRow();
    const int barPos = selected_ - first;
    const int wBar = (count > visibleLines_) ? r.w - 13 : r.w - 6;

    if (barPos >= 0 && barPos < visible) {
        canvas.strokeRectCentered(
            RectI{r.x + 3, top + barPos * kListRowHeight, wBar, kListRowHeight},
            c[0], kFrameLineWidth / 16.0f);
    }

    for (int i = 0; i < visible; ++i) {
        const int index = first + i;
        const ImU32 color = c[index == selected_ ? 5 : 4];
        drawRowText(canvas, r.x + 13, top + kListRowHeight / 2 + i * kListRowHeight, color,
                    items_[static_cast<size_t>(index)]);
    }

    if (count > visibleLines_) {
        const int innerH = r.h - 14;
        const int length = std::max(1, (innerH * visibleLines_) / count);
        const int thumbY = r.y + 6 + (first * (innerH - length)) / count;
        canvas.fillRect(RectI{r.x + r.w - 6, r.y + 4, 3, innerH}, c[6]);
        canvas.fillRect(RectI{r.x + r.w - 5, thumbY, 2, length}, c[2]);
    }
}

bool ListControl::onPress(int deviceX, int deviceY) {
    if (!contains(deviceX, deviceY) || items_.empty()) return false;

    const int count = static_cast<int>(items_.size());
    const int visible = std::min(visibleLines_, count);
    const int first = firstRow();
    const int relativeY = deviceY - (rect_.y + kListTopInset);
    if (relativeY < 0) return false;
    const int row = relativeY / kListRowHeight;
    const int wBar = (count > visibleLines_) ? rect_.w - 13 : rect_.w - 6;
    if (row >= visible) return false;
    if (deviceX < rect_.x + 3 || deviceX >= rect_.x + 3 + wBar) return false;

    setSelected(first + row);
    if (start_ >= 0) start_ = selected_;
    if (onSelect_) onSelect_(selected_);
    return true;
}

void BlankCoverControl::draw(Canvas& canvas) const {
    const ImU32 bg = kBlankSpaceCoverColors[1];
    canvas.fillRect(rect_, bg);
    drawBottomShadow(canvas, rect_, bg);
}

LabelControl::LabelControl(std::string id, std::string text, trakr::hw::FwFont font)
    : Control(std::move(id)), text_(std::move(text)), font_(font) {}

void LabelControl::draw(Canvas& canvas) const {
    if (showBackground_) canvas.fillRect(rect_, bgColor_);
    if (showFrame_) canvas.strokeRectCentered(rect_, frameColor_, kFrameLineWidth / 16.0f);
    canvas.textCentered(font_, rect_, textColor_, text_);
}

} // namespace trakr::ui

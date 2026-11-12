#pragma once

// cMultiLabel semantic port (Performance FX value cell) and the vertical
// cLabel used for FX names.
//
// Firmware references:
//   core/libs/NewDisplay/multiLabel.cpp (cMultiLabel)
//   core/libs/NewDisplay/label.cpp      (cLabel controlStyleVerticalText)

#include "ui/control.h"
#include <array>
#include <functional>
#include <string>

namespace trakr::ui {

struct MultiLabelState {
    std::array<std::string, 4> labels{};
    int activeRow{0};
    std::string topText{"---"};
    bool editing{false};
};

class MultiLabelControl : public Control {
public:
    explicit MultiLabelControl(std::string id) : Control(std::move(id)) {}
    void setState(const MultiLabelState& state) { state_ = state; }
    void setOnTop(std::function<void()> cb) { onTop_ = std::move(cb); }
    void setOnRow(std::function<void(int)> cb) { onRow_ = std::move(cb); }

    void draw(Canvas& canvas) const override;
    bool acceptsInput() const override { return true; }
    bool onPress(int deviceX, int deviceY) override;

private:
    MultiLabelState state_{};
    std::function<void()> onTop_;
    std::function<void(int)> onRow_;
};

class VerticalLabelControl : public Control {
public:
    explicit VerticalLabelControl(std::string id) : Control(std::move(id)) {}
    void setText(std::string text) { text_ = std::move(text); }
    void setColor(ImU32 color) { color_ = color; }
    void setBottomY(int y) { bottomY_ = y; }

    void draw(Canvas& canvas) const override;

private:
    std::string text_;
    ImU32 color_{kWhite};
    int bottomY_{244};
};

} // namespace trakr::ui

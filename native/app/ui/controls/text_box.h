#pragma once

// cTextBox (core/libs/NewDisplay/textBox.cpp): word-wrapped text with
// newline support, drawn at x+5 / y+5 + line * (fontHeight + value).
// Used by the Config item editor description and the Info credits page.

#include "ui/controls/primitives.h"
#include "hw/BitmapText.h"
#include <string>

namespace trakr::ui {

class TextBoxControl : public Control {
public:
    TextBoxControl(std::string id, trakr::hw::FwFont font, int extraLineSpacing = 3)
        : Control(std::move(id)), font_(font), spacing_(extraLineSpacing) {}

    void setText(std::string text) { text_ = std::move(text); }
    const std::string& text() const { return text_; }
    void setColor(ImU32 color) { color_ = color; }

    void draw(Canvas& canvas) const override;

private:
    // cTextBox::getNextLineLength: characters in the line starting at `start`,
    // -1 to stop (no usable break inside the firmware's 30-space window).
    int nextLineLength(const Canvas& canvas, size_t start) const;

    std::string text_;
    trakr::hw::FwFont font_;
    int spacing_{3};
    ImU32 color_{kWhite};
};

} // namespace trakr::ui

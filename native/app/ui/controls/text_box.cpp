#include "text_box.h"

#include <algorithm>
#include <cstring>

namespace trakr::ui {

namespace {
size_t remaining(const std::string& text, size_t pos) {
    return pos >= text.size() ? 0 : text.size() - pos;
}
} // namespace

int TextBoxControl::nextLineLength(const Canvas& canvas, size_t start) const {
    if (start >= text_.size()) return -1;

    const int maxTextWidth = rect_.w - 5;
    const char* position = text_.c_str() + start;
    size_t scan = start;
    size_t lastLength = 0;

    for (int whiteCharCounter = 0; whiteCharCounter < 30; ++whiteCharCounter) {
        const char* tempPos = text_.c_str() + scan;
        const char* enterPos = std::strchr(tempPos, '\n');
        const size_t enterIdx =
            lastLength + (enterPos != nullptr ? static_cast<size_t>(enterPos - tempPos)
                                              : std::strlen(position));

        if (enterIdx == 0) return 0;

        const char* spacePos = std::strchr(tempPos, ' ');
        const size_t spaceIdx =
            lastLength + (spacePos != nullptr ? static_cast<size_t>(spacePos - tempPos)
                                              : std::strlen(position));

        if (enterIdx <= spaceIdx) {
            const size_t length = std::min(enterIdx, remaining(text_, start));
            if (canvas.textWidth(font_, std::string_view(position, length)) > maxTextWidth &&
                whiteCharCounter > 0) {
                return static_cast<int>(lastLength) - 1;
            }
            return static_cast<int>(enterIdx);
        }

        const size_t length = std::min(spaceIdx, remaining(text_, start));
        if (canvas.textWidth(font_, std::string_view(position, length)) > maxTextWidth) {
            if (whiteCharCounter > 0) return static_cast<int>(lastLength) - 1;
            return static_cast<int>(spaceIdx);
        }

        lastLength = spaceIdx + 1; // skip the white character for the next scan
        scan = start + lastLength;
    }

    return -1;
}

void TextBoxControl::draw(Canvas& canvas) const {
    if (text_.empty() || rect_.w <= 5) return;

    const int fontHeight = canvas.textHeight(font_);
    const int linesStep = fontHeight + spacing_;
    if (linesStep <= 0) return;
    const int linesMax = (rect_.h / linesStep) - 1;

    size_t position = 0;
    for (int line = 0; line < linesMax; ++line) {
        const int length = nextLineLength(canvas, position);
        if (length > 0) {
            const size_t chars = std::min(static_cast<size_t>(length), remaining(text_, position));
            canvas.text(font_, rect_.x + 5, rect_.y + 5 + line * linesStep, color_,
                        std::string_view(text_.data() + position, chars));
        } else if (length < 0) {
            break;
        }

        position += static_cast<size_t>(length) + 1;
        if (position > text_.size()) break;
    }
}

} // namespace trakr::ui

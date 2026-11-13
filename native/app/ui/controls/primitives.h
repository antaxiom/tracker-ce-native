#pragma once

// Reusable controls ported from NewDisplay usage. These are semantic ports:
// firmware geometry and painting semantics are reproduced, but each control is
// laid out by the host LayoutContext instead of a fixed framebuffer.

#include "ui/control.h"
#include <functional>
#include <string>
#include <vector>

namespace trakr::ui {

// cFrame: one red 2px frame around the active place.
class FrameControl : public Control {
public:
    FrameControl(std::string id, std::vector<RectI> places, int selected = 0);
    void draw(Canvas& canvas) const override;
    void setSelected(int index);
    int selected() const { return selected_; }

private:
    std::vector<RectI> places_;
    int selected_{0};
};

// cBar: container + bottom shadow + fill. Value is 0..100 (Fill) or -100..100
// (LeftRight). `inactive` swaps in inactiveBarColors, matching cBar's state.
// Compare mirrors controlStyleCompareTwoValues (memory usage bar).
class BarControl : public Control {
public:
    enum class Mode { Fill, LeftRight, Compare };

    BarControl(std::string id, int column, Mode mode = Mode::Fill);
    void setValue(int value) { value_ = value; }
    int value() const { return value_; }
    void setCompareValue(int value) { compare_ = value; }
    void setInactive(bool inactive) { inactive_ = inactive; }
    // Custom 5-entry palette (fill, compare-up, compare-down, container, background).
    void setColors(const ImU32* colors) { colors_ = colors; }
    void setColumn(int column) { column_ = column; }
    int column() const { return column_; }
    void setOnPress(std::function<void(int)> cb) { onPress_ = std::move(cb); }

    void draw(Canvas& canvas) const override;
    bool acceptsInput() const override { return true; }
    bool onPress(int deviceX, int deviceY) override;

private:
    int column_{0};
    int value_{0};
    int compare_{0};
    Mode mode_{Mode::Fill};
    bool inactive_{false};
    const ImU32* colors_{nullptr};
    std::function<void(int)> onPress_;
};

// cList: background, 27px rows, selection frame, scrollbar. Selection is
// clamped and scrolled into view exactly like cList's visible window.
class ListControl : public Control {
public:
    ListControl(std::string id, std::vector<std::string> items, int visibleLines);
    void setSelected(int index);
    int selected() const { return selected_; }
    void setItems(std::vector<std::string> items);
    // Firmware list->start: first row drawn at the top. When unset (negative)
    // the list scrolls to keep the selection visible.
    void setStart(int start) { start_ = start; }
    int start() const { return start_; }
    // Custom 7-entry palette (activeListColors layout).
    void setColors(const ImU32* colors) { colors_ = colors; }
    void setOnSelect(std::function<void(int)> cb) { onSelect_ = std::move(cb); }

    void draw(Canvas& canvas) const override;
    bool acceptsInput() const override { return true; }
    bool onPress(int deviceX, int deviceY) override;

private:
    int firstRow() const;
    std::vector<std::string> items_;
    int visibleLines_{5};
    int selected_{0};
    int start_{-1};
    const ImU32* colors_{nullptr};
    std::function<void(int)> onSelect_;
};

// cLabel with Background | BottomShadow, used as the blank-space cover.
class BlankCoverControl : public Control {
public:
    explicit BlankCoverControl(std::string id) : Control(std::move(id)) {}
    void draw(Canvas& canvas) const override;
};

// cLabel with centered text, used for the firmware bottom button labels.
class LabelControl : public Control {
public:
    LabelControl(std::string id, std::string text, trakr::hw::FwFont font);
    void setText(std::string text) { text_ = std::move(text); }
    void setColors(ImU32 text, ImU32 background, ImU32 frame) {
        textColor_ = text;
        bgColor_ = background;
        frameColor_ = frame;
    }
    void setShowBackground(bool show) { showBackground_ = show; }
    void setShowFrame(bool show) { showFrame_ = show; }
    // cLabel without controlStyleCenterX: text at posX /
    // posY + (18 - fontHeight / 2) instead of centred in the rect.
    void setAlignLeft(bool alignLeft) { alignLeft_ = alignLeft; }

    void draw(Canvas& canvas) const override;

private:
    std::string text_;
    trakr::hw::FwFont font_;
    ImU32 textColor_{kWhite};
    ImU32 bgColor_{kGrey32};
    ImU32 frameColor_{kOneTrueRed};
    bool showBackground_{true};
    bool showFrame_{false};
    bool alignLeft_{false};
};

// Shared cBar/cBgLabel bottom shadow: 12px vertical fade
// (bar.cpp/list.cpp/songPlayer.cpp: grad_y = posY + height - 10, grad_h = 12).
// The firmware only sets controlStyleBottomShadow when the theme's
// bottomScreenShadow > 0 and controlStyleGradient when barShadow > 0
// (gSongEdit.cpp:33-34, gInstrEdit.cpp), so the desktop honors the same flags.
void setChromeShadowFlags(bool bottomShadow, bool barShadow);
bool bottomShadowEnabled();
bool barShadowEnabled();
void drawBottomShadow(Canvas& canvas, const RectI& rect, ImU32 background);

} // namespace trakr::ui

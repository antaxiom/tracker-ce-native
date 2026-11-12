#pragma once

// Base control for the semantic UI. Controls mirror firmware NewDisplay
// control responsibilities: geometry in device pixels, value binding, draw,
// and hit-test. They do not know about ImGui windows or the host layout.

#include "canvas.h"
#include "layout.h"
#include <string>
#include <string_view>

namespace trakr::ui {

class Control {
public:
    explicit Control(std::string id) : id_(std::move(id)) {}
    virtual ~Control() = default;

    const std::string& id() const { return id_; }
    const RectI& rect() const { return rect_; }
    void setRect(const RectI& r) { rect_ = r; }
    void setVisible(bool visible) { visible_ = visible; }
    bool visible() const { return visible_; }

    virtual void draw(Canvas& canvas) const = 0;

    // NewDisplay runs several update passes per control (update/update2/update3)
    // and composites controls in reverse creation order *within each pass*.
    // `layer` models that pass ordering; controls that must paint over another
    // control (e.g. waveform markers over the spectrum) raise their layer.
    virtual void drawLayer(Canvas& canvas, int layer) const {
        if (layer == 0) draw(canvas);
    }

    // True for controls that participate in hit-testing (bars, lists).
    virtual bool acceptsInput() const { return false; }

    // Routes a device-space press. Returns true when the control consumed it.
    virtual bool onPress(int deviceX, int deviceY) {
        (void)deviceX;
        (void)deviceY;
        return false;
    }

    bool contains(int deviceX, int deviceY) const {
        return visible_ && rect_.contains(deviceX, deviceY);
    }

protected:
    std::string id_;
    RectI rect_{};
    bool visible_{true};
};

} // namespace trakr::ui

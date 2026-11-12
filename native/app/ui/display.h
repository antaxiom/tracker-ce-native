#pragma once

// Screen control table mirroring cDisplay semantics:
//   - controls are stored in creation order
//   - renderAll() composites in reverse creation order (cDisplay::step3)
//   - hitTest() walks front-to-back (first created is frontmost)

#include "control.h"
#include <memory>
#include <type_traits>
#include <vector>

namespace trakr::ui {

class Display {
public:
    template <typename T, typename... Args>
    T& add(Args&&... args) {
        static_assert(std::is_base_of_v<Control, T>, "T must derive from Control");
        auto control = std::make_unique<T>(std::forward<Args>(args)...);
        T& ref = *control;
        controls_.push_back(std::move(control));
        return ref;
    }

    void clear() { controls_.clear(); }
    size_t size() const { return controls_.size(); }

    void renderAll(Canvas& canvas) const {
        for (auto it = controls_.rbegin(); it != controls_.rend(); ++it) {
            if ((*it)->visible()) (*it)->draw(canvas);
        }
    }

    Control* hitTest(int deviceX, int deviceY) const {
        for (const auto& control : controls_) {
            if (control->acceptsInput() && control->contains(deviceX, deviceY)) return control.get();
        }
        return nullptr;
    }

    Control* find(std::string_view id) const {
        for (const auto& control : controls_) {
            if (control->id() == id) return control.get();
        }
        return nullptr;
    }

private:
    std::vector<std::unique_ptr<Control>> controls_;
};

} // namespace trakr::ui

#pragma once

namespace trakr {

class View {
public:
    virtual ~View() = default;
    virtual const char* name() const = 0;
    virtual void draw(bool* p_open = nullptr) = 0;
};

} // namespace trakr


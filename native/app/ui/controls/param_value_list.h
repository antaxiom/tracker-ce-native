#pragma once

// cParamValueList semantic port (Config submenu).
//
// Firmware reference: core/libs/NewDisplay/paramValueList.cpp.
// Two-column rows (left name, right-aligned value), 27px rows, selection
// frame and bottom row shadow. No scrollbar is drawn by the control.

#include "ui/control.h"
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace trakr::ui {

class ParamValueListControl : public Control {
public:
    using Row = std::pair<std::string, std::string>;

    ParamValueListControl(std::string id, int visibleLines);

    void setRows(std::vector<Row> rows);
    void setSelected(int index);
    int selected() const { return selected_; }
    // cParamValueList textListPos anchor; negative keeps the selection visible.
    void setStart(int start) { start_ = start; }
    void setOnSelect(std::function<void(int)> cb) { onSelect_ = std::move(cb); }

    void draw(Canvas& canvas) const override;
    bool acceptsInput() const override { return true; }
    bool onPress(int deviceX, int deviceY) override;

private:
    int firstRow() const;
    std::vector<Row> rows_;
    int visibleLines_{14};
    int selected_{0};
    int start_{-1};
    std::function<void(int)> onSelect_;
};

} // namespace trakr::ui

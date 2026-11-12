#pragma once

#include "imgui.h"
#include <string>
#include <vector>
#include <functional>

namespace trakr {

struct SoftKeyDef {
    std::string keyLabel; // e.g. "F1"
    std::string title;    // e.g. "PATTERN"
    bool active{false};
    std::function<void()> onClick;
};

// Renders the signature 8 Polyend hardware soft-keys pinned along the bottom of the screen
void renderPolyendSoftKeys(const std::vector<SoftKeyDef>& keys, float height = 30.0f);

} // namespace trakr


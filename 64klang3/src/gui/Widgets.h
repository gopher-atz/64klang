#pragma once

#include "imgui.h"
#include "NodeConfig.h"
#include <string>

// Custom ImGui widgets for 64klang3

namespace K64GUI {
namespace Widgets {

// Two-line display label (line2 is empty for single-line values)
struct KnobLabel {
    std::string line1;
    std::string line2;
};

// Format a knob value for display based on mapping type
KnobLabel formatKnobValue(double normValue, double range, int mapping, int currentMode, int nodeTypeID);

// Draw a knob (tick ring + circle body + value needle + optional mod needle).
// normVal and normMod must be in [0,1]. alpha (0-255) scales all knob primitives.
void drawKnob(ImDrawList* dl, ImVec2 center, float bodyR, float knobR,
              float needleTipR, float normVal, float normMod, bool showMod,
              unsigned int alpha, float z);

} // namespace Widgets
} // namespace K64GUI

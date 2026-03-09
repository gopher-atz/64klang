#pragma once

#include "imgui.h"
#include "NodeConfig.h"
#include <string>

// Custom ImGui widgets for 64klang3

namespace K64GUI {
namespace Widgets {

// Stereo knob widget with sync checkbox
// Returns true if value changed
bool Knob(const char* id, const InputDef& inputDef, float* valueL, float* valueR,
          float modL, float modR, bool synced, bool singleInput, int nodeTypeID, int currentMode);

// VU meter display (two vertical bars L/R)
void VUMeter(float levelL, float levelR, float width, float height);

// Bit pattern toggle buttons (8 per row for L and R)
bool BitPattern(const char* label, unsigned int* pattern);

// Two-line display label (line2 is empty for single-line values)
struct KnobLabel {
    std::string line1;
    std::string line2;
};

// Format a knob value for display based on mapping type
KnobLabel formatKnobValue(double normValue, double range, int mapping, int currentMode, int nodeTypeID);

} // namespace Widgets
} // namespace K64GUI

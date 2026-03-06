#pragma once

// Custom ImGui widgets for 64klang3
// Knob, VUMeter, BitPattern, Arp editor, etc.

namespace K64GUI {
namespace Widgets {

// Stereo knob widget with sync checkbox
// Returns true if value changed
bool Knob(const char* label, float* valueL, float* valueR, float minVal, float maxVal, bool* synced = nullptr);

// VU meter display
void VUMeter(const char* label, float levelL, float levelR);

// Bit pattern toggle buttons (8 per row)
bool BitPattern(const char* label, unsigned int* pattern);

} // namespace Widgets
} // namespace K64GUI

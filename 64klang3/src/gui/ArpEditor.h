#pragma once

#include "imgui.h"

namespace K64GUI {

// 32-step arpeggiator piano-roll editor for VoiceManager nodes
class ArpEditor
{
public:
    // Draw the arpeggiator editor for the given node
    // Returns true if any data was modified
    static bool draw(int nodeID);

    // Grid dimensions (screen pixels, zoom-independent)
    static constexpr int   kMaxSteps = 32;
    static constexpr int   kOctaves = 4;
    static constexpr int   kNotesPerOctave = 12;
    static constexpr float kStepWidth = 14.f;
    static constexpr float kNoteHeight = 8.f;

private:

    // Step data packing: transpose (6 bits), gate length (2 bits), velocity (8 bits)
    static int getTranspose(int stepData)  { return stepData & 0x3F; }
    static int getGateLen(int stepData)    { return (stepData >> 6) & 0x03; }
    static int getVelocity(int stepData)   { return (stepData >> 8) & 0xFF; }
    static int packStepData(int transpose, int gateLen, int velocity)
    {
        return (transpose & 0x3F) | ((gateLen & 0x03) << 6) | ((velocity & 0xFF) << 8);
    }
};

} // namespace K64GUI

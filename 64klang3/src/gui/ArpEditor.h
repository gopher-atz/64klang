#pragma once

#include "imgui.h"
#include <vector>
#include <unordered_map>

namespace K64GUI {

// One note span in the arpeggiator sequence.
// Mirrors the C# ArpStep data fields exactly.
struct ArpStep
{
    int startStep;           // first grid step this note occupies (0..31)
    int deltaStepFractions;  // total duration in sub-fractions (1 sub-frac = kSubW px; 4 = 1 full step)
    int transpose;           // internal 0-48  (display semitone = transpose-24, range -24..+24)
    int velocity;            // 1-127
};

struct ArpEditorState
{
    std::vector<ArpStep> steps;
    int  loopStart = 0;
    bool loaded    = false;

    // ── main-grid drag (creating / resizing a new step) ──────────────────────
    bool  isDragging     = false;
    float dragStartX     = 0.f;   // screen-space X at click start; set to -1 after dead-zone cleared
    int   dragStartStep  = 0;     // integer step index at click
    int   dragStepIdx    = -1;    // index into steps[] of the step under creation

    // ── transpose-label drag ──────────────────────────────────────────────────
    int   transposeDragIdx    = -1;
    int   transposeStart      = 0;
    float transposeDragStartY = 0.f;

    // ── velocity-label drag ───────────────────────────────────────────────────
    int   velocityDragIdx    = -1;
    int   velocityStart      = 0;
    float velocityDragStartY = 0.f;
};

// ─── ArpEditor ───────────────────────────────────────────────────────────────
// Stateful ImGui arpeggiator step editor replicating the WPF ArpeggiatorEdit
// control 1:1 in layout, interaction model, and data encoding.
//
// Layout (at zoom = 1.0):
//   Row 0  StepGrid     384 × 144 px   (6 zones × 24px tall, 32 columns × 12px wide)
//   Row 1  TransposeGrid 384 × 24 px   transpose labels, drag-to-adjust semitone
//   Row 2  VelocityGrid  384 × 24 px   velocity labels, drag-to-adjust velocity
//
// Data encoding  (same as C# WriteData / ReadData):
//   bits  5:0  =  (internalTranspose + 8) & 0x3F    → stored range 8..56
//   bits  7:6  =  (gate-1) & 0x3                    → gate 1..4 sub-fractions
//   bits 15:8  =  velocity (0 = hold continuation, 1-127 = note start)
class ArpEditor
{
public:
    // ── layout constants (reference pixel sizes at zoom = 1) ─────────────────
    static constexpr int   kMaxSteps = 32;
    static constexpr float kSubW     =  3.0f;   // pixels per sub-fraction
    static constexpr float kStepW    = 12.0f;   // pixels per step  (= 4 * kSubW)
    static constexpr float kZoneH    = 24.0f;   // pixels per octave zone
    static constexpr float kGridH    = 144.0f;  // main grid height  (= 6 * kZoneH)
    static constexpr float kLabelH   = 24.0f;   // transpose / velocity label row height
    static constexpr float kTotalW   = 384.0f;  // = kMaxSteps * kStepW
    static constexpr float kTotalH   = 192.0f;  // = kGridH + 2 * kLabelH

    // Draw the editor at 'origin' in screen space, scaled by 'zoom'.
    // labelFont     – font to use for transpose/velocity label text; pass the
    //                 result of pickFont() so the text stays sharp when zooming.
    //                 Falls back to ImGui::GetFont() when nullptr.
    // labelFontSize – rendered pixel size for that font (e.g. fontSize*0.9f).
    //                 Falls back to a zoom-scaled default when 0.
    // canInteract   – pass false when another panel is on top (suppresses clicks).
    // Returns true when any step data was written to SynthController.
    static bool draw(int nodeID, float zoom, ImVec2 origin,
                     ImFont* labelFont = nullptr, float labelFontSize = 0.f,
                     bool canInteract = true);

    // Invalidate cached editor state for a node (call after patch load).
    static void invalidate(int nodeID);
    static void invalidateAll();

private:
    static std::unordered_map<int, ArpEditorState> s_states;

    static void readData (int nodeID, ArpEditorState& state);
    static void writeData(int nodeID, const ArpEditorState& state);

    // Single-pass overlap resolution (handles one conflicting step per call).
    // Returns the possibly-adjusted index of 'thisIdx'.
    static int  validateInsertion(ArpEditorState& state, int thisIdx);

    // Multi-pass wrapper: repeats until the step list is fully stable.
    // Needed when the mouse moves fast and skips over several existing notes
    // in a single frame — each pass resolves one overlap, so we need up to
    // N passes where N is the number of steps that were in the list before
    // the drag step was inserted.
    static int  validateInsertionFully(ArpEditorState& state, int thisIdx);

    // ceil(fStep/4) - 1  – converts a sub-fraction count to the last step it touches
    static int  stepIndex(int fStep) { return (fStep > 0) ? (fStep - 1) / 4 : 0; }
};

} // namespace K64GUI

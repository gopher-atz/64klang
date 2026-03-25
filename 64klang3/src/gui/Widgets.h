#pragma once

#include "imgui.h"
#include "NodeConfig.h"
#include "core/SynthController.h"
#include <array>
#include <string>
#include <unordered_map>

// Custom ImGui widgets for 64klang3

namespace K64GUI {

// ── Shared font selector ──────────────────────────────────────────────────────
inline ImFont* pickFont(float desiredPx)
{
    auto& fv = ImGui::GetIO().Fonts->Fonts;
    if (fv.Size >= 2 && desiredPx >= fv[1]->FontSize * 0.75f)
        return fv[1];
    return fv[0];
}

// ── Shared edit-panel color constants ────────────────────────────────────────
static constexpr ImU32 kColPanelBorder  = IM_COL32(100, 100, 105, 255);
static constexpr ImU32 kColPanelText    = IM_COL32(  0,   0,   0, 255);
static constexpr ImU32 kColPanelDimText = IM_COL32( 50,  50,  55, 255);

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

// ── Specialized edit-panel section drawers ───────────────────────────────────

// Common drawing context passed to each specialized panel section.
struct EditPanelCtx {
    ImDrawList* dl;
    int         nodeID;
    float       px;        // panel screen-space left edge
    float       pw;        // panel width (already zoom-scaled)
    float       z;         // zoom factor
    float       fontSize;
    ImVec2      mousePos;
    bool        canClick;
};

// Draw the TriggerSequencer pattern grid sub-panel.
void drawTriggerSeqPanel(const EditPanelCtx& ctx, float& curY, SynthController* sc);

// Draw the TextToSpeech (SAPI) text-entry sub-panel.
// textEditBuffers is the per-node text buffer map owned by NodeCanvas.
void drawSAPIPanel(const EditPanelCtx& ctx, float& curY, SynthController* sc,
                   std::unordered_map<int, std::array<char, 4096>>& textEditBuffers);

// Draw the Signal Visualizer (VU meter / oscilloscope / raw) sub-panel.
void drawSignalVisualizerPanel(const EditPanelCtx& ctx, float& curY, SynthController* sc);

} // namespace Widgets
} // namespace K64GUI

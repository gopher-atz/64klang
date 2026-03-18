#include "ArpEditor.h"
#include "core/SynthController.h"

#include <algorithm>
#include <cstdio>
#include <cmath>

#ifdef _WIN32
#include <windows.h>
#endif

namespace K64GUI {

// ─── static storage ──────────────────────────────────────────────────────────
std::unordered_map<int, ArpEditorState> ArpEditor::s_states;

// ─── colors (matching WPF reference exactly) ─────────────────────────────────
static constexpr ImU32 kColZoneLoop   = IM_COL32( 78, 173, 173, 255); // #4EADAD  (teal loop zone)
static constexpr ImU32 kColZoneGray   = IM_COL32(128, 128, 128, 255); // Gray     (octave ±1 and ±2)
static constexpr ImU32 kColZoneMid    = IM_COL32(119, 136, 153, 255); // LightSlateGray (centre octave 0)
static constexpr ImU32 kColBeat       = IM_COL32(255, 255, 255, 128); // white 50 % beat markers
static constexpr ImU32 kColGridLine   = IM_COL32(  0,   0,   0, 255); // black grid lines
static constexpr ImU32 kColNote       = IM_COL32(238, 232, 170, 191); // PaleGoldenrod 75 %
static constexpr ImU32 kColVelBar     = IM_COL32(255, 160, 122, 128); // LightSalmon  50 %
static constexpr ImU32 kColTransLabel = IM_COL32(238, 232, 170, 255); // PaleGoldenrod (transpose label)
static constexpr ImU32 kColVelLabel   = IM_COL32(255, 160, 122, 255); // LightSalmon  (velocity label)
static constexpr ImU32 kColTransGrid  = IM_COL32(220, 220, 220, 255); // Gainsboro
static constexpr ImU32 kColVelGrid    = IM_COL32(206, 206, 206, 255); // #CECECE
static constexpr ImU32 kColPlayPos    = IM_COL32(  0,   0,   0,  77); // black 30 % play cursor
static constexpr ImU32 kColLoopTri    = IM_COL32(  0,   0,   0, 255); // black loop-start triangle
static constexpr ImU32 kColLabelBdr   = IM_COL32(  0,   0,   0, 255); // label border

void ArpEditor::invalidate(int nodeID)
{
    auto it = s_states.find(nodeID);
    if (it != s_states.end())
        it->second.loaded = false;
}

void ArpEditor::invalidateAll()
{
    for (auto& kv : s_states)
        kv.second.loaded = false;
}

// Mirrors C# ArpeggiatorEdit.ReadData() exactly.
void ArpEditor::readData(int nodeID, ArpEditorState& state)
{
    SynthController* sc = SynthController::instance();
    if (!sc) return;

    state.steps.clear();
    int curIdx = -1;

    for (int i = 0; i < kMaxSteps; i++)
    {
        int stepdata = sc->getArpStepData((DWORD)nodeID, (DWORD)i);

        int byte1    = stepdata & 0xff;
        int byte2    = (stepdata >> 8) & 0xff;
        int gate      = (byte1 >> 6) + 1;          // 1..4 sub-fractions
        int transpose = (byte1 & 0x3f) - 32;       // external semitone, nominally −24..+24

        if (byte2 != 0)
        {
            // new note — velocity in byte2
            ArpStep s;
            s.startStep          = i;
            s.deltaStepFractions = gate;
            s.transpose          = transpose + 24;  // store internally as 0..48
            s.velocity           = byte2;
            state.steps.push_back(s);
            curIdx = (int)state.steps.size() - 1;
        }
        else if (byte1 != 0 && byte2 == 0)
        {
            // hold continuation — extend previous note
            if (curIdx >= 0)
                state.steps[curIdx].deltaStepFractions += gate;
        }
    }

    // loop-start index is in the special slot -1
    state.loopStart = sc->getArpStepData((DWORD)nodeID, (DWORD)-1);
    state.loaded    = true;
}

// Mirrors C# ArpeggiatorEdit.WriteData() exactly.
void ArpEditor::writeData(int nodeID, const ArpEditorState& state)
{
    SynthController* sc = SynthController::instance();
    if (!sc) return;

    int steps[kMaxSteps] = {};

    for (const ArpStep& a : state.steps)
    {
        int i  = a.startStep;
        int fs = a.deltaStepFractions;

        // internal transpose (0-48) + 8 → stored in bits 5:0  (range 8..56, fits 6 bits)
        int transpEnc = a.transpose + 8;
        int gate      = std::min(fs - 1, 3);

        int byte1 = (transpEnc & 0x3f) | ((gate & 0x3) << 6);
        int byte2 = a.velocity;
        if (i < kMaxSteps)
            steps[i++] = byte1 | (byte2 << 8);
        fs -= 4;

        // hold cells as needed
        while (fs > 0 && i < kMaxSteps)
        {
            gate  = std::min(fs - 1, 3);
            byte1 = (transpEnc & 0x3f) | ((gate & 0x3) << 6);
            steps[i++] = byte1;   // byte2 = 0 → hold
            fs -= 4;
        }
    }

    for (int i = 0; i < kMaxSteps; i++)
        sc->setArpStepData((DWORD)nodeID, (DWORD)i, (DWORD)steps[i]);
}

// ─── validateInsertion ────────────────────────────────────────────────────────
// Mirrors C# ArpStep.ValidateInsertion() exactly.
// Returns the possibly-adjusted index of the step at thisIdx
// (may decrease when earlier elements are erased).
int ArpEditor::validateInsertion(ArpEditorState& state, int thisIdx)
{
    const int il = state.steps[thisIdx].startStep;
    const int ir = stepIndex(state.steps[thisIdx].startStep * 4
                             + state.steps[thisIdx].deltaStepFractions);

    // Iterate over a snapshot size; loop backwards so erase doesn't skip items.
    for (int i = (int)state.steps.size() - 1; i >= 0; i--)
    {
        if (i == thisIdx) continue;

        ArpStep& a = state.steps[i];
        int al = a.startStep;
        int ar = stepIndex(a.startStep * 4 + a.deltaStepFractions);

        // no overlap
        if (ir < al || il > ar) continue;

        // exact or more overlap: remove the existing step entirely
        if (il <= al && ir >= ar)
        {
            state.steps.erase(state.steps.begin() + i);
            if (i < thisIdx) thisIdx--;
            break;
        }

        // new step fully inside existing step: split it into left + right pieces
        if (il > al && ir < ar)
        {
            int oldDSF = a.deltaStepFractions;
            a.deltaStepFractions = (il - al) * 4;   // trim existing to left portion

            ArpStep rStep;
            rStep.startStep          = ir + 1;
            rStep.transpose          = a.transpose;
            rStep.deltaStepFractions = oldDSF - a.deltaStepFractions - (1 + ir - il) * 4;
            rStep.velocity           = state.steps[thisIdx].velocity;
            if (rStep.deltaStepFractions > 0)
                state.steps.push_back(rStep);
            break;
        }

        // partial overlaps: trim left or right of existing step
        if (ir < ar)
        {
            // new step's right edge is inside existing: trim left of existing
            int dsteps = ir + 1 - al;
            a.startStep          += dsteps;
            a.deltaStepFractions -= dsteps * 4;
            if (a.deltaStepFractions <= 0)
            {
                state.steps.erase(state.steps.begin() + i);
                if (i < thisIdx) thisIdx--;
            }
            break;
        }
        else if (il > al)
        {
            // new step's left edge is inside existing: trim right of existing
            a.deltaStepFractions = (il - al) * 4;
            if (a.deltaStepFractions <= 0)
            {
                state.steps.erase(state.steps.begin() + i);
                if (i < thisIdx) thisIdx--;
            }
            break;
        }
        else if (al == ar)
        {
            // single-cell degenerate step: remove
            state.steps.erase(state.steps.begin() + i);
            if (i < thisIdx) thisIdx--;
            break;
        }
    }

    return thisIdx;
}

// Repeats validateInsertion until no further changes occur. Each single-pass
// call resolves at most one overlap; looping up to kMaxSteps times ensures all
// conflicts are resolved even when the mouse moves multiple steps in one frame.
int ArpEditor::validateInsertionFully(ArpEditorState& state, int thisIdx)
{
    for (int pass = 0; pass < kMaxSteps; pass++)
        thisIdx = validateInsertion(state, thisIdx);
    return thisIdx;
}

bool ArpEditor::draw(int nodeID, float zoom, ImVec2 origin, ImFont* labelFont, float labelFontSize, bool canInteract)
{
    SynthController* sc = SynthController::instance();
    if (!sc) return false;

    ArpEditorState& state = s_states[nodeID];
    if (!state.loaded)
        readData(nodeID, state);

    bool changed = false;

    // Scaled pixel dimensions
    const float sw  = kStepW   * zoom;   // step width
    const float sub = kSubW    * zoom;   // sub-fraction width
    const float zh  = kZoneH   * zoom;   // zone height
    const float gh  = kGridH   * zoom;   // main grid height
    const float lh  = kLabelH  * zoom;   // label row height
    const float tw  = kTotalW  * zoom;   // total width
    const float cr  = 6.0f * zoom;       // corner radius for note bars

    const float transpY = origin.y + gh;        // top of TransposeGrid
    const float velY    = origin.y + gh + lh;   // top of VelocityGrid

    ImDrawList* dl   = ImGui::GetWindowDrawList();
    int playPos      = sc->getArpPlayPos((DWORD)nodeID);

    // ── clip to our total region ──────────────────────────────────────────────
    dl->PushClipRect(origin,
                     ImVec2(origin.x + tw, origin.y + gh + 2.f * lh),
                     true);

    // ═══════════════════════════════════════════════════════════════════════════
    // 1. STEP GRID BACKGROUND  (6 octave zones)
    // ═══════════════════════════════════════════════════════════════════════════
    static constexpr ImU32 zoneCols[6] = {
        kColZoneLoop,   // Y=0   → loop zone (teal)
        kColZoneGray,   // Y=24  → octave +2
        kColZoneGray,   // Y=48  → octave +1
        kColZoneMid,    // Y=72  → octave  0  (LightSlateGray)
        kColZoneGray,   // Y=96  → octave -1
        kColZoneGray,   // Y=120 → octave -2
    };
    for (int z = 0; z < 6; z++)
    {
        float y0 = origin.y + z * zh;
        dl->AddRectFilled(ImVec2(origin.x, y0),
                          ImVec2(origin.x + tw, y0 + zh),
                          zoneCols[z]);
        dl->AddRect(ImVec2(origin.x, y0),
                    ImVec2(origin.x + tw, y0 + zh),
                    kColGridLine, 0.f, 0, 0.5f);
    }

    // ── beat markers: white 50% squares at steps 0, 8, 16, 24 (top zone only) ─
    for (int b = 0; b < 4; b++)
    {
        float bx = origin.x + b * 8.f * sw;
        dl->AddRectFilled(ImVec2(bx, origin.y),
                          ImVec2(bx + sw, origin.y + zh),
                          kColBeat);
    }

    // ── per-step column dividers ───────────────────────────────────────────────
    for (int s = 0; s < kMaxSteps; s++)
    {
        float x = origin.x + s * sw;
        dl->AddRect(ImVec2(x, origin.y),
                    ImVec2(x + sw, origin.y + gh),
                    kColGridLine, 0.f, 0, 0.5f);
    }

    // ═══════════════════════════════════════════════════════════════════════════
    // 2. VELOCITY BARS (in StepGrid, anchored to grid bottom, drawn behind notes)
    //    Bottom is at Y = gridH - 0.5, bar extends upward by 0.9375 * velocity px
    // ═══════════════════════════════════════════════════════════════════════════
    for (const ArpStep& a : state.steps)
    {
        if (a.deltaStepFractions <= 0) continue;
        float x  = origin.x + (0.5f + a.startStep * kStepW) * zoom;
        float w  = (a.deltaStepFractions * kSubW - 1.f) * zoom;
        float h  = 0.9375f * a.velocity * zoom;
        float y0 = origin.y + (143.5f - 0.9375f * a.velocity) * zoom;
        float y1 = origin.y + 143.5f * zoom;
        dl->AddRectFilled(ImVec2(x, y0), ImVec2(x + w, y1), kColVelBar, cr);
    }

    // ═══════════════════════════════════════════════════════════════════════════
    // 3. NOTE BARS (in StepGrid, drawn on top of velocity bars)
    //    Y position encodes semitone: centre (transpose=24 → semitone 0) at Y=72
    // ═══════════════════════════════════════════════════════════════════════════
    for (const ArpStep& a : state.steps)
    {
        if (a.deltaStepFractions <= 0) continue;
        float x = origin.x + (0.5f + a.startStep * kStepW) * zoom;
        float y = origin.y + (0.5f + 72.f - (a.transpose - 24) * 2.f) * zoom;
        float w = (a.deltaStepFractions * kSubW - 1.f) * zoom;
        float h = 23.f * zoom;
        dl->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + h), kColNote, cr);
    }

    // ═══════════════════════════════════════════════════════════════════════════
    // 4. PLAY POSITION CURSOR
    // ═══════════════════════════════════════════════════════════════════════════
    {
        float px2 = origin.x + playPos * sw;
        dl->AddRectFilled(ImVec2(px2, origin.y),
                          ImVec2(px2 + sw, origin.y + gh),
                          kColPlayPos);
        dl->AddRect(ImVec2(px2, origin.y),
                    ImVec2(px2 + sw, origin.y + gh),
                    kColGridLine, 0.f, 0, 0.5f);
    }

    // ═══════════════════════════════════════════════════════════════════════════
    // 5. LOOP START TRIANGLE  (WPF Polygon Points="1,1 12,12 1,23")
    // ═══════════════════════════════════════════════════════════════════════════
    {
        float lx = origin.x + state.loopStart * sw;
        dl->AddTriangleFilled(
            ImVec2(lx + 1.f * zoom, origin.y +  1.f * zoom),
            ImVec2(lx + 12.f * zoom, origin.y + 12.f * zoom),
            ImVec2(lx + 1.f * zoom, origin.y + 23.f * zoom),
            kColLoopTri);
    }

    // ═══════════════════════════════════════════════════════════════════════════
    // 6. TRANSPOSE GRID + VELOCITY GRID backgrounds
    // ═══════════════════════════════════════════════════════════════════════════
    dl->AddRectFilled(ImVec2(origin.x, transpY),
                      ImVec2(origin.x + tw, transpY + lh),
                      kColTransGrid);
    dl->AddRectFilled(ImVec2(origin.x, velY),
                      ImVec2(origin.x + tw, velY + lh),
                      kColVelGrid);

    // ═══════════════════════════════════════════════════════════════════════════
    // 7. TRANSPOSE and VELOCITY LABELS (one per ArpStep)
    //    Width is rounded up to the nearest full step (matches C# UpdateStep logic)
    // ═══════════════════════════════════════════════════════════════════════════
    // Resolve font and size — use caller-supplied values when provided so the
    // labels stay sharp at any zoom level (avoids scaling a bitmap font).
    ImFont* font = labelFont ? labelFont : ImGui::GetFont();
    float fontSize = (labelFontSize > 0.f)
                     ? labelFontSize
                     : std::min(ImGui::GetFontSize(), lh * 0.65f) * zoom;

    for (const ArpStep& a : state.steps)
    {
        if (a.deltaStepFractions <= 0) continue;

        // label width: ceil(dsf/4)*4 sub-fractions → full steps
        int   dsf    = (stepIndex(a.deltaStepFractions) + 1) * 4;
        float labelW = dsf * sub;
        float lx     = origin.x + a.startStep * sw;

        // Transpose label
        {
            float x0 = lx, y0 = transpY;
            float x1 = x0 + labelW, y1 = y0 + lh;
            dl->AddRectFilled(ImVec2(x0, y0), ImVec2(x1, y1), kColTransLabel);
            dl->AddRect      (ImVec2(x0, y0), ImVec2(x1, y1), kColLabelBdr, 0.f, 0, 0.5f);
            if (fontSize >= 6.f)
            {
                char buf[8];
                snprintf(buf, sizeof(buf), "%d", a.transpose - 24);
                ImVec2 tsz = font->CalcTextSizeA(fontSize, FLT_MAX, 0.f, buf);
                dl->AddText(font, fontSize,
                            ImVec2(x0 + (labelW - tsz.x) * 0.5f,
                                   y0 + (lh    - tsz.y) * 0.5f),
                            kColGridLine, buf);
            }
        }

        // Velocity label
        {
            float x0 = lx, y0 = velY;
            float x1 = x0 + labelW, y1 = y0 + lh;
            dl->AddRectFilled(ImVec2(x0, y0), ImVec2(x1, y1), kColVelLabel);
            dl->AddRect      (ImVec2(x0, y0), ImVec2(x1, y1), kColLabelBdr, 0.f, 0, 0.5f);
            if (fontSize >= 6.f)
            {
                char buf[8];
                snprintf(buf, sizeof(buf), "%d", a.velocity);
                ImVec2 tsz = font->CalcTextSizeA(fontSize, FLT_MAX, 0.f, buf);
                dl->AddText(font, fontSize,
                            ImVec2(x0 + (labelW - tsz.x) * 0.5f,
                                   y0 + (lh    - tsz.y) * 0.5f),
                            kColGridLine, buf);
            }
        }
    }

    dl->PopClipRect();

    // ═══════════════════════════════════════════════════════════════════════════
    // 8. MOUSE INTERACTIONS
    // ═══════════════════════════════════════════════════════════════════════════
    // We use raw ImGui mouse queries rather than InvisibleButton to avoid
    // stomping on the NodeCanvas hit-test logic.  All interactions are gated
    // behind canInteract.

    ImVec2 mouse = ImGui::GetIO().MousePos;
    bool mouseDown = ImGui::IsMouseDown(ImGuiMouseButton_Left);

#ifdef _WIN32
    bool ctrlHeld = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
#else
    bool ctrlHeld = ImGui::GetIO().KeyCtrl;
#endif

    // Helper: is a screen point inside the StepGrid?
    auto inGrid = [&](ImVec2 p) -> bool {
        return p.x >= origin.x && p.x < origin.x + tw &&
               p.y >= origin.y && p.y < origin.y + gh;
    };
    // Helper: is a screen point inside the TransposeGrid?
    auto inTransGrid = [&](ImVec2 p) -> bool {
        return p.x >= origin.x && p.x < origin.x + tw &&
               p.y >= transpY  && p.y < transpY + lh;
    };
    // Helper: is a screen point inside the VelocityGrid?
    auto inVelGrid = [&](ImVec2 p) -> bool {
        return p.x >= origin.x && p.x < origin.x + tw &&
               p.y >= velY     && p.y < velY + lh;
    };

    // ── 8a. TRANSPOSE LABEL DRAG (start) ─────────────────────────────────────
    if (canInteract &&
        state.transposeDragIdx < 0 &&
        ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
        inTransGrid(mouse))
    {
        float lx2 = mouse.x - origin.x;
        for (int i = 0; i < (int)state.steps.size(); i++)
        {
            const ArpStep& a = state.steps[i];
            int   dsf    = (stepIndex(a.deltaStepFractions) + 1) * 4;
            float lpos   = a.startStep * sw;
            float labelW = dsf * sub;
            if (lx2 >= lpos && lx2 < lpos + labelW)
            {
                state.transposeDragIdx    = i;
                state.transposeStart      = a.transpose;
                state.transposeDragStartY = mouse.y;
                break;
            }
        }
    }

    // ── 8b. TRANSPOSE LABEL DRAG (ongoing / release) ─────────────────────────
    if (state.transposeDragIdx >= 0)
    {
        if (mouseDown)
        {
            ArpStep& a  = state.steps[state.transposeDragIdx];
            float    dy = (state.transposeDragStartY - mouse.y) / (4.f * zoom);
            int newT    = state.transposeStart + (int)dy;
            newT = std::max(0, std::min(48, newT));
            if (newT != a.transpose)
            {
                a.transpose = newT;
                writeData(nodeID, state);
                changed = true;
            }
        }
        else
        {
            // Mouse released — finalise (mirrors C# TransposeLabel_MouseLeftButtonUp)
            state.transposeDragIdx = -1;
            writeData(nodeID, state);
            changed = true;
        }
    }

    // ── 8c. VELOCITY LABEL DRAG (start) ──────────────────────────────────────
    if (canInteract &&
        state.velocityDragIdx < 0 &&
        ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
        inVelGrid(mouse))
    {
        float lx2 = mouse.x - origin.x;
        for (int i = 0; i < (int)state.steps.size(); i++)
        {
            const ArpStep& a = state.steps[i];
            int   dsf    = (stepIndex(a.deltaStepFractions) + 1) * 4;
            float lpos   = a.startStep * sw;
            float labelW = dsf * sub;
            if (lx2 >= lpos && lx2 < lpos + labelW)
            {
                state.velocityDragIdx    = i;
                state.velocityStart      = a.velocity;
                state.velocityDragStartY = mouse.y;
                break;
            }
        }
    }

    // ── 8d. VELOCITY LABEL DRAG (ongoing / release) ───────────────────────────
    if (state.velocityDragIdx >= 0)
    {
        if (mouseDown)
        {
            ArpStep& a  = state.steps[state.velocityDragIdx];
            float    dy = (state.velocityDragStartY - mouse.y) / (2.f * zoom);
            int newV    = state.velocityStart + (int)dy;
            newV = std::max(1, std::min(127, newV));
            if (newV != a.velocity)
            {
                a.velocity = newV;
                writeData(nodeID, state);
                changed = true;
            }
        }
        else
        {
            // Mouse released — finalise (mirrors C# VelocityLabel_MouseLeftButtonUp)
            state.velocityDragIdx = -1;
            writeData(nodeID, state);
            changed = true;
        }
    }

    // ── 8e. MAIN GRID: click starts drag (or sets loop start) ────────────────
    if (canInteract &&
        !state.isDragging &&
        state.transposeDragIdx < 0 &&
        state.velocityDragIdx  < 0 &&
        ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
        inGrid(mouse))
    {
        float lx2 = mouse.x - origin.x;
        float ly2 = mouse.y - origin.y;

        int clickStep = (int)(lx2 / sw);
        int clickZone = 3 - (int)(ly2 / zh);   // 3=loop zone (top), 2..−2 = octaves

        bool clickHandled = false;

        // ── Ctrl+Click on existing note bar: delete it ────────────────────────
        if (ctrlHeld && clickZone <= 2)
        {
            for (int i = (int)state.steps.size() - 1; i >= 0; i--)
            {
                const ArpStep& a = state.steps[i];
                float nx2 = (0.5f + a.startStep * kStepW) * zoom;
                float ny2 = (0.5f + 72.f - (a.transpose - 24) * 2.f) * zoom;
                float nw2 = (a.deltaStepFractions * kSubW - 1.f) * zoom;
                float nh2 = 23.f * zoom;
                if (lx2 >= nx2 && lx2 < nx2 + nw2 &&
                    ly2 >= ny2 && ly2 < ny2 + nh2)
                {
                    state.steps.erase(state.steps.begin() + i);
                    writeData(nodeID, state);
                    changed      = true;
                    clickHandled = true;
                    break;
                }
            }
        }

        if (!clickHandled)
        {
            if (clickZone > 2)
            {
                // ── Loop zone: set loop-start position ────────────────────────
                if (clickStep >= 0 && clickStep < kMaxSteps)
                {
                    state.loopStart = clickStep;
                    sc->setArpStepData((DWORD)nodeID, (DWORD)-1, (DWORD)clickStep);
                    changed = true;
                }
            }
            else
            {
                // ── Create a new note and begin drag ──────────────────────────
                if (clickStep < 0)         clickStep = 0;
                if (clickStep >= kMaxSteps) clickStep = kMaxSteps - 1;

                int octave          = std::max(-2, std::min(2, clickZone));
                int externalTransp  = octave * 12;   // −24, −12, 0, +12, +24

                ArpStep newStep;
                newStep.startStep          = clickStep;
                newStep.deltaStepFractions = 4;
                newStep.transpose          = externalTransp + 24;  // internal 0..48
                newStep.velocity           = 127;

                state.steps.push_back(newStep);
                int newIdx = (int)state.steps.size() - 1;
                newIdx = validateInsertionFully(state, newIdx);

                state.isDragging    = true;
                state.dragStartX    = lx2;
                state.dragStartStep = clickStep;
                state.dragStepIdx   = newIdx;
            }
        }
    }

    // ── 8f. MAIN GRID: drag in progress ───────────────────────────────────────
    if (state.isDragging)
    {
        if (mouseDown)
        {
            float lx2 = mouse.x - origin.x;
            float ly2 = mouse.y - origin.y;

            // Dead-zone: ignore tiny horizontal movement until at least 2 px (scaled)
            if (std::abs(state.dragStartX - lx2) < 2.f * zoom)
            {
                // not moved enough — do nothing this frame
            }
            else
            {
                state.dragStartX = -1.f;   // dead-zone cleared; always update from now on

                // sub-step position (each sub = kSubW * zoom px)
                int clickSubStep = (int)(lx2 / sub);
                int clickZone    = 3 - (int)(ly2 / zh);
                clickZone        = std::max(-2, std::min(2, clickZone));
                int clickOctave  = clickZone * 12;   // −24..+24

                if (clickSubStep > kMaxSteps * 4)
                    clickSubStep = kMaxSteps * 4;

                int deltaSteps = clickSubStep - (state.dragStartStep * 4);
                if (deltaSteps < 1) deltaSteps = 1;

                if (state.dragStepIdx >= 0 &&
                    state.dragStepIdx < (int)state.steps.size())
                {
                    state.steps[state.dragStepIdx].transpose          = clickOctave + 24;
                    state.steps[state.dragStepIdx].deltaStepFractions = deltaSteps;
                    state.dragStepIdx = validateInsertionFully(state, state.dragStepIdx);
                    changed = true;
                }
            }
        }
        else
        {
            // Mouse released — finalise
            state.isDragging  = false;
            state.dragStepIdx = -1;
            writeData(nodeID, state);
            changed = true;
        }
    }

    // ── advance ImGui cursor past the whole editor ─────────────────────────────
    ImGui::SetCursorScreenPos(ImVec2(origin.x, velY + lh));
    ImGui::Dummy(ImVec2(tw, 0.f));

    return changed;
}

} // namespace K64GUI

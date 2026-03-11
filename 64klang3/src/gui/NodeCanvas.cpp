#include "NodeCanvas.h"
#include "NodeConfig.h"
#include "ImGuiPlugin.h"
#include "Widgets.h"
#include "ArpEditor.h"
#include "core/SynthController.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <chrono>
#include <cstdio>
#include <sstream>

#ifdef _WIN32
#include <commdlg.h>
#endif

namespace K64GUI {

NodeCanvas::NodeCanvas() {}
NodeCanvas::~NodeCanvas() {}

// Scroll the canvas to center on a root node and update selection.
// channel == -1  → SynthRoot: center + select that node only (no recursion).
// channel 0..15 → ChannelRoot: center + recursively select (same as Shift+click).
void NodeCanvas::jumpToChannel(int channel)
{
    SynthController* sc = SynthController::instance();
    if (!sc) return;

    bool wantSynthRoot = (channel == -1);
    int n = sc->numGUINodes();
    for (int i = 0; i < n; i++)
    {
        int nodeType = sc->gnType(i);
        bool match = wantSynthRoot ? (nodeType == SYNTHROOT_ID)
                                   : (nodeType == CHANNELROOT_ID && sc->gnChannel(i) == channel);
        if (!match)
            continue;

        int nodeID = sc->gnID(i);

        // Center the canvas on this node's center point.
        int   numIn        = effectiveInputCount(i);
        bool  hasEditBtn   = nodeHasEditButton(i);
        bool  hasChBtns    = (sc->gnType(i) == CHANNELROOT_ID);
        float halfW        = kNodeWidth * 0.5f;
        float halfH        = nodeHeight(numIn, hasEditBtn, /*hasAddInput=*/false, hasChBtns) * 0.5f;
        float cx = (float)sc->gnX(i) + halfW;
        float cy = (float)sc->gnY(i) + halfH;
        if (canvasSizeCache.x > 0 && canvasSizeCache.y > 0)
        {
            offsetX = canvasSizeCache.x / (2.f * zoom) - cx;
            offsetY = canvasSizeCache.y / (2.f * zoom) - cy;
        }

        // SynthRoot: select only that node.
        // ChannelRoot: recursively select it and all upstream inputs.
        selectedNodeIDs.clear();
        if (wantSynthRoot)
        {
            selectedNodeIDs.insert(nodeID);
        }
        else
        {
            std::unordered_set<int> visited;
            recursiveSelect(nodeID, visited);
        }
        syncSelectionToCore();
        return;
    }
}

// Pick the font whose loaded size is closest to the requested pixel size.
// Fonts[0] = 14 px (sharp at zoom ≤ 1.5×), Fonts[1] = 32 px (sharp at zoom ≥ 2×).
static ImFont* pickFont(float desiredPx)
{
    auto& fv = ImGui::GetIO().Fonts->Fonts;
    if (fv.Size >= 2 && desiredPx >= fv[1]->FontSize * 0.75f)
        return fv[1];
    return fv[0];
}

// Check if a gnInput() return value represents a real connection
// (not unwired constant0, not 0, not -1)
static bool isRealConnection(int srcID, SynthController* sc)
{
    if (srcID == 0 || srcID == -1)
        return false;
    if (sc && sc->constant0() && srcID == (int)sc->constant0()->valueOffset)
        return false;
    return true;
}

float NodeCanvas::nodeHeight(int numInputs, bool hasEditBtn, bool hasAddInput, bool hasChannelBtns) const
{
    float h = kHeaderHeight + (float)numInputs * kRowHeight;
    if (hasEditBtn)
        h += kEditButtonHeight;
    if (hasAddInput)
        h += kRowHeight; // "Add Input" pill row
    if (hasChannelBtns)
        h += kEditButtonHeight * 2.f; // Load + Save channel buttons
    return h;
}

int NodeCanvas::effectiveInputCount(int guiIndex) const
{
    SynthController* sc = SynthController::instance();
    if (!sc) return 0;
    // Input-category nodes (Midi CC, Constant, voice params) have no connectable
    // signal inputs — their numInputs stores Scale/mode constants, not wiring slots.
    {
        int nt = sc->gnType(guiIndex);
        if (nt == MIDISIGNAL_ID || nt >= CONSTANT_ID)
            return 0;
    }
    int maxSignals = sc->gnNodeMaxSignals(guiIndex);
    // For non-variable-input nodes, maxSignals already excludes mode inputs.
    // For variable-input nodes (NoteController, MultiAdd), maxSignals is 0;
    // use gnNodeInputs() which returns the actual live count.
    if (maxSignals > 0)
        return maxSignals;
    return sc->gnNodeInputs(guiIndex);
}

bool NodeCanvas::nodeHasEditButton(int guiIndex) const
{
    SynthController* sc = SynthController::instance();
    if (!sc) return false;

    int nodeType = sc->gnType(guiIndex);

    // Variable-input nodes with nothing to configure
    if (nodeType == NOTECONTROLLER_ID || nodeType == MULTIADD_ID)
        return false;

    // VoiceManager always shows edit button (for arpeggiator editor)
    if (nodeType == VOICEMANAGER_ID)
        return true;

    // These nodes have custom editors that replace or supplement the generic panel
    if (nodeType == TRIGGERSEQ_ID || nodeType == SAPI_ID || nodeType == FORMULA_ID)
        return true;

    const NodeTypeDef* typeDef = NodeConfig::instance().getNodeType(nodeType);
    if (!typeDef) return false;

    // Has editable parameters (more inputs than required signals)
    bool hasParams = (typeDef->numMaxGUIInputs > typeDef->numReqGUIInputs);

    // Has mode groups or flags
    bool hasModes = false;
    for (const auto& input : typeDef->inputs)
    {
        if (input.isMode())
        {
            hasModes = true;
            break;
        }
    }

    // Constants/voice params always show edit button for value display
    if (nodeType >= CONSTANT_ID)
        return true;

    return hasParams || hasModes;
}

std::string NodeCanvas::buildModeText(int guiIndex) const
{
    SynthController* sc = SynthController::instance();
    if (!sc) return "";

    int nodeType = sc->gnType(guiIndex);
    int nodeID = sc->gnID(guiIndex);
    const NodeTypeDef* typeDef = NodeConfig::instance().getNodeType(nodeType);
    if (!typeDef) return "";

    char buf[64];

    // Constants and OsRand: show L/R values of first parameter.
    // Constant stores its value in node->out (index -1); voice params store Scale at input[0].
    if (nodeType == 35 || nodeType >= CONSTANT_ID) // OsRand=35, constants>=64
    {
        DWORD valIdx = (nodeType == CONSTANT_ID) ? (DWORD)-1 : 0u;
        double l = sc->getInputValue((DWORD)nodeID, valIdx, 0);
        double r = sc->getInputValue((DWORD)nodeID, valIdx, 1);
        snprintf(buf, sizeof(buf), "%.2f / %.2f", l, r);
        return buf;
    }

    // SynthRoot, ChannelRoot, Shaper, Clip, CrossMix, OnePole, OneZero, Scaler
    if (nodeType < 2 || nodeType == 12 || nodeType == 14 || nodeType == 19 ||
        nodeType == 10 || nodeType == 11 || nodeType == 31)
    {
        double l = sc->getInputValue((DWORD)nodeID, 1, 0);
        double r = sc->getInputValue((DWORD)nodeID, 1, 1);
        snprintf(buf, sizeof(buf), "%.2f / %.2f", l, r);
        return buf;
    }

    // Mix
    if (nodeType == 20)
    {
        double l = sc->getInputValue((DWORD)nodeID, 2, 0);
        double r = sc->getInputValue((DWORD)nodeID, 2, 1);
        snprintf(buf, sizeof(buf), "%.2f / %.2f", l, r);
        return buf;
    }

    // Panning
    if (nodeType == 13)
    {
        double l = sc->getInputValue((DWORD)nodeID, 1, 0);
        snprintf(buf, sizeof(buf), "%.2f", l);
        return buf;
    }

    // General mode text from mode input
    int maxGUISignals = typeDef->numMaxGUIInputs;
    // MidiSignal needs +1
    if (nodeType == 29)
        maxGUISignals++;

    if (maxGUISignals < typeDef->numInputs && maxGUISignals < (int)typeDef->inputs.size())
    {
        const InputDef& modeDef = typeDef->inputs[maxGUISignals];
        int initialBits = sc->getInputMode((DWORD)nodeID, (DWORD)maxGUISignals);

        std::string mode;
        for (const auto& mg : modeDef.modeGroups)
        {
            if (mg.hideModeText)
                continue;
            int currentBits = (initialBits & (int)mg.mask) >> mg.shift;

            for (const auto& mi : mg.items)
            {
                if (currentBits == mi.value)
                {
                    if (!mode.empty()) mode += " ";
                    mode += mi.name;
                }
            }
        }

        if (mode.empty())
            mode = "Configuration";
        return mode;
    }

    return "Configuration";
}

ImVec2 NodeCanvas::nodeScreenPos(double nx, double ny, const ImVec2& canvasOrigin) const
{
    return ImVec2(canvasOrigin.x + ((float)nx + offsetX) * zoom,
                  canvasOrigin.y + ((float)ny + offsetY) * zoom);
}

ImVec2 NodeCanvas::outputPinPos(const ImVec2& nodePos) const
{
    return ImVec2(nodePos.x + (kNodeWidth - kPinInset) * zoom, nodePos.y + kOutputPinY * zoom);
}

ImVec2 NodeCanvas::inputPinPos(const ImVec2& nodePos, int pinIndex) const
{
    return ImVec2(nodePos.x + kPinInset * zoom, nodePos.y + (kFirstPinY + (float)pinIndex * kRowHeight) * zoom);
}

// ── Hit Testing ──────────────────────────────────────────────────────────

int NodeCanvas::hitTestNode(const ImVec2& mousePos, const ImVec2& canvasOrigin) const
{
    SynthController* sc = SynthController::instance();
    if (!sc) return -1;

    int numNodes = sc->numGUINodes();
    int hitID = -1;

    // Iterate forward; last match = topmost (painter's order)
    for (int i = 0; i < numNodes; i++)
    {
        if (!sc->gnIsVisible(i))
            continue;
        double nx = sc->gnX(i);
        double ny = sc->gnY(i);
        int numSignals = effectiveInputCount(i);
        bool hasEditBtn = nodeHasEditButton(i);
        int nodeTypeI = sc->gnType(i);
        bool hasAddInput = (nodeTypeI == MULTIADD_ID || nodeTypeI == NOTECONTROLLER_ID);
        bool hasChannelBtnsI = (nodeTypeI == CHANNELROOT_ID);

        ImVec2 pos = nodeScreenPos(nx, ny, canvasOrigin);
        float w = kNodeWidth * zoom;
        float h = nodeHeight(numSignals, hasEditBtn, hasAddInput, hasChannelBtnsI) * zoom;

        if (mousePos.x >= pos.x && mousePos.x <= pos.x + w &&
            mousePos.y >= pos.y && mousePos.y <= pos.y + h)
        {
            hitID = sc->gnID(i);
        }
    }
    return hitID;
}

int NodeCanvas::findGuiIndex(int nodeID) const
{
    SynthController* sc = SynthController::instance();
    if (!sc) return -1;

    int numNodes = sc->numGUINodes();
    for (int i = 0; i < numNodes; i++)
    {
        if (sc->gnID(i) == nodeID)
            return i;
    }
    return -1;
}

bool NodeCanvas::nodeFullyInsideRect(int guiIndex, ImVec2 rectMin, ImVec2 rectMax,
                                     const ImVec2& canvasOrigin) const
{
    SynthController* sc = SynthController::instance();
    if (!sc) return false;

    double nx = sc->gnX(guiIndex);
    double ny = sc->gnY(guiIndex);
    int numSignals = effectiveInputCount(guiIndex);
    bool hasEditBtn = nodeHasEditButton(guiIndex);
    int nodeTypeRI = sc->gnType(guiIndex);
    bool hasAddInputRI = (nodeTypeRI == MULTIADD_ID || nodeTypeRI == NOTECONTROLLER_ID);
    bool hasChannelBtnsRI = (nodeTypeRI == CHANNELROOT_ID);

    ImVec2 pos = nodeScreenPos(nx, ny, canvasOrigin);
    float w = kNodeWidth * zoom;
    float h = nodeHeight(numSignals, hasEditBtn, hasAddInputRI, hasChannelBtnsRI) * zoom;

    // Normalize rect
    float rMinX = std::min(rectMin.x, rectMax.x);
    float rMinY = std::min(rectMin.y, rectMax.y);
    float rMaxX = std::max(rectMin.x, rectMax.x);
    float rMaxY = std::max(rectMin.y, rectMax.y);

    return pos.x >= rMinX && pos.y >= rMinY &&
           pos.x + w <= rMaxX && pos.y + h <= rMaxY;
}

void NodeCanvas::recursiveSelect(int nodeID, std::unordered_set<int>& visited)
{
    if (visited.count(nodeID))
        return;
    visited.insert(nodeID);
    selectedNodeIDs.insert(nodeID);

    SynthController* sc = SynthController::instance();
    if (!sc) return;

    int gi = findGuiIndex(nodeID);
    if (gi < 0) return;

    int numSignals = effectiveInputCount(gi);
    for (int pin = 0; pin < numSignals; pin++)
    {
        int srcID = sc->gnInput(gi, pin);
        if (isRealConnection(srcID, sc))
            recursiveSelect(srcID, visited);
    }
}

void NodeCanvas::syncSelectionToCore()
{
    SynthController* sc = SynthController::instance();
    if (!sc) return;

    sc->clearSelection();
    for (int id : selectedNodeIDs)
        sc->setSelected((DWORD)id, 1);
}

void NodeCanvas::deleteNodeMaybeSmart(int nodeID, bool singleNodeOnly)
{
    SynthController* sc = SynthController::instance();
    if (!sc) return;

    int gi = findGuiIndex(nodeID);
    if (gi < 0) return;

    int nodeType = sc->gnType(gi);
    const NodeTypeDef* typeDef = NodeConfig::instance().getNodeType(nodeType);

    if (singleNodeOnly && typeDef && typeDef->allowSignalInsertion)
    {
        int input0ID = sc->gnInput(gi, 0);
        if (isRealConnection(input0ID, sc))
        {
            // Collect (toNodeID, pinIndex) where this node is the source
            std::vector<std::pair<int, int>> outputs;
            int numNodes = sc->numGUINodes();
            for (int toIdx = 0; toIdx < numNodes; toIdx++)
            {
                if (!sc->gnIsVisible(toIdx)) continue;
                int toID = sc->gnID(toIdx);
                if (toID == nodeID) continue;
                int numSig = effectiveInputCount(toIdx);
                for (int pin = 0; pin < numSig; pin++)
                {
                    if (sc->gnInput(toIdx, pin) == nodeID)
                        outputs.push_back({toID, pin});
                }
            }
            sc->deleteNode((DWORD)nodeID);
            for (const auto& p : outputs)
                sc->connectInput((DWORD)input0ID, (DWORD)p.first, (DWORD)p.second);
            return;
        }
    }
    sc->deleteNode((DWORD)nodeID);
}

// ── Pin Hit Testing ─────────────────────────────────────────────────────

int NodeCanvas::hitTestOutputPin(const ImVec2& mousePos, const ImVec2& canvasOrigin) const
{
    SynthController* sc = SynthController::instance();
    if (!sc) return -1;

    int numNodes = sc->numGUINodes();
    float hitRadius = kPinRadius * zoom * 1.5f;

    for (int i = 0; i < numNodes; i++)
    {
        if (!sc->gnIsVisible(i))
            continue;
        double nx = sc->gnX(i);
        double ny = sc->gnY(i);
        ImVec2 pos = nodeScreenPos(nx, ny, canvasOrigin);
        ImVec2 pin = outputPinPos(pos);

        float dx = mousePos.x - pin.x;
        float dy = mousePos.y - pin.y;
        if (dx * dx + dy * dy <= hitRadius * hitRadius)
            return sc->gnID(i);
    }
    return -1;
}

NodeCanvas::PinHit NodeCanvas::hitTestInputPin(const ImVec2& mousePos, const ImVec2& canvasOrigin) const
{
    SynthController* sc = SynthController::instance();
    if (!sc) return {-1, -1};

    int numNodes = sc->numGUINodes();
    float hitRadius = kPinRadius * zoom * 1.5f;

    for (int i = 0; i < numNodes; i++)
    {
        if (!sc->gnIsVisible(i))
            continue;
        double nx = sc->gnX(i);
        double ny = sc->gnY(i);
        int numSignals = effectiveInputCount(i);
        int nodeTypeHT = sc->gnType(i);
        bool hasEditBtnHT = nodeHasEditButton(i);
        ImVec2 pos = nodeScreenPos(nx, ny, canvasOrigin);
        float w = kNodeWidth * zoom;

        // Regular input pins
        for (int pin = 0; pin < numSignals; pin++)
        {
            ImVec2 pinPos = inputPinPos(pos, pin);
            float dx = mousePos.x - pinPos.x;
            float dy = mousePos.y - pinPos.y;
            if (dx * dx + dy * dy <= hitRadius * hitRadius)
                return {sc->gnID(i), pin};
        }

        // "Add Input" pill for variable-input nodes (index ignored by connectInput)
        if ((nodeTypeHT == MULTIADD_ID || nodeTypeHT == NOTECONTROLLER_ID) && numSignals < 16)
        {
            float pillBaseY = pos.y + nodeHeight(numSignals, hasEditBtnHT, false) * zoom;
            float pillH     = kRowHeight * zoom;
            float pillInset = 8.f * zoom;
            ImVec2 pillMin(pos.x + pillInset, pillBaseY + 2.f * zoom);
            ImVec2 pillMax(pos.x + w - pillInset, pillBaseY + pillH - 2.f * zoom);
            if (mousePos.x >= pillMin.x && mousePos.x <= pillMax.x &&
                mousePos.y >= pillMin.y && mousePos.y <= pillMax.y)
                return {sc->gnID(i), numSignals}; // index ignored for variable-input nodes
        }
    }
    return {-1, -1};
}

// Point-to-segment squared distance (avoids sqrt)
static float distSqPointToSegment(const ImVec2& p, const ImVec2& a, const ImVec2& b)
{
    ImVec2 ab(b.x - a.x, b.y - a.y);
    ImVec2 ap(p.x - a.x, p.y - a.y);
    float t = (ab.x * ap.x + ab.y * ap.y) / (ab.x * ab.x + ab.y * ab.y + 1e-9f);
    t = (t < 0.f) ? 0.f : (t > 1.f) ? 1.f : t;
    float dx = a.x + t * ab.x - p.x;
    float dy = a.y + t * ab.y - p.y;
    return dx * dx + dy * dy;
}

NodeCanvas::WireHit NodeCanvas::hitTestWire(const ImVec2& mousePos, const ImVec2& canvasOrigin) const
{
    SynthController* sc = SynthController::instance();
    if (!sc) return {-1, -1, -1};

    int numNodes = sc->numGUINodes();
    float hitRadius = kWireThickness * zoom * 3.f;  // generous hit area
    float hitRadiusSq = hitRadius * hitRadius;

    for (int toIdx = 0; toIdx < numNodes; toIdx++)
    {
        if (!sc->gnIsVisible(toIdx)) continue;

        int toNodeID = sc->gnID(toIdx);
        int numSignals = effectiveInputCount(toIdx);
        double toNX = sc->gnX(toIdx);
        double toNY = sc->gnY(toIdx);
        ImVec2 toPos = nodeScreenPos(toNX, toNY, canvasOrigin);

        for (int pin = 0; pin < numSignals; pin++)
        {
            int srcID = sc->gnInput(toIdx, pin);
            if (!isRealConnection(srcID, sc)) continue;

            int fromIdx = findGuiIndex(srcID);
            if (fromIdx < 0 || !sc->gnIsVisible(fromIdx)) continue;

            double fromNX = sc->gnX(fromIdx);
            double fromNY = sc->gnY(fromIdx);
            ImVec2 fromPos = nodeScreenPos(fromNX, fromNY, canvasOrigin);

            ImVec2 p0 = outputPinPos(fromPos);
            ImVec2 p3 = inputPinPos(toPos, pin);
            ImVec2 p1 = ImVec2(p0.x + kWireStubLen * zoom, p0.y);
            ImVec2 p2 = ImVec2(p3.x - kWireStubLen * zoom, p3.y);

            float d1 = distSqPointToSegment(mousePos, p0, p1);
            float d2 = distSqPointToSegment(mousePos, p1, p2);
            float d3 = distSqPointToSegment(mousePos, p2, p3);
            if (d1 <= hitRadiusSq || d2 <= hitRadiusSq || d3 <= hitRadiusSq)
                return {srcID, toNodeID, pin};
        }
    }
    return {-1, -1, -1};
}

void NodeCanvas::drawGhostWire(ImDrawList* dl, const ImVec2& canvasOrigin)
{
    if (!isWireDragging || wireDragFromNodeID < 0)
        return;

    SynthController* sc = SynthController::instance();
    if (!sc) return;

    int gi = findGuiIndex(wireDragFromNodeID);
    if (gi < 0) return;

    ImVec2 fromPos = nodeScreenPos(sc->gnX(gi), sc->gnY(gi), canvasOrigin);
    ImVec2 p0 = outputPinPos(fromPos);
    ImVec2 p3 = wireDragCurrentPos;
    ImVec2 p1 = ImVec2(p0.x + kWireStubLen * zoom, p0.y);
    ImVec2 p2 = ImVec2(p3.x - kWireStubLen * zoom, p3.y);

    ImU32 ghostColor = IM_COL32(255, 50, 50, 200);
    dl->AddLine(p0, p1, ghostColor, kWireThickness * zoom);
    dl->AddLine(p1, p2, ghostColor, kWireThickness * zoom);
    dl->AddLine(p2, p3, ghostColor, kWireThickness * zoom);
}

// ── Interaction ──────────────────────────────────────────────────────────

void NodeCanvas::handleNodeInteraction(const ImVec2& canvasPos, const ImVec2& canvasSize)
{
    // While renaming, only handle rename-related input
    if (isRenaming)
        return;

    // Block new canvas interactions when mouse is over an edit panel.
    // Exception: allow in-progress node drags, rubber-band selections, and wire drags
    // to keep tracking the mouse — otherwise movement over a panel causes stuck/jump behaviour.
    if (mouseOverEditPanel && !isDragging && !isRubberBanding && !isWireDragging && pressedNodeID == -1)
        return;

    SynthController* sc = SynthController::instance();
    if (!sc || !sc->isInitialized())
        return;

    ImGuiIO& io = ImGui::GetIO();
    ImVec2 mousePos = io.MousePos;

    // ── Wire drag follows mouse every frame (click-to-start, click-to-finish) ──
    if (isWireDragging)
    {
        wireDragCurrentPos = mousePos;
    }

    // ── Left mouse press ──
    if (canvasHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        // If already wire-dragging: click on input pin connects (stays in drag mode),
        // click on background cancels, click on node/output is ignored (stays in drag mode)
        if (isWireDragging)
        {
            PinHit target = hitTestInputPin(mousePos, canvasPos);
            if (target.nodeID != -1)
            {
                // Validate connection — self-connections (feedback) are allowed by the engine
                bool valid = true;
                int fromGI = findGuiIndex(wireDragFromNodeID);
                int toGI   = findGuiIndex(target.nodeID);

                if (fromGI >= 0 && toGI >= 0)
                {
                    bool fromGlobal = sc->gnIsGlobal(fromGI);
                    bool toGlobal   = sc->gnIsGlobal(toGI);
                    int  fromType   = sc->gnType(fromGI);
                    int  toType     = sc->gnType(toGI);

                    // Case 1: variable-input node at max capacity
                    if ((toType == NOTECONTROLLER_ID || toType == MULTIADD_ID) &&
                        sc->gnNodeInputs(toGI) >= 16)
                    {
                        showToast("No more than 16 inputs are allowed!");
                        valid = false;
                    }
                    // Case 2: voice node output → global node input
                    if (valid && !fromGlobal && toGlobal)
                    {
                        if (!(fromType == VOICEROOT_ID && toType == VOICEMANAGER_ID))
                        {
                            showToast("A voice node output cannot be connected to a global node input!");
                            valid = false;
                        }
                    }
                    // Case 3: VoiceRoot → VoiceManager but not at pin 0
                    if (valid && fromType == VOICEROOT_ID && toType == VOICEMANAGER_ID && target.pinIndex != 0)
                    {
                        showToast("Voice Root can only be attached to Voice Manager's Voice Root input!");
                        valid = false;
                    }
                    // Case 4: NoteController only accepts VoiceManager
                    if (valid && toType == NOTECONTROLLER_ID && fromType != VOICEMANAGER_ID)
                    {
                        showToast("Only Voice Manager can be input for Note Controller!");
                        valid = false;
                    }
                    // Case 5: SampleRecorder → only SamplePlayer[0], WTFOsc[0], or EventSignal
                    if (valid && fromType == SAMPLEREC_ID)
                    {
                        bool ok = (toType == SAMPLER_ID   && target.pinIndex == 0) ||
                                  (toType == WTFOSC_ID    && target.pinIndex == 0) ||
                                  (toType == EVENTSIGNAL_ID);
                        if (!ok)
                        {
                            showToast("Sample Recorder can only be input for Sample Player or WTF Oscillator In input!");
                            valid = false;
                        }
                    }
                    // Case 6: TextToSpeech → only SamplePlayer[0] or WTFOsc[0]
                    if (valid && fromType == SAPI_ID)
                    {
                        bool ok = (toType == SAMPLER_ID && target.pinIndex == 0) ||
                                  (toType == WTFOSC_ID  && target.pinIndex == 0);
                        if (!ok)
                        {
                            showToast("TextToSpeech can only be input for Sample Player or WTF Oscillator In input!");
                            valid = false;
                        }
                    }
                    // Case 7: GM.DLS → only SamplePlayer[0] or WTFOsc[0]
                    if (valid && fromType == GMDLS_ID)
                    {
                        bool ok = (toType == SAMPLER_ID && target.pinIndex == 0) ||
                                  (toType == WTFOSC_ID  && target.pinIndex == 0);
                        if (!ok)
                        {
                            showToast("GM.DLS can only be input for Sample Player or WTF Oscillator In input!");
                            valid = false;
                        }
                    }
                    // Case 8: SamplePlayer In[0] only from SampleRecorder, SAPI, or GM.DLS
                    if (valid && toType == SAMPLER_ID && target.pinIndex == 0)
                    {
                        if (fromType != SAMPLEREC_ID && fromType != SAPI_ID && fromType != GMDLS_ID)
                        {
                            showToast("Sample Player's In can only be Sample Recorder, TextToSpeech or GM.DLS!");
                            valid = false;
                        }
                    }
                    // Case 9: WTFOsc In[0] only from SampleRecorder, SAPI, or GM.DLS
                    if (valid && toType == WTFOSC_ID && target.pinIndex == 0)
                    {
                        if (fromType != SAMPLEREC_ID && fromType != SAPI_ID && fromType != GMDLS_ID)
                        {
                            showToast("WTF Oscillator's In can only be Sample Recorder, TextToSpeech or GM.DLS!");
                            valid = false;
                        }
                    }
                    // Case 10: SyncOsc sync input[1] only from Oscillator or LFO
                    if (valid && toType == OSCSYNC_ID && target.pinIndex == 1)
                    {
                        if (fromType != OSCILLATOR_ID && fromType != LFO_ID)
                        {
                            showToast("Only Oscillator or LFO can be input for SyncOsc!");
                            valid = false;
                        }
                    }
                }

                if (valid)
                {
                    sc->killVoices();
                    sc->connectInput((DWORD)wireDragFromNodeID, (DWORD)target.nodeID, (DWORD)target.pinIndex);
                    sc->numGUINodes();
                }
                return; // stay in wire drag mode
            }

            // Click hit a node body or output pin — just consume the click, stay wiring
            int nodeHit = hitTestNode(mousePos, canvasPos);
            if (nodeHit != -1)
                return;

            // Click on empty background — open menu to insert node and continue (continuous wire)
            // If user dismisses menu without selecting, wire drag ends (handled in popup close)
            wireDragInsertMode = true;
            wireDragInsertCanvasPos.x = (float)((mousePos.x - canvasPos.x) / zoom - offsetX);
            wireDragInsertCanvasPos.y = (float)((mousePos.y - canvasPos.y) / zoom - offsetY);
            contextMenuCanvasPos = wireDragInsertCanvasPos;
            showContextMenu = true;
            ImGui::OpenPopup("##nodeMenu");
            return;
        }

        // Test pins BEFORE node body
        // Shift+click on output pin = recursive select; only start wire drag when shift not held.
#ifdef _WIN32
        bool shiftHeld = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
#else
        bool shiftHeld = io.KeyShift;
#endif
        int outputHit = hitTestOutputPin(mousePos, canvasPos);
        if (outputHit != -1 && !shiftHeld)
        {
            // Start wire drag from output pin (click-to-start)
            isWireDragging = true;
            wireDragFromNodeID = outputHit;
            wireDragCurrentPos = mousePos;
            pressedNodeID = -1;
            return;
        }

        PinHit inputHit = hitTestInputPin(mousePos, canvasPos);
        if (inputHit.nodeID != -1)
        {
            // Check if this input is already wired — disconnect it.
            // Guard: skip the "Add Input" pill (pinIndex == numInputs for variable-input nodes).
            int gi = findGuiIndex(inputHit.nodeID);
            if (gi >= 0)
            {
                int nodeTypeHit = sc->gnType(gi);
                bool isAddInputPill =
                    (nodeTypeHit == MULTIADD_ID || nodeTypeHit == NOTECONTROLLER_ID) &&
                    (inputHit.pinIndex >= sc->gnNodeInputs(gi));
                if (!isAddInputPill)
                {
                    int srcID = sc->gnInput(gi, inputHit.pinIndex);
                    if (isRealConnection(srcID, sc))
                    {
                        sc->killVoices();
                        sc->disconnectInput((DWORD)inputHit.nodeID, (DWORD)inputHit.pinIndex);
                        sc->numGUINodes(); // refresh accessor
                        return;
                    }
                }
            }
        }

        int hitID = hitTestNode(mousePos, canvasPos);
        double now = ImGui::GetTime();

        // Double-click detection
        bool isDoubleClick = false;
        if (hitID != -1 && hitID == lastClickNodeID &&
            (now - lastClickTime) < kDoubleClickTime)
        {
            isDoubleClick = true;
            lastClickNodeID = -1;
            lastClickTime = 0.0;
        }
        else
        {
            lastClickNodeID = hitID;
            lastClickTime = now;
        }

        if (hitID != -1)
        {
            if (isDoubleClick)
            {
                // Double-click: mute toggle for ChannelRoot/VoiceManager, rename for others
                int gi = findGuiIndex(hitID);
                int nodeType = (gi >= 0) ? sc->gnType(gi) : -1;

                if (nodeType == CHANNELROOT_ID || nodeType == VOICEMANAGER_ID)
                {
                    // Toggle mute
                    if (mutedNodeIDs.count(hitID))
                    {
                        mutedNodeIDs.erase(hitID);
                        sc->setNodeProcessingFlags((DWORD)hitID, NODE_PROCESSING_DEFAULT);
                    }
                    else
                    {
                        mutedNodeIDs.insert(hitID);
                        sc->setNodeProcessingFlags((DWORD)hitID, NODE_PROCESSING_MUTE);
                    }
                }
                else if (nodeType > (int)VOICEMANAGER_ID)
                {
                    // Start inline rename
                    isRenaming = true;
                    renamingNodeID = hitID;
                    std::string name = (gi >= 0) ? sc->gnName(gi) : "";
                    std::strncpy(renameBuffer, name.c_str(), sizeof(renameBuffer) - 1);
                    renameBuffer[sizeof(renameBuffer) - 1] = '\0';
                }
            }
            else
            {
                // Single click on node
                // Skip selection/drag if click lands on channel buttons (Load/Save Channel)
                int gi = findGuiIndex(hitID);
                int nodeType = (gi >= 0) ? sc->gnType(gi) : -1;
                bool clickedChannelBtn = false;
                if (nodeType == CHANNELROOT_ID && gi >= 0)
                {
                    int numSig = effectiveInputCount(gi);
                    bool hasEdit = nodeHasEditButton(gi);
                    ImVec2 nodePos = nodeScreenPos(sc->gnX(gi), sc->gnY(gi), canvasPos);
                    float btnBaseY = nodePos.y + (kHeaderHeight + (float)numSig * kRowHeight
                                                  + (hasEdit ? kEditButtonHeight : 0.f)) * zoom;
                    float btnTotalH = kEditButtonHeight * zoom * 2.f;
                    clickedChannelBtn = (mousePos.y >= btnBaseY && mousePos.y <= btnBaseY + btnTotalH);
                }

                if (!clickedChannelBtn)
                {
                    // Use GetKeyState for Ctrl so modifiers work when plugin lacks keyboard focus
#ifdef _WIN32
                    bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
#else
                    bool ctrl = io.KeyCtrl;
#endif
                    // shiftHeld already computed above for output-pin check

                    if (shiftHeld)
                    {
                        // Shift+click: recursive upstream select
                        if (!ctrl)
                            selectedNodeIDs.clear();
                        std::unordered_set<int> visited;
                        recursiveSelect(hitID, visited);
                    }
                    else if (ctrl)
                    {
                        // Ctrl+click: toggle
                        if (selectedNodeIDs.count(hitID))
                            selectedNodeIDs.erase(hitID);
                        else
                            selectedNodeIDs.insert(hitID);
                    }
                    else
                    {
                        // Plain click
                        if (!selectedNodeIDs.count(hitID))
                        {
                            selectedNodeIDs.clear();
                            selectedNodeIDs.insert(hitID);
                        }
                        // If already selected, keep selection (for group drag)
                    }

                    syncSelectionToCore();

                    // Prepare for potential drag
                    pressedNodeID = hitID;
                    dragStartMouse = mousePos;
                    isDragging = false;
                }
            }
        }
        else
        {
            // Click on empty canvas
#ifdef _WIN32
            if ((GetKeyState(VK_CONTROL) & 0x8000) == 0)
#else
            if (!io.KeyCtrl)
#endif
            {
                selectedNodeIDs.clear();
                syncSelectionToCore();
            }

            // Start rubber-band
            isRubberBanding = true;
            rubberBandStart = mousePos;
            rubberBandCurrent = mousePos;
            pressedNodeID = -1;
        }
    }

    // ── Left mouse held: node drag or rubber-banding ──
    if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && !isWireDragging)
    {
        if (pressedNodeID != -1)
        {
            // Node drag
            float dx = mousePos.x - dragStartMouse.x;
            float dy = mousePos.y - dragStartMouse.y;

            if (!isDragging)
            {
                // Check drag threshold
                if (std::abs(dx) > kDragThreshold || std::abs(dy) > kDragThreshold)
                    isDragging = true;
            }

            if (isDragging)
            {
                float dxNode = dx / zoom;
                float dyNode = dy / zoom;

                for (int id : selectedNodeIDs)
                {
                    int gi = findGuiIndex(id);
                    if (gi >= 0)
                    {
                        sc->setX((DWORD)id, sc->gnX(gi) + dxNode);
                        sc->setY((DWORD)id, sc->gnY(gi) + dyNode);
                    }
                }

                // Reset for incremental delta
                dragStartMouse = mousePos;
                // Must re-query numGUINodes to refresh accessor after position changes
                sc->numGUINodes();
            }
        }
        else if (isRubberBanding)
        {
            rubberBandCurrent = mousePos;

            // Update selection based on rubber-band rect
            bool ctrl = io.KeyCtrl;
            if (!ctrl)
                selectedNodeIDs.clear();

            int numNodes = sc->numGUINodes();
            for (int i = 0; i < numNodes; i++)
            {
                if (nodeFullyInsideRect(i, rubberBandStart, rubberBandCurrent, canvasPos))
                    selectedNodeIDs.insert(sc->gnID(i));
            }
            syncSelectionToCore();
        }
    }

    // ── Left mouse release ──
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
    {
        isDragging = false;
        pressedNodeID = -1;
        isRubberBanding = false;
    }

    // ── Right-button release: open context menu if no panning occurred ──
    // (Right-drag pans; wire drag state is preserved during pan, same as zoom)
    if (canvasHovered && ImGui::IsMouseReleased(ImGuiMouseButton_Right) &&
        !isDragging && !isWireDragging && !didPan)
    {
        WireHit wireHit = hitTestWire(mousePos, canvasPos);
        if (wireHit.fromID >= 0 && wireHit.toID >= 0)
        {
            // Right-release on wire → open menu for "insert node in middle"
            contextWireFromID = wireHit.fromID;
            contextWireToID = wireHit.toID;
            contextWirePinIndex = wireHit.pinIndex;
            // Position new node at wire midpoint (between p1 and p2)
            int fromGI = findGuiIndex(wireHit.fromID);
            int toGI = findGuiIndex(wireHit.toID);
            if (fromGI >= 0 && toGI >= 0)
            {
                ImVec2 fromPos = nodeScreenPos(sc->gnX(fromGI), sc->gnY(fromGI), canvasPos);
                ImVec2 toPos = nodeScreenPos(sc->gnX(toGI), sc->gnY(toGI), canvasPos);
                ImVec2 p0 = outputPinPos(fromPos);
                ImVec2 p3 = inputPinPos(toPos, wireHit.pinIndex);
                ImVec2 p1(p0.x + kWireStubLen * zoom, p0.y);
                ImVec2 p2(p3.x - kWireStubLen * zoom, p3.y);
                float mx = (p1.x + p2.x) * 0.5f;
                float my = (p1.y + p2.y) * 0.5f;
                contextMenuCanvasPos.x = (float)((mx - canvasPos.x) / zoom - offsetX);
                contextMenuCanvasPos.y = (float)((my - canvasPos.y) / zoom - offsetY);
            }
            else
            {
                contextMenuCanvasPos = ImVec2(
                    (mousePos.x - canvasPos.x) / zoom - offsetX,
                    (mousePos.y - canvasPos.y) / zoom - offsetY
                );
            }
            showContextMenu = true;
            ImGui::OpenPopup("##nodeMenu");
        }
        else
        {
            int hitID = hitTestNode(mousePos, canvasPos);
            if (hitID == -1)
            {
                // Right-release on empty canvas → open node creation menu
                contextWireFromID = -1;
                contextWireToID = -1;
                contextWirePinIndex = -1;
                contextMenuCanvasPos = ImVec2(
                    (mousePos.x - canvasPos.x) / zoom - offsetX,
                    (mousePos.y - canvasPos.y) / zoom - offsetY
                );
                showContextMenu = true;
                ImGui::OpenPopup("##nodeMenu");
            }
        }
    }

    // ── Delete key ──
    if (!isRenaming && canvasHovered && ImGui::IsKeyPressed(ImGuiKey_Delete))
    {
        sc->killVoices();
        std::vector<int> toDelete(selectedNodeIDs.begin(), selectedNodeIDs.end());
        for (int id : toDelete)
        {
            int gi = findGuiIndex(id);
            int type = (gi >= 0) ? sc->gnType(gi) : -1;
            // Protect structural nodes (SynthRoot/ChannelRoot/NoteController/VoiceManager)
            if (type >= 0 && type <= (int)VOICEMANAGER_ID)
                continue;
            deleteNodeMaybeSmart(id, (int)toDelete.size() == 1);
        }
        selectedNodeIDs.clear();
        syncSelectionToCore();
        sc->numGUINodes(); // refresh accessor
    }

    // ── Copy (Ctrl+C) ──
    if (!isRenaming && canvasHovered && io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_C))
    {
        clipboard.clear();
        if (!selectedNodeIDs.empty())
        {
            // Compute centroid
            double cx = 0, cy = 0;
            int count = 0;
            std::vector<int> selIDs(selectedNodeIDs.begin(), selectedNodeIDs.end());
            for (int id : selIDs)
            {
                int gi = findGuiIndex(id);
                if (gi >= 0)
                {
                    cx += sc->gnX(gi);
                    cy += sc->gnY(gi);
                    count++;
                }
            }
            if (count > 0) { cx /= count; cy /= count; }

            // Build ID→clipboard index map
            std::unordered_map<int, int> idToClipIdx;
            for (int idx = 0; idx < (int)selIDs.size(); idx++)
                idToClipIdx[selIDs[idx]] = idx;

            for (int id : selIDs)
            {
                int gi = findGuiIndex(id);
                if (gi < 0) continue;

                ClipboardNode cn;
                cn.typeID = sc->gnType(gi);
                cn.channel = sc->gnChannel(gi);
                cn.isGlobal = sc->gnIsGlobal(gi);
                cn.relX = sc->gnX(gi) - cx;
                cn.relY = sc->gnY(gi) - cy;

                int numSignals = effectiveInputCount(gi);
                cn.inputs.resize(numSignals);
                cn.internalWires.resize(numSignals, -1);

                for (int p = 0; p < numSignals; p++)
                {
                    cn.inputs[p].valL = sc->gnInputValue(gi, p, 0);
                    cn.inputs[p].valR = sc->gnInputValue(gi, p, 1);
                    cn.inputs[p].mode = sc->gnInputMode(gi, p);

                    int srcID = sc->gnInput(gi, p);
                    if (isRealConnection(srcID, sc))
                    {
                        auto it = idToClipIdx.find(srcID);
                        if (it != idToClipIdx.end())
                            cn.internalWires[p] = it->second;
                    }
                }

                clipboard.push_back(std::move(cn));
            }
        }
    }

    // ── Paste (Ctrl+V) ──
    if (!isRenaming && canvasHovered && io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_V))
    {
        if (!clipboard.empty())
        {
            // Convert mouse pos to canvas coords
            double mouseCanvasX = (mousePos.x - canvasPos.x) / zoom - offsetX;
            double mouseCanvasY = (mousePos.y - canvasPos.y) / zoom - offsetY;

            sc->killVoices();

            std::vector<int> newNodeIDs;
            for (const auto& cn : clipboard)
            {
                double px = mouseCanvasX + cn.relX;
                double py = mouseCanvasY + cn.relY;
                SynthNode* newNode = sc->createGUINode((DWORD)cn.typeID, (DWORD)cn.channel,
                                                        (DWORD)(cn.isGlobal ? 1 : 0), px, py);
                int newID = newNode ? (int)newNode->valueOffset : -1;
                newNodeIDs.push_back(newID);
            }

            // Set values and modes
            for (int i = 0; i < (int)clipboard.size(); i++)
            {
                if (newNodeIDs[i] < 0) continue;
                const auto& cn = clipboard[i];
                int numParams = std::min((int)cn.inputs.size(), 16);
                for (int p = 0; p < numParams; p++)
                {
                    sc->setInputValue((DWORD)newNodeIDs[i], (DWORD)p,
                                      cn.inputs[p].valL, cn.inputs[p].valR);
                    if (cn.inputs[p].mode != 0)
                        sc->setInputMode((DWORD)newNodeIDs[i], (DWORD)p,
                                         (DWORD)cn.inputs[p].mode);
                }
            }

            // Reconnect internal wires
            for (int i = 0; i < (int)clipboard.size(); i++)
            {
                if (newNodeIDs[i] < 0) continue;
                const auto& cn = clipboard[i];
                for (int p = 0; p < (int)cn.internalWires.size(); p++)
                {
                    int srcClipIdx = cn.internalWires[p];
                    if (srcClipIdx >= 0 && srcClipIdx < (int)newNodeIDs.size() && newNodeIDs[srcClipIdx] >= 0)
                    {
                        sc->connectInput((DWORD)newNodeIDs[srcClipIdx], (DWORD)newNodeIDs[i], (DWORD)p);
                    }
                }
            }

            // Select all pasted nodes
            selectedNodeIDs.clear();
            for (int id : newNodeIDs)
            {
                if (id >= 0)
                    selectedNodeIDs.insert(id);
            }
            syncSelectionToCore();
            sc->numGUINodes(); // refresh
        }
    }
}

// ── Rename Overlay ───────────────────────────────────────────────────────

void NodeCanvas::drawRenameOverlay(const ImVec2& canvasOrigin)
{
    if (!isRenaming || renamingNodeID < 0)
        return;

    SynthController* sc = SynthController::instance();
    if (!sc) return;

    int gi = findGuiIndex(renamingNodeID);
    if (gi < 0)
    {
        isRenaming = false;
        return;
    }

    double nx = sc->gnX(gi);
    double ny = sc->gnY(gi);
    ImVec2 pos = nodeScreenPos(nx, ny, canvasOrigin);
    float w = kNodeWidth * zoom;

    // Position the input text over the header
    float inputW = std::max(w, 150.f);
    float inputH = kHeaderHeight * zoom;
    ImVec2 inputPos(pos.x, pos.y);

    ImGui::SetNextWindowPos(inputPos);
    ImGui::SetNextWindowSize(ImVec2(inputW, inputH + 8.f));
    ImGui::Begin("##rename", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                 ImGuiWindowFlags_NoSavedSettings);

    ImGui::SetNextItemWidth(inputW - 8.f);

    // Auto-focus on first frame
    if (ImGui::IsWindowAppearing())
        ImGui::SetKeyboardFocusHere();

    bool enterPressed = ImGui::InputText("##renameInput", renameBuffer, sizeof(renameBuffer),
                                          ImGuiInputTextFlags_EnterReturnsTrue |
                                          ImGuiInputTextFlags_AutoSelectAll);

    // Commit on Enter
    if (enterPressed)
    {
        commitRename();
    }
    // Cancel on Escape
    else if (ImGui::IsKeyPressed(ImGuiKey_Escape))
    {
        isRenaming = false;
        renamingNodeID = -1;
    }
    // Cancel on click outside the rename window
    else if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
             ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        commitRename();
    }

    ImGui::End();
}

void NodeCanvas::commitRename()
{
    if (!isRenaming || renamingNodeID < 0)
        return;

    SynthController* sc = SynthController::instance();
    if (sc)
        sc->setName((DWORD)renamingNodeID, std::string(renameBuffer));

    isRenaming = false;
    renamingNodeID = -1;
}

// ── Edit Panel ──────────────────────────────────────────────────────────

void NodeCanvas::updateMouseOverEditPanel(const ImVec2& canvasOrigin)
{
    mouseOverEditPanel = false;
    if (openEditPanels.empty())
        return;

    // Also consider mouse captured if a knob is being dragged
    if (knobDragNodeID >= 0)
    {
        mouseOverEditPanel = true;
        return;
    }

    SynthController* sc = SynthController::instance();
    if (!sc || !sc->isInitialized())
        return;

    ImVec2 mousePos = ImGui::GetIO().MousePos;

    // Iterate in reverse (last = topmost visually); stop on first hit.
    for (int j = (int)openEditPanels.size() - 1; j >= 0; --j)
    {
        int nodeID = openEditPanels[j];
        int gi = findGuiIndex(nodeID);
        if (gi < 0) continue;

        int nodeType = sc->gnType(gi);
        const NodeTypeDef* typeDef = NodeConfig::instance().getNodeType(nodeType);
        if (!typeDef) continue;

        double nx = sc->gnX(gi);
        double ny = sc->gnY(gi);
        ImVec2 nodePos = nodeScreenPos(nx, ny, canvasOrigin);
        float nodeW = kNodeWidth * zoom;
        float px = nodePos.x + nodeW;
        float py = nodePos.y;
        float pw = kEditPanelWidth * zoom;

        // Estimate panel height (same formula as in drawEditPanel)
        int modeInputIdx = typeDef->numMaxGUIInputs;
        if (!(modeInputIdx < typeDef->numInputs && modeInputIdx < (int)typeDef->inputs.size()))
            modeInputIdx = -1;
        // Midi CC: Mode is at inputs[1], not inputs[numMaxGUIInputs=0]
        if (nodeType == MIDISIGNAL_ID)
            modeInputIdx = 1;

        int paramStart = typeDef->numReqGUIInputs;
        int paramEnd = typeDef->numMaxGUIInputs;
        int numParams = 0;
        for (int i = paramStart; i < paramEnd && i < (int)typeDef->inputs.size(); i++)
            numParams++;
        if (nodeType == CONSTANT_ID || nodeType == MIDISIGNAL_ID || nodeType > CONSTANT_ID)
            numParams = 1;

        float paramRowH = (kEditKnobDiam);
        float flagsH = 0.f;
        if (modeInputIdx >= 0 && modeInputIdx < (int)typeDef->inputs.size())
        {
            const InputDef& modeDef = typeDef->inputs[modeInputIdx];
            if (!modeDef.modeGroups.empty() || !modeDef.modeFlags.empty())
            {
                flagsH = kEditLabelH + kEditFlagH;
                for (size_t j = 0; j < modeDef.modeGroups.size(); j++)
                    flagsH += kEditFlagH;
            }
        }
        float totalH = kEditHeaderH + (float)numParams * paramRowH + flagsH + 4.f;
        float ph = totalH * zoom;

        if (nodeType == VOICEMANAGER_ID)
        {
            pw = std::max(pw, ArpEditor::kTotalW * zoom + 8.f * zoom);
            ph += 4.f * zoom + ArpEditor::kTotalH * zoom + 4.f * zoom;
        }
        if (nodeType == TRIGGERSEQ_ID)
        {
            int tsMode  = sc->getInputMode((DWORD)nodeID, TRIGGERSEQ_MODE);
            int tsCount = tsMode & (int)TRIGGERSEQ_COUNTMASK;
            if (tsCount < 1 || tsCount > 16) tsCount = 16;
            ph += (4.f + 20.f + 20.f + 12.f + tsCount * 14.f + 4.f) * zoom;
        }
        if (nodeType == SAPI_ID || nodeType == FORMULA_ID)
            ph += (4.f + 72.f + 4.f + 20.f + 8.f) * zoom;

        if (mousePos.x >= px && mousePos.x <= px + pw &&
            mousePos.y >= py && mousePos.y <= py + ph)
        {
            mouseOverEditPanel = true;
            return;
        }
    }
}

void NodeCanvas::drawEditPanel(ImDrawList* dl, const ImVec2& canvasOrigin)
{
    if (openEditPanels.empty())
        return;

    SynthController* sc = SynthController::instance();
    if (!sc || !sc->isInitialized())
    {
        openEditPanels.clear();
        return;
    }

    ImGuiIO& io = ImGui::GetIO();
    ImVec2 mousePos = io.MousePos;
    const float PI = 3.14159265359f;
    const float sweepDeg = 280.f;
    const float startAngle = (180.f + (360.f - sweepDeg) * 0.5f);

    // Use GetKeyState so Ctrl works even when the plugin window lacks keyboard focus
    // (io.KeyCtrl requires WM_KEYDOWN, which many hosts don't forward to plugin windows).
#ifdef _WIN32
    const bool ctrlHeld = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
#else
    const bool ctrlHeld = io.KeyCtrl;
#endif

    // Release knob drag when mouse released
    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
    {
        knobDragNodeID = -1;
        knobDragParam = -1;
    }

    // Snapshot copy so the loop body can safely erase from openEditPanels.
    // Order is preserved: index 0 = bottommost panel, back = topmost (last drawn).
    std::vector<int> panelIDs = openEditPanels;

    // Pre-pass: find the topmost panel (highest index) whose bounding rect contains
    // the mouse.  Only that panel will respond to click events this frame.
    int topmostUnderMouse = -1;
    {
        ImVec2 mpos = ImGui::GetIO().MousePos;
        for (int j = (int)panelIDs.size() - 1; j >= 0; --j)
        {
            int pid = panelIDs[j];
            int pgi = findGuiIndex(pid);
            if (pgi < 0) continue;
            int ptype = sc->gnType(pgi);
            const NodeTypeDef* pdef = NodeConfig::instance().getNodeType(ptype);
            if (!pdef) continue;

            float z2   = zoom;
            ImVec2 np2 = nodeScreenPos(sc->gnX(pgi), sc->gnY(pgi), canvasOrigin);
            float ppx  = np2.x + kNodeWidth * z2;
            float ppy  = np2.y;
            float ppw  = kEditPanelWidth * z2;

            int pmodeIdx = pdef->numMaxGUIInputs;
            if (!(pmodeIdx < pdef->numInputs && pmodeIdx < (int)pdef->inputs.size()))
                pmodeIdx = -1;
            if (ptype == MIDISIGNAL_ID)
                pmodeIdx = 1;
            int pnp = 0;
            for (int k = pdef->numReqGUIInputs; k < pdef->numMaxGUIInputs && k < (int)pdef->inputs.size(); k++)
                pnp++;
            if (ptype == MIDISIGNAL_ID || ptype >= CONSTANT_ID)
                pnp = 1;
            float pfh = 0.f;
            if (pmodeIdx >= 0 && pmodeIdx < (int)pdef->inputs.size())
            {
                const InputDef& md = pdef->inputs[pmodeIdx];
                if (!md.modeGroups.empty() || !md.modeFlags.empty())
                    pfh = (kEditLabelH + kEditFlagH) + (float)md.modeGroups.size() * kEditFlagH;
            }
            float pph = (kEditHeaderH + pnp * (kEditKnobDiam) + pfh + 4.f) * z2;
            if (ptype == VOICEMANAGER_ID)
            {
                ppw = std::max(ppw, ArpEditor::kTotalW * z2 + 8.f * z2);
                pph += 4.f * z2 + ArpEditor::kTotalH * z2 + 4.f * z2;
            }
            if (ptype == TRIGGERSEQ_ID)
            {
                int tsMode  = sc->getInputMode((DWORD)pid, TRIGGERSEQ_MODE);
                int tsCount = tsMode & (int)TRIGGERSEQ_COUNTMASK;
                if (tsCount < 1 || tsCount > 16) tsCount = 16;
                pph += (4.f + 20.f + 20.f + 12.f + tsCount * 14.f + 4.f) * z2;
            }
            if (ptype == SAPI_ID || ptype == FORMULA_ID)
            {
                // text area (72px) + separator + button row (20px) + padding
                pph += (4.f + 72.f + 4.f + 20.f + 8.f) * z2;
            }
            if (mpos.x >= ppx && mpos.x <= ppx + ppw && mpos.y >= ppy && mpos.y <= ppy + pph)
            {
                topmostUnderMouse = pid;
                break;
            }
        }
    }

    // While any ImGui popup is open (e.g. a combo dropdown), suppress all custom
    // click handling so a selectable inside the popup cannot inadvertently trigger
    // knobs, checkboxes, or other controls on panels rendered behind the popup.
    // The popup's own Selectable input is managed by ImGui internally and is unaffected.
    if (ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel))
        topmostUnderMouse = -1;

    for (int nodeID : panelIDs)
    {
        int gi = findGuiIndex(nodeID);
        if (gi < 0)
        {
            openEditPanels.erase(std::find(openEditPanels.begin(), openEditPanels.end(), nodeID));
            for (auto it = paramSyncState.begin(); it != paramSyncState.end(); )
                it = ((uint32_t)(it->first >> 32) == (uint32_t)nodeID) ? paramSyncState.erase(it) : std::next(it);
            textEditBuffers.erase(nodeID);
            continue;
        }

        int nodeType = sc->gnType(gi);
        const NodeTypeDef* typeDef = NodeConfig::instance().getNodeType(nodeType);
        if (!typeDef)
        {
            openEditPanels.erase(std::find(openEditPanels.begin(), openEditPanels.end(), nodeID));
            for (auto it = paramSyncState.begin(); it != paramSyncState.end(); )
                it = ((uint32_t)(it->first >> 32) == (uint32_t)nodeID) ? paramSyncState.erase(it) : std::next(it);
            textEditBuffers.erase(nodeID);
            continue;
        }

        double nx = sc->gnX(gi);
        double ny = sc->gnY(gi);
        ImVec2 nodePos = nodeScreenPos(nx, ny, canvasOrigin);
        float nodeW = kNodeWidth * zoom;
        float z = zoom; // shorthand

        // Panel origin: right edge of node, top-aligned
        float px = nodePos.x + nodeW;
        float py = nodePos.y;
        float pw = kEditPanelWidth * z;
        float fontSize = 12.f * z;
        float headerFontSize = 15.f * z;

        // Compute panel height
        int modeInputIdx = typeDef->numMaxGUIInputs;
        if (!(modeInputIdx < typeDef->numInputs && modeInputIdx < (int)typeDef->inputs.size()))
            modeInputIdx = -1;
        // Midi CC: Mode is at inputs[1]; numMaxGUIInputs=0 would wrongly point at Scale (inputs[0]).
        if (nodeType == MIDISIGNAL_ID)
            modeInputIdx = 1;
        int currentMode = (modeInputIdx >= 0) ? sc->getInputMode((DWORD)nodeID, (DWORD)modeInputIdx) : 0;

        int paramStart = typeDef->numReqGUIInputs;
        int paramEnd = typeDef->numMaxGUIInputs;
        int numParams = 0;
        for (int i = paramStart; i < paramEnd && i < (int)typeDef->inputs.size(); i++)
            numParams++;

        // Constant nodes store their value in node->out (no config inputs); inject one synthetic row.
        bool isConstant = (nodeType == CONSTANT_ID);
        if (isConstant) numParams = 1;
        // Voice param and Midi CC nodes have a Scale input at inputs[0] but numMaxGUIInputs=0,
        // so the regular loop skips it; inject one row to show the Scale knob.
        bool isVoiceInput = (nodeType == MIDISIGNAL_ID || nodeType > CONSTANT_ID);
        if (isVoiceInput) numParams = 1;
        // Constant and voice params use -1..1 signal range; Midi CC keeps its -128..128 config range.
        bool isSignalRange = (isConstant || nodeType > CONSTANT_ID);
        InputDef signalRangeDef;  // used for Constant and voice params
        if (isSignalRange)
        {
            signalRangeDef.name           = isConstant ? "Value" : typeDef->inputs[0].name;
            signalRangeDef.range          = 1;
            signalRangeDef.minVal         = -1.0;
            signalRangeDef.maxVal         =  1.0;
            signalRangeDef.displayMapping = 0;
            signalRangeDef.singleInput    = false;
        }

        // Per param row: knob height + padding (label+sync fit in left column at same height)
        float paramRowH = (kEditKnobDiam);
        float flagsH = 0.f;
        int numFlags = 0;
        if (modeInputIdx >= 0 && modeInputIdx < (int)typeDef->inputs.size())
        {
            const InputDef& modeDef = typeDef->inputs[modeInputIdx];
            if (!modeDef.modeGroups.empty() || !modeDef.modeFlags.empty())
            {
                flagsH = kEditLabelH; // "Flags" label
                // Count visible flags for inline layout
                bool isGlobal = sc->gnIsGlobal(gi);
                for (const auto& mf : modeDef.modeFlags)
                {
                    if (mf.visible != 0 &&
                        ((mf.visible == 1 && isGlobal) || (mf.visible == 2 && !isGlobal)))
                        continue;
                    numFlags++;
                }
                flagsH += kEditFlagH; // one row of inline checkboxes
                for (const auto& mg : modeDef.modeGroups)
                {
                    flagsH += kEditFlagH;
                    (void)mg;
                }
            }
        }

        float totalH = kEditHeaderH + (float)numParams * paramRowH + flagsH + 4.f;
        float ph = totalH * z;

        // VoiceManager: widen/extend panel to include arpeggiator section below params+modes
        bool isVoiceManager = (nodeType == VOICEMANAGER_ID);
        if (isVoiceManager)
        {
            pw = std::max(pw, ArpEditor::kTotalW * z + 8.f * z);
            ph += 4.f * z + ArpEditor::kTotalH * z + 4.f * z;
        }

        // TriggerSequencer: extend panel for Max Patterns control + BPM row + N pattern rows
        bool isTriggerSeq = (nodeType == TRIGGERSEQ_ID);
        if (isTriggerSeq)
        {
            int tsMode  = sc->getInputMode((DWORD)nodeID, TRIGGERSEQ_MODE);
            int tsCount = tsMode & (int)TRIGGERSEQ_COUNTMASK;
            if (tsCount < 1 || tsCount > 16) tsCount = 16;
            ph += (4.f + 20.f + 20.f + 12.f + tsCount * 14.f + 4.f) * z;
        }

        // TextToSpeech / Formula: extend for text area + button
        bool isSAPI    = (nodeType == SAPI_ID);
        bool isFormula = (nodeType == FORMULA_ID);
        if (isSAPI || isFormula)
            ph += (4.f + 72.f + 4.f + 20.f + 8.f) * z;

        // Only the topmost panel under the mouse may consume click events.
        const bool canClick = (nodeID == topmostUnderMouse);

        // ── Draw panel background ──
        ImU32 panelBg = IM_COL32(180, 180, 180, 255);
        ImU32 panelBorder = IM_COL32(100, 100, 105, 255);
        ImU32 headerBg = IM_COL32(160, 160, 165, 255);
        ImU32 textCol = IM_COL32(0, 0, 0, 255);
        ImU32 dimTextCol = IM_COL32(50, 50, 55, 255);

        dl->AddRectFilled(ImVec2(px, py), ImVec2(px + pw, py + ph), panelBg);
        dl->AddRectFilled(ImVec2(px, py), ImVec2(px + pw, py + kEditHeaderH * z), headerBg);
        dl->AddRect(ImVec2(px, py), ImVec2(px + pw, py + ph), panelBorder, 0.f, 0, 1.f);

        // ── Header: title + X button ──
        if (headerFontSize >= 6.f)
        {
            ImVec2 ts = ImGui::CalcTextSize(typeDef->name.c_str());
            float tscale = headerFontSize / ImGui::GetFontSize();
            float tx = px + 6.f * z;
            float ty = py + (kEditHeaderH * z - headerFontSize) * 0.5f;
            dl->AddText(pickFont(headerFontSize), headerFontSize, ImVec2(tx, ty), textCol, typeDef->name.c_str());
            dl->AddText(pickFont(headerFontSize), headerFontSize, ImVec2(tx + 1.f, ty), textCol, typeDef->name.c_str());
        }

        // Red X close button (top-right, same style as node delete button)
        float xbSz = kDeleteBtnSize * z;
        ImVec2 xbMin(px + pw - xbSz - 2.f * z, py + 2.f * z);
        ImVec2 xbMax(xbMin.x + xbSz, xbMin.y + xbSz);
        dl->AddRectFilled(xbMin, xbMax, IM_COL32(200, 40, 40, 255));
        float xpad = 3.f * z;
        dl->AddLine(ImVec2(xbMin.x + xpad, xbMin.y + xpad), ImVec2(xbMax.x - xpad, xbMax.y - xpad), IM_COL32(255, 255, 255, 255), 1.5f);
        dl->AddLine(ImVec2(xbMax.x - xpad, xbMin.y + xpad), ImVec2(xbMin.x + xpad, xbMax.y - xpad), IM_COL32(255, 255, 255, 255), 1.5f);

        // Blue "0" reset-to-defaults button (left of X close button)
        float rbSz = xbSz;
        ImVec2 rbMin(xbMin.x - rbSz - 3.f * z, py + 2.f * z);
        ImVec2 rbMax(rbMin.x + rbSz, rbMin.y + rbSz);
        dl->AddRectFilled(rbMin, rbMax, IM_COL32(30, 90, 200, 255));
        if (fontSize >= 6.f)
        {
            float fw = pickFont(fontSize)->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, "0").x;
            dl->AddText(pickFont(fontSize), fontSize,
                        ImVec2(rbMin.x + (rbSz - fw) * 0.5f, rbMin.y + (rbSz - fontSize) * 0.5f),
                        IM_COL32(255, 255, 255, 255), "0");
        }

        if (canClick &&
            ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
            mousePos.x >= xbMin.x && mousePos.x <= xbMax.x &&
            mousePos.y >= xbMin.y && mousePos.y <= xbMax.y)
        {
            openEditPanels.erase(std::find(openEditPanels.begin(), openEditPanels.end(), nodeID));
            for (auto it = paramSyncState.begin(); it != paramSyncState.end(); )
                it = ((uint32_t)(it->first >> 32) == (uint32_t)nodeID) ? paramSyncState.erase(it) : std::next(it);
            textEditBuffers.erase(nodeID);
            continue;
        }

        // Reset-to-defaults click on "0" button
        if (canClick &&
            ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
            mousePos.x >= rbMin.x && mousePos.x <= rbMax.x &&
            mousePos.y >= rbMin.y && mousePos.y <= rbMax.y)
        {
            sc->resetNodeToDefaults((DWORD)nodeID, (DWORD)nodeType, sc->gnIsGlobal(gi));
            for (auto it = paramSyncState.begin(); it != paramSyncState.end(); )
                it = ((uint32_t)(it->first >> 32) == (uint32_t)nodeID) ? paramSyncState.erase(it) : std::next(it);
        }

        // Separator line under header
        float sepY = py + kEditHeaderH * z;
        dl->AddLine(ImVec2(px, sepY), ImVec2(px + pw, sepY), panelBorder, 1.f);

        // ── Parameter rows ──
        float curY = sepY;
        bool runOnce = (isConstant || isVoiceInput); // these nodes always expose exactly one knob row
        for (int i = runOnce ? 0 : paramStart;
             runOnce ? (i < 1) : (i < paramEnd && i < (int)typeDef->inputs.size());
             i++)
        {
            DWORD paramIdx = isConstant ? (DWORD)-1 : (DWORD)i;
            const InputDef& inputDef = isSignalRange ? signalRangeDef : typeDef->inputs[i];
            float range = (float)inputDef.range;
            if (range <= 0.f) range = 128.f;
            float minVal  = (float)inputDef.minVal;
            float maxVal  = (float)inputDef.maxVal;
            float valRange = maxVal - minVal;   // used for needle normalization only

            // getInputValue returns raw stored value (value/range convention).
            // Display value = rawL * range, which maps to [minVal..maxVal].
            // e.g. Transpose: rawL=0 → valL=0 (center of -64..64).
            // For CONSTANT_ID, paramIdx=(DWORD)-1 and range=1 so rawL IS the display value.
            float rawL = (float)sc->getInputValue((DWORD)nodeID, paramIdx, 0);
            float rawR = (float)sc->getInputValue((DWORD)nodeID, paramIdx, 1);
            float valL = rawL * range;
            float valR = rawR * range;

            // Lazy-init sync state from value equality on first encounter
            uint64_t syncKey = ((uint64_t)(uint32_t)nodeID << 32) | (uint64_t)(uint32_t)paramIdx;
            if (paramSyncState.find(syncKey) == paramSyncState.end())
                paramSyncState[syncKey] = (std::abs(rawL - rawR) < 0.0001f);
            bool synced = paramSyncState[syncKey];

            // Separator line
            dl->AddLine(ImVec2(px, curY), ImVec2(px + pw, curY), panelBorder, 0.5f);

            // Row geometry: left column (label+sync), right area (two knobs)
            float rowH   = (kEditKnobDiam) * z;
            float knobR  = kEditKnobDiam * 0.5f * z;  // outer boundary — ticks fill to here
            float bodyR  = knobR * (17.f / 25.f);     // knob circle (WPF: 17px of 25px half-cell)
            float needleTipR = knobR * 0.82f;         // tip extends past inner tick edge (19/25=0.76)
            float leftColW   = 80.f * z;
            float rightAreaX = px + leftColW;
            float rightAreaW = pw - leftColW;
            float knobCY     = curY + rowH * 0.5f;   // knob center Y (vertically centered)

            // ── Left column: label on top, sync checkbox below ──
            if (fontSize >= 6.f)
                dl->AddText(pickFont(fontSize), fontSize, ImVec2(px + 6.f * z, curY + 2.f * z), textCol, inputDef.name.c_str());

            if (!inputDef.singleInput && fontSize >= 6.f)
            {
                float cbSz = kEditCheckboxSz * z;
                float cbX  = px + 6.f * z;
                float cbY  = curY + (kEditLabelH + 2.f) * z;
                ImVec2 cbMin(cbX, cbY);
                ImVec2 cbMax(cbX + cbSz, cbY + cbSz);
                dl->AddRectFilled(cbMin, cbMax, IM_COL32(30, 30, 35, 255));
                dl->AddRect(cbMin, cbMax, IM_COL32(120, 120, 125, 255), 0.f, 0, 1.f);
                if (synced)
                {
                    dl->AddLine(ImVec2(cbMin.x + 2*z, cbMin.y + cbSz*0.5f),
                                ImVec2(cbMin.x + cbSz*0.4f, cbMax.y - 2*z), IM_COL32(100, 200, 255, 255), 1.5f);
                    dl->AddLine(ImVec2(cbMin.x + cbSz*0.4f, cbMax.y - 2*z),
                                ImVec2(cbMax.x - 2*z, cbMin.y + 2*z), IM_COL32(100, 200, 255, 255), 1.5f);
                }
                dl->AddText(pickFont(fontSize * 0.9f), fontSize * 0.9f,
                            ImVec2(cbMax.x + 3.f * z, cbY), textCol, "Sync");

                // Click to toggle sync
                if (canClick &&
                    ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
                    mousePos.x >= cbMin.x && mousePos.x <= cbMax.x + 35.f * z &&
                    mousePos.y >= cbMin.y && mousePos.y <= cbMax.y)
                {
                    bool newSynced = !synced;
                    paramSyncState[syncKey] = newSynced;
                    if (newSynced)
                        sc->setInputValue((DWORD)nodeID, paramIdx, (double)rawL, (double)rawL);
                }
            }

            // ── Right area: left knob and right knob side by side ──
            // Each knob occupies half of the right area; value text to its right
            float slotW    = rightAreaW * 0.5f;
            float knobL_cx = rightAreaX + knobR + 4.f * z;
            float knobR_cx = rightAreaX + slotW + knobR + 4.f * z;

            // Tick table: WPF-derived. 17 ticks at 17.5° intervals over ±140°.
            // type 2=fat(5 ticks), 1=medium(4 ticks), 0=thin(8 ticks).
            static const struct { float a; int t; } kKnobTicks[] = {
                {   0.f, 2 },
                {  17.5f, 0 }, {  35.f, 1 }, {  52.5f, 0 }, {  70.f, 2 },
                {  87.5f, 0 }, { 105.f, 1 }, { 122.5f, 0 }, { 140.f, 2 },
                { -17.5f, 0 }, { -35.f, 1 }, { -52.5f, 0 }, { -70.f, 2 },
                { -87.5f, 0 }, {-105.f, 1 }, {-122.5f, 0 }, {-140.f, 2 },
            };

            // ── Left knob ──
            {
                ImVec2 center(knobL_cx, knobCY);

                // Tick marks: radii as exact WPF fractions of knobR (bounding=25, inner edge=19)
                for (const auto& tk : kKnobTicks)
                {
                    float rad = tk.a * PI / 180.f;
                    float rO = (tk.t == 2) ? knobR : (tk.t == 1) ? knobR * (23.f/25.f) : knobR * (22.5f/25.f);
                    float rI = knobR * (19.f / 25.f);
                    float th = (tk.t == 2) ? 2.f * z : (tk.t == 1) ? 1.2f * z : 0.75f * z;
                    dl->AddLine(ImVec2(center.x + sinf(rad) * rO, center.y - cosf(rad) * rO),
                                ImVec2(center.x + sinf(rad) * rI, center.y - cosf(rad) * rI),
                                IM_COL32(20, 20, 20, 220), th);
                }

                dl->AddCircleFilled(center, bodyR, IM_COL32(40, 40, 45, 255));
                dl->AddCircle(center, bodyR, IM_COL32(80, 80, 85, 255), 0, 1.5f);

                // Needle: filled triangle, tip extends into tick ring.
                // Perp direction in screen space: (cos a, sin a) for forward (sin a, -cos a).
                float normL = (valRange > 0.f) ? (valL - minVal) / valRange : 0.f;
                normL = std::max(0.f, std::min(1.f, normL));
                float angleDeg = startAngle + normL * sweepDeg;
                float angleRad = angleDeg * PI / 180.f;
                {
                    float sa = sinf(angleRad), ca = cosf(angleRad);
                    float hw = 2.5f * z;
                    dl->AddTriangleFilled(
                        ImVec2(center.x - ca * hw, center.y - sa * hw),
                        ImVec2(center.x + ca * hw, center.y + sa * hw),
                        ImVec2(center.x + sa * needleTipR, center.y - ca * needleTipR),
                        IM_COL32(255, 255, 255, 230));
                    dl->AddCircleFilled(center, hw, IM_COL32(100, 100, 105, 255));
                }

                // Modulator needle: only drawn when a wire is actually connected to this input.
                // getNodeSignal reads the ModAdder's stale out field — gate on connection state first.
                if (!isConstant && sc->inputIsModulated((DWORD)nodeID, paramIdx))
                {
                    double liveL = sc->getNodeSignal((DWORD)nodeID, 0, paramIdx);
                    float normModL = (valRange > 0.f) ? ((float)(liveL * range) - minVal) / valRange : 0.f;
                    normModL = std::max(0.f, std::min(1.f, normModL));
                    float mr = (startAngle + normModL * sweepDeg) * PI / 180.f;
                    float sm = sinf(mr), cm = cosf(mr), hw = 1.5f * z;
                    dl->AddTriangleFilled(
                        ImVec2(center.x - cm * hw, center.y - sm * hw),
                        ImVec2(center.x + cm * hw, center.y + sm * hw),
                        ImVec2(center.x + sm * needleTipR, center.y - cm * needleTipR),
                        IM_COL32(255, 60, 60, 200));
                }

                // Knob drag interaction
                float dxm = mousePos.x - center.x;
                float dym = mousePos.y - center.y;
                if (canClick && dxm*dxm + dym*dym <= knobR*knobR)
                {
                    if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                    {
                        // Reset left channel (and right if synced) to factory default
                        if (isConstant)
                        {
                            sc->setInputValue((DWORD)nodeID, paramIdx, 0.0, synced ? 0.0 : (double)rawR);
                        }
                        else
                        {
                            double defL, defR;
                            sc->getNodeInputDefault((DWORD)nodeType, (DWORD)i, sc->gnIsGlobal(gi), defL, defR);
                            if (synced)
                                sc->setInputValue((DWORD)nodeID, paramIdx, defL, defL);
                            else
                                sc->setInputValue((DWORD)nodeID, paramIdx, defL, (double)rawR);
                        }
                        knobDragNodeID = -1;
                        knobDragParam  = -1;
                    }
                    else if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                    {
                        knobDragNodeID  = nodeID;
                        knobDragParam   = i;
                        knobDragIsRight = false;
                        knobDragAccum   = 0.f;
                    }
                }
                if (knobDragNodeID == nodeID && knobDragParam == i && !knobDragIsRight &&
                    ImGui::IsMouseDragging(ImGuiMouseButton_Left))
                {
                    float delta = -io.MouseDelta.y * 0.5f;
                    float quantStep;
                    if (isSignalRange)
                    {
                        // Signal-range (-1..1): fine step in normal mode, very fine with Ctrl.
                        delta /= 128.f;
                        quantStep = ctrlHeld ? (1.f / (128.f * 128.f)) : (1.f / 128.f);
                    }
                    else
                    {
                        // Normal: integer steps, ~2px each. Ctrl (fine): 1/128 steps.
                        quantStep = ctrlHeld ? (1.f / 128.f) : 1.f;
                        if (ctrlHeld) delta /= 128.f;
                    }
                    knobDragAccum += delta;
                    int stepCount = (int)(knobDragAccum / quantStep);
                    if (stepCount != 0)
                    {
                        float applied = (float)stepCount * quantStep;
                        valL = std::max(minVal, std::min(maxVal, valL + applied));
                        knobDragAccum -= applied;
                        if (synced)
                            sc->setInputValue((DWORD)nodeID, paramIdx, (double)(valL/range), (double)(valL/range));
                        else
                            sc->setInputValue((DWORD)nodeID, paramIdx, (double)(valL/range), (double)(valR/range));
                    }
                }

                // Value text to the right of knob, vertically centered (two lines when applicable)
                if (fontSize >= 6.f)
                {
                    Widgets::KnobLabel vlbl = Widgets::formatKnobValue(valL, range, inputDef.displayMapping, currentMode, nodeType);
                    float valFontSz = fontSize * 0.85f;
                    float valX = center.x + knobR + 3.f * z;
                    if (vlbl.line2.empty())
                    {
                        float valY = center.y - valFontSz * 0.5f;
                        dl->AddText(pickFont(valFontSz), valFontSz, ImVec2(valX, valY), dimTextCol, vlbl.line1.c_str());
                    }
                    else
                    {
                        float valY = center.y - valFontSz;
                        dl->AddText(pickFont(valFontSz), valFontSz, ImVec2(valX, valY),              dimTextCol, vlbl.line1.c_str());
                        dl->AddText(pickFont(valFontSz), valFontSz, ImVec2(valX, valY + valFontSz),  dimTextCol, vlbl.line2.c_str());
                    }
                }
            }

            // ── Right knob (if stereo) ──
            if (!inputDef.singleInput)
            {
                ImVec2 center(knobR_cx, knobCY);

                // 50% transparent when synced, fully opaque when independent
                unsigned int knobAlpha = synced ? 128 : 255;
                unsigned int tickAlpha = synced ? 100 : 200;

                // Tick marks (same WPF proportions as left knob, faded when synced)
                for (const auto& tk : kKnobTicks)
                {
                    float rad = tk.a * PI / 180.f;
                    float rO = (tk.t == 2) ? knobR : (tk.t == 1) ? knobR * (23.f/25.f) : knobR * (22.5f/25.f);
                    float rI = knobR * (19.f / 25.f);
                    float th = (tk.t == 2) ? 2.f * z : (tk.t == 1) ? 1.2f * z : 0.75f * z;
                    dl->AddLine(ImVec2(center.x + sinf(rad) * rO, center.y - cosf(rad) * rO),
                                ImVec2(center.x + sinf(rad) * rI, center.y - cosf(rad) * rI),
                                IM_COL32(20, 20, 20, tickAlpha), th);
                }

                dl->AddCircleFilled(center, bodyR, IM_COL32(40, 40, 45, knobAlpha));
                dl->AddCircle(center, bodyR, IM_COL32(80, 80, 85, knobAlpha), 0, 1.5f);

                float normR = (valRange > 0.f) ? (valR - minVal) / valRange : 0.f;
                normR = std::max(0.f, std::min(1.f, normR));
                float angleDeg = startAngle + normR * sweepDeg;
                float angleRad = angleDeg * PI / 180.f;
                {
                    float sa = sinf(angleRad), ca = cosf(angleRad);
                    float hw = 2.5f * z;
                    dl->AddTriangleFilled(
                        ImVec2(center.x - ca * hw, center.y - sa * hw),
                        ImVec2(center.x + ca * hw, center.y + sa * hw),
                        ImVec2(center.x + sa * needleTipR, center.y - ca * needleTipR),
                        IM_COL32(255, 255, 255, knobAlpha));
                    dl->AddCircleFilled(center, hw, IM_COL32(100, 100, 105, knobAlpha));
                }

                // Modulator needle for right channel (always drawn when wire is connected, same as left)
                if (!isConstant && sc->inputIsModulated((DWORD)nodeID, paramIdx))
                {
                    double liveR = sc->getNodeSignal((DWORD)nodeID, 1, paramIdx);
                    float normModR = (valRange > 0.f) ? ((float)(liveR * range) - minVal) / valRange : 0.f;
                    normModR = std::max(0.f, std::min(1.f, normModR));
                    float mr = (startAngle + normModR * sweepDeg) * PI / 180.f;
                    float sm = sinf(mr), cm = cosf(mr), hw = 1.5f * z;
                    dl->AddTriangleFilled(
                        ImVec2(center.x - cm * hw, center.y - sm * hw),
                        ImVec2(center.x + cm * hw, center.y + sm * hw),
                        ImVec2(center.x + sm * needleTipR, center.y - cm * needleTipR),
                        IM_COL32(255, 60, 60, 200));
                }

                // Only allow interaction when not synced
                if (!synced)
                {
                    float dxm = mousePos.x - center.x;
                    float dym = mousePos.y - center.y;
                    if (canClick && dxm*dxm + dym*dym <= knobR*knobR)
                    {
                        if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                        {
                            // Reset right channel only to factory default
                            if (isConstant)
                            {
                                sc->setInputValue((DWORD)nodeID, paramIdx, (double)rawL, 0.0);
                            }
                            else
                            {
                                double defL, defR;
                                sc->getNodeInputDefault((DWORD)nodeType, (DWORD)i, sc->gnIsGlobal(gi), defL, defR);
                                sc->setInputValue((DWORD)nodeID, paramIdx, (double)rawL, defR);
                            }
                            knobDragNodeID = -1;
                            knobDragParam  = -1;
                        }
                        else if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                        {
                            knobDragNodeID  = nodeID;
                            knobDragParam   = i;
                            knobDragIsRight = true;
                            knobDragAccum   = 0.f;
                        }
                    }
                    if (knobDragNodeID == nodeID && knobDragParam == i && knobDragIsRight &&
                        ImGui::IsMouseDragging(ImGuiMouseButton_Left))
                    {
                        float delta = -io.MouseDelta.y * 0.5f;
                        float quantStep;
                        if (isSignalRange)
                        {
                            delta /= 128.f;
                            quantStep = ctrlHeld ? (1.f / (128.f * 128.f)) : (1.f / 128.f);
                        }
                        else
                        {
                            quantStep = ctrlHeld ? (1.f / 128.f) : 1.f;
                            if (ctrlHeld) delta /= 128.f;
                        }
                        knobDragAccum += delta;
                        int stepCount = (int)(knobDragAccum / quantStep);
                        if (stepCount != 0)
                        {
                            float applied = (float)stepCount * quantStep;
                            valR = std::max(minVal, std::min(maxVal, valR + applied));
                            knobDragAccum -= applied;
                            sc->setInputValue((DWORD)nodeID, paramIdx, (double)(valL/range), (double)(valR/range));
                        }
                    }
                }

                if (fontSize >= 6.f)
                {
                    Widgets::KnobLabel vlbl = Widgets::formatKnobValue(valR, range, inputDef.displayMapping, currentMode, nodeType);
                    float valFontSz  = fontSize * 0.85f;
                    float valX = center.x + knobR + 3.f * z;
                    unsigned int txtAlpha = synced ? 128 : 255;
                    ImU32 valCol = IM_COL32(50, 50, 55, txtAlpha);
                    if (vlbl.line2.empty())
                    {
                        float valY = center.y - valFontSz * 0.5f;
                        dl->AddText(pickFont(valFontSz), valFontSz, ImVec2(valX, valY), valCol, vlbl.line1.c_str());
                    }
                    else
                    {
                        float valY = center.y - valFontSz;
                        dl->AddText(pickFont(valFontSz), valFontSz, ImVec2(valX, valY),              valCol, vlbl.line1.c_str());
                        dl->AddText(pickFont(valFontSz), valFontSz, ImVec2(valX, valY + valFontSz),  valCol, vlbl.line2.c_str());
                    }
                }
            }

            curY += rowH;
        }

        // ── Flags section ──
        if (modeInputIdx >= 0 && modeInputIdx < (int)typeDef->inputs.size())
        {
            const InputDef& modeDef = typeDef->inputs[modeInputIdx];
            bool isGlobal = sc->gnIsGlobal(gi);

            if (!modeDef.modeGroups.empty() || !modeDef.modeFlags.empty())
            {
                dl->AddLine(ImVec2(px, curY), ImVec2(px + pw, curY), panelBorder, 0.5f);

                if (fontSize >= 6.f)
                    dl->AddText(pickFont(fontSize), fontSize, ImVec2(px + 6.f * z, curY + 2.f * z), textCol, "Flags");
                curY += kEditLabelH * z;

                int currentBits = sc->getInputMode((DWORD)nodeID, (DWORD)modeInputIdx);

                // Mode groups (draw as combo-box / dropdown)
                int groupIdx = 0;
                for (const auto& mg : modeDef.modeGroups)
                {
                    int groupVal = (currentBits & (int)mg.mask) >> mg.shift;
                    const char* activeName = "???";
                    int activeIdx = 0;
                    for (int j = 0; j < (int)mg.items.size(); j++)
                    {
                        if (mg.items[j].value == groupVal)
                        {
                            activeName = mg.items[j].name.c_str();
                            activeIdx = j;
                        }
                    }

                    // Draw combo-box button
                    float btnX = px + 6.f * z;
                    float btnW = pw - 12.f * z;
                    float btnH = (kEditFlagH - 2.f) * z;
                    float btnY = curY + 1.f * z;
                    ImVec2 btnMin(btnX, btnY);
                    ImVec2 btnMax(btnX + btnW, btnY + btnH);
                    bool hov = mousePos.x >= btnMin.x && mousePos.x <= btnMax.x &&
                               mousePos.y >= btnMin.y && mousePos.y <= btnMax.y;
                    dl->AddRectFilled(btnMin, btnMax, hov ? IM_COL32(60, 60, 70, 255) : IM_COL32(40, 40, 48, 255));
                    dl->AddRect(btnMin, btnMax, IM_COL32(120, 120, 130, 255), 0.f, 0, 1.f);

                    // Label text inside button
                    if (fontSize >= 6.f)
                    {
                        char label[128];
                        snprintf(label, sizeof(label), "%s: %s", mg.name.c_str(), activeName);
                        float labelFontSz = fontSize * 0.9f;
                        float labelY = btnY + (btnH - labelFontSz) * 0.5f;
                        dl->AddText(pickFont(labelFontSz), labelFontSz, ImVec2(btnX + 4.f * z, labelY),
                                    IM_COL32(220, 220, 230, 255), label);
                    }

                    // Drop arrow on right side
                    {
                        float arrMidX = btnMax.x - 10.f * z;
                        float arrMidY = btnY + btnH * 0.5f;
                        float arrHalf = 4.f * z;
                        dl->AddTriangleFilled(
                            ImVec2(arrMidX - arrHalf, arrMidY - arrHalf * 0.5f),
                            ImVec2(arrMidX + arrHalf, arrMidY - arrHalf * 0.5f),
                            ImVec2(arrMidX, arrMidY + arrHalf * 0.5f),
                            IM_COL32(200, 200, 210, 255));
                    }

                    // Open popup on click
                    char popupID[64];
                    snprintf(popupID, sizeof(popupID), "##mg_%d_%d", nodeID, groupIdx);

                    if (canClick && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && hov)
                        ImGui::OpenPopup(popupID);

                    // Popup list — capped at 8 visible items, scrollbar if more
                    {
                        float itemH   = fontSize * 1.35f;
                        float padY    = ImGui::GetStyle().WindowPadding.y * 2.f;
                        float maxH    = itemH * 8.f + padY;
                        ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(FLT_MAX, maxH));
                    }
                    ImGui::SetNextWindowPos(ImVec2(btnMin.x, btnMax.y));
                    if (ImGui::BeginPopup(popupID))
                    {
                        ImGui::SetWindowFontScale(fontSize / ImGui::GetFont()->FontSize);
                        for (int j = 0; j < (int)mg.items.size(); j++)
                        {
                            bool selected = (j == activeIdx);
                            if (ImGui::Selectable(mg.items[j].name.c_str(), selected, 0, ImVec2(btnW, 0)))
                            {
                                int newBits = (currentBits & ~(int)mg.mask) | (mg.items[j].value << mg.shift);
                                sc->setInputMode((DWORD)nodeID, (DWORD)modeInputIdx, (DWORD)newBits, (DWORD)mg.mask);
                                currentBits = newBits;
                            }
                            if (selected)
                                ImGui::SetItemDefaultFocus();
                        }
                        ImGui::EndPopup();
                    }

                    curY += kEditFlagH * z;
                    groupIdx++;
                }

                // Mode flags as inline checkboxes
                float flagX = px + 6.f * z;
                for (const auto& mf : modeDef.modeFlags)
                {
                    if (mf.visible != 0)
                    {
                        if ((mf.visible == 1 && isGlobal) || (mf.visible == 2 && !isGlobal))
                            continue;
                    }

                    bool flagSet = (currentBits & mf.value) != 0;
                    float cbSz = kEditCheckboxSz * z;

                    // Wrap to next line if needed
                    ImVec2 ts = ImGui::CalcTextSize(mf.name.c_str());
                    float tscale = (fontSize * 0.9f) / ImGui::GetFontSize();
                    float itemW = cbSz + 4.f * z + ts.x * tscale + 8.f * z;
                    if (flagX + itemW > px + pw - 4.f * z)
                    {
                        flagX = px + 6.f * z;
                        curY += kEditFlagH * z;
                    }

                    float cbY = curY + 1.f * z;
                    ImVec2 cbMin(flagX, cbY);
                    ImVec2 cbMax(flagX + cbSz, cbY + cbSz);
                    dl->AddRectFilled(cbMin, cbMax, IM_COL32(30, 30, 35, 255));
                    dl->AddRect(cbMin, cbMax, IM_COL32(120, 120, 125, 255), 0.f, 0, 1.f);
                    if (flagSet)
                    {
                        dl->AddLine(ImVec2(cbMin.x + 2*z, cbMin.y + cbSz*0.5f),
                                    ImVec2(cbMin.x + cbSz*0.4f, cbMax.y - 2*z), IM_COL32(100, 200, 255, 255), 1.5f);
                        dl->AddLine(ImVec2(cbMin.x + cbSz*0.4f, cbMax.y - 2*z),
                                    ImVec2(cbMax.x - 2*z, cbMin.y + 2*z), IM_COL32(100, 200, 255, 255), 1.5f);
                    }
                    if (fontSize >= 6.f)
                    {
                        dl->AddText(pickFont(fontSize * 0.9f), fontSize * 0.9f,
                                    ImVec2(cbMax.x + 2.f * z, cbY), dimTextCol, mf.name.c_str());
                    }

                    // Click checkbox
                    if (canClick &&
                        ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
                        mousePos.x >= cbMin.x && mousePos.x <= cbMin.x + itemW &&
                        mousePos.y >= cbMin.y && mousePos.y <= cbMax.y)
                    {
                        int newBits;
                        if (flagSet)
                            newBits = currentBits & ~mf.value;
                        else
                            newBits = currentBits | mf.value;
                        sc->setInputMode((DWORD)nodeID, (DWORD)modeInputIdx, (DWORD)newBits, 0xffffffff);
                        currentBits = newBits;
                    }

                    flagX += itemW;
                }
            }
        }

        // ── VoiceManager: Arpeggiator editor ──
        if (isVoiceManager)
        {
            // The inline-flags drawing loop never advances curY past the last
            // checkbox row, so we must do it here before the separator line.
            curY += kEditFlagH * z;
            dl->AddLine(ImVec2(px, curY), ImVec2(px + pw, curY), panelBorder, 0.5f);
            curY += 4.f * z;
            ArpEditor::draw(nodeID, z, ImVec2(px + 4.f * z, curY),
                            pickFont(fontSize * 0.9f), fontSize * 0.9f, canClick);
            curY += ArpEditor::kTotalH * z + 4.f * z;
        }

        // ── TriggerSequencer: Max Patterns, BPM sync, then N rows of 8L+8R tick cells ──
        // Data layout: each PATTERN mode word holds 4 patterns packed as bytes.
        // Pattern p: wordIdx = PATTERN0_3L + p/4, byteOfs = (p%4)*8, ticks in bits [byteOfs..byteOfs+7].
        if (isTriggerSeq)
        {
            static const char* kTsBpmNames[33] = {
                "1/128","1/64T","1/128D","1/64","1/32T","1/64D","1/32","1/16T",
                "1/32D","1/16","1/8T","1/16D","1/8","1/4T","1/8D","1/4",
                "1/2T","1/4D","1/2","1T","1/2D","1","1D","3/8",
                "5/8","7/8","9/8","11/8","13/8","15/8","3/4","5/4","7/4"
            };

            int modeWord    = sc->getInputMode((DWORD)nodeID, TRIGGERSEQ_MODE);
            int maxPatterns = modeWord & (int)TRIGGERSEQ_COUNTMASK;
            if (maxPatterns < 1 || maxPatterns > 16) maxPatterns = 16;
            int bpmIdx      = (modeWord & (int)TRIGGERSEQ_BPMMASK) >> 8;
            if (bpmIdx < 0 || bpmIdx > 32) bpmIdx = 0;

            float ctrlH  = 20.f * z;
            float cellH  = 14.f * z;
            // Layout per row: [labelW][8 L cells][gap][8 R cells] + left/right padding
            float labelW  = 18.f * z;
            float cellGap = 4.f * z;
            float cellW   = (pw - 8.f*z - labelW - cellGap) / 16.f;

            curY += 4.f * z;

            // ── Max Patterns row ──
            if (fontSize >= 6.f)
                dl->AddText(pickFont(fontSize), fontSize,
                            ImVec2(px + 4.f*z, curY + (ctrlH - fontSize) * 0.5f),
                            textCol, "Max Patterns:");

            float btnSz = 16.f * z, btnGap = 2.f * z;
            char mpVal[8]; snprintf(mpVal, sizeof(mpVal), "%d", maxPatterns);
            ImVec2 plusMin (px + pw - btnSz - 4.f*z,           curY + (ctrlH - btnSz)*0.5f);
            ImVec2 plusMax (plusMin.x + btnSz,                  plusMin.y + btnSz);
            ImVec2 minusMin(plusMin.x - btnGap - btnSz,         plusMin.y);
            ImVec2 minusMax(minusMin.x + btnSz,                 plusMin.y + btnSz);
            ImVec2 valMin  (minusMin.x - btnGap - btnSz*1.2f,  plusMin.y);
            ImVec2 valMax  (valMin.x + btnSz*1.2f,              plusMin.y + btnSz);
            dl->AddRectFilled(minusMin, minusMax, IM_COL32(70, 70, 80, 255));
            dl->AddRectFilled(plusMin,  plusMax,  IM_COL32(70, 70, 80, 255));
            dl->AddRectFilled(valMin,   valMax,   IM_COL32(50, 50, 58, 255));
            if (fontSize >= 6.f)
            {
                float fw;
                fw = pickFont(fontSize)->CalcTextSizeA(fontSize, FLT_MAX, 0.f, "-").x;
                dl->AddText(pickFont(fontSize), fontSize,
                            ImVec2(minusMin.x + (btnSz-fw)*0.5f, minusMin.y + (btnSz-fontSize)*0.5f),
                            IM_COL32(255,255,255,255), "-");
                fw = pickFont(fontSize)->CalcTextSizeA(fontSize, FLT_MAX, 0.f, "+").x;
                dl->AddText(pickFont(fontSize), fontSize,
                            ImVec2(plusMin.x + (btnSz-fw)*0.5f, plusMin.y + (btnSz-fontSize)*0.5f),
                            IM_COL32(255,255,255,255), "+");
                fw = pickFont(fontSize)->CalcTextSizeA(fontSize, FLT_MAX, 0.f, mpVal).x;
                dl->AddText(pickFont(fontSize), fontSize,
                            ImVec2(valMin.x + (valMax.x-valMin.x-fw)*0.5f, valMin.y + (btnSz-fontSize)*0.5f),
                            IM_COL32(220,220,220,255), mpVal);
            }
            if (canClick && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                if (mousePos.x >= minusMin.x && mousePos.x < minusMax.x &&
                    mousePos.y >= minusMin.y && mousePos.y < minusMax.y && maxPatterns > 1)
                    sc->setInputMode((DWORD)nodeID, TRIGGERSEQ_MODE,
                                     (DWORD)(maxPatterns - 1), (DWORD)TRIGGERSEQ_COUNTMASK);
                if (mousePos.x >= plusMin.x && mousePos.x < plusMax.x &&
                    mousePos.y >= plusMin.y && mousePos.y < plusMax.y && maxPatterns < 16)
                    sc->setInputMode((DWORD)nodeID, TRIGGERSEQ_MODE,
                                     (DWORD)(maxPatterns + 1), (DWORD)TRIGGERSEQ_COUNTMASK);
            }
            curY += ctrlH;

            // ── BPM sync row — full-width combo, same style as mode-group dropdowns ──
            {
                float btnX = px + 6.f * z;
                float btnW = pw - 12.f * z;
                float btnH = (ctrlH - 2.f * z);
                float btnY = curY + 1.f * z;
                ImVec2 btnMin(btnX, btnY);
                ImVec2 btnMax(btnX + btnW, btnY + btnH);
                bool hov = mousePos.x >= btnMin.x && mousePos.x <= btnMax.x &&
                           mousePos.y >= btnMin.y && mousePos.y <= btnMax.y;
                dl->AddRectFilled(btnMin, btnMax, hov ? IM_COL32(60,60,70,255) : IM_COL32(40,40,48,255));
                dl->AddRect(btnMin, btnMax, IM_COL32(120,120,130,255), 0.f, 0, 1.f);

                if (fontSize >= 6.f)
                {
                    char label[64];
                    snprintf(label, sizeof(label), "BPM Sync: %s", kTsBpmNames[bpmIdx]);
                    float lfsz = fontSize * 0.9f;
                    float labelY = btnY + (btnH - lfsz) * 0.5f;
                    dl->AddText(pickFont(lfsz), lfsz, ImVec2(btnX + 4.f*z, labelY),
                                IM_COL32(220,220,230,255), label);
                }

                // Drop arrow
                float arrMidX = btnMax.x - 10.f*z;
                float arrMidY = btnY + btnH * 0.5f;
                float arrHalf = 4.f * z;
                dl->AddTriangleFilled(
                    ImVec2(arrMidX - arrHalf, arrMidY - arrHalf * 0.5f),
                    ImVec2(arrMidX + arrHalf, arrMidY - arrHalf * 0.5f),
                    ImVec2(arrMidX, arrMidY + arrHalf * 0.5f),
                    IM_COL32(200, 200, 210, 255));

                char popupId[64];
                snprintf(popupId, sizeof(popupId), "##tsbpm%d", nodeID);

                if (canClick && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && hov)
                    ImGui::OpenPopup(popupId);

                {
                    float itemH = fontSize * 1.35f;
                    float padY  = ImGui::GetStyle().WindowPadding.y * 2.f;
                    ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(FLT_MAX, itemH * 8.f + padY));
                }
                ImGui::SetNextWindowPos(ImVec2(btnMin.x, btnMax.y));
                ImGui::PushFont(pickFont(fontSize));
                if (ImGui::BeginPopup(popupId))
                {
                    ImGui::SetWindowFontScale(fontSize / ImGui::GetFont()->FontSize);
                    for (int i = 0; i < 33; i++)
                    {
                        bool sel = (i == bpmIdx);
                        if (ImGui::Selectable(kTsBpmNames[i], sel, 0, ImVec2(btnW, 0)))
                            sc->setInputMode((DWORD)nodeID, TRIGGERSEQ_MODE,
                                             (DWORD)(i << 8), (DWORD)TRIGGERSEQ_BPMMASK);
                        if (sel) ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndPopup();
                }
                ImGui::PopFont();
            }
            curY += ctrlH;

            // ── Column header ──
            {
                float gridX = px + 4.f*z + labelW;
                float hdrH  = 12.f * z;
                float smallFsz = fontSize * 0.85f;
                // Tick number labels (1-8 for L, 1-8 for R)
                for (int t = 0; t < 8; t++)
                {
                    char tn[4]; snprintf(tn, sizeof(tn), "%d", t+1);
                    float tx = gridX + t * cellW + cellW * 0.5f;
                    float tw = pickFont(smallFsz)->CalcTextSizeA(smallFsz, FLT_MAX, 0.f, tn).x;
                    if (smallFsz >= 5.f)
                        dl->AddText(pickFont(smallFsz), smallFsz,
                                    ImVec2(tx - tw*0.5f, curY + (hdrH - smallFsz)*0.5f),
                                    dimTextCol, tn);
                    float rx = gridX + 8.f*cellW + cellGap + t*cellW + cellW*0.5f;
                    if (smallFsz >= 5.f)
                        dl->AddText(pickFont(smallFsz), smallFsz,
                                    ImVec2(rx - tw*0.5f, curY + (hdrH - smallFsz)*0.5f),
                                    dimTextCol, tn);
                }
                curY += hdrH;
            }

            // ── Live playback cursor ──
            int playPos     = sc->getTriggerSeqPlayPos((DWORD)nodeID);
            int liveTick    = (playPos >= 0) ? (playPos & 0xFF)        : -1;
            int livePattern = (playPos >= 0) ? ((playPos >> 8) & 0xFF) : -1;

            // ── Pattern rows ──
            for (int p = 0; p < maxPatterns; p++)
            {
                // Row label (pattern index, 1-based)
                char rowLbl[8]; snprintf(rowLbl, sizeof(rowLbl), "%d", p + 1);
                if (fontSize >= 6.f)
                {
                    float lw = pickFont(fontSize)->CalcTextSizeA(fontSize, FLT_MAX, 0.f, rowLbl).x;
                    dl->AddText(pickFont(fontSize), fontSize,
                                ImVec2(px + 4.f*z + (labelW - 4.f*z - lw)*0.5f,
                                       curY + (cellH - fontSize)*0.5f),
                                dimTextCol, rowLbl);
                }

                // Active pattern row: faint highlight behind the row label
                if (p == livePattern)
                    dl->AddRectFilled(ImVec2(px + 4.f*z, curY),
                                      ImVec2(px + 4.f*z + labelW, curY + cellH),
                                      IM_COL32(255, 220, 50, 30));

                // wordIdx: which PATTERN input word holds pattern p
                // byteOfs: which byte within that word (each pattern = 8 bits = 1 byte)
                int wordBase = p / 4;
                int byteOfs  = (p % 4) * 8;
                int lWordIdx = TRIGGERSEQ_PATTERN0_3L + wordBase;
                int rWordIdx = TRIGGERSEQ_PATTERN0_3R + wordBase;
                int lWord    = sc->getInputMode((DWORD)nodeID, (DWORD)lWordIdx);
                int rWord    = sc->getInputMode((DWORD)nodeID, (DWORD)rWordIdx);

                float gridX = px + 4.f*z + labelW;

                for (int t = 0; t < 8; t++)
                {
                    // L cell
                    bool lActive = (lWord >> (byteOfs + t)) & 1;
                    float lcx = gridX + t * cellW;
                    ImVec2 lcMin(lcx + 1.f, curY + 1.f);
                    ImVec2 lcMax(lcx + cellW - 1.f, curY + cellH - 1.f);
                    dl->AddRectFilled(lcMin, lcMax,
                        lActive ? IM_COL32(100, 200, 100, 255) : IM_COL32(45, 50, 45, 255));
                    dl->AddRect(lcMin, lcMax, IM_COL32(30, 30, 30, 160));
                    if (canClick && ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
                        mousePos.x >= lcMin.x && mousePos.x < lcMax.x &&
                        mousePos.y >= lcMin.y && mousePos.y < lcMax.y)
                    {
                        int bit     = byteOfs + t;
                        int newWord = lWord ^ (1 << bit);
                        sc->setInputMode((DWORD)nodeID, (DWORD)lWordIdx,
                                         (DWORD)newWord, (DWORD)(1 << bit));
                    }

                    // R cell
                    bool rActive = (rWord >> (byteOfs + t)) & 1;
                    float rcx = gridX + 8.f*cellW + cellGap + t * cellW;
                    ImVec2 rcMin(rcx + 1.f, curY + 1.f);
                    ImVec2 rcMax(rcx + cellW - 1.f, curY + cellH - 1.f);
                    dl->AddRectFilled(rcMin, rcMax,
                        rActive ? IM_COL32(100, 160, 220, 255) : IM_COL32(45, 45, 55, 255));
                    dl->AddRect(rcMin, rcMax, IM_COL32(30, 30, 30, 160));
                    if (canClick && ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
                        mousePos.x >= rcMin.x && mousePos.x < rcMax.x &&
                        mousePos.y >= rcMin.y && mousePos.y < rcMax.y)
                    {
                        int bit     = byteOfs + t;
                        int newWord = rWord ^ (1 << bit);
                        sc->setInputMode((DWORD)nodeID, (DWORD)rWordIdx,
                                         (DWORD)newWord, (DWORD)(1 << bit));
                    }
                }

                // Live cursor: bright border on the active tick column, active pattern row only
                if (p == livePattern && liveTick >= 0 && liveTick < 8)
                {
                    float lcx = gridX + liveTick * cellW;
                    float rcx = gridX + 8.f*cellW + cellGap + liveTick * cellW;
                    dl->AddRect(ImVec2(lcx, curY), ImVec2(lcx + cellW, curY + cellH),
                                IM_COL32(255, 220, 50, 220), 0.f, 0, 1.5f);
                    dl->AddRect(ImVec2(rcx, curY), ImVec2(rcx + cellW, curY + cellH),
                                IM_COL32(255, 220, 50, 220), 0.f, 0, 1.5f);
                }

                curY += cellH;
            }
            curY += 4.f * z;
        }

        // ── TextToSpeech (SAPI): multiline text entry + Speak button ──
        if (isSAPI)
        {
            // Lazy-init: read current text from core on first display
            if (textEditBuffers.find(nodeID) == textEditBuffers.end())
            {
                std::string txt = sc->getSAPIText((DWORD)nodeID);
                auto& arr = textEditBuffers[nodeID];
                arr.fill(0);
                txt.copy(arr.data(), std::min(txt.size(), arr.size() - 1));
            }
            auto& buf = textEditBuffers[nodeID];

            float textAreaH = 72.f * z;
            float btnH      = 20.f * z;
            float btnW      = 70.f * z;

            dl->AddLine(ImVec2(px, curY), ImVec2(px + pw, curY), panelBorder, 0.5f);
            curY += 4.f * z;

            ImGui::SetCursorScreenPos(ImVec2(px + 4.f*z, curY));
            ImGui::PushFont(pickFont(fontSize));
            std::string textId = "##sapitext" + std::to_string(nodeID);
            ImGui::InputTextMultiline(textId.c_str(), buf.data(), buf.size(),
                                      ImVec2(pw - 8.f*z, textAreaH));
            ImGui::PopFont();
            curY += textAreaH + 4.f * z;

            float btnX = px + pw - btnW - 4.f*z;
            ImVec2 btnMin(btnX, curY);
            ImVec2 btnMax(btnX + btnW, curY + btnH);
            dl->AddRectFilled(btnMin, btnMax, IM_COL32(50, 140, 50, 255));
            if (fontSize >= 6.f)
            {
                const char* lbl = "Update";
                float fw = pickFont(fontSize)->CalcTextSizeA(fontSize, FLT_MAX, 0.f, lbl).x;
                dl->AddText(pickFont(fontSize), fontSize,
                            ImVec2(btnMin.x + (btnW - fw)*0.5f, btnMin.y + (btnH - fontSize)*0.5f),
                            IM_COL32(255, 255, 255, 255), lbl);
            }
            if (canClick && ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
                mousePos.x >= btnMin.x && mousePos.x <= btnMax.x &&
                mousePos.y >= btnMin.y && mousePos.y <= btnMax.y)
            {
                sc->setSAPIText((DWORD)nodeID, std::string(buf.data()));
            }
            curY += btnH + 4.f * z;
        }

        // ── Formula: multiline text entry + Update button ──
        if (isFormula)
        {
            // Lazy-init: read formula text from core on first display
            if (textEditBuffers.find(nodeID) == textEditBuffers.end())
            {
                std::string txt = sc->getFormulaText((DWORD)nodeID);
                auto& arr = textEditBuffers[nodeID];
                arr.fill(0);
                txt.copy(arr.data(), std::min(txt.size(), arr.size() - 1));
            }
            auto& buf = textEditBuffers[nodeID];

            float textAreaH = 72.f * z;
            float btnH      = 20.f * z;
            float btnW      = 70.f * z;

            dl->AddLine(ImVec2(px, curY), ImVec2(px + pw, curY), panelBorder, 0.5f);
            curY += 4.f * z;

            ImGui::SetCursorScreenPos(ImVec2(px + 4.f*z, curY));
            ImGui::PushFont(pickFont(fontSize));
            std::string textId = "##formulatext" + std::to_string(nodeID);
            ImGui::InputTextMultiline(textId.c_str(), buf.data(), buf.size(),
                                      ImVec2(pw - 8.f*z, textAreaH));
            ImGui::PopFont();
            curY += textAreaH + 4.f * z;

            float btnX = px + pw - btnW - 4.f*z;
            ImVec2 btnMin(btnX, curY);
            ImVec2 btnMax(btnX + btnW, curY + btnH);
            dl->AddRectFilled(btnMin, btnMax, IM_COL32(50, 140, 50, 255));
            if (fontSize >= 6.f)
            {
                const char* lbl = "Update";
                float fw = pickFont(fontSize)->CalcTextSizeA(fontSize, FLT_MAX, 0.f, lbl).x;
                dl->AddText(pickFont(fontSize), fontSize,
                            ImVec2(btnMin.x + (btnW - fw)*0.5f, btnMin.y + (btnH - fontSize)*0.5f),
                            IM_COL32(255, 255, 255, 255), lbl);
            }
            if (canClick && ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
                mousePos.x >= btnMin.x && mousePos.x <= btnMax.x &&
                mousePos.y >= btnMin.y && mousePos.y <= btnMax.y)
            {
                // Pass the same text as both formula and RPN (full RPN compiler is deferred).
                std::string txt(buf.data());
                sc->setFormulaText((DWORD)nodeID, txt, txt);
            }
            curY += btnH + 4.f * z;
        }
    }
}

// ── Rubber Band Drawing ─────────────────────────────────────────────────

void NodeCanvas::drawRubberBand(ImDrawList* dl)
{
    if (!isRubberBanding)
        return;

    ImVec2 rMin(std::min(rubberBandStart.x, rubberBandCurrent.x),
                std::min(rubberBandStart.y, rubberBandCurrent.y));
    ImVec2 rMax(std::max(rubberBandStart.x, rubberBandCurrent.x),
                std::max(rubberBandStart.y, rubberBandCurrent.y));

    dl->AddRectFilled(rMin, rMax, colorRubberBandFill());
    dl->AddRect(rMin, rMax, colorRubberBandBorder(), 0.f, 0, 1.f);
}

// ── Pan/Zoom (with drag guard) ───────────────────────────────────────────

void NodeCanvas::handlePanZoom(const ImVec2& canvasPos, const ImVec2& canvasSize)
{
    ImGuiIO& io = ImGui::GetIO();

    // Zoom with mouse wheel
    if (canvasHovered && io.MouseWheel != 0.f)
    {
        float oldZoom = zoom;
        zoom *= (io.MouseWheel > 0) ? 1.1f : (1.f / 1.1f);
        zoom = std::max(0.03f, std::min(zoom, 5.0f));

        // Zoom toward mouse position
        ImVec2 mouseRel = ImVec2(io.MousePos.x - canvasPos.x,
                                  io.MousePos.y - canvasPos.y);
        float zoomRatio = zoom / oldZoom;
        offsetX = mouseRel.x / zoom - (mouseRel.x / oldZoom - offsetX);
        offsetY = mouseRel.y / zoom - (mouseRel.y / oldZoom - offsetY);
    }

    // Pan with right mouse button drag — guard against node dragging and context menu
    if (canvasHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right) && !isDragging && !showContextMenu)
    {
        isPanning = true;
        didPan = false;
        panStart = io.MousePos;
    }
    if (isPanning)
    {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Right))
        {
            ImVec2 delta = ImVec2(io.MousePos.x - panStart.x, io.MousePos.y - panStart.y);
            if (std::abs(delta.x) > 2.f || std::abs(delta.y) > 2.f)
                didPan = true;
            offsetX += delta.x / zoom;
            offsetY += delta.y / zoom;
            panStart = io.MousePos;
        }
        else
        {
            isPanning = false;
        }
    }
}

// ── Node Drawing ─────────────────────────────────────────────────────────

bool NodeCanvas::drawNode(ImDrawList* dl, int guiIndex, const ImVec2& canvasOrigin)
{
    SynthController* sc = SynthController::instance();
    if (!sc) return false;

    int nodeID = sc->gnID(guiIndex);
    double nx = sc->gnX(guiIndex);
    double ny = sc->gnY(guiIndex);
    bool isGlobal = sc->gnIsGlobal(guiIndex);
    int numSignals = effectiveInputCount(guiIndex);
    int numReq = sc->gnNodeReqSignals(guiIndex);
    int nodeType = sc->gnType(guiIndex);
    bool hasEditBtn = nodeHasEditButton(guiIndex);
    bool hasAddInput = (nodeType == MULTIADD_ID || nodeType == NOTECONTROLLER_ID);
    bool hasChannelBtns = (nodeType == CHANNELROOT_ID);

    ImVec2 pos = nodeScreenPos(nx, ny, canvasOrigin);
    float w = kNodeWidth * zoom;
    float h = nodeHeight(numSignals, hasEditBtn, hasAddInput, hasChannelBtns) * zoom;

    bool isSelected = selectedNodeIDs.count(nodeID) > 0;
    bool isMuted = mutedNodeIDs.count(nodeID) > 0;

    // Node background — selected nodes get gold bg
    ImU32 bgColor = isSelected ? colorSelectedNode()
                               : (isGlobal ? colorGlobalNode() : colorVoiceNode());
    ImU32 borderColor = colorNodeBorder();
    ImU32 textColor = colorNodeText();

    if (isMuted)
    {
        bgColor = withMuteAlpha(bgColor);
        borderColor = withMuteAlpha(borderColor);
        textColor = withMuteAlpha(textColor);
    }

    dl->AddRectFilled(pos, ImVec2(pos.x + w, pos.y + h), bgColor, 2.f * zoom);

    // Selection: gold 2px border; normal: 1px black border
    if (isSelected)
    {
        ImU32 selBorder = isMuted ? withMuteAlpha(colorSelectedNode()) : colorSelectedNode();
        dl->AddRect(pos, ImVec2(pos.x + w, pos.y + h), selBorder, 2.f * zoom, 0, 2.f);
    }
    else
    {
        dl->AddRect(pos, ImVec2(pos.x + w, pos.y + h), borderColor, 2.f * zoom, 0, 1.f);
    }

    // Search highlight: cyan outer border when the node matches the filter
    if (!searchFilter.empty())
    {
        const NodeTypeDef* sDef   = NodeConfig::instance().getNodeType(nodeType);
        const char*        sType  = sDef ? sDef->name.c_str() : "";
        std::string        sName  = sc->gnName(guiIndex);
        const char*        sCheck = sName.empty() ? sType : sName.c_str();

        // Case-insensitive substring match against custom name and type name
        auto toLower = [](std::string s) {
            for (auto& c : s) c = (char)tolower((unsigned char)c);
            return s;
        };
        std::string needleLow = toLower(searchFilter);
        bool match = toLower(std::string(sCheck)).find(needleLow) != std::string::npos
                  || toLower(std::string(sType )).find(needleLow) != std::string::npos;
        if (match)
        {
            float ex = 2.f;
            dl->AddRect(ImVec2(pos.x - ex, pos.y - ex),
                        ImVec2(pos.x + w + ex, pos.y + h + ex),
                        colorSelectedNode(), (2.f + ex) * zoom, 0, 2.f);
        }
    }

    // Red X delete button (top-left corner)
    // VoiceRoot is deletable; only SynthRoot/ChannelRoot/NoteController/VoiceManager are protected.
    bool isStructural = (nodeType >= 0 && nodeType <= (int)VOICEMANAGER_ID);
    if (!isStructural)
    {
        float xbtnSz = kDeleteBtnSize * zoom;
        float xInset = 2.f * zoom;
        ImVec2 xMin(pos.x + xInset, pos.y + xInset);
        ImVec2 xMax(pos.x + xInset + xbtnSz, pos.y + xInset + xbtnSz);

        // Red background square
        ImU32 xBg = isMuted ? withMuteAlpha(IM_COL32(200, 40, 40, 255)) : IM_COL32(200, 40, 40, 255);
        dl->AddRectFilled(xMin, xMax, xBg);

        // White X lines
        float pad = 3.f * zoom;
        ImU32 xLineCol = isMuted ? withMuteAlpha(IM_COL32(255, 255, 255, 255)) : IM_COL32(255, 255, 255, 255);
        dl->AddLine(ImVec2(xMin.x + pad, xMin.y + pad), ImVec2(xMax.x - pad, xMax.y - pad), xLineCol, 1.5f);
        dl->AddLine(ImVec2(xMax.x - pad, xMin.y + pad), ImVec2(xMin.x + pad, xMax.y - pad), xLineCol, 1.5f);

        // Hit-test X button
        ImGuiIO& io = ImGui::GetIO();
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !isWireDragging && !isRenaming && !mouseOverEditPanel && canvasHovered)
        {
            ImVec2 mpos = io.MousePos;
            if (mpos.x >= xMin.x && mpos.x <= xMax.x &&
                mpos.y >= xMin.y && mpos.y <= xMax.y)
            {
                sc->killVoices();
                // If this node is selected, delete all selected nodes
                if (isSelected && !selectedNodeIDs.empty())
                {
                    std::vector<int> toDelete(selectedNodeIDs.begin(), selectedNodeIDs.end());
                    for (int id : toDelete)
                    {
                        int gi = findGuiIndex(id);
                        int tp = (gi >= 0) ? sc->gnType(gi) : -1;
                        if (tp >= 0 && tp <= (int)VOICEMANAGER_ID) continue;
                        deleteNodeMaybeSmart(id, (int)toDelete.size() == 1);
                    }
                    selectedNodeIDs.clear();
                }
                else
                {
                    // Delete just this node (smart reconnect when applicable)
                    deleteNodeMaybeSmart(nodeID, true);
                    selectedNodeIDs.erase(nodeID);
                }
                syncSelectionToCore();
                return true; // node deleted — outer loop must stop
            }
        }
    }

    // Header text — bold and slightly larger
    const NodeTypeDef* typeDef = NodeConfig::instance().getNodeType(nodeType);
    const char* displayName = typeDef ? typeDef->name.c_str() : "???";

    // Use custom name if set; ChannelRoot uses "Channel N" or "N : name"
    std::string customName = sc->gnName(guiIndex);
    std::string channelDisplayName;
    if (nodeType == CHANNELROOT_ID)
    {
        int ch1 = sc->gnChannel(guiIndex) + 1;
        if (customName.empty())
        {
            char buf[32];
            snprintf(buf, sizeof(buf), "Channel %d", ch1);
            channelDisplayName = buf;
        }
        else
        {
            char buf[288];
            snprintf(buf, sizeof(buf), "%d : %s", ch1, customName.c_str());
            channelDisplayName = buf;
        }
        displayName = channelDisplayName.c_str();
    }
    else if (!customName.empty())
        displayName = customName.c_str();

    float fontSize = 12.f * zoom;         // body text size
    float headerFontSize = 15.f * zoom;   // header is larger
    if (headerFontSize >= 6.f)
    {
        // Draw header text twice offset by 1px for faux bold
        ImVec2 textSize = ImGui::CalcTextSize(displayName);
        float textScale = headerFontSize / ImGui::GetFontSize();
        float headerTextW = textSize.x * textScale;
        // Center in header, leaving room for X button on left
        float textX = pos.x + (w - headerTextW) * 0.5f;
        float textY = pos.y + (kHeaderHeight * zoom - headerFontSize) * 0.5f;
        // Faux bold: draw at +0 and +1 pixel offset
        dl->AddText(pickFont(headerFontSize), headerFontSize, ImVec2(textX, textY), textColor, displayName);
        dl->AddText(pickFont(headerFontSize), headerFontSize, ImVec2(textX + 1.f, textY), textColor, displayName);
    }

    // VU meter overlay on header (SynthRoot / ChannelRoot)
    if (nodeType == SYNTHROOT_ID || nodeType == CHANNELROOT_ID)
    {
        auto it = liveDataCache.find(nodeID);
        if (it != liveDataCache.end())
        {
            float vuW = 6.f * zoom;
            float vuH = (kHeaderHeight - 4.f) * zoom;
            float vuX = pos.x + w - 18.f * zoom;
            float vuY = pos.y + 2.f * zoom;

            // Left bar
            float lvlL = std::min(it->second.vuL, 1.f);
            float barHL = lvlL * vuH;
            ImU32 colL = (lvlL < 0.85f) ? IM_COL32(40, 200, 40, 200) : IM_COL32(220, 40, 40, 200);
            dl->AddRectFilled(ImVec2(vuX, vuY + vuH - barHL), ImVec2(vuX + vuW, vuY + vuH), colL);

            // Right bar
            float lvlR = std::min(it->second.vuR, 1.f);
            float barHR = lvlR * vuH;
            ImU32 colR = (lvlR < 0.85f) ? IM_COL32(40, 200, 40, 200) : IM_COL32(220, 40, 40, 200);
            dl->AddRectFilled(ImVec2(vuX + vuW + 1.f * zoom, vuY + vuH - barHR),
                              ImVec2(vuX + 2 * vuW + 1.f * zoom, vuY + vuH), colR);
        }
    }

    // Voice count on VoiceManager
    if (nodeType == VOICEMANAGER_ID && fontSize >= 6.f)
    {
        auto it = liveDataCache.find(nodeID);
        if (it != liveDataCache.end() && it->second.voiceCount > 0)
        {
            char vcBuf[16];
            snprintf(vcBuf, sizeof(vcBuf), "%d", it->second.voiceCount);
            float vcX = pos.x + w - 20.f * zoom;
            float vcY = pos.y + 3.f * zoom;
            dl->AddText(pickFont(fontSize * 0.8f), fontSize * 0.8f, ImVec2(vcX, vcY),
                        IM_COL32(255, 255, 255, 220), vcBuf);
        }
    }

    // Header separator line
    float sepY = pos.y + kHeaderHeight * zoom;
    dl->AddLine(ImVec2(pos.x, sepY), ImVec2(pos.x + w, sepY), borderColor, 1.f);

    // Output pin (right side of header) — green if connected, red if not
    ImVec2 outPin = outputPinPos(pos);
    bool outputConnected = connectedOutputIDs.count(nodeID) > 0;
    ImU32 outPinFill = outputConnected ? colorPinWired() : colorPinRequired();
    if (isMuted) outPinFill = withMuteAlpha(outPinFill);
    ImU32 outPinBorder = isMuted ? withMuteAlpha(colorNodeBorder()) : colorNodeBorder();
    dl->AddCircleFilled(outPin, kPinRadius * zoom, outPinFill);
    dl->AddCircle(outPin, kPinRadius * zoom, outPinBorder, 0, 1.f);

    // Input pins
    for (int i = 0; i < numSignals; i++)
    {
        ImVec2 pinPos = inputPinPos(pos, i);
        int srcID = sc->gnInput(guiIndex, i);
        bool isWired = isRealConnection(srcID, sc);

        ImU32 pinColor;
        if (isWired)
            pinColor = colorPinWired();
        else if (i < numReq)
            pinColor = colorPinRequired();
        else
            pinColor = colorPinOptional();

        ImU32 pinBorder = colorNodeBorder();
        if (isMuted)
        {
            pinColor = withMuteAlpha(pinColor);
            pinBorder = withMuteAlpha(pinBorder);
        }

        dl->AddCircleFilled(pinPos, kPinRadius * zoom, pinColor);
        dl->AddCircle(pinPos, kPinRadius * zoom, pinBorder, 0, 1.f);

        // Input label (to the right of the pin, inside the node)
        if (typeDef && i < (int)typeDef->inputs.size() && fontSize >= 6.f)
        {
            const char* label = typeDef->inputs[i].name.c_str();
            float labelX = pinPos.x + (kPinRadius + 3.f) * zoom;
            float labelY = pinPos.y - fontSize * 0.5f;
            dl->AddText(pickFont(fontSize), fontSize,
                        ImVec2(labelX, labelY), textColor, label);
        }
    }

    // Edit button at bottom of node
    if (hasEditBtn)
    {
        float btnY = pos.y + (kHeaderHeight + (float)numSignals * kRowHeight) * zoom;
        float btnH = kEditButtonHeight * zoom;
        ImVec2 btnMin(pos.x, btnY);
        ImVec2 btnMax(pos.x + w, btnY + btnH);

        // Gray button-like background with slight 3D effect
        ImU32 btnBg = IM_COL32(180, 180, 180, 255);
        if (isMuted)
            btnBg = withMuteAlpha(btnBg);
        float btnInset = 2.f * zoom;
        ImVec2 btnInMin(btnMin.x + btnInset, btnMin.y + 1.f * zoom);
        ImVec2 btnInMax(btnMax.x - btnInset, btnMax.y - 1.f * zoom);
        dl->AddRectFilled(btnInMin, btnInMax, btnBg, 2.f * zoom);
        // Light top edge, dark bottom edge for 3D look
        ImU32 btnHi = isMuted ? withMuteAlpha(IM_COL32(220, 220, 220, 255)) : IM_COL32(220, 220, 220, 255);
        ImU32 btnLo = isMuted ? withMuteAlpha(IM_COL32(120, 120, 120, 255)) : IM_COL32(120, 120, 120, 255);
        dl->AddLine(ImVec2(btnInMin.x, btnInMin.y), ImVec2(btnInMax.x, btnInMin.y), btnHi, 1.f);
        dl->AddLine(ImVec2(btnInMin.x, btnInMax.y), ImVec2(btnInMax.x, btnInMax.y), btnLo, 1.f);

        // Mode text
        if (fontSize >= 6.f)
        {
            std::string modeText = buildModeText(guiIndex);
            ImVec2 textSize = ImGui::CalcTextSize(modeText.c_str());
            float textScale = fontSize / ImGui::GetFontSize();
            float textX = pos.x + (w - textSize.x * textScale) * 0.5f;
            float textY = btnY + (btnH - fontSize) * 0.5f;
            dl->AddText(pickFont(fontSize), fontSize,
                        ImVec2(textX, textY), textColor, modeText.c_str());
        }

        // Hit-test: detect click on button to open/toggle edit panel
        ImGuiIO& io = ImGui::GetIO();
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !isWireDragging && !isRenaming && !mouseOverEditPanel && canvasHovered)
        {
            ImVec2 mpos = io.MousePos;
            if (mpos.x >= btnMin.x && mpos.x <= btnMax.x &&
                mpos.y >= btnMin.y && mpos.y <= btnMax.y)
            {
                {
                    auto _it = std::find(openEditPanels.begin(), openEditPanels.end(), nodeID);
                    if (_it != openEditPanels.end())
                    {
                        openEditPanels.erase(_it);
                        for (auto it = paramSyncState.begin(); it != paramSyncState.end(); )
                            it = ((uint32_t)(it->first >> 32) == (uint32_t)nodeID) ? paramSyncState.erase(it) : std::next(it);
                    }
                    else
                        openEditPanels.push_back(nodeID);  // push_back = topmost (last drawn)
                }
            }
        }
    }

    // ── Load/Save Channel buttons (ChannelRoot only) ──
    if (hasChannelBtns)
    {
        int ch = sc->gnChannel(guiIndex);
        float btnBaseY = pos.y + (kHeaderHeight + (float)numSignals * kRowHeight
                                  + (hasEditBtn ? kEditButtonHeight : 0.f)) * zoom;
        float btnH = kEditButtonHeight * zoom;

        const char* btnLabels[2] = { "Load Channel", "Save Channel" };
        for (int bi = 0; bi < 2; bi++)
        {
            float btnY = btnBaseY + bi * btnH;
            ImVec2 btnMin(pos.x, btnY);
            ImVec2 btnMax(pos.x + w, btnY + btnH);

            ImU32 btnBg = isMuted ? withMuteAlpha(IM_COL32(160, 160, 160, 255)) : IM_COL32(160, 160, 160, 255);
            float btnInset = 2.f * zoom;
            ImVec2 btnInMin(btnMin.x + btnInset, btnMin.y + 1.f * zoom);
            ImVec2 btnInMax(btnMax.x - btnInset, btnMax.y - 1.f * zoom);
            dl->AddRectFilled(btnInMin, btnInMax, btnBg, 2.f * zoom);
            ImU32 btnHi = isMuted ? withMuteAlpha(IM_COL32(200, 200, 200, 255)) : IM_COL32(200, 200, 200, 255);
            ImU32 btnLo = isMuted ? withMuteAlpha(IM_COL32(100, 100, 100, 255)) : IM_COL32(100, 100, 100, 255);
            dl->AddLine(ImVec2(btnInMin.x, btnInMin.y), ImVec2(btnInMax.x, btnInMin.y), btnHi, 1.f);
            dl->AddLine(ImVec2(btnInMin.x, btnInMax.y), ImVec2(btnInMax.x, btnInMax.y), btnLo, 1.f);

            if (fontSize >= 6.f)
            {
                ImVec2 ts = ImGui::CalcTextSize(btnLabels[bi]);
                float tscale = fontSize / ImGui::GetFontSize();
                float tx = pos.x + (w - ts.x * tscale) * 0.5f;
                float ty = btnY + (btnH - fontSize) * 0.5f;
                dl->AddText(pickFont(fontSize), fontSize, ImVec2(tx, ty), textColor, btnLabels[bi]);
            }

            // Click handling
            ImGuiIO& io = ImGui::GetIO();
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !isWireDragging && !isRenaming && !mouseOverEditPanel && canvasHovered)
            {
                ImVec2 mpos = io.MousePos;
                if (mpos.x >= btnMin.x && mpos.x <= btnMax.x &&
                    mpos.y >= btnMin.y && mpos.y <= btnMax.y)
                {
#ifdef _WIN32
                    char buf[512] = {"MyChannel.64k2Channel"};
                    OPENFILENAMEA ofn  = {};
                    ofn.lStructSize    = sizeof(ofn);
                    ofn.hwndOwner      = (HWND)K64GUI::getWindowHandle();
                    ofn.lpstrFilter    = "64klang2 Channel\0*.64k2Channel\0All Files\0*.*\0";
                    ofn.lpstrFile      = buf;
                    ofn.nMaxFile       = sizeof(buf);
                    ofn.lpstrDefExt    = "64k2Channel";
                    ofn.Flags          = OFN_NOCHANGEDIR;
                    if (bi == 0)
                    {
                        ofn.Flags |= OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
                        bool ok = GetOpenFileNameA(&ofn) != 0;
                        { MSG m; while (PeekMessageA(&m, nullptr, WM_KEYFIRST, WM_KEYLAST, PM_REMOVE)) {} }
                        ImGui::GetIO().AddMouseButtonEvent(0, false);
                        if (ok && sc->loadChannel(ch, std::string(buf)))
                        {
                            selectedNodeIDs.clear();
                            syncSelectionToCore();
                            sc->numGUINodes(); // rebuild _nodesGUIAccessor — loadChannel invalidated it
                            return true;       // break draw loop, same as node deletion
                        }
                    }
                    else
                    {
                        ofn.Flags |= OFN_OVERWRITEPROMPT;
                        bool ok = GetSaveFileNameA(&ofn) != 0;
                        { MSG m; while (PeekMessageA(&m, nullptr, WM_KEYFIRST, WM_KEYLAST, PM_REMOVE)) {} }
                        ImGui::GetIO().AddMouseButtonEvent(0, false);
                        if (ok)
                            sc->saveChannel(ch, std::string(buf));
                    }
#endif
                }
            }
        }
    }

    // ── "Add Input" pill (MultiAdd / NoteController only) ──
    if (hasAddInput && numSignals < 16)
    {
        // Position: immediately below all existing inputs (and edit button if any)
        float pillBaseY = pos.y + nodeHeight(numSignals, hasEditBtn, false) * zoom;
        float pillH     = kRowHeight * zoom;
        float pillInset = 8.f * zoom;
        ImVec2 pillMin(pos.x + pillInset, pillBaseY + 2.f * zoom);
        ImVec2 pillMax(pos.x + w - pillInset, pillBaseY + pillH - 2.f * zoom);

        // Highlight when a wire is being dragged toward it
        bool wireHover = isWireDragging && ImGui::IsMouseHoveringRect(pillMin, pillMax, false);
        ImU32 pillBg = wireHover
            ? IM_COL32(50, 180, 80, 200)
            : IM_COL32(40, 40, 45, 160);
        ImU32 pillBorder = wireHover
            ? IM_COL32(100, 255, 130, 255)
            : IM_COL32(120, 120, 130, 180);
        ImU32 pillText = wireHover
            ? IM_COL32(220, 255, 220, 255)
            : IM_COL32(160, 160, 170, 255);

        float pillRadius = (pillMax.y - pillMin.y) * 0.4f;
        dl->AddRectFilled(pillMin, pillMax, pillBg, pillRadius);
        dl->AddRect(pillMin, pillMax, pillBorder, pillRadius, 0, 1.f);

        if (fontSize >= 6.f)
        {
            const char* label = "+ Add Input";
            ImVec2 ts = ImGui::CalcTextSize(label);
            float tscale = fontSize / ImGui::GetFontSize();
            float tx = pillMin.x + ((pillMax.x - pillMin.x) - ts.x * tscale) * 0.5f;
            float ty = pillMin.y + ((pillMax.y - pillMin.y) - fontSize) * 0.5f;
            dl->AddText(pickFont(fontSize), fontSize, ImVec2(tx, ty), pillText, label);
        }
    }

    return false;
}

// ── Wire Drawing ─────────────────────────────────────────────────────────

void NodeCanvas::drawWires(ImDrawList* dl, const ImVec2& canvasOrigin)
{
    SynthController* sc = SynthController::instance();
    if (!sc) return;

    int numNodes = sc->numGUINodes();

    for (int toIdx = 0; toIdx < numNodes; toIdx++)
    {
        if (!sc->gnIsVisible(toIdx))
            continue;

        int toNodeID = sc->gnID(toIdx);
        int numSignals = effectiveInputCount(toIdx);
        double toNX = sc->gnX(toIdx);
        double toNY = sc->gnY(toIdx);
        ImVec2 toPos = nodeScreenPos(toNX, toNY, canvasOrigin);

        bool toSelected = selectedNodeIDs.count(toNodeID) > 0;

        for (int pin = 0; pin < numSignals; pin++)
        {
            int srcID = sc->gnInput(toIdx, pin);
            if (!isRealConnection(srcID, sc))
                continue;

            // Find the source node's GUI index
            int fromIdx = -1;
            for (int j = 0; j < numNodes; j++)
            {
                if (sc->gnID(j) == srcID)
                {
                    fromIdx = j;
                    break;
                }
            }
            if (fromIdx < 0 || !sc->gnIsVisible(fromIdx))
                continue;

            double fromNX = sc->gnX(fromIdx);
            double fromNY = sc->gnY(fromIdx);
            ImVec2 fromPos = nodeScreenPos(fromNX, fromNY, canvasOrigin);

            ImVec2 p0 = outputPinPos(fromPos);
            ImVec2 p3 = inputPinPos(toPos, pin);
            ImVec2 p1 = ImVec2(p0.x + kWireStubLen * zoom, p0.y);
            ImVec2 p2 = ImVec2(p3.x - kWireStubLen * zoom, p3.y);

            // Wire color: context wire (insertion target) = SpringGreen, selected = gold, else by scope
            ImU32 wireColor;
            bool isContextWire = (contextWireFromID == (int)srcID && contextWireToID == toNodeID && contextWirePinIndex == pin);
            if (isContextWire)
                wireColor = colorContextWire();
            else if (toSelected)
                wireColor = colorSelectedWire();
            else
            {
                bool fromGlobal = sc->gnIsGlobal(fromIdx);
                wireColor = fromGlobal ? colorGlobalWire() : colorVoiceWire();
            }

            dl->AddLine(p0, p1, wireColor, kWireThickness * zoom);
            dl->AddLine(p1, p2, wireColor, kWireThickness * zoom);
            dl->AddLine(p2, p3, wireColor, kWireThickness * zoom);
        }
    }
}

// ── Render ───────────────────────────────────────────────────────────────

void NodeCanvas::render()
{
    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    canvasSizeCache = canvasSize;

    // Create an invisible button covering the canvas area to capture input
    ImGui::InvisibleButton("##canvas", canvasSize,
                           ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
    canvasHovered = ImGui::IsItemHovered();

    // Suppress canvas interaction while any popup (combo dropdown, context menu, …) is open.
    // Without this, the click that dismisses the popup also hits the canvas.
    if (ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel))
        canvasHovered = false;

    SynthController* sc = SynthController::instance();

    // Patch-load reset: if node count changed drastically, reset view.
    // Small changes (±1..2 nodes) are normal add/delete — only update the count.
    if (sc && sc->isInitialized())
    {
        int currentCount = sc->numGUINodes();
        if (currentCount != lastNodeCount)
        {
            // Only do a full reset when transitioning from 0 nodes (actual patch load).
            // For ordinary add/delete — even bulk paste/delete — do NOT clear edit panels;
            // the stale-node cleanup in drawEditPanel handles any panels whose node was deleted.
            if (lastNodeCount == 0)
            {
                selectedNodeIDs.clear();
                mutedNodeIDs.clear();
                isRenaming = false;
                renamingNodeID = -1;
                isDragging = false;
                pressedNodeID = -1;
                isRubberBanding = false;
                isWireDragging = false;
                wireDragFromNodeID = -1;
                wireDragInsertMode = false;
                showContextMenu = false;
                openEditPanels.clear();
                paramSyncState.clear();
                syncSelectionToCore();
            }
            lastNodeCount = currentCount;
        }

        // Center view on the node graph (nodes are placed around 16384,16384)
        if (needsInitialView && canvasSize.x > 0 && canvasSize.y > 0)
        {
            needsInitialView = false;
            zoom = 0.25f;
            // offset such that canvas center (16384,16384) maps to screen center
            // screenPos = canvasOrigin + (nodeCoord + offset) * zoom
            // We want screen center = canvasOrigin + canvasSize/2
            // So: canvasSize/2 = (16384 + offset) * zoom
            // => offset = canvasSize / (2 * zoom) - 16384
            offsetX = canvasSize.x / (2.f * zoom) - 16384.f;
            offsetY = canvasSize.y / (2.f * zoom) - 16384.f;
        }
    }

    updateMouseOverEditPanel(canvasPos);
    handlePanZoom(canvasPos, canvasSize);
    handleNodeInteraction(canvasPos, canvasSize);

    ImDrawList* dl = ImGui::GetWindowDrawList();

    // Clip to canvas area
    dl->PushClipRect(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), true);

    // Draw background grid
    float gridStep = 50.f * zoom;
    if (gridStep > 5.f)
    {
        ImU32 gridColor = IM_COL32(50, 50, 55, 100);
        float gridOffX = fmodf(offsetX * zoom, gridStep);
        float gridOffY = fmodf(offsetY * zoom, gridStep);
        for (float x = gridOffX; x < canvasSize.x; x += gridStep)
            dl->AddLine(ImVec2(canvasPos.x + x, canvasPos.y),
                        ImVec2(canvasPos.x + x, canvasPos.y + canvasSize.y), gridColor);
        for (float y = gridOffY; y < canvasSize.y; y += gridStep)
            dl->AddLine(ImVec2(canvasPos.x, canvasPos.y + y),
                        ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + y), gridColor);
    }

    if (sc && sc->isInitialized())
    {
        // Live signal readback (zero-timeout lock to avoid audio glitches)
        if (SynthController::DataAccessMutex.try_lock_for(std::chrono::milliseconds(0)))
        {
            int nNodes = sc->numGUINodes();
            for (int i = 0; i < nNodes; i++)
            {
                if (!sc->gnIsVisible(i))
                    continue;
                int nodeType = sc->gnType(i);
                int nid = sc->gnID(i);

                if (nodeType == SYNTHROOT_ID || nodeType == CHANNELROOT_ID)
                {
                    auto& ld = liveDataCache[nid];
                    float newL = (float)std::abs(sc->getNodeSignal((DWORD)nid, 0, -2));
                    float newR = (float)std::abs(sc->getNodeSignal((DWORD)nid, 1, -2));
                    ld.vuL = std::max(ld.vuL * 0.95f, newL);
                    ld.vuR = std::max(ld.vuR * 0.95f, newR);
                }
                else if (nodeType == VOICEMANAGER_ID)
                {
                    liveDataCache[nid].voiceCount = sc->getNumActiveVoices((DWORD)nid);
                }
            }
            SynthController::DataAccessMutex.unlock();
        }

        // Build set of nodes whose output is connected (for output pin coloring)
        connectedOutputIDs.clear();
        {
            int nWireNodes = sc->numGUINodes();
            for (int i = 0; i < nWireNodes; i++)
            {
                if (!sc->gnIsVisible(i)) continue;
                int nSig = effectiveInputCount(i);
                for (int p = 0; p < nSig; p++)
                {
                    int srcID = sc->gnInput(i, p);
                    if (isRealConnection(srcID, sc))
                        connectedOutputIDs.insert(srcID);
                }
            }
        }

        // Draw wires first (behind nodes)
        drawWires(dl, canvasPos);

        // Draw ghost wire during wire drag
        drawGhostWire(dl, canvasPos);

        // Draw nodes (skip internal/helper nodes)
        // drawNode returns true if the node was deleted; stop the loop immediately
        // to avoid accessing the now-stale numNodes count.
        int numNodes = sc->numGUINodes();
        for (int i = 0; i < numNodes; i++)
        {
            if (!sc->gnIsVisible(i))
                continue;
            if (drawNode(dl, i, canvasPos))
                break;
        }

        // Draw rubber-band overlay on top
        drawRubberBand(dl);
    }
    else
    {
        // No patch loaded — show placeholder
        const char* msg = "No patch loaded";
        ImVec2 textSize = ImGui::CalcTextSize(msg);
        dl->AddText(ImVec2(canvasPos.x + (canvasSize.x - textSize.x) * 0.5f,
                           canvasPos.y + (canvasSize.y - textSize.y) * 0.5f),
                    IM_COL32(180, 180, 180, 255), msg);
    }

    // Toggle debug overlay with Ctrl+Shift+D; Wave File dialog with Ctrl+W
    {
        ImGuiIO& io = ImGui::GetIO();
        if (io.KeyCtrl && io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_D, false))
            showDebugOverlay = !showDebugOverlay;
        if (!isRenaming && io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_W, false))
            showWaveFileDialog = !showWaveFileDialog;

        if (showDebugOverlay)
        {
            SynthController* sc2 = SynthController::instance();
            bool winHovered = ImGui::IsWindowHovered();
            int hitID = -1;
            int numNodes2 = 0;
            if (sc2 && sc2->isInitialized())
            {
                numNodes2 = sc2->numGUINodes();
                hitID = hitTestNode(io.MousePos, canvasPos);
            }
            // Canvas-projected mouse position: inverse of nodeScreenPos
            float canvasMouseX = (io.MousePos.x - canvasPos.x) / zoom - offsetX;
            float canvasMouseY = (io.MousePos.y - canvasPos.y) / zoom - offsetY;

            char dbg[3][256];
            snprintf(dbg[0], sizeof(dbg[0]),
                     "Mouse=%.1f,%.1f  CanvasOrigin=%.1f,%.1f  DisplaySize=%.0fx%.0f",
                     io.MousePos.x, io.MousePos.y,
                     canvasPos.x, canvasPos.y,
                     io.DisplaySize.x, io.DisplaySize.y);
            snprintf(dbg[1], sizeof(dbg[1]),
                     "CanvasXY=%.1f,%.1f  Zoom=%.3f  Offset=%.1f,%.1f",
                     canvasMouseX, canvasMouseY,
                     zoom, offsetX, offsetY);
            snprintf(dbg[2], sizeof(dbg[2]),
                     "ItemHov=%d WinHov=%d MB0=%d MB1=%d  Hit=%d Sel=%d Nodes=%d",
                     (int)canvasHovered, (int)winHovered,
                     (int)io.MouseDown[0], (int)io.MouseDown[1],
                     hitID, (int)selectedNodeIDs.size(), numNodes2);

            float lineH = ImGui::CalcTextSize("X").y + 2.f;
            float maxW = 0;
            for (auto& line : dbg)
                maxW = std::max(maxW, ImGui::CalcTextSize(line).x);
            ImVec2 dbgPos(canvasPos.x + 4, canvasPos.y + 4);
            dl->AddRectFilled(dbgPos,
                              ImVec2(dbgPos.x + maxW + 8, dbgPos.y + lineH * 3 + 6),
                              IM_COL32(0, 0, 0, 200));
            for (int li = 0; li < 3; li++)
                dl->AddText(ImVec2(dbgPos.x + 4, dbgPos.y + 3 + li * lineH),
                            IM_COL32(255, 255, 0, 255), dbg[li]);
        }
    }

    // Draw edit panels (world-space, inside clip rect)
    drawEditPanel(dl, canvasPos);

    dl->PopClipRect();

    // Context menu popup (must be outside clip rect)
    if (showContextMenu)
    {
        if (ImGui::BeginPopup("##nodeMenu"))
        {
            const float menuFontScale = 18.f / ImGui::GetFont()->FontSize;
            ImGui::SetWindowFontScale(menuFontScale);

            // "Stop wiring" entry — only shown during continuous wire mode
            if (wireDragInsertMode)
            {
                if (ImGui::MenuItem("Stop wiring"))
                {
                    wireDragInsertMode = false;
                    isWireDragging = false;
                    wireDragFromNodeID = -1;
                    ImGui::CloseCurrentPopup();
                    showContextMenu = false;
                }
                ImGui::Separator();
            }

            const auto& cats = NodeConfig::instance().getCategories();
            for (const auto& cat : cats)
            {
                if (ImGui::BeginMenu(cat.c_str()))
                {
                    const auto& nodes = NodeConfig::instance().getNodesInCategory(cat);
                    for (const auto* nodeDef : nodes)
                    {
                        if (ImGui::MenuItem(nodeDef->name.c_str()))
                        {
                            int channel = -2;
                            bool isGlobal = false;
                            bool doWireDragInsert = wireDragInsertMode && wireDragFromNodeID >= 0;
#ifdef _WIN32
                            bool ctrlHeld = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
#else
                            bool ctrlHeld = ImGui::GetIO().KeyCtrl;
#endif
                            if (ctrlHeld)
                            {
                                // Ctrl: explicitly create global node (same for wire-drag and regular menu)
                                isGlobal = true;
                                channel = -2;
                            }
                            else if (doWireDragInsert)
                            {
                                // Continuous wiring: default to voice (no derivation from wire source)
                                isGlobal = false;
                                channel = -2;
                            }
                            else if (sc)
                            {
                                // Regular menu: derive from context wire or selection
                                int refID = contextWireFromID >= 0 ? contextWireFromID
                                          : (!selectedNodeIDs.empty() ? *selectedNodeIDs.begin() : -1);
                                if (refID >= 0)
                                {
                                    int gi = findGuiIndex(refID);
                                    if (gi >= 0)
                                    {
                                        channel = sc->gnChannel(gi);
                                        isGlobal = sc->gnIsGlobal(gi);
                                    }
                                }
                                // else defaults: voice, channel -2
                            }
                            // Wire insertion: context wire set + node allows insertion
                            bool doInsert = (contextWireFromID >= 0 && contextWireToID >= 0 &&
                                            contextWirePinIndex >= 0 && nodeDef->allowSignalInsertion);
                            if (doInsert)
                            {
                                int fromGI = findGuiIndex(contextWireFromID);
                                int toGI = findGuiIndex(contextWireToID);
                                if (fromGI >= 0 && toGI >= 0)
                                {
                                    int fromType = sc->gnType(fromGI);
                                    int toType = sc->gnType(toGI);
                                    // Don't insert on VoiceRoot→VoiceManager or VoiceManager→NoteController
                                    bool forbidden = (fromType == (int)VOICEROOT_ID && toType == (int)VOICEMANAGER_ID) ||
                                                    (fromType == (int)VOICEMANAGER_ID && toType == (int)NOTECONTROLLER_ID);
                                    if (forbidden)
                                        doInsert = false;
                                }
                                else
                                    doInsert = false;
                            }
                            if (sc)
                            {
                                // Continuous wiring: prevent voice→global (same rule as pin connection)
                                bool blockWireDragInsert = false;
                                if (doWireDragInsert && isGlobal)
                                {
                                    int fromGI = findGuiIndex(wireDragFromNodeID);
                                    if (fromGI >= 0 && !sc->gnIsGlobal(fromGI))
                                    {
                                        int fromType = sc->gnType(fromGI);
                                        // VoiceRoot→VoiceManager is allowed; other voice→global is not
                                        if (fromType != (int)VOICEROOT_ID)
                                        {
                                            showToast("A voice node output cannot be connected to a global node input!");
                                            blockWireDragInsert = true;
                                            wireDragInsertMode = false;  // stay in wire drag, user can pick a voice node
                                        }
                                    }
                                }
                                if (!blockWireDragInsert)
                                {
                                    sc->killVoices();
                                    ImVec2 createPos = doWireDragInsert ? wireDragInsertCanvasPos : contextMenuCanvasPos;
                                    SynthNode* newNode = sc->createGUINode((DWORD)nodeDef->id, (DWORD)channel,
                                                                           (DWORD)(isGlobal ? 1 : 0),
                                                                           createPos.x, createPos.y);
                                    if (doWireDragInsert && newNode)
                                    {
                                        int newID = (int)newNode->valueOffset;
                                        sc->connectInput((DWORD)wireDragFromNodeID, (DWORD)newID, 0);
                                        wireDragFromNodeID = newID;
                                        wireDragCurrentPos = ImGui::GetIO().MousePos;
                                        wireDragInsertMode = false;
                                    }
                                    else if (doInsert && newNode)
                                {
                                    int newID = (int)newNode->valueOffset;
                                    sc->disconnectInput((DWORD)contextWireToID, (DWORD)contextWirePinIndex);
                                    sc->connectInput((DWORD)contextWireFromID, (DWORD)newID, 0);
                                    // Variable-input targets ignore index and append
                                    sc->connectInput((DWORD)newID, (DWORD)contextWireToID, (DWORD)contextWirePinIndex);
                                }
                                if (!doWireDragInsert)
                                {
                                    contextWireFromID = -1;
                                    contextWireToID = -1;
                                    contextWirePinIndex = -1;
                                    wireDragInsertMode = false;
                                }
                                }
                                sc->numGUINodes();
                            }
                        }
                    }
                    ImGui::EndMenu();
                }
            }
            ImGui::EndPopup();
        }
        else
        {
            showContextMenu = false;
            contextWireFromID = -1;
            contextWireToID = -1;
            contextWirePinIndex = -1;
            if (wireDragInsertMode)
            {
                wireDragInsertMode = false;
                isWireDragging = false;
                wireDragFromNodeID = -1;
            }
        }
    }

    // Draw rename overlay (uses ImGui windows, must be outside clip rect)
    drawRenameOverlay(canvasPos);

    // Draw toast notifications (screen-centered, on top of everything)
    drawToasts(canvasPos, canvasSize);

    // Wave File References dialog (Ctrl+W)
    drawWaveFileDialog();
}

// ── Wave File References Dialog ───────────────────────────────────────────

void NodeCanvas::drawWaveFileDialog()
{
    if (!showWaveFileDialog)
        return;

    SynthController* sc = SynthController::instance();
    if (!sc || !sc->isInitialized())
    {
        showWaveFileDialog = false;
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(600.f, 520.f), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Wave File References (Ctrl+W to close)", &showWaveFileDialog))
    {
        ImGui::End();
        return;
    }

    static const char* kRateNames[] = { "11 kHz", "22 kHz", "44 kHz" };

    ImGuiTableFlags tableFlags = ImGuiTableFlags_BordersInnerV
                               | ImGuiTableFlags_ScrollY
                               | ImGuiTableFlags_RowBg
                               | ImGuiTableFlags_SizingFixedFit;

    if (ImGui::BeginTable("##wftable", 5, tableFlags))
    {
        ImGui::TableSetupScrollFreeze(0, 1); // freeze header row

        ImGui::TableSetupColumn("#",     ImGuiTableColumnFlags_WidthFixed,   30.f);
        ImGui::TableSetupColumn("File",  ImGuiTableColumnFlags_WidthStretch, 0.f);
        ImGui::TableSetupColumn("Bytes", ImGuiTableColumnFlags_WidthFixed,   80.f);
        ImGui::TableSetupColumn("Rate",  ImGuiTableColumnFlags_WidthFixed,   72.f);
        ImGui::TableSetupColumn("",      ImGuiTableColumnFlags_WidthFixed,  104.f);
        ImGui::TableHeadersRow();

        for (int i = 0; i < 32; i++)
        {
            ImGui::PushID(i);
            ImGui::TableNextRow();

            // ── Column 0: slot number (1-based) ──
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%d", i + 1);

            // ── Column 1: filename (basename only, tooltip = full path) ──
            ImGui::TableSetColumnIndex(1);
            std::string fullPath = sc->getWaveFileName(i);
            std::string displayName = "-";
            if (!fullPath.empty())
            {
                size_t slash = fullPath.find_last_of("/\\");
                displayName = (slash != std::string::npos) ? fullPath.substr(slash + 1) : fullPath;
            }
            ImGui::TextUnformatted(displayName.c_str());
            if (!fullPath.empty() && ImGui::IsItemHovered())
                ImGui::SetTooltip("%s", fullPath.c_str());

            // ── Column 2: compressed size ──
            ImGui::TableSetColumnIndex(2);
            int compSize = sc->getWaveFileCompressedSize(i);
            if (compSize > 0)
                ImGui::Text("%d", compSize);
            else
                ImGui::TextUnformatted("-");

            // ── Column 3: sample rate combo ──
            ImGui::TableSetColumnIndex(3);
            int freq = sc->getWaveFileFrequency(i);
            if (freq < 0 || freq > 2) freq = 2;
            ImGui::SetNextItemWidth(-1.f);
            if (ImGui::Combo("##rate", &freq, kRateNames, 3) && !fullPath.empty())
                sc->setWaveFileReference(i, 0, freq, fullPath);

            // ── Column 4: Load / Clear ──
            ImGui::TableSetColumnIndex(4);
            if (ImGui::Button("Load", ImVec2(48.f, 0.f)))
            {
#ifdef _WIN32
                char filenameBuf[MAX_PATH] = {};
                OPENFILENAMEA ofn = {};
                ofn.lStructSize   = sizeof(ofn);
                ofn.lpstrFilter   = "Wave Files\0*.wav\0All Files\0*.*\0";
                ofn.lpstrFile     = filenameBuf;
                ofn.nMaxFile      = MAX_PATH;
                ofn.Flags         = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
                ofn.lpstrDefExt   = "wav";
                bool wavOk = GetOpenFileNameA(&ofn) != 0;
                { MSG m; while (PeekMessageA(&m, nullptr, WM_KEYFIRST, WM_KEYLAST, PM_REMOVE)) {} }
                if (wavOk)
                    sc->setWaveFileReference(i, 0, freq, std::string(filenameBuf));
#endif
            }
            ImGui::SameLine(0.f, 4.f);
            if (ImGui::Button("Clear", ImVec2(48.f, 0.f)))
                sc->setWaveFileReference(i, 0, 0, "");

            ImGui::PopID();
        }
        ImGui::EndTable();
    }

    ImGui::End();
}

// ─────────────────────────────────────────────────────────────────────────────

void NodeCanvas::showToast(const char* msg)
{
    double now = ImGui::GetTime();
    // Replace any existing toast with the same message (avoid stacking duplicates)
    for (auto& t : toasts)
        if (t.message == msg) { t.expireTime = now + 3.0; return; }
    toasts.push_back({ msg, now + 3.0 });
}

void NodeCanvas::drawToasts(const ImVec2& canvasPos, const ImVec2& canvasSize)
{
    double now = ImGui::GetTime();
    toasts.erase(std::remove_if(toasts.begin(), toasts.end(),
        [now](const Toast& t) { return now >= t.expireTime; }), toasts.end());
    if (toasts.empty())
        return;

    const float fontSz  = 18.f;
    const float padX    = 20.f;
    const float padY    = 9.f;
    const float rowGap  = 6.f;
    const float radius  = 5.f;

    // Measure all toasts and compute total block height
    float totalH = 0.f;
    for (const auto& t : toasts)
        totalH += fontSz + padY * 2.f + rowGap;
    totalH -= rowGap;

    // Center of the canvas in screen space
    float cx = canvasPos.x + canvasSize.x * 0.5f;
    float cy = canvasPos.y + canvasSize.y * 0.5f;
    float curY = cy - totalH * 0.5f;

    ImDrawList* dl = ImGui::GetForegroundDrawList();

    for (const auto& t : toasts)
    {
        // Fade out over the last 0.5 s
        double remaining = t.expireTime - now;
        float alpha = (float)std::min(1.0, remaining * 2.0);
        unsigned int a8  = (unsigned int)(alpha * 210);
        unsigned int ta8 = (unsigned int)(alpha * 255);

        float tw = ImGui::GetFont()->CalcTextSizeA(fontSz, FLT_MAX, 0.f,
                       t.message.c_str()).x;
        float rw = tw + padX * 2.f;
        float rh = fontSz + padY * 2.f;
        float rx = cx - rw * 0.5f;

        dl->AddRectFilled(ImVec2(rx, curY), ImVec2(rx + rw, curY + rh),
                          IM_COL32(18, 18, 22, a8), radius);
        dl->AddRect(ImVec2(rx, curY), ImVec2(rx + rw, curY + rh),
                    IM_COL32(200, 70, 70, ta8), radius, 0, 1.5f);
        dl->AddText(pickFont(fontSz), fontSz,
                    ImVec2(rx + padX, curY + padY),
                    IM_COL32(255, 230, 230, ta8), t.message.c_str());

        curY += rh + rowGap;
    }
}

} // namespace K64GUI

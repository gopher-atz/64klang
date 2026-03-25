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
#include <windows.h>
#endif

namespace K64GUI {

// ── Named color constants for edit panels and knobs ──────────────────────────
static constexpr ImU32 kColPanelBg      = IM_COL32(180, 180, 180, 255);
static constexpr ImU32 kColPanelHeader  = IM_COL32(160, 160, 165, 255);
// kColPanelBorder / kColPanelText / kColPanelDimText are defined in Widgets.h

static constexpr ImU32 kColCheckboxBg   = IM_COL32( 30,  30,  35, 255);
static constexpr ImU32 kColCheckboxRim  = IM_COL32(120, 120, 125, 255);
static constexpr ImU32 kColCheckmark    = IM_COL32(100, 200, 255, 255);
static constexpr ImU32 kColCloseBtnBg   = IM_COL32(200,  40,  40, 255);
static constexpr ImU32 kColResetBtnBg   = IM_COL32( 30,  90, 200, 255);
static constexpr ImU32 kColGhostWire    = IM_COL32(255,  50,  50, 200);

// ── Platform-agnostic Ctrl key query ─────────────────────────────────────────
static inline bool isCtrlHeld()
{
#ifdef _WIN32
    return (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
#else
    return ImGui::GetIO().KeyCtrl;
#endif
}

// ── Platform-agnostic Shift key query ────────────────────────────────────────
static inline bool isShiftHeld()
{
#ifdef _WIN32
    return (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
#else
    return ImGui::GetIO().KeyShift;
#endif
}

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
        for (int nid : selectedNodeIDs)
            bringToFront(nid);
        syncSelectionToCore();
        return;
    }
}

// Pick the font whose loaded size is closest to the requested pixel size.
// Fonts[0] = 14 px (sharp at zoom ≤ 1.5×), Fonts[1] = 32 px (sharp at zoom ≥ 2×).
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
    {
        h += kEditButtonHeight * 2.f; // Load + Save channel buttons
        h += kRowHeight; // Channel name label (centered below buttons)
    }
    return h;
}

int NodeCanvas::effectiveInputCount(int guiIndex) const
{
    SynthController* sc = SynthController::instance();
    if (!sc) return 0;
    // Input-category nodes (Midi CC, Constant, voice params) have no connectable
    // signal inputs — their numInputs stores Scale/mode constants, not wiring slots.
    int nt = sc->gnType(guiIndex);
    if (nt == MIDISIGNAL_ID || (nt >= CONSTANT_ID && nt != (int)SIGNAL_VISUALIZER_ID))
        return 0;
    int maxSignals = sc->gnNodeMaxSignals(guiIndex);
    // For non-variable-input nodes, maxSignals already excludes mode inputs.
    // For variable-input nodes (NoteController, MultiAdd), maxSignals is 0;
    // use gnNodeInputs() which returns the actual live count.
    if (maxSignals > 0)
        return maxSignals;
    // Only fall back to live input count for variable-input nodes.
    // Fixed-input nodes with 0 signal inputs (e.g. GMDLS whose only input is a mode)
    // must return 0 so their mode constant is not drawn as a connectable pin.
    if (nt == MULTIADD_ID || nt == NOTECONTROLLER_ID)
        return sc->gnNodeInputs(guiIndex);
    return 0;
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
    if ((nodeType == 35 || nodeType >= CONSTANT_ID) && nodeType != (int)SIGNAL_VISUALIZER_ID) // OsRand=35, constants>=64
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

// ── Z-order management ───────────────────────────────────────────────────

void NodeCanvas::rebuildZOrder()
{
    SynthController* sc = SynthController::instance();
    if (!sc) return;
    int n = sc->numGUINodes();
    // Preserve existing order for surviving nodes, append any new ones at the back.
    std::vector<int> old = nodeZOrder;
    nodeZOrder.clear();
    std::unordered_set<int> seen;
    for (int nid : old)
    {
        if (findGuiIndex(nid) >= 0)
        {
            nodeZOrder.push_back(nid);
            seen.insert(nid);
        }
    }
    for (int i = 0; i < n; i++)
    {
        if (!sc->gnIsVisible(i)) continue;
        int nid = sc->gnID(i);
        if (!seen.count(nid))
            nodeZOrder.push_back(nid);
    }
}

void NodeCanvas::bringToFront(int nodeID)
{
    auto it = std::find(nodeZOrder.begin(), nodeZOrder.end(), nodeID);
    if (it == nodeZOrder.end())
        nodeZOrder.push_back(nodeID);
    else if (std::next(it) != nodeZOrder.end())
    {
        nodeZOrder.erase(it);
        nodeZOrder.push_back(nodeID);
    }
}

// ── Hit Testing ──────────────────────────────────────────────────────────

int NodeCanvas::hitTestNode(const ImVec2& mousePos, const ImVec2& canvasOrigin) const
{
    SynthController* sc = SynthController::instance();
    if (!sc) return -1;

    int hitID = -1;

    // Iterate in Z-order (back = frontmost); last match = topmost node.
    for (int nid : nodeZOrder)
    {
        int i = findGuiIndex(nid);
        if (i < 0 || !sc->gnIsVisible(i))
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
            hitID = nid;
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

bool NodeCanvas::nodeOverlapsRect(int guiIndex, ImVec2 rectMin, ImVec2 rectMax,
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

    return pos.x < rMaxX && pos.x + w > rMinX &&
           pos.y < rMaxY && pos.y + h > rMinY;
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
    nodeZOrder.erase(std::remove(nodeZOrder.begin(), nodeZOrder.end(), nodeID), nodeZOrder.end());

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

    float hitRadius = kPinRadius * zoom * 1.5f;
    int hitID = -1;

    for (int nid : nodeZOrder)
    {
        int i = findGuiIndex(nid);
        if (i < 0 || !sc->gnIsVisible(i))
            continue;
        ImVec2 pos = nodeScreenPos(sc->gnX(i), sc->gnY(i), canvasOrigin);
        ImVec2 pin = outputPinPos(pos);

        float dx = mousePos.x - pin.x;
        float dy = mousePos.y - pin.y;
        if (dx * dx + dy * dy <= hitRadius * hitRadius)
            hitID = nid;
    }
    return hitID;
}

NodeCanvas::PinHit NodeCanvas::hitTestInputPin(const ImVec2& mousePos, const ImVec2& canvasOrigin) const
{
    SynthController* sc = SynthController::instance();
    if (!sc) return {-1, -1};

    float hitRadius = kPinRadius * zoom * 1.5f;
    PinHit best = {-1, -1};

    for (int nid : nodeZOrder)
    {
        int i = findGuiIndex(nid);
        if (i < 0 || !sc->gnIsVisible(i))
            continue;
        int numSignals = effectiveInputCount(i);
        int nodeTypeHT = sc->gnType(i);
        bool hasEditBtnHT = nodeHasEditButton(i);
        ImVec2 pos = nodeScreenPos(sc->gnX(i), sc->gnY(i), canvasOrigin);
        float w = kNodeWidth * zoom;

        for (int pin = 0; pin < numSignals; pin++)
        {
            ImVec2 pinPos = inputPinPos(pos, pin);
            float dx = mousePos.x - pinPos.x;
            float dy = mousePos.y - pinPos.y;
            if (dx * dx + dy * dy <= hitRadius * hitRadius)
                best = {nid, pin};
        }

        if ((nodeTypeHT == MULTIADD_ID || nodeTypeHT == NOTECONTROLLER_ID) && numSignals < 16)
        {
            float pillBaseY = pos.y + nodeHeight(numSignals, hasEditBtnHT, false) * zoom;
            float pillH     = kRowHeight * zoom;
            float pillInset = 8.f * zoom;
            ImVec2 pillMin(pos.x + pillInset, pillBaseY + 2.f * zoom);
            ImVec2 pillMax(pos.x + w - pillInset, pillBaseY + pillH - 2.f * zoom);
            if (mousePos.x >= pillMin.x && mousePos.x <= pillMax.x &&
                mousePos.y >= pillMin.y && mousePos.y <= pillMax.y)
                best = {nid, numSignals};
        }
    }
    return best;
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

    ImU32 ghostColor = kColGhostWire;
    dl->AddLine(p0, p1, ghostColor, kWireThickness * zoom);
    dl->AddLine(p1, p2, ghostColor, kWireThickness * zoom);
    dl->AddLine(p2, p3, ghostColor, kWireThickness * zoom);
}

// ── Interaction ──────────────────────────────────────────────────────────

void NodeCanvas::handleNodeInteraction(const ImVec2& canvasPos, const ImVec2& canvasSize)
{
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
                    // Case 4b: VoiceManager output only connects to NoteController
                    if (valid && fromType == VOICEMANAGER_ID && toType != NOTECONTROLLER_ID)
                    {
                        showToast("VoiceManager can only connect to NoteController!");
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
                    bringToFront(wireDragFromNodeID);
                    bringToFront(target.nodeID);
                }
                return; // stay in wire drag mode
            }

            // Click hit a node body or output pin — just consume the click, stay wiring
            int nodeHit = hitTestNode(mousePos, canvasPos);
            if (nodeHit != -1)
                return;

            // VoiceManager output can only connect to NoteController — no valid intermediate node exists.
            {
                int fromGI = findGuiIndex(wireDragFromNodeID);
                if (fromGI >= 0 && sc->gnType(fromGI) == (int)VOICEMANAGER_ID)
                {
                    showToast("VoiceManager can only connect to NoteController!");
                    isWireDragging = false;
                    wireDragFromNodeID = -1;
                    return;
                }
            }

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

        // Node body hit is computed first — a foreground node body occludes pins of nodes behind it.
        int hitID = hitTestNode(mousePos, canvasPos);

        // Test pins BEFORE node body selection, but AFTER node occlusion check.
        // Shift+click on output pin = recursive select; only start wire drag when shift not held.
        bool shiftHeld = isShiftHeld();
        int outputHit = hitTestOutputPin(mousePos, canvasPos);
        if (outputHit != -1 && !shiftHeld)
        {
            // Start wire drag from output pin (click-to-start)
            isWireDragging = true;
            wireDragFromNodeID = outputHit;
            wireDragCurrentPos = mousePos;
            pressedNodeID = -1;
            bringToFront(outputHit);
            return;
        }

        PinHit inputHit = hitTestInputPin(mousePos, canvasPos);
        // Only disconnect if no node body is occluding this pin from a different node.
        if (inputHit.nodeID != -1 && (hitID == -1 || hitID == inputHit.nodeID))
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
                        bringToFront(inputHit.nodeID);
                        sc->disconnectInput((DWORD)inputHit.nodeID, (DWORD)inputHit.pinIndex);
                        sc->numGUINodes(); // refresh accessor
                        return;
                    }
                }
            }
        }
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
            bringToFront(hitID);
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
                    bool ctrl = isCtrlHeld();
                    // shiftHeld already computed above for output-pin check

                    if (shiftHeld)
                    {
                        // Shift+click: recursive upstream select
                        if (!ctrl)
                            selectedNodeIDs.clear();
                        std::unordered_set<int> visited;
                        recursiveSelect(hitID, visited);
                        for (int nid : selectedNodeIDs)
                            bringToFront(nid);
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
            if (!isCtrlHeld())
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
                if (nodeOverlapsRect(i, rubberBandStart, rubberBandCurrent, canvasPos))
                {
                    int nid = sc->gnID(i);
                    selectedNodeIDs.insert(nid);
                    bringToFront(nid);
                }
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
            // Position new node at the right-click position (upper-left corner at cursor)
            int fromGI = findGuiIndex(wireHit.fromID);
            int toGI = findGuiIndex(wireHit.toID);
            if (fromGI >= 0 && toGI >= 0)
            {
                contextMenuCanvasPos.x = (float)((mousePos.x - canvasPos.x) / zoom - offsetX);
                contextMenuCanvasPos.y = (float)((mousePos.y - canvasPos.y) / zoom - offsetY);
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
                // Right-release on empty canvas → open node creation menu.
                // Capture current selection into clipboard for Linux-style paste.
                buildClipboardFromSelection();
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
    if (canvasHovered && ImGui::IsKeyPressed(ImGuiKey_Delete))
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
}

// ── Copy / Paste (Linux-style: selection is clipboard) ───────────────────────

// Never include these structural node types in the clipboard.
static bool isStructuralNodeType(int type)
{
    return type == (int)SYNTHROOT_ID ||
           type == (int)CHANNELROOT_ID ||
           type == (int)NOTECONTROLLER_ID;
}

// Returns true for node types that can never be global
// (VoiceRoot and all voice-parameter nodes with typeID > CONSTANT_ID).
static bool isVoiceOnlyNodeType(int type)
{
    return type == (int)VOICEROOT_ID || (type > (int)CONSTANT_ID && type != (int)SIGNAL_VISUALIZER_ID);
}

void NodeCanvas::buildClipboardFromSelection()
{
    clipboard.clear();
    if (selectedNodeIDs.empty())
        return;

    SynthController* sc = SynthController::instance();
    if (!sc)
        return;

    // Filter: skip structural nodes
    std::vector<int> selIDs;
    for (int id : selectedNodeIDs)
    {
        int gi = findGuiIndex(id);
        if (gi < 0)
            continue;
        if (isStructuralNodeType(sc->gnType(gi)))
            continue;
        selIDs.push_back(id);
    }
    if (selIDs.empty())
        return;

    // Compute centroid of filtered set
    double cx = 0, cy = 0;
    for (int id : selIDs)
    {
        int gi = findGuiIndex(id);
        if (gi >= 0) { cx += sc->gnX(gi); cy += sc->gnY(gi); }
    }
    cx /= (double)selIDs.size();
    cy /= (double)selIDs.size();

    // Build ID → clipboard-index map
    std::unordered_map<int, int> idToClipIdx;
    for (int idx = 0; idx < (int)selIDs.size(); idx++)
        idToClipIdx[selIDs[idx]] = idx;

    for (int id : selIDs)
    {
        int gi = findGuiIndex(id);
        if (gi < 0)
            continue;

        ClipboardNode cn;
        cn.typeID   = sc->gnType(gi);
        cn.channel  = sc->gnChannel(gi);
        cn.isGlobal = sc->gnIsGlobal(gi);
        cn.relX     = sc->gnX(gi) - cx;
        cn.relY     = sc->gnY(gi) - cy;

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

void NodeCanvas::pasteNodes(bool forceGlobal, bool forceVoice)
{
    if (clipboard.empty())
        return;

    SynthController* sc = SynthController::instance();
    if (!sc)
        return;

    // Build a filtered list of clipboard indices eligible to paste:
    //   "Paste as Global": skip VoiceRoot and voice-only parameter nodes (cannot be global)
    //   "Paste as Voice":  skip VoiceManager (cannot be a voice node)
    std::vector<int> validIndices;
    for (int i = 0; i < (int)clipboard.size(); i++)
    {
        const auto& cn = clipboard[i];
        if (forceGlobal && isVoiceOnlyNodeType(cn.typeID))
            continue;
        if (forceVoice && cn.typeID == (int)VOICEMANAGER_ID)
            continue;
        validIndices.push_back(i);
    }
    if (validIndices.empty())
        return;

    std::unordered_set<int> validSet(validIndices.begin(), validIndices.end());

    sc->killVoices();

    // Create new nodes; map clipboard index → new node ID
    std::unordered_map<int, int> clipToNewID;
    for (int i : validIndices)
    {
        const auto& cn = clipboard[i];
        double px = contextMenuCanvasPos.x + cn.relX;
        double py = contextMenuCanvasPos.y + cn.relY;

        bool isGlobal = cn.isGlobal;
        if (forceGlobal) isGlobal = true;
        if (forceVoice)  isGlobal = false;
        // Voice-parameter types can never be global regardless of override
        if (isVoiceOnlyNodeType(cn.typeID)) isGlobal = false;

        SynthNode* newNode = sc->createGUINode((DWORD)cn.typeID, (DWORD)cn.channel,
                                               (DWORD)(isGlobal ? 1 : 0), px, py);
        clipToNewID[i] = newNode ? (int)newNode->valueOffset : -1;
    }

    // Restore parameter values and modes
    for (int i : validIndices)
    {
        auto it = clipToNewID.find(i);
        if (it == clipToNewID.end() || it->second < 0)
            continue;
        const auto& cn = clipboard[i];
        int newID = it->second;
        int numParams = std::min((int)cn.inputs.size(), 16);
        for (int p = 0; p < numParams; p++)
        {
            sc->setInputValue((DWORD)newID, (DWORD)p, cn.inputs[p].valL, cn.inputs[p].valR);
            if (cn.inputs[p].mode != 0)
                sc->setInputMode((DWORD)newID, (DWORD)p, (DWORD)cn.inputs[p].mode);
        }
    }

    // Reconnect internal wires (only between nodes that survived the filter)
    for (int i : validIndices)
    {
        auto it = clipToNewID.find(i);
        if (it == clipToNewID.end() || it->second < 0)
            continue;
        const auto& cn = clipboard[i];
        int newToID = it->second;
        for (int p = 0; p < (int)cn.internalWires.size(); p++)
        {
            int srcClipIdx = cn.internalWires[p];
            if (srcClipIdx < 0 || !validSet.count(srcClipIdx))
                continue;
            auto srcIt = clipToNewID.find(srcClipIdx);
            if (srcIt == clipToNewID.end() || srcIt->second < 0)
                continue;
            sc->connectInput((DWORD)srcIt->second, (DWORD)newToID, (DWORD)p);
        }
    }

    // Clear old selection; select and bring all pasted nodes to front
    selectedNodeIDs.clear();
    for (auto& [clipIdx, newID] : clipToNewID)
    {
        if (newID >= 0)
        {
            selectedNodeIDs.insert(newID);
            bringToFront(newID);
        }
    }
    syncSelectionToCore();
    sc->numGUINodes(); // refresh accessor
}

// ── Edit Panel helpers ────────────────────────────────────────────────────────

// Compute the pixel size of an edit panel (before zoom) for a given node.
// Called by updateMouseOverEditPanel, drawEditPanel pre-pass, and drawEditPanel main loop.
NodeCanvas::EditPanelSize NodeCanvas::calcEditPanelSize(int nodeID, int nodeType,
                                                         const NodeTypeDef* typeDef) const
{
    SynthController* sc = SynthController::instance();

    float pw = kEditPanelWidth;

    int modeInputIdx = typeDef->numMaxGUIInputs;
    if (!(modeInputIdx < typeDef->numInputs && modeInputIdx < (int)typeDef->inputs.size()))
        modeInputIdx = -1;
    if (nodeType == MIDISIGNAL_ID)
        modeInputIdx = 1;

    int numParams = 0;
    for (int i = typeDef->numReqGUIInputs; i < typeDef->numMaxGUIInputs && i < (int)typeDef->inputs.size(); i++)
        numParams++;
    if (nodeType == CONSTANT_ID || nodeType == MIDISIGNAL_ID || (nodeType > CONSTANT_ID && nodeType != (int)SIGNAL_VISUALIZER_ID))
        numParams = 1;

    float flagsH = 0.f;
    if (modeInputIdx >= 0 && modeInputIdx < (int)typeDef->inputs.size())
    {
        const InputDef& modeDef = typeDef->inputs[modeInputIdx];
        if (!modeDef.modeGroups.empty() || !modeDef.modeFlags.empty())
        {
            flagsH = kEditLabelH + kEditFlagH;
            flagsH += (float)modeDef.modeGroups.size() * kEditFlagH;
        }
    }

    float ph = kEditHeaderH + (float)numParams * kEditKnobDiam + flagsH + 4.f;

    if (nodeType == VOICEMANAGER_ID)
    {
        pw = std::max(pw, ArpEditor::kTotalW + 8.f);
        ph += 4.f + ArpEditor::kTotalH + 4.f;
    }
    if (nodeType == TRIGGERSEQ_ID && sc)
    {
        int tsMode  = sc->getInputMode((DWORD)nodeID, TRIGGERSEQ_MODE);
        int tsCount = tsMode & (int)TRIGGERSEQ_COUNTMASK;
        if (tsCount < 1 || tsCount > 16) tsCount = 16;
        ph += 4.f + 20.f + 20.f + 12.f + tsCount * 14.f + 4.f;
    }
    if (nodeType == SAPI_ID || nodeType == FORMULA_ID)
        ph += 4.f + 72.f + 4.f + 20.f + 8.f;
    if (nodeType == (int)SIGNAL_VISUALIZER_ID)
    {
        int svMode = sc ? sc->getInputMode((DWORD)nodeID, (DWORD)SIGNAL_VISUALIZER_MODE) : 0;
        if ((svMode & SIGNAL_VISUALIZER_DISPLAYMASK) == (int)SIGNAL_VISUALIZER_RAW)
            ph += 4.f + 60.f + 4.f + 80.f + 4.f + 18.f + 4.f; // bars(60)+history(80)+combo(18)
        else
            ph += 4.f + 120.f + 4.f;
    }

    return { pw, ph };
}

// Remove an edit panel and clean up all associated per-panel state.
void NodeCanvas::closeEditPanel(int nodeID)
{
    openEditPanels.erase(std::find(openEditPanels.begin(), openEditPanels.end(), nodeID));
    for (auto it = paramSyncState.begin(); it != paramSyncState.end(); )
        it = ((uint32_t)(it->first >> 32) == (uint32_t)nodeID) ? paramSyncState.erase(it) : std::next(it);
    textEditBuffers.erase(nodeID);
}

// Draw a checkbox (bg + border + optional checkmark) at the given screen position.
void NodeCanvas::drawCheckbox(ImDrawList* dl, ImVec2 min, float sz, bool checked, float z) const
{
    ImVec2 max(min.x + sz, min.y + sz);
    dl->AddRectFilled(min, max, kColCheckboxBg);
    dl->AddRect(min, max, kColCheckboxRim, 0.f, 0, 1.f);
    if (checked)
    {
        dl->AddLine(ImVec2(min.x + 2*z, min.y + sz*0.5f),
                    ImVec2(min.x + sz*0.4f, max.y - 2*z), kColCheckmark, 1.5f);
        dl->AddLine(ImVec2(min.x + sz*0.4f, max.y - 2*z),
                    ImVec2(max.x - 2*z, min.y + 2*z), kColCheckmark, 1.5f);
    }
}

void NodeCanvas::updateMouseOverEditPanel(const ImVec2& canvasOrigin)
{
    mouseOverEditPanel = false;
    if (openEditPanels.empty())
        return;

    if (knobDragNodeID >= 0)
    {
        mouseOverEditPanel = true;
        return;
    }

    SynthController* sc = SynthController::instance();
    if (!sc || !sc->isInitialized())
        return;

    ImVec2 mousePos = ImGui::GetIO().MousePos;

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
        float px = nodePos.x + kNodeWidth * zoom;
        float py = nodePos.y;

        EditPanelSize eps = calcEditPanelSize(nodeID, nodeType, typeDef);
        float pw = eps.pw * zoom;
        float ph = eps.ph * zoom;

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

    const bool ctrlHeld = isCtrlHeld();

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

            ImVec2 np2 = nodeScreenPos(sc->gnX(pgi), sc->gnY(pgi), canvasOrigin);
            float ppx = np2.x + kNodeWidth * zoom;
            float ppy = np2.y;
            EditPanelSize eps = calcEditPanelSize(pid, ptype, pdef);
            float ppw = eps.pw * zoom;
            float pph = eps.ph * zoom;

            if (mpos.x >= ppx && mpos.x <= ppx + ppw && mpos.y >= ppy && mpos.y <= ppy + pph)
            {
                topmostUnderMouse = pid;
                break;
            }
        }
    }

    // While any ImGui popup is open, suppress all custom click handling.
    if (ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel))
        topmostUnderMouse = -1;

    // Bring the interacted panel's node to front on click.
    if (topmostUnderMouse != -1 && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        bringToFront(topmostUnderMouse);

    for (int nodeID : panelIDs)
    {
        int gi = findGuiIndex(nodeID);
        if (gi < 0)
        {
            closeEditPanel(nodeID);
            continue;
        }

        int nodeType = sc->gnType(gi);
        const NodeTypeDef* typeDef = NodeConfig::instance().getNodeType(nodeType);
        if (!typeDef)
        {
            closeEditPanel(nodeID);
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

        // Resolve mode input index and current mode
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
        bool isVoiceInput = (nodeType == MIDISIGNAL_ID || (nodeType > CONSTANT_ID && nodeType != (int)SIGNAL_VISUALIZER_ID));
        if (isVoiceInput) numParams = 1;
        // Constant and voice params use -1..1 signal range; Midi CC keeps its -128..128 config range.
        bool isSignalRange = (isConstant || (nodeType > CONSTANT_ID && nodeType != (int)SIGNAL_VISUALIZER_ID));
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

        // Panel size from shared helper
        EditPanelSize eps = calcEditPanelSize(nodeID, nodeType, typeDef);
        pw = eps.pw * z;
        float ph = eps.ph * z;

        bool isVoiceManager = (nodeType == VOICEMANAGER_ID);

        // Count visible flags (for layout)
        float flagsH = 0.f;
        int numFlags = 0;
        if (modeInputIdx >= 0 && modeInputIdx < (int)typeDef->inputs.size())
        {
            const InputDef& modeDef = typeDef->inputs[modeInputIdx];
            if (!modeDef.modeGroups.empty() || !modeDef.modeFlags.empty())
            {
                flagsH = kEditLabelH;
                bool isGlobal = sc->gnIsGlobal(gi);
                for (const auto& mf : modeDef.modeFlags)
                {
                    if (mf.visible != 0 &&
                        ((mf.visible == 1 && isGlobal) || (mf.visible == 2 && !isGlobal)))
                        continue;
                    numFlags++;
                }
                flagsH += kEditFlagH;
                for (const auto& mg : modeDef.modeGroups)
                {
                    flagsH += kEditFlagH;
                    (void)mg;
                }
            }
        }

        const bool canClick = (nodeID == topmostUnderMouse);

        dl->AddRectFilled(ImVec2(px, py), ImVec2(px + pw, py + ph), kColPanelBg);
        dl->AddRectFilled(ImVec2(px, py), ImVec2(px + pw, py + kEditHeaderH * z), kColPanelHeader);
        dl->AddRect(ImVec2(px, py), ImVec2(px + pw, py + ph), kColPanelBorder, 0.f, 0, 1.f);

        if (headerFontSize >= 6.f)
        {
            float tx = px + 6.f * z;
            float ty = py + (kEditHeaderH * z - headerFontSize) * 0.5f;
            dl->AddText(pickFont(headerFontSize), headerFontSize, ImVec2(tx, ty),      kColPanelText, typeDef->name.c_str());
            dl->AddText(pickFont(headerFontSize), headerFontSize, ImVec2(tx + 1.f, ty), kColPanelText, typeDef->name.c_str());
        }

        // Red X close button (top-right)
        float xbSz = kDeleteBtnSize * z;
        ImVec2 xbMin(px + pw - xbSz - 2.f * z, py + 2.f * z);
        ImVec2 xbMax(xbMin.x + xbSz, xbMin.y + xbSz);
        dl->AddRectFilled(xbMin, xbMax, kColCloseBtnBg);
        float xpad = 3.f * z;
        dl->AddLine(ImVec2(xbMin.x + xpad, xbMin.y + xpad), ImVec2(xbMax.x - xpad, xbMax.y - xpad), IM_COL32(255, 255, 255, 255), 1.5f);
        dl->AddLine(ImVec2(xbMax.x - xpad, xbMin.y + xpad), ImVec2(xbMin.x + xpad, xbMax.y - xpad), IM_COL32(255, 255, 255, 255), 1.5f);

        // Blue "0" reset-to-defaults button (left of X close button)
        float rbSz = xbSz;
        ImVec2 rbMin(xbMin.x - rbSz - 3.f * z, py + 2.f * z);
        ImVec2 rbMax(rbMin.x + rbSz, rbMin.y + rbSz);
        dl->AddRectFilled(rbMin, rbMax, kColResetBtnBg);
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
            closeEditPanel(nodeID);
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
        dl->AddLine(ImVec2(px, sepY), ImVec2(px + pw, sepY), kColPanelBorder, 1.f);

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
            dl->AddLine(ImVec2(px, curY), ImVec2(px + pw, curY), kColPanelBorder, 0.5f);

            // Row geometry: left column (label+sync), right area (two knobs)
            float rowH   = (kEditKnobDiam) * z;
            float knobR  = kEditKnobDiam * 0.5f * z;
            float bodyR  = knobR * (17.f / 25.f);
            float needleTipR = knobR * 0.82f;
            float leftColW   = 80.f * z;
            float rightAreaX = px + leftColW;
            float rightAreaW = pw - leftColW;
            float knobCY     = curY + rowH * 0.5f;

            // Left column: label + sync checkbox
            if (fontSize >= 6.f)
                dl->AddText(pickFont(fontSize), fontSize, ImVec2(px + 6.f * z, curY + 2.f * z), kColPanelText, inputDef.name.c_str());

            if (!inputDef.singleInput && fontSize >= 6.f)
            {
                float cbSz = kEditCheckboxSz * z;
                float cbX  = px + 6.f * z;
                float cbY  = curY + (kEditLabelH + 2.f) * z;
                ImVec2 cbMin(cbX, cbY);
                drawCheckbox(dl, cbMin, cbSz, synced, z);
                dl->AddText(pickFont(fontSize * 0.9f), fontSize * 0.9f,
                            ImVec2(cbMin.x + cbSz + 3.f * z, cbY), kColPanelText, "Sync");

                if (canClick &&
                    ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
                    mousePos.x >= cbMin.x && mousePos.x <= cbMin.x + cbSz + 35.f * z &&
                    mousePos.y >= cbMin.y && mousePos.y <= cbMin.y + cbSz)
                {
                    bool newSynced = !synced;
                    paramSyncState[syncKey] = newSynced;
                    if (newSynced)
                        sc->setInputValue((DWORD)nodeID, paramIdx, (double)rawL, (double)rawL);
                }
            }

            // Right area: left knob and right knob
            float slotW    = rightAreaW * 0.5f;
            float knobL_cx = rightAreaX + knobR + 4.f * z;
            float knobR_cx = rightAreaX + slotW + knobR + 4.f * z;

            bool showModNeedle = !isConstant && (sc->inputIsModulated((DWORD)nodeID, paramIdx) || isVoiceInput);
            int sigInp = (nodeType == MIDISIGNAL_ID) ? -3 : (nodeType > CONSTANT_ID) ? -4 : (int)paramIdx;

            // Left knob
            {
                ImVec2 center(knobL_cx, knobCY);
                float normL = (valRange > 0.f) ? std::max(0.f, std::min(1.f, (valL - minVal) / valRange)) : 0.f;
                float normModL = 0.f;
                if (showModNeedle)
                {
                    double liveL = sc->getNodeSignal((DWORD)nodeID, 0, sigInp);
                    normModL = (valRange > 0.f) ? std::max(0.f, std::min(1.f, ((float)(liveL * range) - minVal) / valRange)) : 0.f;
                }
                Widgets::drawKnob(dl, center, bodyR, knobR, needleTipR, normL, normModL, showModNeedle, 255, z);

                float dxm = mousePos.x - center.x, dym = mousePos.y - center.y;
                if (canClick && dxm*dxm + dym*dym <= knobR*knobR)
                {
                    if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                    {
                        if (isConstant)
                            sc->setInputValue((DWORD)nodeID, paramIdx, 0.0, synced ? 0.0 : (double)rawR);
                        else
                        {
                            double defL, defR;
                            sc->getNodeInputDefault((DWORD)nodeType, (DWORD)i, sc->gnIsGlobal(gi), defL, defR);
                            sc->setInputValue((DWORD)nodeID, paramIdx, defL, synced ? defL : (double)rawR);
                        }
                        knobDragNodeID = -1; knobDragParam = -1;
                    }
                    else if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                    {
                        knobDragNodeID = nodeID; knobDragParam = i;
                        knobDragIsRight = false; knobDragAccum = 0.f;
                    }
                }
                if (knobDragNodeID == nodeID && knobDragParam == i && !knobDragIsRight &&
                    ImGui::IsMouseDragging(ImGuiMouseButton_Left))
                {
                    float delta = -io.MouseDelta.y * 0.5f;
                    float quantStep;
                    if (isSignalRange) { delta /= 128.f; quantStep = ctrlHeld ? (1.f/(128.f*128.f)) : (1.f/128.f); }
                    else               { quantStep = ctrlHeld ? (1.f/128.f) : 1.f; if (ctrlHeld) delta /= 128.f; }
                    knobDragAccum += delta;
                    int stepCount = (int)(knobDragAccum / quantStep);
                    if (stepCount != 0)
                    {
                        float applied = (float)stepCount * quantStep;
                        valL = std::max(minVal, std::min(maxVal, valL + applied));
                        knobDragAccum -= applied;
                        sc->setInputValue((DWORD)nodeID, paramIdx, (double)(valL/range),
                                          synced ? (double)(valL/range) : (double)(valR/range));
                    }
                }

                if (fontSize >= 6.f)
                {
                    Widgets::KnobLabel vlbl = Widgets::formatKnobValue(valL, range, inputDef.displayMapping, currentMode, nodeType);
                    float valFontSz = fontSize * 0.85f;
                    float valX = center.x + knobR + 3.f * z;
                    if (vlbl.line2.empty())
                        dl->AddText(pickFont(valFontSz), valFontSz, ImVec2(valX, center.y - valFontSz * 0.5f), kColPanelDimText, vlbl.line1.c_str());
                    else
                    {
                        dl->AddText(pickFont(valFontSz), valFontSz, ImVec2(valX, center.y - valFontSz),             kColPanelDimText, vlbl.line1.c_str());
                        dl->AddText(pickFont(valFontSz), valFontSz, ImVec2(valX, center.y), kColPanelDimText, vlbl.line2.c_str());
                    }
                }
            }

            // Right knob (stereo only)
            if (!inputDef.singleInput)
            {
                ImVec2 center(knobR_cx, knobCY);
                unsigned int knobAlpha = synced ? 128u : 255u;

                float normR = (valRange > 0.f) ? std::max(0.f, std::min(1.f, (valR - minVal) / valRange)) : 0.f;
                float normModR = 0.f;
                if (showModNeedle)
                {
                    double liveR = sc->getNodeSignal((DWORD)nodeID, 1, sigInp);
                    normModR = (valRange > 0.f) ? std::max(0.f, std::min(1.f, ((float)(liveR * range) - minVal) / valRange)) : 0.f;
                }
                Widgets::drawKnob(dl, center, bodyR, knobR, needleTipR, normR, normModR, showModNeedle, knobAlpha, z);

                if (!synced)
                {
                    float dxm = mousePos.x - center.x, dym = mousePos.y - center.y;
                    if (canClick && dxm*dxm + dym*dym <= knobR*knobR)
                    {
                        if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                        {
                            if (isConstant)
                                sc->setInputValue((DWORD)nodeID, paramIdx, (double)rawL, 0.0);
                            else
                            {
                                double defL, defR;
                                sc->getNodeInputDefault((DWORD)nodeType, (DWORD)i, sc->gnIsGlobal(gi), defL, defR);
                                sc->setInputValue((DWORD)nodeID, paramIdx, (double)rawL, defR);
                            }
                            knobDragNodeID = -1; knobDragParam = -1;
                        }
                        else if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                        {
                            knobDragNodeID = nodeID; knobDragParam = i;
                            knobDragIsRight = true; knobDragAccum = 0.f;
                        }
                    }
                    if (knobDragNodeID == nodeID && knobDragParam == i && knobDragIsRight &&
                        ImGui::IsMouseDragging(ImGuiMouseButton_Left))
                    {
                        float delta = -io.MouseDelta.y * 0.5f;
                        float quantStep;
                        if (isSignalRange) { delta /= 128.f; quantStep = ctrlHeld ? (1.f/(128.f*128.f)) : (1.f/128.f); }
                        else               { quantStep = ctrlHeld ? (1.f/128.f) : 1.f; if (ctrlHeld) delta /= 128.f; }
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
                    float valFontSz = fontSize * 0.85f;
                    float valX = center.x + knobR + 3.f * z;
                    ImU32 valCol = (kColPanelDimText & 0x00FFFFFF) | ((unsigned int)(50 * knobAlpha / 255) << 24);
                    // reuse alpha scaling more simply:
                    unsigned int txtAlpha = knobAlpha;
                    ImU32 dimCol = IM_COL32(50, 50, 55, txtAlpha);
                    if (vlbl.line2.empty())
                        dl->AddText(pickFont(valFontSz), valFontSz, ImVec2(valX, center.y - valFontSz * 0.5f), dimCol, vlbl.line1.c_str());
                    else
                    {
                        dl->AddText(pickFont(valFontSz), valFontSz, ImVec2(valX, center.y - valFontSz),  dimCol, vlbl.line1.c_str());
                        dl->AddText(pickFont(valFontSz), valFontSz, ImVec2(valX, center.y), dimCol, vlbl.line2.c_str());
                    }
                }
            }

            curY += rowH;
        }

        if (modeInputIdx >= 0 && modeInputIdx < (int)typeDef->inputs.size())
        {
            const InputDef& modeDef = typeDef->inputs[modeInputIdx];
            bool isGlobal = sc->gnIsGlobal(gi);

            if (!modeDef.modeGroups.empty() || !modeDef.modeFlags.empty())
            {
                dl->AddLine(ImVec2(px, curY), ImVec2(px + pw, curY), kColPanelBorder, 0.5f);

                if (fontSize >= 6.f)
                    dl->AddText(pickFont(fontSize), fontSize, ImVec2(px + 6.f * z, curY + 2.f * z), kColPanelText, "Flags");
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
                    drawCheckbox(dl, cbMin, cbSz, flagSet, z);
                    if (fontSize >= 6.f)
                    {
                        dl->AddText(pickFont(fontSize * 0.9f), fontSize * 0.9f,
                                    ImVec2(cbMin.x + cbSz + 2.f * z, cbY), kColPanelDimText, mf.name.c_str());
                    }

                    // Click checkbox
                    if (canClick &&
                        ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
                        mousePos.x >= cbMin.x && mousePos.x <= cbMin.x + itemW &&
                        mousePos.y >= cbMin.y && mousePos.y <= cbMin.y + cbSz)
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
            dl->AddLine(ImVec2(px, curY), ImVec2(px + pw, curY), kColPanelBorder, 0.5f);
            curY += 4.f * z;
            ArpEditor::draw(nodeID, z, ImVec2(px + 4.f * z, curY),
                            pickFont(fontSize * 0.9f), fontSize * 0.9f, canClick);
            curY += ArpEditor::kTotalH * z + 4.f * z;
        }

        // ── TriggerSequencer: Max Patterns, BPM sync, then N rows of 8L+8R tick cells ──
        if (nodeType == TRIGGERSEQ_ID)
        {
            Widgets::EditPanelCtx ctx{ dl, nodeID, px, pw, z, fontSize, mousePos, canClick };
            Widgets::drawTriggerSeqPanel(ctx, curY, sc);
        }

        // ── TextToSpeech (SAPI): multiline text entry + Speak button ──
        if (nodeType == SAPI_ID)
        {
            Widgets::EditPanelCtx ctx{ dl, nodeID, px, pw, z, fontSize, mousePos, canClick };
            Widgets::drawSAPIPanel(ctx, curY, sc, textEditBuffers);
        }

        // ── Signal Visualizer: VU meter or oscilloscope ──
        if (nodeType == (int)SIGNAL_VISUALIZER_ID)
        {
            Widgets::EditPanelCtx ctx{ dl, nodeID, px, pw, z, fontSize, mousePos, canClick };
            Widgets::drawSignalVisualizerPanel(ctx, curY, sc);
        }
    }
}

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

    // VoiceManager: pulsating greenish frame when it has active voices
    if (nodeType == VOICEMANAGER_ID)
    {
        auto it = liveDataCache.find(nodeID);
        if (it != liveDataCache.end() && it->second.voiceCount > 0)
        {
            float t = (float)ImGui::GetTime();
            float pulse = 0.4f + 0.6f * (0.5f + 0.5f * sinf(t * 10.f));
            int g = (int)(128 + 127 * pulse);
            ImU32 pulseCol = IM_COL32(64, g, 64, 255);
            dl->AddRect(pos, ImVec2(pos.x + w, pos.y + h), pulseCol, 2.f * zoom, 0, 5.f);
        }
    }

    // Red X delete button (top-left corner)
    // Only SynthRoot/ChannelRoot/NoteController are protected; Voice Manager is deletable.
    bool isStructural = (nodeType >= 0 && nodeType <= (int)NOTECONTROLLER_ID);
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
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !isWireDragging && !mouseOverEditPanel && canvasHovered
                && hitTestNode(io.MousePos, canvasOrigin) == nodeID)
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
                        if (tp >= 0 && tp <= (int)NOTECONTROLLER_ID) continue;
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

    // ChannelRoot always shows "Channel N"; other nodes use custom name if set
    std::string customName = sc->gnName(guiIndex);
    if (nodeType == CHANNELROOT_ID)
    {
        int ch1 = sc->gnChannel(guiIndex) + 1;
        char buf[32];
        snprintf(buf, sizeof(buf), "Channel %d", ch1);
        displayName = buf;
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
        // Center in header; reserve left margin for VU meter on SynthRoot/ChannelRoot
        float leftMargin = (nodeType == SYNTHROOT_ID || nodeType == CHANNELROOT_ID)
            ? (2.f * 6.f + 1.f + 4.f) * zoom : 0.f;
        float textX = pos.x + leftMargin + (w - leftMargin - headerTextW) * 0.5f;
        float textY = pos.y + (kHeaderHeight * zoom - headerFontSize) * 0.5f;
        // Faux bold: draw at +0 and +1 pixel offset
        dl->AddText(pickFont(headerFontSize), headerFontSize, ImVec2(textX, textY), textColor, displayName);
        dl->AddText(pickFont(headerFontSize), headerFontSize, ImVec2(textX + 1.f, textY), textColor, displayName);
    }

    // VU meter on header left (SynthRoot / ChannelRoot) — black background
    if (nodeType == SYNTHROOT_ID || nodeType == CHANNELROOT_ID)
    {
        auto it = liveDataCache.find(nodeID);
        if (it != liveDataCache.end())
        {
            float vuW = 6.f * zoom;
            float vuH = (kHeaderHeight - 4.f) * zoom;
            float vuInset = 2.f * zoom;
            float vuX = pos.x + vuInset;
            float vuY = pos.y + 2.f * zoom;
            ImVec2 vuMin(vuX, vuY);
            ImVec2 vuMax(vuX + 2.f * vuW + 1.f * zoom, vuY + vuH);

            dl->AddRectFilled(vuMin, vuMax, IM_COL32(0, 0, 0, 255));

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

    // Voice count on VoiceManager: in node body, vertical center, 75% to the right, 2x input font
    if (nodeType == VOICEMANAGER_ID)
    {
        auto it = liveDataCache.find(nodeID);
        if (it != liveDataCache.end() && it->second.voiceCount > 0)
        {
            char vcBuf[16];
            snprintf(vcBuf, sizeof(vcBuf), "%d", it->second.voiceCount);
            float vcFontSize = fontSize * 2.f;
            float bodyH = (float)numSignals * kRowHeight * zoom;
            float bodyCenterY = sepY + bodyH * 0.5f;
            ImVec2 vcSize = ImGui::CalcTextSize(vcBuf);
            float vcScale = vcFontSize / ImGui::GetFontSize();
            float vcX = pos.x + w * 0.75f - vcSize.x * vcScale * 0.5f;
            float vcY = bodyCenterY - vcFontSize * 0.5f;
            dl->AddText(pickFont(vcFontSize), vcFontSize, ImVec2(vcX, vcY),
                        IM_COL32(0, 0, 0, 255), vcBuf);
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
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !isWireDragging && !mouseOverEditPanel && canvasHovered
                && hitTestNode(io.MousePos, canvasOrigin) == nodeID)
        {
            ImVec2 mpos = io.MousePos;
            if (mpos.x >= btnMin.x && mpos.x <= btnMax.x &&
                mpos.y >= btnMin.y && mpos.y <= btnMax.y)
            {
                {
                    auto _it = std::find(openEditPanels.begin(), openEditPanels.end(), nodeID);
                    if (_it != openEditPanels.end())
                        closeEditPanel(nodeID);
                    else
                        openEditPanels.push_back(nodeID);  // push_back = topmost (last drawn)
                    bringToFront(nodeID);
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
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !isWireDragging && !mouseOverEditPanel && canvasHovered)
            {
                ImVec2 mpos = io.MousePos;
                if (mpos.x >= btnMin.x && mpos.x <= btnMax.x &&
                    mpos.y >= btnMin.y && mpos.y <= btnMax.y)
                {
                    char buf[512] = {"MyChannel.64k2Channel"};
                    bool forSave = (bi != 0);
                    bool ok = K64GUI::openFileDialog(
                        buf, (int)sizeof(buf),
                        "64klang2 Channel\0*.64k2Channel\0All Files\0*.*\0",
                        "64k2Channel", forSave);
                    if (ok)
                    {
                        if (!forSave)
                        {
                            if (sc->loadChannel(ch, std::string(buf)))
                            {
                                selectedNodeIDs.clear();
                                syncSelectionToCore();
                                sc->numGUINodes(); // rebuild _nodesGUIAccessor — loadChannel invalidated it
                                rebuildZOrder();   // new channel nodes weren't in nodeZOrder yet
                                return true;       // break draw loop, same as node deletion
                            }
                        }
                        else
                        {
                            sc->saveChannel(ch, std::string(buf));
                        }
                    }
                }
            }
        }
    }

    // ── Channel name label (ChannelRoot only): below Load/Save, centered, as wide as needed ──
    if (hasChannelBtns)
    {
        std::string channelName = sc->gnName(guiIndex);
        if (!channelName.empty())
        {
            float labelBaseY = pos.y + (kHeaderHeight + (float)numSignals * kRowHeight
                                        + (hasEditBtn ? kEditButtonHeight : 0.f)
                                        + kEditButtonHeight * 2.f) * zoom;
            ImVec2 textSize = ImGui::CalcTextSize(channelName.c_str());
            float textScale = fontSize / ImGui::GetFontSize();
            float textW = textSize.x * textScale;
            float textH = fontSize;
            float pad = 4.f * zoom;
            float labelW = textW + pad * 2.f;
            float labelH = kRowHeight * zoom;
            // Center horizontally with node body
            float labelX = pos.x + (w - labelW) * 0.5f;
            float labelY = labelBaseY;
            ImVec2 labelMin(labelX, labelY);
            ImVec2 labelMax(labelX + labelW, labelY + labelH);
            ImU32 labelBg = isMuted ? withMuteAlpha(colorGlobalNode()) : colorGlobalNode();
            dl->AddRectFilled(labelMin, labelMax, labelBg, 2.f * zoom);
            float textX = labelX + pad;
            float textY = labelBaseY + (labelH - textH) * 0.5f;
            dl->AddText(pickFont(fontSize), fontSize, ImVec2(textX, textY), textColor, channelName.c_str());
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
                isDragging = false;
                pressedNodeID = -1;
                isRubberBanding = false;
                isWireDragging = false;
                wireDragFromNodeID = -1;
                wireDragInsertMode = false;
                showContextMenu = false;
                openEditPanels.clear();
                paramSyncState.clear();
                nodeZOrder.clear();
                syncSelectionToCore();
            }
            lastNodeCount = currentCount;
        }

        // Rebuild Z-order if empty (first frame after patch load, or first boot).
        if (nodeZOrder.empty() && sc->numGUINodes() > 0)
            rebuildZOrder();

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
        // Live signal readback
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

        // Draw nodes in Z-order (back-to-front, back = first drawn = behind)
        for (int zid : nodeZOrder)
        {
            int i = findGuiIndex(zid);
            if (i < 0 || !sc->gnIsVisible(i))
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

    // Show debug overlay
    if (showDebugOverlay)
    {
        ImGuiIO& io = ImGui::GetIO();

        // Frame counter + FPS
        debugFrameCount++;

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

        char dbg[6][256];
        snprintf(dbg[0], sizeof(dbg[0]),
                    "Frame=%-8llu  FPS=%.1f  dt=%.3fms",
                    (unsigned long long)debugFrameCount, io.Framerate, io.DeltaTime * 1000.f);
        snprintf(dbg[1], sizeof(dbg[1]),
                    "Mouse=%.1f,%.1f  CanvasOrigin=%.1f,%.1f  DisplaySize=%.0fx%.0f",
                    io.MousePos.x, io.MousePos.y,
                    canvasPos.x, canvasPos.y,
                    io.DisplaySize.x, io.DisplaySize.y);
        snprintf(dbg[2], sizeof(dbg[2]),
                    "CanvasXY=%.1f,%.1f  Zoom=%.3f  Offset=%.1f,%.1f",
                    canvasMouseX, canvasMouseY,
                    zoom, offsetX, offsetY);
        snprintf(dbg[3], sizeof(dbg[3]),
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
                            ImVec2(dbgPos.x + maxW + 8, dbgPos.y + lineH * 6 + 6),
                            IM_COL32(0, 0, 0, 200));
        for (int li = 0; li < 6; li++)
            dl->AddText(ImVec2(dbgPos.x + 4, dbgPos.y + 3 + li * lineH),
                        IM_COL32(255, 255, 0, 255), dbg[li]);
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

            // Paste entries — only on background context menu (not wire insert, not wire right-click)
            if (!wireDragInsertMode && contextWireFromID == -1 && contextWireToID == -1 &&
                !clipboard.empty())
            {
                if (ImGui::MenuItem("Paste selection"))
                {
                    pasteNodes(false, false);
                    ImGui::CloseCurrentPopup();
                    showContextMenu = false;
                }
                if (ImGui::MenuItem("Paste as Voice"))
                {
                    pasteNodes(false, true);
                    ImGui::CloseCurrentPopup();
                    showContextMenu = false;
                }
                if (ImGui::MenuItem("Paste as Global"))
                {
                    pasteNodes(true, false);
                    ImGui::CloseCurrentPopup();
                    showContextMenu = false;
                }
                ImGui::Separator();
            }

            // Signal Visualizer — wire right-click shortcut (inserts on the wire)
            if (!wireDragInsertMode && contextWireFromID >= 0 && contextWireToID >= 0 && contextWirePinIndex >= 0)
            {
                if (ImGui::MenuItem("Signal Visualizer"))
                {
                    int channel = -2;
                    bool isGlobal = false;
                    if (sc)
                    {
                        int refID = contextWireFromID >= 0 ? contextWireFromID
                                  : (!selectedNodeIDs.empty() ? *selectedNodeIDs.begin() : -1);
                        if (refID >= 0)
                        {
                            int gi2 = findGuiIndex(refID);
                            if (gi2 >= 0)
                            {
                                channel  = sc->gnChannel(gi2);
                                isGlobal = sc->gnIsGlobal(gi2);
                            }
                        }
                        bool doInsert = (contextWireFromID >= 0 && contextWireToID >= 0 &&
                                         contextWirePinIndex >= 0);
                        if (doInsert)
                        {
                            int fromGI2 = findGuiIndex(contextWireFromID);
                            int toGI2   = findGuiIndex(contextWireToID);
                            if (fromGI2 < 0 || toGI2 < 0)
                                doInsert = false;
                            else
                            {
                                int fromType = sc->gnType(fromGI2);
                                int toType   = sc->gnType(toGI2);
                                bool forbidden = (fromType == (int)VOICEROOT_ID && toType == (int)VOICEMANAGER_ID) ||
                                                 (fromType == (int)VOICEMANAGER_ID && toType == (int)NOTECONTROLLER_ID);
                                if (forbidden)
                                    doInsert = false;
                            }
                        }
                        sc->killVoices();
                        SynthNode* newNode = sc->createGUINode((DWORD)SIGNAL_VISUALIZER_ID,
                                                               (DWORD)channel,
                                                               (DWORD)(isGlobal ? 1 : 0),
                                                               contextMenuCanvasPos.x,
                                                               contextMenuCanvasPos.y);
                        if (newNode)
                        {
                            int newID = (int)newNode->valueOffset;
                            bringToFront(newID);
                            if (doInsert)
                            {
                                sc->disconnectInput((DWORD)contextWireToID, (DWORD)contextWirePinIndex);
                                sc->connectInput((DWORD)contextWireFromID, (DWORD)newID, 0);
                                sc->connectInput((DWORD)newID, (DWORD)contextWireToID, (DWORD)contextWirePinIndex);
                            }
                        }
                        contextWireFromID = -1;
                        contextWireToID = -1;
                        contextWirePinIndex = -1;
                        sc->numGUINodes();
                    }
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
                            bool ctrlHeld = isCtrlHeld();
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
                            // Voice type inputs (id > CONSTANT_ID) can never be global; ignore Ctrl
                            if ((int)nodeDef->id > (int)CONSTANT_ID)
                                isGlobal = false;
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
                                    if (newNode)
                                        bringToFront((int)newNode->valueOffset);
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
                char filenameBuf[512] = {};
                bool wavOk = K64GUI::openFileDialog(
                    filenameBuf, (int)sizeof(filenameBuf),
                    "Wave Files\0*.wav\0All Files\0*.*\0",
                    "wav", false);
                if (wavOk)
                    sc->setWaveFileReference(i, 0, freq, std::string(filenameBuf));
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

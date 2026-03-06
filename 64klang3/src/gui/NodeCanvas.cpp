#include "NodeCanvas.h"
#include "NodeConfig.h"
#include "core/SynthController.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace K64GUI {

NodeCanvas::NodeCanvas() {}
NodeCanvas::~NodeCanvas() {}

float NodeCanvas::nodeHeight(int numInputs) const
{
    return kHeaderHeight + (float)numInputs * kRowHeight;
}

int NodeCanvas::effectiveInputCount(int guiIndex) const
{
    SynthController* sc = SynthController::instance();
    if (!sc) return 0;
    // For variable-input nodes (NoteController, MultiAdd), gnNodeMaxSignals()
    // returns 0 (the static type definition). Use gnNodeInputs() which returns
    // the actual live numInputs count for those nodes.
    int maxSignals = sc->gnNodeMaxSignals(guiIndex);
    int liveInputs = sc->gnNodeInputs(guiIndex);
    return (liveInputs > maxSignals) ? liveInputs : maxSignals;
}

ImVec2 NodeCanvas::nodeScreenPos(double nx, double ny, const ImVec2& canvasOrigin) const
{
    return ImVec2(canvasOrigin.x + ((float)nx + offsetX) * zoom,
                  canvasOrigin.y + ((float)ny + offsetY) * zoom);
}

ImVec2 NodeCanvas::outputPinPos(const ImVec2& nodePos) const
{
    return ImVec2(nodePos.x + kNodeWidth * zoom, nodePos.y + kOutputPinY * zoom);
}

ImVec2 NodeCanvas::inputPinPos(const ImVec2& nodePos, int pinIndex) const
{
    return ImVec2(nodePos.x, nodePos.y + (kFirstPinY + (float)pinIndex * kRowHeight) * zoom);
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

        ImVec2 pos = nodeScreenPos(nx, ny, canvasOrigin);
        float w = kNodeWidth * zoom;
        float h = nodeHeight(numSignals) * zoom;

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

    ImVec2 pos = nodeScreenPos(nx, ny, canvasOrigin);
    float w = kNodeWidth * zoom;
    float h = nodeHeight(numSignals) * zoom;

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
        if (srcID != 0 && srcID != -1)
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

// ── Interaction ──────────────────────────────────────────────────────────

void NodeCanvas::handleNodeInteraction(const ImVec2& canvasPos, const ImVec2& canvasSize)
{
    // While renaming, only handle rename-related input
    if (isRenaming)
        return;

    SynthController* sc = SynthController::instance();
    if (!sc || !sc->isInitialized())
        return;

    ImGuiIO& io = ImGui::GetIO();
    ImVec2 mousePos = io.MousePos;

    // ── Left mouse press ──
    if (canvasHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
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
                bool ctrl = io.KeyCtrl;
                bool shift = io.KeyShift;

                if (shift)
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
        else
        {
            // Click on empty canvas
            if (!io.KeyCtrl)
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

    // ── Left mouse held: dragging or rubber-banding ──
    if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
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
        zoom = std::max(0.1f, std::min(zoom, 5.0f));

        // Zoom toward mouse position
        ImVec2 mouseRel = ImVec2(io.MousePos.x - canvasPos.x,
                                  io.MousePos.y - canvasPos.y);
        float zoomRatio = zoom / oldZoom;
        offsetX = mouseRel.x / zoom - (mouseRel.x / oldZoom - offsetX);
        offsetY = mouseRel.y / zoom - (mouseRel.y / oldZoom - offsetY);
    }

    // Pan with right mouse button drag — guard against node dragging
    if (canvasHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right) && !isDragging)
    {
        isPanning = true;
        panStart = io.MousePos;
    }
    if (isPanning)
    {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Right))
        {
            ImVec2 delta = ImVec2(io.MousePos.x - panStart.x, io.MousePos.y - panStart.y);
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

void NodeCanvas::drawNode(ImDrawList* dl, int guiIndex, const ImVec2& canvasOrigin)
{
    SynthController* sc = SynthController::instance();
    if (!sc) return;

    int nodeID = sc->gnID(guiIndex);
    double nx = sc->gnX(guiIndex);
    double ny = sc->gnY(guiIndex);
    bool isGlobal = sc->gnIsGlobal(guiIndex);
    int numSignals = effectiveInputCount(guiIndex);
    int numReq = sc->gnNodeReqSignals(guiIndex);
    int nodeType = sc->gnType(guiIndex);

    ImVec2 pos = nodeScreenPos(nx, ny, canvasOrigin);
    float w = kNodeWidth * zoom;
    float h = nodeHeight(numSignals) * zoom;

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

    // Header text
    const NodeTypeDef* typeDef = NodeConfig::instance().getNodeType(nodeType);
    const char* displayName = typeDef ? typeDef->name.c_str() : "???";

    // Use custom name if set
    std::string customName = sc->gnName(guiIndex);
    if (!customName.empty())
        displayName = customName.c_str();

    float fontSize = 14.f * zoom;
    if (fontSize >= 6.f) // only draw text if readable
    {
        ImVec2 textSize = ImGui::CalcTextSize(displayName);
        float textScale = fontSize / ImGui::GetFontSize();
        float textX = pos.x + (w - textSize.x * textScale) * 0.5f;
        float textY = pos.y + 3.f * zoom;
        dl->AddText(ImGui::GetFont(), fontSize, ImVec2(textX, textY), textColor, displayName);
    }

    // Header separator line
    float sepY = pos.y + kHeaderHeight * zoom;
    dl->AddLine(ImVec2(pos.x, sepY), ImVec2(pos.x + w, sepY), borderColor, 1.f);

    // Output pin (right side of header)
    ImVec2 outPin = outputPinPos(pos);
    ImU32 outPinFill = isMuted ? withMuteAlpha(colorPinWired()) : colorPinWired();
    ImU32 outPinBorder = isMuted ? withMuteAlpha(colorNodeBorder()) : colorNodeBorder();
    dl->AddCircleFilled(outPin, kPinRadius * zoom, outPinFill);
    dl->AddCircle(outPin, kPinRadius * zoom, outPinBorder, 0, 1.f);

    // Input pins
    for (int i = 0; i < numSignals; i++)
    {
        ImVec2 pinPos = inputPinPos(pos, i);
        int srcID = sc->gnInput(guiIndex, i);
        bool isWired = (srcID != 0 && srcID != -1);

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

        // Input label
        if (typeDef && i < (int)typeDef->inputs.size() && fontSize >= 6.f)
        {
            const char* label = typeDef->inputs[i].name.c_str();
            float labelX = pinPos.x + (kPinRadius + 4.f) * zoom;
            float labelY = pinPos.y - fontSize * 0.5f;
            dl->AddText(ImGui::GetFont(), std::max(fontSize * 0.85f, 8.f),
                        ImVec2(labelX, labelY), textColor, label);
        }
    }
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
            if (srcID == 0 || srcID == -1)
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

            // Wire color: selected destination = gold, else global=DeepPink / voice=LightSkyBlue
            ImU32 wireColor;
            if (toSelected)
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

    // Create an invisible button covering the canvas area to capture input
    ImGui::InvisibleButton("##canvas", canvasSize,
                           ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
    canvasHovered = ImGui::IsItemHovered();

    SynthController* sc = SynthController::instance();

    // Patch-load reset: if node count changed, clear interactive state
    if (sc && sc->isInitialized())
    {
        int currentCount = sc->numGUINodes();
        if (currentCount != lastNodeCount)
        {
            selectedNodeIDs.clear();
            mutedNodeIDs.clear();
            isRenaming = false;
            renamingNodeID = -1;
            isDragging = false;
            pressedNodeID = -1;
            isRubberBanding = false;
            lastNodeCount = currentCount;
            needsInitialView = true;
            syncSelectionToCore();
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
        // Draw wires first (behind nodes)
        drawWires(dl, canvasPos);

        // Draw nodes (skip internal/helper nodes)
        int numNodes = sc->numGUINodes();
        for (int i = 0; i < numNodes; i++)
        {
            if (!sc->gnIsVisible(i))
                continue;
            drawNode(dl, i, canvasPos);
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

    // Toggle debug overlay with Ctrl+Shift+D
    {
        ImGuiIO& io = ImGui::GetIO();
        if (io.KeyCtrl && io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_D, false))
            showDebugOverlay = !showDebugOverlay;

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
            char dbg[512];
            snprintf(dbg, sizeof(dbg),
                     "ItemHov=%d WinHov=%d MB0=%d MB1=%d Mouse=%.0f,%.0f Hit=%d Sel=%d Nodes=%d Zoom=%.2f",
                     (int)canvasHovered, (int)winHovered,
                     (int)io.MouseDown[0], (int)io.MouseDown[1],
                     io.MousePos.x, io.MousePos.y,
                     hitID, (int)selectedNodeIDs.size(), numNodes2, zoom);
            ImVec2 textSize = ImGui::CalcTextSize(dbg);
            ImVec2 dbgPos(canvasPos.x + 4, canvasPos.y + 4);
            dl->AddRectFilled(dbgPos, ImVec2(dbgPos.x + textSize.x + 8, dbgPos.y + textSize.y + 4),
                              IM_COL32(0, 0, 0, 200));
            dl->AddText(ImVec2(dbgPos.x + 4, dbgPos.y + 2),
                        IM_COL32(255, 255, 0, 255), dbg);
        }
    }

    dl->PopClipRect();

    // Draw rename overlay (uses ImGui windows, must be outside clip rect)
    drawRenameOverlay(canvasPos);
}

} // namespace K64GUI

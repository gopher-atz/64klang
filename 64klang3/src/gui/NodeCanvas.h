#pragma once

#include "imgui.h"
#include <unordered_set>

namespace K64GUI {

class NodeCanvas
{
public:
    NodeCanvas();
    ~NodeCanvas();

    void render();

    void setOffset(float x, float y) { offsetX = x; offsetY = y; }
    void setZoom(float z) { zoom = z; }
    float getZoom() const { return zoom; }

private:
    float offsetX = 0.f;
    float offsetY = 0.f;
    float zoom = 1.f;

    bool isPanning = false;
    ImVec2 panStart = {0, 0};

    // Selection
    std::unordered_set<int> selectedNodeIDs;

    // Drag
    bool   isDragging = false;
    int    pressedNodeID = -1;
    ImVec2 dragStartMouse = {0, 0};

    // Rubber-band
    bool   isRubberBanding = false;
    ImVec2 rubberBandStart = {0, 0};
    ImVec2 rubberBandCurrent = {0, 0};

    // Mute
    std::unordered_set<int> mutedNodeIDs;

    // Rename
    bool   isRenaming = false;
    int    renamingNodeID = -1;
    char   renameBuffer[256] = {};

    // Double-click detection
    double lastClickTime = 0.0;
    int    lastClickNodeID = -1;

    // Patch-load reset tracking
    int    lastNodeCount = 0;
    bool   needsInitialView = true;

    // Cached per-frame hover state from InvisibleButton
    bool   canvasHovered = false;

    // Debug overlay toggle (Ctrl+Shift+D)
    bool   showDebugOverlay = false;

    // Visual constants matching original WPF GUI
    static constexpr float kNodeWidth = 120.f;
    static constexpr float kHeaderHeight = 25.f;
    static constexpr float kRowHeight = 20.f;
    static constexpr float kPinRadius = 6.f;
    static constexpr float kWireStubLen = 20.f;
    static constexpr float kWireThickness = 3.5f;
    static constexpr float kOutputPinY = 14.f;   // center of header
    static constexpr float kFirstPinY = 37.f;     // first input pin Y offset
    static constexpr float kDragThreshold = 4.f;
    static constexpr double kDoubleClickTime = 0.3;

    // Colors
    static ImU32 colorVoiceNode()     { return IM_COL32(127, 163, 186, 255); }  // #7FA3BA
    static ImU32 colorGlobalNode()    { return IM_COL32(183, 160, 183, 255); }  // #B7A0B7
    static ImU32 colorSelectedNode()  { return IM_COL32(255, 205, 80, 255);  }  // #FFCD50
    static ImU32 colorNodeBorder()    { return IM_COL32(0, 0, 0, 255); }
    static ImU32 colorNodeText()      { return IM_COL32(0, 0, 0, 255); }
    static ImU32 colorVoiceWire()     { return IM_COL32(135, 206, 250, 255); }  // LightSkyBlue
    static ImU32 colorGlobalWire()    { return IM_COL32(255, 20, 147, 255);  }  // DeepPink
    static ImU32 colorSelectedWire()  { return IM_COL32(255, 205, 80, 255);  }  // Amber
    static ImU32 colorPinWired()      { return IM_COL32(144, 238, 144, 255); }  // LightGreen
    static ImU32 colorPinRequired()   { return IM_COL32(255, 60, 60, 255);   }  // Red
    static ImU32 colorPinOptional()   { return IM_COL32(80, 80, 80, 255);    }  // Dark
    static ImU32 colorRubberBandFill(){ return IM_COL32(255, 255, 255, 77);  }  // white 30%
    static ImU32 colorRubberBandBorder(){ return IM_COL32(255, 255, 255, 200); }

    // Alpha scaling for muted nodes
    static ImU32 withMuteAlpha(ImU32 col) {
        unsigned int a = (col >> 24) & 0xFF;
        a = (unsigned int)(a * 0.33f);
        return (col & 0x00FFFFFF) | (a << 24);
    }

    void drawNode(ImDrawList* dl, int guiIndex, const ImVec2& canvasOrigin);
    void drawWires(ImDrawList* dl, const ImVec2& canvasOrigin);
    void handlePanZoom(const ImVec2& canvasPos, const ImVec2& canvasSize);
    void handleNodeInteraction(const ImVec2& canvasPos, const ImVec2& canvasSize);

    int  hitTestNode(const ImVec2& mousePos, const ImVec2& canvasOrigin) const;
    int  findGuiIndex(int nodeID) const;
    bool nodeFullyInsideRect(int guiIndex, ImVec2 rectMin, ImVec2 rectMax, const ImVec2& canvasOrigin) const;
    void recursiveSelect(int nodeID, std::unordered_set<int>& visited);
    void syncSelectionToCore();

    void drawRubberBand(ImDrawList* dl);
    void drawRenameOverlay(const ImVec2& canvasOrigin);
    void commitRename();

    float nodeHeight(int numInputs) const;
    int   effectiveInputCount(int guiIndex) const;
    ImVec2 nodeScreenPos(double nx, double ny, const ImVec2& canvasOrigin) const;
    ImVec2 outputPinPos(const ImVec2& nodePos) const;
    ImVec2 inputPinPos(const ImVec2& nodePos, int pinIndex) const;
};

} // namespace K64GUI

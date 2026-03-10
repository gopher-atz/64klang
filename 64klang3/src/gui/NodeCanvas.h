#pragma once

#include "imgui.h"
#include <array>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <string>

namespace K64GUI {

class NodeCanvas
{
public:
    NodeCanvas();
    ~NodeCanvas();

    void render();

    void setOffset(float x, float y) { offsetX = x; offsetY = y; }
    void getOffset(float& x, float& y) const { x = offsetX; y = offsetY; }
    void setZoom(float z) { zoom = z; }
    float getZoom() const { return zoom; }

    // Restore a previously saved viewport — also suppresses the first-frame
    // auto-center so the saved position is not overwritten.
    void restoreViewport(float x, float y, float z)
    {
        offsetX = x; offsetY = y; zoom = z;
        needsInitialView = false;
    }

    // Toolbar integration
    void toggleWaveFileDialog() { showWaveFileDialog = !showWaveFileDialog; }
    void jumpToChannel(int channel);           // scroll canvas to center on ChannelRoot N
    void setSearchFilter(const char* text) { searchFilter = text; }

private:
    float offsetX = 0.f;
    float offsetY = 0.f;
    float zoom = 1.f;

    bool isPanning = false;
    bool didPan = false;
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

    // Wire drag
    bool   isWireDragging = false;
    int    wireDragFromNodeID = -1;
    ImVec2 wireDragCurrentPos = {0, 0};

    // Context menu
    bool   showContextMenu = false;
    ImVec2 contextMenuCanvasPos = {0, 0};

    // Edit panels (multiple can be open); vector preserves open order (back = topmost/last-opened)
    std::vector<int> openEditPanels;
    bool   mouseOverEditPanel = false;  // set per-frame, blocks canvas interaction

    // Per-parameter sync state: key = (uint64_t)nodeID<<32 | paramIdx
    // Initialised lazily when a panel opens (from L==R equality).
    // Stored explicitly so the user can toggle it independently of the values.
    std::unordered_map<uint64_t, bool> paramSyncState;

    // Knob drag tracking for edit panels
    int    knobDragNodeID = -1;
    int    knobDragParam = -1;
    bool   knobDragIsRight = false;
    float  knobDragAccum = 0.f;  // sub-step accumulator for integer-quantized dragging

    // Edit panel layout constants (in canvas/world coords, scaled by zoom)
    static constexpr float kEditPanelWidth = 230.f;
    static constexpr float kEditHeaderH = 25.f;
    static constexpr float kEditLabelH = 16.f;
    static constexpr float kEditKnobDiam = 36.f;
    static constexpr float kEditKnobTextH = 14.f;
    static constexpr float kEditFlagH = 18.f;
    static constexpr float kEditCheckboxSz = 12.f;

    // VU / live signal cache
    struct LiveData {
        float vuL = 0.f, vuR = 0.f;
        int   voiceCount = 0;
    };
    std::unordered_map<int, LiveData> liveDataCache;

    // Tracks which node outputs have at least one wire going out (rebuilt each frame)
    std::unordered_set<int> connectedOutputIDs;

    // Text edit buffers for TextToSpeech (SAPI, TypeID 50) and Formula (TypeID 54) panels.
    // Keyed by nodeID; lazily populated when a panel is first displayed.
    std::unordered_map<int, std::array<char, 4096>> textEditBuffers;

    // Clipboard for copy/paste
    struct ClipboardInput {
        double valL = 0, valR = 0;
        int mode = 0;
    };
    struct ClipboardNode {
        int typeID = 0, channel = -2;
        bool isGlobal = false;
        double relX = 0, relY = 0;
        std::vector<ClipboardInput> inputs;
        std::vector<int> internalWires; // per-pin: clipboard index or -1
    };
    std::vector<ClipboardNode> clipboard;

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

    // Search filter (set from toolbar; empty = no highlight)
    std::string searchFilter;

    // Canvas size cached from last render() call (used by jumpToChannel)
    ImVec2 canvasSizeCache = {0, 0};

    // Cached per-frame hover state from InvisibleButton
    bool   canvasHovered = false;

    // Debug overlay toggle (Ctrl+Shift+D)
    bool   showDebugOverlay = false;

    // Wave File References dialog (Ctrl+W)
    bool   showWaveFileDialog = false;

    // Toast notifications
    struct Toast { std::string message; double expireTime; };
    std::vector<Toast> toasts;
    void showToast(const char* msg);
    void drawToasts(const ImVec2& canvasPos, const ImVec2& canvasSize);

    // Visual constants matching original WPF GUI
    static constexpr float kNodeWidth = 120.f;
    static constexpr float kHeaderHeight = 25.f;
    static constexpr float kRowHeight = 18.f;
    static constexpr float kPinRadius = 5.f;      // matches text line height
    static constexpr float kPinInset = 5.f;        // pin center inset = radius, circle touches border
    static constexpr float kWireStubLen = 20.f;
    static constexpr float kWireThickness = 3.5f;
    static constexpr float kOutputPinY = 14.f;     // center of header
    static constexpr float kFirstPinY = 35.f;      // first input pin Y offset
    static constexpr float kDragThreshold = 4.f;
    static constexpr double kDoubleClickTime = 0.3;
    static constexpr float kDeleteBtnSize = 14.f;  // X button size

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
    static ImU32 colorPinOptional()   { return IM_COL32(30, 30, 30, 255);    }  // Black
    static ImU32 colorRubberBandFill(){ return IM_COL32(255, 255, 255, 77);  }  // white 30%
    static ImU32 colorRubberBandBorder(){ return IM_COL32(255, 255, 255, 200); }

    // Alpha scaling for muted nodes
    static ImU32 withMuteAlpha(ImU32 col) {
        unsigned int a = (col >> 24) & 0xFF;
        a = (unsigned int)(a * 0.33f);
        return (col & 0x00FFFFFF) | (a << 24);
    }

    bool drawNode(ImDrawList* dl, int guiIndex, const ImVec2& canvasOrigin); // returns true if node was deleted
    void drawWires(ImDrawList* dl, const ImVec2& canvasOrigin);
    void handlePanZoom(const ImVec2& canvasPos, const ImVec2& canvasSize);
    void handleNodeInteraction(const ImVec2& canvasPos, const ImVec2& canvasSize);

    int  hitTestNode(const ImVec2& mousePos, const ImVec2& canvasOrigin) const;
    int  findGuiIndex(int nodeID) const;
    bool nodeFullyInsideRect(int guiIndex, ImVec2 rectMin, ImVec2 rectMax, const ImVec2& canvasOrigin) const;
    void recursiveSelect(int nodeID, std::unordered_set<int>& visited);
    void syncSelectionToCore();

    // Pin hit-testing for wire drag
    int  hitTestOutputPin(const ImVec2& mousePos, const ImVec2& canvasOrigin) const;
    struct PinHit { int nodeID; int pinIndex; };
    PinHit hitTestInputPin(const ImVec2& mousePos, const ImVec2& canvasOrigin) const;
    void drawGhostWire(ImDrawList* dl, const ImVec2& canvasOrigin);

    void drawRubberBand(ImDrawList* dl);
    void drawRenameOverlay(const ImVec2& canvasOrigin);
    void drawEditPanel(ImDrawList* dl, const ImVec2& canvasOrigin);
    void updateMouseOverEditPanel(const ImVec2& canvasOrigin);
    void drawWaveFileDialog();
    void commitRename();

    static constexpr float kEditButtonHeight = 20.f;

    float nodeHeight(int numInputs, bool hasEditBtn, bool hasAddInput = false, bool hasChannelBtns = false) const;
    int   effectiveInputCount(int guiIndex) const;
    bool  nodeHasEditButton(int guiIndex) const;
    std::string buildModeText(int guiIndex) const;
    ImVec2 nodeScreenPos(double nx, double ny, const ImVec2& canvasOrigin) const;
    ImVec2 outputPinPos(const ImVec2& nodePos) const;
    ImVec2 inputPinPos(const ImVec2& nodePos, int pinIndex) const;
};

} // namespace K64GUI

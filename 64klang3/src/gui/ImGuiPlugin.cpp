#include "ImGuiPlugin.h"
#include "NodeCanvas.h"
#include "NodeConfig.h"
#include "imgui.h"
#include "core/SynthController.h"

#include <cstdio>

namespace K64GUI {

static bool s_initialized = false;
static NodeCanvas* s_canvas = nullptr;

void init()
{
    // Load node type definitions from embedded XML config
    if (!NodeConfig::instance().load())
        fprintf(stderr, "64klang3: Failed to parse embedded node config\n");

    s_canvas = new NodeCanvas();
    s_initialized = true;
}

void render()
{
    if (!s_initialized || !s_canvas)
        return;

    // Full-window canvas covering the entire editor area
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::Begin("##64klang3Canvas",
                 nullptr,
                 ImGuiWindowFlags_NoTitleBar |
                 ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoMove |
                 ImGuiWindowFlags_NoScrollbar |
                 ImGuiWindowFlags_NoScrollWithMouse |
                 ImGuiWindowFlags_NoBringToFrontOnFocus |
                 ImGuiWindowFlags_NoBackground);

    s_canvas->render();

    ImGui::End();
}

void shutdown()
{
    delete s_canvas;
    s_canvas = nullptr;
    s_initialized = false;
}

bool isInitialized()
{
    return s_initialized;
}

} // namespace K64GUI

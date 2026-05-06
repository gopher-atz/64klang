#pragma once

#include "pluginterfaces/gui/iplugview.h"
#include "public.sdk/source/common/pluginview.h"

#include <cstdint>    // uintptr_t

namespace Steinberg {
namespace Vst {

class K64PluginView : public CPluginView
{
public:
    K64PluginView();
    ~K64PluginView() override;

    // IPlugView overrides
    tresult PLUGIN_API isPlatformTypeSupported(FIDString type) override;
    tresult PLUGIN_API attached(void* parent, FIDString type) override;
    tresult PLUGIN_API removed() override;
    tresult PLUGIN_API onSize(ViewRect* newSize) override;
    tresult PLUGIN_API getSize(ViewRect* size) override;
    tresult PLUGIN_API canResize() override;
    tresult PLUGIN_API checkSizeConstraint(ViewRect* rect) override;

    static constexpr int32 kDefaultWidth  = 1280;
    static constexpr int32 kDefaultHeight = 800;

#ifdef _WIN32
    void renderFrameWin();         // called from Windows render thread
    void runWinRenderLoop();       // render-thread entry: owns WGL context and loops
#endif

#ifdef __APPLE__
    void renderFrameMacOS();       // called by NSTimer in PluginView_macOS.mm
#endif

#if defined(__linux__) && !defined(__APPLE__)
    void renderFrameLinux();       // called from background render thread
    void runLinuxRenderLoop();     // render-thread entry: sets up GL context then loops
#endif

private:
    void* nativeHandle = nullptr;
    bool guiInitialized = false;

#ifdef _WIN32
    // WGL context helpers — operate on process-wide singleton GL state.
    bool createWGLContext(void* hwnd);
    void destroyWGLContext();
#endif // _WIN32
};

} // namespace Vst
} // namespace Steinberg

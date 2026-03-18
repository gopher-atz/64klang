#pragma once

#include "pluginterfaces/gui/iplugview.h"
#include "public.sdk/source/common/pluginview.h"

#include <cstdint>    // uintptr_t
#include <mutex>



#if defined(__linux__) && !defined(__APPLE__)
#include <pthread.h>
// Forward-declare X11/GLX types without pulling in the full headers here
typedef unsigned long XID;
typedef XID Window;
struct _XDisplay;
typedef struct _XDisplay Display;
struct __GLXcontextRec;
typedef struct __GLXcontextRec* GLXContext;
#endif

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
    // WGL/OpenGL resources (void* to avoid <windows.h> in this header; HGLRC/HDC in .cpp)
    void*     winGLCtx   = nullptr;   // HGLRC
    void*     winDC      = nullptr;   // HDC
    int       viewWidth  = kDefaultWidth;
    int       viewHeight = kDefaultHeight;

    bool createWGLContext(void* hwnd);
    void destroyWGLContext();

    // Render thread (mirrors Linux pattern)
    void*         winRenderThread  = nullptr;  // HANDLE
    volatile bool winRenderRunning = false;
#endif // _WIN32

    // Protects D3D11/GL resources and ImGui state against concurrent access
    // between the timer-driven render and host lifecycle calls (removed/onSize).
    // renderFrame() uses try_lock (skip tick if busy); lifecycle methods use
    // lock (block until any in-progress frame completes before modifying state).
    std::mutex  renderMutex;

#if defined(__linux__) && !defined(__APPLE__)
    Display*    linuxDisplay = nullptr;
    Window      linuxWindow  = 0;
    GLXContext  linuxGLCtx   = nullptr;
    pthread_t   renderThread = 0;
    volatile bool renderRunning = false;
    int         viewWidth  = kDefaultWidth;
    int         viewHeight = kDefaultHeight;
#endif
};

} // namespace Vst
} // namespace Steinberg

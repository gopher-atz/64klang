#pragma once

#include "pluginterfaces/gui/iplugview.h"
#include "public.sdk/source/common/pluginview.h"

#include <cstdint>    // uintptr_t

#ifdef _WIN32
struct ID3D11Device;
struct ID3D11DeviceContext;
struct IDXGISwapChain;
struct ID3D11RenderTargetView;
struct ID3D11Texture2D;
#endif

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
    void renderFrame();            // called by Win32 timer callback
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
    // D3D11 resources
    ID3D11Device*            d3dDevice = nullptr;
    ID3D11DeviceContext*     d3dContext = nullptr;
    IDXGISwapChain*          swapChain = nullptr;
    ID3D11RenderTargetView*  mainRTV = nullptr;

    // MSAA offscreen target
    ID3D11Texture2D*         msaaTex = nullptr;
    ID3D11RenderTargetView*  msaaRTV = nullptr;
    int                      msaaWidth = 0;
    int                      msaaHeight = 0;

    bool createD3D11(void* hwnd);
    void destroyD3D11();
    void resizeSwapChain(int width, int height);
    void createMSAATarget(int width, int height);

    // Timer ID for render loop (UINT_PTR on Win32, stored as uintptr_t to avoid Windows header in this header)
    uintptr_t timerID = 0;
#endif // _WIN32

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

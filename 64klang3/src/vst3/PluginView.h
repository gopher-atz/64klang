#pragma once

#include "pluginterfaces/gui/iplugview.h"
#include "public.sdk/source/common/pluginview.h"

#ifdef _WIN32
struct ID3D11Device;
struct ID3D11DeviceContext;
struct IDXGISwapChain;
struct ID3D11RenderTargetView;
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

private:
    void* nativeHandle = nullptr;
    bool guiInitialized = false;

    static constexpr int32 kDefaultWidth = 1280;
    static constexpr int32 kDefaultHeight = 800;

#ifdef _WIN32
    // D3D11 resources
    ID3D11Device*            d3dDevice = nullptr;
    ID3D11DeviceContext*     d3dContext = nullptr;
    IDXGISwapChain*          swapChain = nullptr;
    ID3D11RenderTargetView*  mainRTV = nullptr;

    bool createD3D11(void* hwnd);
    void destroyD3D11();
    void resizeSwapChain(int width, int height);
    void renderFrame();

    // Timer for render loop
    static void __stdcall timerCallback(void* hwnd, unsigned int msg, unsigned long long id, unsigned long time);
    unsigned long long timerID = 0;
#endif
};

} // namespace Vst
} // namespace Steinberg

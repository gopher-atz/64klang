#include "PluginView.h"
#include "pluginterfaces/base/fstrdefs.h"

#ifdef _WIN32
#include <d3d11.h>
#include <dxgi.h>
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include "gui/ImGuiPlugin.h"

// Forward-declare the Win32 ImGui message handler
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Store view pointer for timer callback
static Steinberg::Vst::K64PluginView* g_activeView = nullptr;

// Subclass proc for the host's editor HWND
static WNDPROC g_originalWndProc = nullptr;
static LRESULT CALLBACK editorWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return 1;
    return CallWindowProc(g_originalWndProc, hWnd, msg, wParam, lParam);
}
#endif

namespace Steinberg {
namespace Vst {

K64PluginView::K64PluginView()
    : CPluginView(nullptr)
{
    rect.left = 0;
    rect.top = 0;
    rect.right = kDefaultWidth;
    rect.bottom = kDefaultHeight;
}

K64PluginView::~K64PluginView()
{
}

tresult PLUGIN_API K64PluginView::isPlatformTypeSupported(FIDString type)
{
#ifdef _WIN32
    if (FIDStringsEqual(type, kPlatformTypeHWND))
        return kResultTrue;
#elif defined(__APPLE__)
    if (FIDStringsEqual(type, kPlatformTypeNSView))
        return kResultTrue;
#else
    if (FIDStringsEqual(type, kPlatformTypeX11EmbedWindowID))
        return kResultTrue;
#endif
    return kResultFalse;
}

tresult PLUGIN_API K64PluginView::attached(void* parent, FIDString type)
{
    nativeHandle = parent;

#ifdef _WIN32
    if (!createD3D11(parent))
        return kResultFalse;

    // Init ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr; // no imgui.ini

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;
    style.FrameRounding = 2.0f;
    style.ScrollbarRounding = 2.0f;

    ImGui_ImplWin32_Init((HWND)parent);
    ImGui_ImplDX11_Init(d3dDevice, d3dContext);

    // Subclass the host window to receive input events
    g_originalWndProc = (WNDPROC)SetWindowLongPtr((HWND)parent, GWLP_WNDPROC, (LONG_PTR)editorWndProc);

    // Init the 64klang GUI layer
    K64GUI::init();

    // Start render timer (~60 fps)
    g_activeView = this;
    timerID = SetTimer((HWND)parent, 1, 16, (TIMERPROC)timerCallback);
#endif

    guiInitialized = true;
    return kResultOk;
}

tresult PLUGIN_API K64PluginView::removed()
{
#ifdef _WIN32
    if (nativeHandle)
    {
        KillTimer((HWND)nativeHandle, (UINT_PTR)timerID);
        timerID = 0;
        g_activeView = nullptr;

        // Restore original wndproc
        if (g_originalWndProc)
        {
            SetWindowLongPtr((HWND)nativeHandle, GWLP_WNDPROC, (LONG_PTR)g_originalWndProc);
            g_originalWndProc = nullptr;
        }

        K64GUI::shutdown();

        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();

        destroyD3D11();
    }
#endif

    guiInitialized = false;
    nativeHandle = nullptr;
    return kResultOk;
}

tresult PLUGIN_API K64PluginView::onSize(ViewRect* newSize)
{
    if (!newSize)
        return kInvalidArgument;

    rect = *newSize;

#ifdef _WIN32
    if (swapChain)
    {
        int w = newSize->right - newSize->left;
        int h = newSize->bottom - newSize->top;
        resizeSwapChain(w, h);
    }
#endif

    return kResultOk;
}

tresult PLUGIN_API K64PluginView::getSize(ViewRect* size)
{
    if (!size)
        return kInvalidArgument;

    *size = rect;
    return kResultOk;
}

#ifdef _WIN32

bool K64PluginView::createD3D11(void* hwnd)
{
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 1;
    sd.BufferDesc.Width = kDefaultWidth;
    sd.BufferDesc.Height = kDefaultHeight;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = (HWND)hwnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    D3D_FEATURE_LEVEL featureLevel;
    UINT flags = 0;
#ifdef _DEBUG
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags,
        nullptr, 0, D3D11_SDK_VERSION,
        &sd, &swapChain, &d3dDevice, &featureLevel, &d3dContext);

    if (FAILED(hr))
        return false;

    // Create render target view
    ID3D11Texture2D* backBuffer = nullptr;
    swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
    d3dDevice->CreateRenderTargetView(backBuffer, nullptr, &mainRTV);
    backBuffer->Release();

    return true;
}

void K64PluginView::destroyD3D11()
{
    if (mainRTV)  { mainRTV->Release();  mainRTV = nullptr; }
    if (swapChain) { swapChain->Release(); swapChain = nullptr; }
    if (d3dContext) { d3dContext->Release(); d3dContext = nullptr; }
    if (d3dDevice) { d3dDevice->Release(); d3dDevice = nullptr; }
}

void K64PluginView::resizeSwapChain(int width, int height)
{
    if (!swapChain || width <= 0 || height <= 0)
        return;

    if (mainRTV) { mainRTV->Release(); mainRTV = nullptr; }

    swapChain->ResizeBuffers(0, (UINT)width, (UINT)height, DXGI_FORMAT_UNKNOWN, 0);

    ID3D11Texture2D* backBuffer = nullptr;
    swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
    d3dDevice->CreateRenderTargetView(backBuffer, nullptr, &mainRTV);
    backBuffer->Release();
}

void K64PluginView::renderFrame()
{
    if (!d3dDevice || !swapChain || !mainRTV)
        return;

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // Render the 64klang GUI
    K64GUI::render();

    ImGui::Render();

    const float clearColor[4] = { 0.12f, 0.12f, 0.14f, 1.0f };
    d3dContext->OMSetRenderTargets(1, &mainRTV, nullptr);
    d3dContext->ClearRenderTargetView(mainRTV, clearColor);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    swapChain->Present(1, 0); // vsync
}

void __stdcall K64PluginView::timerCallback(void* hwnd, unsigned int msg,
                                             unsigned long long id, unsigned long time)
{
    (void)hwnd; (void)msg; (void)id; (void)time;
    if (g_activeView)
        g_activeView->renderFrame();
}

#endif // _WIN32

} // namespace Vst
} // namespace Steinberg

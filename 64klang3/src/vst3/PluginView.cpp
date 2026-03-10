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

static void CALLBACK timerCallback(HWND, UINT, UINT_PTR, DWORD)
{
    if (g_activeView)
        g_activeView->renderFrame();
}

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

    // Two font sizes: small for zoom ≤ 1.5x, large for zoom > 1.5x.
    // ImGui renders text sharpest when the requested pixel size is close to
    // the loaded size.  A single 32 px font looks blurry at the 12-15 px
    // sizes used at normal (1×) zoom.
    io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 14.0f);  // Fonts[0] – default
    io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 32.0f);  // Fonts[1] – high zoom
    io.Fonts->Build();

    // Subclass the host window to receive input events
    g_originalWndProc = (WNDPROC)SetWindowLongPtr((HWND)parent, GWLP_WNDPROC, (LONG_PTR)editorWndProc);

    // Init the 64klang GUI layer and pass the HWND for file dialogs
    K64GUI::init();
    K64GUI::setWindowHandle(parent);

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

        K64GUI::setWindowHandle(nullptr);
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
    if (swapChain && nativeHandle)
    {
        // Always resize the swap chain to the actual HWND client rect, not the
        // ViewRect.  The host may add chrome (e.g. a keyboard bar) inside the
        // same HWND, making the HWND taller than the ViewRect.  Using the HWND
        // size prevents DWM from stretching the smaller buffer to fit.
        RECT hwndRect = {};
        ::GetClientRect((HWND)nativeHandle, &hwndRect);
        int w = (hwndRect.right  > 0) ? (hwndRect.right  - hwndRect.left) : (newSize->right  - newSize->left);
        int h = (hwndRect.bottom > 0) ? (hwndRect.bottom - hwndRect.top)  : (newSize->bottom - newSize->top);
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
    // Use the actual HWND client size so the swap chain matches exactly.
    // If the swap chain is smaller than the HWND, DWM stretches the presented
    // frame, causing a Y-scale mismatch between io.MousePos and rendered pixels.
    RECT clientRect = {};
    ::GetClientRect((HWND)hwnd, &clientRect);
    UINT initW = (clientRect.right  > 0) ? (UINT)(clientRect.right  - clientRect.left) : (UINT)kDefaultWidth;
    UINT initH = (clientRect.bottom > 0) ? (UINT)(clientRect.bottom - clientRect.top)  : (UINT)kDefaultHeight;

    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 1;
    sd.BufferDesc.Width = initW;
    sd.BufferDesc.Height = initH;
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

    // Create 4x MSAA offscreen target at the same real size
    createMSAATarget((int)initW, (int)initH);

    return true;
}

void K64PluginView::destroyD3D11()
{
    if (msaaRTV) { msaaRTV->Release(); msaaRTV = nullptr; }
    if (msaaTex) { msaaTex->Release(); msaaTex = nullptr; }
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

    createMSAATarget(width, height);
}

void K64PluginView::createMSAATarget(int width, int height)
{
    if (msaaRTV) { msaaRTV->Release(); msaaRTV = nullptr; }
    if (msaaTex) { msaaTex->Release(); msaaTex = nullptr; }

    D3D11_TEXTURE2D_DESC td = {};
    td.Width = (UINT)width;
    td.Height = (UINT)height;
    td.MipLevels = 1;
    td.ArraySize = 1;
    td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    td.SampleDesc.Count = 4;
    td.SampleDesc.Quality = 0;
    td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_RENDER_TARGET;

    // Check if 4x MSAA is supported, fall back to 1x if not
    UINT qualityLevels = 0;
    d3dDevice->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM, 4, &qualityLevels);
    if (qualityLevels == 0)
    {
        // 4x not supported, no MSAA
        msaaWidth = 0;
        msaaHeight = 0;
        return;
    }

    HRESULT hr = d3dDevice->CreateTexture2D(&td, nullptr, &msaaTex);
    if (FAILED(hr))
        return;

    hr = d3dDevice->CreateRenderTargetView(msaaTex, nullptr, &msaaRTV);
    if (FAILED(hr))
    {
        msaaTex->Release();
        msaaTex = nullptr;
        return;
    }

    msaaWidth = width;
    msaaHeight = height;
}

void K64PluginView::renderFrame()
{
    if (!d3dDevice || !swapChain || !mainRTV)
        return;

    // Guard against re-entrant calls: blocking operations inside a frame
    // (e.g. GetOpenFileNameA) pump the Win32 message loop which can fire
    // WM_TIMER again, causing a second NewFrame() before the first Render().
    static bool s_inFrame = false;
    if (s_inFrame)
        return;
    s_inFrame = true;

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();

    // The Win32 backend sets io.DisplaySize from GetClientRect, which includes
    // any host chrome (keyboard bar, etc.) added inside the HWND.  Clamp it to
    // the plugin's own ViewRect so ImGui's coordinate space matches only the
    // plugin area.  The swap chain is kept at the full HWND size to prevent DWM
    // from stretching the buffer.
    {
        ImGuiIO& io = ImGui::GetIO();
        float pluginW = (float)(rect.right  - rect.left);
        float pluginH = (float)(rect.bottom - rect.top);
        if (io.DisplaySize.x > pluginW) io.DisplaySize.x = pluginW;
        if (io.DisplaySize.y > pluginH) io.DisplaySize.y = pluginH;
    }

    ImGui::NewFrame();

    // Render the 64klang GUI
    K64GUI::render();

    ImGui::Render();

    const float clearColor[4] = { 0.12f, 0.12f, 0.14f, 1.0f };

    // Render into MSAA target if available, then resolve to back buffer
    if (msaaRTV)
    {
        d3dContext->OMSetRenderTargets(1, &msaaRTV, nullptr);
        d3dContext->ClearRenderTargetView(msaaRTV, clearColor);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        // Resolve MSAA → back buffer
        ID3D11Texture2D* backBuffer = nullptr;
        swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
        d3dContext->ResolveSubresource(backBuffer, 0, msaaTex, 0, DXGI_FORMAT_R8G8B8A8_UNORM);
        backBuffer->Release();
    }
    else
    {
        d3dContext->OMSetRenderTargets(1, &mainRTV, nullptr);
        d3dContext->ClearRenderTargetView(mainRTV, clearColor);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }

    swapChain->Present(1, 0); // vsync
    s_inFrame = false;
}

#endif // _WIN32

} // namespace Vst
} // namespace Steinberg

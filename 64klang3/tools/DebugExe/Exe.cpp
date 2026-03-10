// 64klang3 DebugExe — standalone test host
// No C++/CLI. Direct SynthController calls + D3D11 ImGui window.

#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <chrono>

#include "core/SynthController.h"
#include "gui/ImGuiPlugin.h"

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

// AudioOut.h must come before MidiIn.h (defines CHECK and helper templates)
#include "AudioOut.h"
#include "MidiIn.h"

// Set to your MIDI device name substring, or L"" to disable MIDI input
static const wchar_t* MidiInName = L"A-PRO 1";

// ---------------------------------------------------------------------------
// MIDI queue (lockless ring buffer — the LO in YOLO stands for lockless)
// ---------------------------------------------------------------------------

static MidiEvent midiQueue[256];
static volatile int mqRead  = 0;
static volatile int mqWrite = 0;

static void ApplyEvent(MidiEvent e)
{
    unsigned char status  = e.cmd & 0xf0;
    unsigned char channel = e.cmd & 0x0f;

    if (status == 0x90 || status == 0x80)
    {
        unsigned char note     = e.d1 & 0x7f;
        unsigned char velocity = e.d2 & 0x7f;
        if (status == 0x80 || velocity == 0)
            SynthController::instance()->noteOff(channel, note, velocity);
        else
            SynthController::instance()->noteOn(channel, note, velocity);
    }
    else if (status == 0xA0)
    {
        SynthController::instance()->noteAftertouch(channel, e.d1 & 0x7f, e.d2 & 0x7f);
    }
    else if (status == 0xB0)
    {
        SynthController::instance()->midiSignal(channel, e.d2 & 0x7f, e.d1 & 0x7f);
    }
    else if (status == 0xE0)
    {
        int value = (((int)(e.d2 & 0x7f)) << 7) + (e.d1 & 0x7f);
        SynthController::instance()->midiSignal(channel, (value >> 6), 0); // 0-255, center 128
    }
}

static void midiCallback(MidiEvent e)
{
    midiQueue[mqWrite++ & 0xff] = e;
}

// ---------------------------------------------------------------------------
// Audio callback — runs on the WASAPI thread
// ---------------------------------------------------------------------------

static void audioCallback(float* buffer, int samples)
{
    const int TEMPSIZE = 16384;
    static float tempL[TEMPSIZE];
    static float tempR[TEMPSIZE];

    auto* sc = SynthController::instance();
    if (!sc || !sc->isInitialized())
        goto silence;

    if (!SynthController::DataAccessMutex.try_lock_for(std::chrono::milliseconds(1)))
        goto silence;

    while (samples > 0)
    {
        while ((mqWrite - mqRead) > 0)
            ApplyEvent(midiQueue[mqRead++ & 0xff]);

        int todo = Min(samples, TEMPSIZE);
        sc->tick(tempL, tempR, todo);

        for (int i = 0; i < todo; i++)
        {
            volatile float l = tempL[i];
            volatile float r = tempR[i];
            *buffer++ = Clamp(l == l ? l : 0.f, -0.999f, 0.999f);
            *buffer++ = Clamp(r == r ? r : 0.f, -0.999f, 0.999f);
        }

        samples -= todo;
    }

    SynthController::DataAccessMutex.unlock();
    return;

silence:
    memset(buffer, 0, sizeof(float) * 2 * samples);
}

// ---------------------------------------------------------------------------
// D3D11 resources
// ---------------------------------------------------------------------------

static ID3D11Device*           g_d3dDevice  = nullptr;
static ID3D11DeviceContext*    g_d3dContext = nullptr;
static IDXGISwapChain*         g_swapChain  = nullptr;
static ID3D11RenderTargetView* g_mainRTV    = nullptr;
static ID3D11Texture2D*        g_msaaTex    = nullptr;
static ID3D11RenderTargetView* g_msaaRTV    = nullptr;

static void createMSAATarget(int w, int h)
{
    if (g_msaaRTV) { g_msaaRTV->Release(); g_msaaRTV = nullptr; }
    if (g_msaaTex) { g_msaaTex->Release(); g_msaaTex = nullptr; }

    UINT qualityLevels = 0;
    g_d3dDevice->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM, 4, &qualityLevels);
    if (qualityLevels == 0)
        return;

    D3D11_TEXTURE2D_DESC td = {};
    td.Width            = (UINT)w;
    td.Height           = (UINT)h;
    td.MipLevels        = 1;
    td.ArraySize        = 1;
    td.Format           = DXGI_FORMAT_R8G8B8A8_UNORM;
    td.SampleDesc.Count = 4;
    td.Usage            = D3D11_USAGE_DEFAULT;
    td.BindFlags        = D3D11_BIND_RENDER_TARGET;

    if (FAILED(g_d3dDevice->CreateTexture2D(&td, nullptr, &g_msaaTex)))
        return;
    if (FAILED(g_d3dDevice->CreateRenderTargetView(g_msaaTex, nullptr, &g_msaaRTV)))
    {
        g_msaaTex->Release(); g_msaaTex = nullptr;
    }
}

static bool createD3D11(HWND hwnd)
{
    RECT cr = {};
    GetClientRect(hwnd, &cr);
    UINT w = Max((int)(cr.right - cr.left), 1);
    UINT h = Max((int)(cr.bottom - cr.top), 1);

    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount                        = 1;
    sd.BufferDesc.Width                   = w;
    sd.BufferDesc.Height                  = h;
    sd.BufferDesc.Format                  = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator   = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage                        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow                       = hwnd;
    sd.SampleDesc.Count                   = 1;
    sd.Windowed                           = TRUE;
    sd.SwapEffect                         = DXGI_SWAP_EFFECT_DISCARD;

    UINT flags = 0;
#ifdef _DEBUG
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL featureLevel;
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags,
        nullptr, 0, D3D11_SDK_VERSION,
        &sd, &g_swapChain, &g_d3dDevice, &featureLevel, &g_d3dContext);
    if (FAILED(hr))
        return false;

    ID3D11Texture2D* backBuffer = nullptr;
    g_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
    g_d3dDevice->CreateRenderTargetView(backBuffer, nullptr, &g_mainRTV);
    backBuffer->Release();

    createMSAATarget((int)w, (int)h);
    return true;
}

static void resizeSwapChain(int w, int h)
{
    if (!g_swapChain || w <= 0 || h <= 0)
        return;

    if (g_mainRTV) { g_mainRTV->Release(); g_mainRTV = nullptr; }

    g_swapChain->ResizeBuffers(0, (UINT)w, (UINT)h, DXGI_FORMAT_UNKNOWN, 0);

    ID3D11Texture2D* backBuffer = nullptr;
    g_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
    g_d3dDevice->CreateRenderTargetView(backBuffer, nullptr, &g_mainRTV);
    backBuffer->Release();

    createMSAATarget(w, h);
}

static void destroyD3D11()
{
    if (g_msaaRTV)  { g_msaaRTV->Release();  g_msaaRTV  = nullptr; }
    if (g_msaaTex)  { g_msaaTex->Release();  g_msaaTex  = nullptr; }
    if (g_mainRTV)  { g_mainRTV->Release();  g_mainRTV  = nullptr; }
    if (g_swapChain){ g_swapChain->Release(); g_swapChain= nullptr; }
    if (g_d3dContext){ g_d3dContext->Release();g_d3dContext=nullptr;}
    if (g_d3dDevice){ g_d3dDevice->Release(); g_d3dDevice= nullptr; }
}

// ---------------------------------------------------------------------------
// Render frame
// ---------------------------------------------------------------------------

static void renderFrame()
{
    if (!g_d3dDevice || !g_swapChain || !g_mainRTV)
        return;

    // Guard against re-entrant calls: file dialogs pump the message loop,
    // which would fire WM_TIMER again before the current frame finishes.
    static bool s_inFrame = false;
    if (s_inFrame)
        return;
    s_inFrame = true;

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    K64GUI::render();

    ImGui::Render();

    const float clearColor[4] = { 0.12f, 0.12f, 0.14f, 1.0f };

    if (g_msaaRTV)
    {
        g_d3dContext->OMSetRenderTargets(1, &g_msaaRTV, nullptr);
        g_d3dContext->ClearRenderTargetView(g_msaaRTV, clearColor);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        ID3D11Texture2D* backBuffer = nullptr;
        g_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
        g_d3dContext->ResolveSubresource(backBuffer, 0, g_msaaTex, 0, DXGI_FORMAT_R8G8B8A8_UNORM);
        backBuffer->Release();
    }
    else
    {
        g_d3dContext->OMSetRenderTargets(1, &g_mainRTV, nullptr);
        g_d3dContext->ClearRenderTargetView(g_mainRTV, clearColor);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }

    g_swapChain->Present(1, 0);
    s_inFrame = false;
}

// ---------------------------------------------------------------------------
// Window procedure
// ---------------------------------------------------------------------------

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return 1;

    switch (msg)
    {
    case WM_SIZE:
        if (g_swapChain && wParam != SIZE_MINIMIZED)
            resizeSwapChain(LOWORD(lParam), HIWORD(lParam));
        return 0;

    case WM_TIMER:
        renderFrame();
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    // Register window class
    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = L"64klang3DebugExe";
    RegisterClassExW(&wc);

    HWND hwnd = CreateWindowExW(
        0, L"64klang3DebugExe", L"64klang3 Debug Host",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1280, 800,
        nullptr, nullptr, hInstance, nullptr);

    if (!createD3D11(hwnd))
        return 1;

    // ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding   = 0.0f;
    style.FrameRounding    = 2.0f;
    style.ScrollbarRounding= 2.0f;

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_d3dDevice, g_d3dContext);

    io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 14.0f);
    io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 32.0f);
    io.Fonts->Build();

    // GUI
    K64GUI::createCanvas();
    K64GUI::init();
    K64GUI::setWindowHandle(hwnd);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // Audio + MIDI
    CoInitialize(nullptr);
    StartAudio(44100, audioCallback);
    StartMidi(MidiInName, midiCallback);

    // Render timer ~60 fps
    SetTimer(hwnd, 1, 16, nullptr);

    // Message loop
    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Cleanup
    KillTimer(hwnd, 1);
    StopMidi();
    StopAudio();

    K64GUI::setWindowHandle(nullptr);
    K64GUI::shutdown();
    K64GUI::destroyCanvas();

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    destroyD3D11();
    CoUninitialize();

    return (int)msg.wParam;
}

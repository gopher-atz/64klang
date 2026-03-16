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

#elif defined(__APPLE__)

// Bridge functions implemented in PluginView_macOS.mm
bool k64_macOS_createView(void* parentNSView, int width, int height,
                           Steinberg::Vst::K64PluginView* view);
void k64_macOS_destroyView();
void k64_macOS_resizeView(int width, int height);
void k64_macOS_renderFrame();

#else
// Linux — X11 + OpenGL3
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "gui/ImGuiPlugin.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <unistd.h>    // usleep
#include <cstdio>      // fprintf

static void* renderThreadEntryLinux(void* arg);

#endif

namespace Steinberg {
namespace Vst {

// Persists window size and position across view create/destroy cycles within a
#ifdef _WIN32
// Session-static window geometry — position and total HWND size (including host
// chrome) captured from GetWindowRect so SetWindowPos round-trips cleanly.
static POINT s_savedPos      = { -1, -1 };  // -1,-1 = not yet saved
static int   s_savedWinW     = 0;
static int   s_savedWinH     = 0;
static bool  s_pendingRestore = false;
#endif

K64PluginView::K64PluginView()
    : CPluginView(nullptr)
{
    rect = { 0, 0, kDefaultWidth, kDefaultHeight };
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

    // Arm lazy restore — applied on the first rendered frame so the host has
    // finished placing the window before we reposition/resize it.
    s_pendingRestore = true;

#elif defined(__APPLE__)

    int w = rect.right  - rect.left;
    int h = rect.bottom - rect.top;
    if (w <= 0) w = kDefaultWidth;
    if (h <= 0) h = kDefaultHeight;
    if (!k64_macOS_createView(parent, w, h, this))
        return kResultFalse;

#else // Linux — X11 + OpenGL3

    viewWidth  = rect.right  - rect.left;
    viewHeight = rect.bottom - rect.top;
    if (viewWidth  <= 0) viewWidth  = kDefaultWidth;
    if (viewHeight <= 0) viewHeight = kDefaultHeight;

    // Open connection to the X server
    XInitThreads();
    linuxDisplay = XOpenDisplay(nullptr);
    if (!linuxDisplay)
    {
        fprintf(stderr, "64klang3: XOpenDisplay failed\n");
        return kResultFalse;
    }

    int screen = DefaultScreen(linuxDisplay);

    // Choose a visual with double-buffered OpenGL
    static const int visualAttribs[] = {
        GLX_RGBA, GLX_DOUBLEBUFFER,
        GLX_RED_SIZE, 8, GLX_GREEN_SIZE, 8, GLX_BLUE_SIZE, 8,
        GLX_DEPTH_SIZE, 0,
        None
    };
    XVisualInfo* vi = glXChooseVisual(linuxDisplay, screen, const_cast<int*>(visualAttribs));
    if (!vi)
    {
        fprintf(stderr, "64klang3: glXChooseVisual failed\n");
        XCloseDisplay(linuxDisplay);
        linuxDisplay = nullptr;
        return kResultFalse;
    }

    // Create a child X11 window embedded in the host's window
    XSetWindowAttributes swa = {};
    swa.colormap   = XCreateColormap(linuxDisplay,
                                     RootWindow(linuxDisplay, vi->screen),
                                     vi->visual, AllocNone);
    swa.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask
                   | ButtonPressMask | ButtonReleaseMask | PointerMotionMask
                   | StructureNotifyMask;

    linuxWindow = XCreateWindow(
        linuxDisplay,
        (Window)(uintptr_t)parent,   // parent from host
        0, 0,
        (unsigned)viewWidth, (unsigned)viewHeight,
        0,
        vi->depth, InputOutput, vi->visual,
        CWColormap | CWEventMask, &swa);

    XMapWindow(linuxDisplay, linuxWindow);
    XFlush(linuxDisplay);

    // Create GLX context
    linuxGLCtx = glXCreateContext(linuxDisplay, vi, nullptr, GL_TRUE);
    XFree(vi);
    if (!linuxGLCtx)
    {
        fprintf(stderr, "64klang3: glXCreateContext failed\n");
        XDestroyWindow(linuxDisplay, linuxWindow);
        XCloseDisplay(linuxDisplay);
        linuxDisplay = nullptr;
        linuxWindow  = 0;
        return kResultFalse;
    }

    // Make current on this thread briefly to initialise ImGui, then release —
    // the render thread will own the context for the rest of the session.
    glXMakeCurrent(linuxDisplay, linuxWindow, linuxGLCtx);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename  = nullptr;
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;
    style.FrameRounding  = 2.0f;

    // Fonts — try common Linux paths
    const char* fontPaths[] = {
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/TTF/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/freefont/FreeSans.ttf",
        nullptr
    };
    bool loadedSmall = false, loadedLarge = false;
    for (int i = 0; fontPaths[i] && (!loadedSmall || !loadedLarge); ++i)
    {
        if (!loadedSmall && io.Fonts->AddFontFromFileTTF(fontPaths[i], 14.0f))
            loadedSmall = true;
        if (!loadedLarge && io.Fonts->AddFontFromFileTTF(fontPaths[i], 32.0f))
            loadedLarge = true;
    }
    if (!loadedSmall) io.Fonts->AddFontDefault();
    if (!loadedLarge) io.Fonts->AddFontDefault();
    io.Fonts->Build();

    ImGui_ImplOpenGL3_Init("#version 130");

    K64GUI::init();

    glXMakeCurrent(linuxDisplay, None, nullptr); // release; render thread takes over

    // Start background render thread
    renderRunning = true;
    pthread_create(&renderThread, nullptr, renderThreadEntryLinux, this);

#endif

    guiInitialized = true;
    return kResultOk;
}

tresult PLUGIN_API K64PluginView::removed()
{
#ifdef _WIN32
    if (nativeHandle)
    {
        // Save full HWND rect for session-static restore on next open.
        // Use the total window size (not the VST3 ViewRect) so that
        // SetWindowPos round-trips without shrinking host chrome each cycle.
        RECT wr = {};
        ::GetWindowRect((HWND)nativeHandle, &wr);
        s_savedPos  = { wr.left, wr.top };
        s_savedWinW = wr.right  - wr.left;
        s_savedWinH = wr.bottom - wr.top;

        KillTimer((HWND)nativeHandle, (UINT_PTR)timerID);
        timerID = 0;
        g_activeView = nullptr;

        // Block until any in-progress renderFrame() finishes before touching
        // D3D11 resources or ImGui state. The timer has been killed above so
        // no new frame will start after we acquire the lock.
        std::lock_guard<std::mutex> lock(renderMutex);

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

#elif defined(__APPLE__)

    k64_macOS_destroyView();

#else // Linux

    renderRunning = false;
    if (renderThread)
    {
        pthread_join(renderThread, nullptr);
        renderThread = 0;
    }

    if (linuxDisplay)
    {
        glXMakeCurrent(linuxDisplay, linuxWindow, linuxGLCtx);

        K64GUI::shutdown();
        ImGui_ImplOpenGL3_Shutdown();
        ImGui::DestroyContext();

        glXMakeCurrent(linuxDisplay, None, nullptr);
        glXDestroyContext(linuxDisplay, linuxGLCtx);
        linuxGLCtx = nullptr;

        XDestroyWindow(linuxDisplay, linuxWindow);
        linuxWindow = 0;
        XCloseDisplay(linuxDisplay);
        linuxDisplay = nullptr;
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
    int w = newSize->right  - newSize->left;
    int h = newSize->bottom - newSize->top;

#ifdef _WIN32
    if (swapChain && nativeHandle)
    {
        // Always resize the swap chain to the actual HWND client rect, not the
        // ViewRect.  The host may add chrome (e.g. a keyboard bar) inside the
        // same HWND, making the HWND taller than the ViewRect.  Using the HWND
        // size prevents DWM from stretching the smaller buffer to fit.
        RECT hwndRect = {};
        ::GetClientRect((HWND)nativeHandle, &hwndRect);
        int hw = (hwndRect.right  > 0) ? (hwndRect.right  - hwndRect.left) : w;
        int hh = (hwndRect.bottom > 0) ? (hwndRect.bottom - hwndRect.top)  : h;
        // Wait for any in-progress frame to finish before resizing D3D11 buffers.
        std::lock_guard<std::mutex> lock(renderMutex);
        resizeSwapChain(hw, hh);
    }

#elif defined(__APPLE__)

    if (guiInitialized && w > 0 && h > 0)
        k64_macOS_resizeView(w, h);

#else // Linux

    if (guiInitialized && linuxDisplay && linuxWindow && w > 0 && h > 0)
    {
        // Wait for any in-progress renderFrameLinux() to finish before
        // modifying the dimensions the render thread reads each frame.
        std::lock_guard<std::mutex> lock(renderMutex);
        viewWidth  = w;
        viewHeight = h;
        XResizeWindow(linuxDisplay, linuxWindow, (unsigned)w, (unsigned)h);
        XFlush(linuxDisplay);
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

tresult PLUGIN_API K64PluginView::canResize()
{
    return kResultTrue;
}

tresult PLUGIN_API K64PluginView::checkSizeConstraint(ViewRect* rect)
{
    if (!rect)
        return kInvalidArgument;

    static constexpr int32 kMinWidth  = 640;
    static constexpr int32 kMinHeight = 400;

    if (rect->right  - rect->left < kMinWidth)
        rect->right  = rect->left + kMinWidth;
    if (rect->bottom - rect->top  < kMinHeight)
        rect->bottom = rect->top  + kMinHeight;

    return kResultTrue;
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

    // Skip rendering when the window is minimized: Present(vsync=1) can block
    // indefinitely or return DXGI_STATUS_OCCLUDED, and D3D11 resources may be
    // in the middle of a resize triggered by the host.
    if (IsIconic((HWND)nativeHandle))
        return;

    // Non-blocking acquire: if removed() or onSize() is currently holding the
    // mutex to modify D3D11 resources, or a prior frame has not yet finished
    // (e.g. a file-dialog pumped WM_TIMER before the frame completed), skip
    // this tick rather than racing or re-entering ImGui.
    if (!renderMutex.try_lock())
        return;

    // Lazy position + size restore: applied on the first frame after attach so
    // the host has finished placing the window before we override it.
    if (s_pendingRestore && nativeHandle)
    {
        s_pendingRestore = false;
        // Must release renderMutex BEFORE calling SetWindowPos.  SetWindowPos
        // sends WM_SIZE synchronously on the same calling thread when the size
        // changes (resize→close→reopen scenario), which re-enters onSize() and
        // tries to acquire renderMutex via lock_guard.  std::mutex on Windows
        // uses SRWLOCK which is NOT re-entrant; acquiring it twice on the same
        // thread deadlocks (and the host reports it as a plugin crash).
        // Releasing here is safe: the timer is the only producer of frames, and
        // we are returning immediately so D3D resources won't be touched until
        // the next tick (by which time onSize() will have resized the swapchain).
        renderMutex.unlock();
        if (s_savedPos.x != -1)
            SetWindowPos((HWND)nativeHandle, nullptr,
                         s_savedPos.x, s_savedPos.y, s_savedWinW, s_savedWinH,
                         SWP_NOZORDER | SWP_NOACTIVATE);
        return; // skip this one frame; next tick renders with the correct swapchain size
    }

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
    renderMutex.unlock();
}

#endif // _WIN32

// ─────────────────────────────────────────────────────────────────────────────
// macOS: thin wrapper — actual work is in PluginView_macOS.mm
// ─────────────────────────────────────────────────────────────────────────────
#ifdef __APPLE__

void K64PluginView::renderFrameMacOS()
{
    k64_macOS_renderFrame();
}

#endif // __APPLE__

// ─────────────────────────────────────────────────────────────────────────────
// Linux: X11 + OpenGL3 render thread
// ─────────────────────────────────────────────────────────────────────────────
#if defined(__linux__) && !defined(__APPLE__)

void K64PluginView::renderFrameLinux()
{
    if (!linuxDisplay || !linuxWindow || !linuxGLCtx)
        return;

    // Hold the mutex for the entire frame so that onSize() (host thread) and
    // removed() (after pthread_join) cannot race with resource access.
    std::lock_guard<std::mutex> lock(renderMutex);

    // Handle X11 events (key/mouse) — forward to ImGui
    while (XPending(linuxDisplay))
    {
        XEvent ev;
        XNextEvent(linuxDisplay, &ev);
        ImGuiIO& io = ImGui::GetIO();

        switch (ev.type)
        {
        case MotionNotify:
            io.AddMousePosEvent((float)ev.xmotion.x, (float)ev.xmotion.y);
            break;
        case ButtonPress:
        case ButtonRelease:
        {
            bool down = (ev.type == ButtonPress);
            if (ev.xbutton.button == Button1) io.AddMouseButtonEvent(0, down);
            if (ev.xbutton.button == Button3) io.AddMouseButtonEvent(1, down);
            if (ev.xbutton.button == Button4) io.AddMouseWheelEvent(0.0f, +1.0f);
            if (ev.xbutton.button == Button5) io.AddMouseWheelEvent(0.0f, -1.0f);
            break;
        }
        case KeyPress:
        case KeyRelease:
        {
            // Minimal key forwarding — translate keycode to ImGuiKey
            // (full mapping omitted for brevity; extend as needed)
            bool down = (ev.type == KeyPress);
            KeySym sym = XLookupKeysym(&ev.xkey, 0);
            (void)sym; (void)down;
            break;
        }
        case ConfigureNotify:
            io.DisplaySize = ImVec2((float)ev.xconfigure.width,
                                    (float)ev.xconfigure.height);
            glViewport(0, 0, ev.xconfigure.width, ev.xconfigure.height);
            break;
        default:
            break;
        }
    }

    // Render frame
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)viewWidth, (float)viewHeight);
    io.DeltaTime   = 1.0f / 60.0f;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    K64GUI::render();

    ImGui::Render();

    glClearColor(0.12f, 0.12f, 0.14f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glXSwapBuffers(linuxDisplay, linuxWindow);
}

void K64PluginView::runLinuxRenderLoop()
{
    glXMakeCurrent(linuxDisplay, linuxWindow, linuxGLCtx);
    while (renderRunning)
    {
        renderFrameLinux();
        usleep(16000); // ~60 fps
    }
    glXMakeCurrent(linuxDisplay, None, nullptr);
}

static void* renderThreadEntryLinux(void* arg)
{
    static_cast<K64PluginView*>(arg)->runLinuxRenderLoop();
    return nullptr;
}

#endif // Linux

} // namespace Vst
} // namespace Steinberg

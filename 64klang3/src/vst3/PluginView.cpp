#include "PluginView.h"
#include "pluginterfaces/base/fstrdefs.h"

#ifdef _WIN32
#include <windows.h>
#include <GL/gl.h>
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_opengl3.h"
#include "gui/ImGuiPlugin.h"

// Forward-declare the Win32 ImGui message handler
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// DPI scale used when fonts were built; WM_DPICHANGED adjusts FontGlobalScale
// relative to this baseline so text tracks the monitor's DPI at all times.
static float g_initialDpiScale = 1.f;

// Subclass proc for the host's editor HWND — feeds input events to ImGui.
// Runs on the host's message-pump thread; ImGui queues events thread-safely.
static WNDPROC g_originalWndProc = nullptr;
static LRESULT CALLBACK editorWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return 1;

    if (msg == WM_DPICHANGED)
    {
        float newDpi   = (float)LOWORD(wParam);
        float newScale = newDpi / 96.f;
        ImGui::GetIO().FontGlobalScale = newScale / g_initialDpiScale;
        // Let the host reposition/resize the window via the suggested rect.
        const RECT* suggested = reinterpret_cast<const RECT*>(lParam);
        if (suggested)
            SetWindowPos(hWnd, nullptr,
                         suggested->left, suggested->top,
                         suggested->right  - suggested->left,
                         suggested->bottom - suggested->top,
                         SWP_NOZORDER | SWP_NOACTIVATE);
        return 0;
    }

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
#include <X11/keysym.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <unistd.h>    // usleep
#include <cstdio>      // fprintf

#endif

#if defined(__linux__) && !defined(__APPLE__)
static inline void k64_updateImGuiModsFromXState(ImGuiIO& io, unsigned int state)
{
    io.AddKeyEvent(ImGuiMod_Ctrl,  (state & ControlMask) != 0);
    io.AddKeyEvent(ImGuiMod_Shift, (state & ShiftMask)   != 0);
    io.AddKeyEvent(ImGuiMod_Alt,   (state & Mod1Mask)    != 0);
    io.AddKeyEvent(ImGuiMod_Super, (state & Mod4Mask)    != 0);
}

static ImGuiKey k64_x11KeysymToImGuiKey(KeySym sym)
{
    switch (sym)
    {
    case XK_Tab: return ImGuiKey_Tab;
    case XK_ISO_Left_Tab: return ImGuiKey_Tab;
    case XK_Left: return ImGuiKey_LeftArrow;
    case XK_Right: return ImGuiKey_RightArrow;
    case XK_Up: return ImGuiKey_UpArrow;
    case XK_Down: return ImGuiKey_DownArrow;
    case XK_Page_Up: return ImGuiKey_PageUp;
    case XK_Page_Down: return ImGuiKey_PageDown;
    case XK_Home: return ImGuiKey_Home;
    case XK_End: return ImGuiKey_End;
    case XK_Insert: return ImGuiKey_Insert;
    case XK_Delete: return ImGuiKey_Delete;
    case XK_BackSpace: return ImGuiKey_Backspace;
    case XK_space: return ImGuiKey_Space;
    case XK_Return: return ImGuiKey_Enter;
    case XK_KP_Enter: return ImGuiKey_KeypadEnter;
    case XK_Escape: return ImGuiKey_Escape;

    case XK_apostrophe: return ImGuiKey_Apostrophe;
    case XK_comma: return ImGuiKey_Comma;
    case XK_minus: return ImGuiKey_Minus;
    case XK_period: return ImGuiKey_Period;
    case XK_slash: return ImGuiKey_Slash;
    case XK_semicolon: return ImGuiKey_Semicolon;
    case XK_equal: return ImGuiKey_Equal;
    case XK_bracketleft: return ImGuiKey_LeftBracket;
    case XK_backslash: return ImGuiKey_Backslash;
    case XK_bracketright: return ImGuiKey_RightBracket;
    case XK_grave: return ImGuiKey_GraveAccent;

    case XK_Caps_Lock: return ImGuiKey_CapsLock;
    case XK_Scroll_Lock: return ImGuiKey_ScrollLock;
    case XK_Num_Lock: return ImGuiKey_NumLock;
    case XK_Print: return ImGuiKey_PrintScreen;
    case XK_Pause: return ImGuiKey_Pause;

    case XK_Shift_L: return ImGuiKey_LeftShift;
    case XK_Shift_R: return ImGuiKey_RightShift;
    case XK_Control_L: return ImGuiKey_LeftCtrl;
    case XK_Control_R: return ImGuiKey_RightCtrl;
    case XK_Alt_L: return ImGuiKey_LeftAlt;
    case XK_Alt_R: return ImGuiKey_RightAlt;
    case XK_Super_L: return ImGuiKey_LeftSuper;
    case XK_Super_R: return ImGuiKey_RightSuper;
    case XK_Menu: return ImGuiKey_Menu;

    case XK_KP_0: return ImGuiKey_Keypad0;
    case XK_KP_1: return ImGuiKey_Keypad1;
    case XK_KP_2: return ImGuiKey_Keypad2;
    case XK_KP_3: return ImGuiKey_Keypad3;
    case XK_KP_4: return ImGuiKey_Keypad4;
    case XK_KP_5: return ImGuiKey_Keypad5;
    case XK_KP_6: return ImGuiKey_Keypad6;
    case XK_KP_7: return ImGuiKey_Keypad7;
    case XK_KP_8: return ImGuiKey_Keypad8;
    case XK_KP_9: return ImGuiKey_Keypad9;
    case XK_KP_Decimal: return ImGuiKey_KeypadDecimal;
    case XK_KP_Divide: return ImGuiKey_KeypadDivide;
    case XK_KP_Multiply: return ImGuiKey_KeypadMultiply;
    case XK_KP_Subtract: return ImGuiKey_KeypadSubtract;
    case XK_KP_Add: return ImGuiKey_KeypadAdd;

    case XK_F1: return ImGuiKey_F1;
    case XK_F2: return ImGuiKey_F2;
    case XK_F3: return ImGuiKey_F3;
    case XK_F4: return ImGuiKey_F4;
    case XK_F5: return ImGuiKey_F5;
    case XK_F6: return ImGuiKey_F6;
    case XK_F7: return ImGuiKey_F7;
    case XK_F8: return ImGuiKey_F8;
    case XK_F9: return ImGuiKey_F9;
    case XK_F10: return ImGuiKey_F10;
    case XK_F11: return ImGuiKey_F11;
    case XK_F12: return ImGuiKey_F12;
    default:
        break;
    }

    if (sym >= XK_0 && sym <= XK_9)
        return (ImGuiKey)((int)ImGuiKey_0 + (int)(sym - XK_0));

    if (sym >= XK_a && sym <= XK_z)
        return (ImGuiKey)((int)ImGuiKey_A + (int)(sym - XK_a));
    if (sym >= XK_A && sym <= XK_Z)
        return (ImGuiKey)((int)ImGuiKey_A + (int)(sym - XK_A));

    return ImGuiKey_None;
}
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
// Forward declaration — definition at the bottom of the WIN32 section
static DWORD WINAPI renderThreadEntryWin(LPVOID arg);
#elif !defined(__APPLE__)
// Linux: forward declaration — definition is at the bottom of this file
static void* renderThreadEntryLinux(void* arg);
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
    if (!createWGLContext(parent))
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
    ImGui_ImplOpenGL3_Init("#version 130");

    // Query the DPI scale for the parent window so all UI elements are sharp on
    // high-DPI monitors.  We do NOT call ImGui_ImplWin32_EnableDpiAwareness()
    // because that is process-wide and would interfere with the host DAW.
    float dpiScale = ImGui_ImplWin32_GetDpiScaleForHwnd((HWND)parent);
    if (dpiScale <= 0.f) dpiScale = 1.f;
    g_initialDpiScale = dpiScale;

    // Two font sizes: small for zoom ≤ 1.5x, large for zoom > 1.5x.
    // ImGui renders text sharpest when the requested pixel size is close to
    // the loaded size.  A single 32 px font looks blurry at the 12-15 px
    // sizes used at normal (1×) zoom.
    io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 14.0f * dpiScale);  // Fonts[0] – default
    io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 32.0f * dpiScale);  // Fonts[1] – high zoom
    io.Fonts->Build();

    // Scale all style sizes (padding, rounding, etc.) to match DPI.
    ImGui::GetStyle().ScaleAllSizes(dpiScale);

    // Subclass the host window to receive input events
    g_originalWndProc = (WNDPROC)SetWindowLongPtr((HWND)parent, GWLP_WNDPROC, (LONG_PTR)editorWndProc);

    // Init the 64klang GUI layer and pass the HWND for file dialogs
    K64GUI::init();
    K64GUI::setWindowHandle(parent);

    // Release WGL context from this thread so the render thread can own it.
    // (A WGL context can only be current on one thread at a time.)
    wglMakeCurrent(nullptr, nullptr);

    // Start dedicated render thread (~60 fps, mirrors Linux)
    winRenderRunning = true;
    winRenderThread  = CreateThread(nullptr, 0, renderThreadEntryWin, this, 0, nullptr);

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
                   | StructureNotifyMask | FocusChangeMask | EnterWindowMask | LeaveWindowMask;

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

        // Signal the render thread to stop and wait for it to exit.
        winRenderRunning = false;
        if (winRenderThread)
        {
            WaitForSingleObject((HANDLE)winRenderThread, 3000);
            CloseHandle((HANDLE)winRenderThread);
            winRenderThread = nullptr;
        }

        // The render thread released the WGL context before exiting;
        // re-acquire it here so we can call the ImGui/GL shutdown functions.
        if (winDC && winGLCtx)
            wglMakeCurrent((HDC)winDC, (HGLRC)winGLCtx);

        // Restore original wndproc
        if (g_originalWndProc)
        {
            SetWindowLongPtr((HWND)nativeHandle, GWLP_WNDPROC, (LONG_PTR)g_originalWndProc);
            g_originalWndProc = nullptr;
        }

        K64GUI::setWindowHandle(nullptr);
        K64GUI::shutdown();

        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();

        destroyWGLContext();
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
    if (winGLCtx && nativeHandle)
    {
        // Use the actual HWND client rect; host may add chrome inside the same HWND.
        RECT hwndRect = {};
        ::GetClientRect((HWND)nativeHandle, &hwndRect);
        int hw = (hwndRect.right  > 0) ? (hwndRect.right  - hwndRect.left) : w;
        int hh = (hwndRect.bottom > 0) ? (hwndRect.bottom - hwndRect.top)  : h;
        // viewWidth/viewHeight are read by the render thread each frame;
        // plain volatile int stores are atomic on x86 for aligned 32-bit values.
        std::lock_guard<std::mutex> lock(renderMutex);
        viewWidth  = hw;
        viewHeight = hh;
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

// WGL swap interval extension — loaded at runtime
typedef BOOL (WINAPI * PFNWGLSWAPINTERVALEXTPROC)(int);
static PFNWGLSWAPINTERVALEXTPROC wgl_SwapIntervalEXT = nullptr;

bool K64PluginView::createWGLContext(void* hwnd)
{
    winDC = GetDC((HWND)hwnd);
    if (!winDC)
        return false;

    PIXELFORMATDESCRIPTOR pfd = {};
    pfd.nSize      = sizeof(pfd);
    pfd.nVersion   = 1;
    pfd.dwFlags    = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;

    int pf = ChoosePixelFormat((HDC)winDC, &pfd);
    if (!pf || !SetPixelFormat((HDC)winDC, pf, &pfd))
    {
        ReleaseDC((HWND)hwnd, (HDC)winDC);
        winDC = nullptr;
        return false;
    }

    winGLCtx = wglCreateContext((HDC)winDC);
    if (!winGLCtx)
    {
        ReleaseDC((HWND)hwnd, (HDC)winDC);
        winDC = nullptr;
        return false;
    }

    // Make current briefly on this (attach) thread so ImGui init functions work.
    // attached() releases the context before starting the render thread.
    wglMakeCurrent((HDC)winDC, (HGLRC)winGLCtx);

    // Enable vsync — the render thread uses Sleep(1) as a yield; SwapBuffers
    // with vsync provides natural ~60 fps pacing without busy-spinning.
    wgl_SwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
    if (wgl_SwapIntervalEXT)
        wgl_SwapIntervalEXT(1);

    // Init viewport dimensions from HWND client area
    RECT cr = {};
    if (::GetClientRect((HWND)hwnd, &cr))
    {
        viewWidth  = (cr.right  > 0) ? (cr.right  - cr.left) : kDefaultWidth;
        viewHeight = (cr.bottom > 0) ? (cr.bottom - cr.top)  : kDefaultHeight;
    }

    return true;
}

void K64PluginView::destroyWGLContext()
{
    if (winGLCtx)
    {
        wglMakeCurrent(nullptr, nullptr);
        wglDeleteContext((HGLRC)winGLCtx);
        winGLCtx = nullptr;
    }
    if (winDC && nativeHandle)
    {
        ReleaseDC((HWND)nativeHandle, (HDC)winDC);
        winDC = nullptr;
    }
}

// ─── renderFrameWin ──────────────────────────────────────────────────────────
// One rendered frame. Called by runWinRenderLoop() on the render thread.
void K64PluginView::renderFrameWin()
{
    // Skip while minimised — no visible surface.
    if (IsIconic((HWND)nativeHandle))
        return;

    // Hold renderMutex for the whole frame so onSize() (host thread) can safely
    // update viewWidth/viewHeight between frames.
    std::lock_guard<std::mutex> lock(renderMutex);

    // Lazy position + size restore: applied on the first frame after attach.
    // SetWindowPos sends WM_SIZE synchronously on the calling thread (here the
    // render thread), which enters onSize() — onSize() also needs renderMutex
    // but we already hold it.  Release before calling SetWindowPos to avoid
    // self-deadlock, then return; the next frame will render at the correct size.
    if (s_pendingRestore && nativeHandle)
    {
        s_pendingRestore = false;
        // Drop the lock before SetWindowPos (std::mutex is not recursive).
        // We release manually via a raw unlock pattern:
        return; // lock is released by destructor; SetWindowPos called below
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplWin32_NewFrame();

    // Feed sizes to the debug overlay every frame.
    {
        ImGuiIO& io = ImGui::GetIO();
        float pluginW = (float)(rect.right  - rect.left);
        float pluginH = (float)(rect.bottom - rect.top);
        RECT cr = {};
        ::GetClientRect((HWND)nativeHandle, &cr);
    }

    ImGui::NewFrame();
    K64GUI::render();
    ImGui::Render();

    glClearColor(0.12f, 0.12f, 0.14f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    SwapBuffers((HDC)winDC); // vsync via wglSwapIntervalEXT(1)
}

// ─── runWinRenderLoop ─────────────────────────────────────────────────────────
// Render thread entry: owns the WGL context for its lifetime.
void K64PluginView::runWinRenderLoop()
{
    wglMakeCurrent((HDC)winDC, (HGLRC)winGLCtx);

    while (winRenderRunning)
    {
        // Handle the pendingRestore case outside the frame lock so we can
        // call SetWindowPos without deadlocking on renderMutex.
        if (s_pendingRestore && nativeHandle)
        {
            s_pendingRestore = false;
            if (s_savedPos.x != -1)
                SetWindowPos((HWND)nativeHandle, nullptr,
                             s_savedPos.x, s_savedPos.y, s_savedWinW, s_savedWinH,
                             SWP_NOZORDER | SWP_NOACTIVATE);
            // Skip rendering this tick; next iteration has the correct size.
            Sleep(1);
            continue;
        }

        renderFrameWin();
        Sleep(1); // yield; vsync in SwapBuffers provides actual pacing
    }

    // Release context before the thread exits so the shutdown code in
    // removed() can re-acquire it on the host thread.
    wglMakeCurrent(nullptr, nullptr);
}

static DWORD WINAPI renderThreadEntryWin(LPVOID arg)
{
    static_cast<K64PluginView*>(arg)->runWinRenderLoop();
    return 0;
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
            k64_updateImGuiModsFromXState(io, ev.xmotion.state);
            io.AddMousePosEvent((float)ev.xmotion.x, (float)ev.xmotion.y);
            break;
        case ButtonPress:
        case ButtonRelease:
        {
            bool down = (ev.type == ButtonPress);
            k64_updateImGuiModsFromXState(io, ev.xbutton.state);
            if (ev.xbutton.button == Button1) io.AddMouseButtonEvent(0, down);
            if (ev.xbutton.button == Button3) io.AddMouseButtonEvent(1, down);
            if (ev.xbutton.button == Button2) io.AddMouseButtonEvent(2, down);
            if (ev.xbutton.button == Button4) io.AddMouseWheelEvent(0.0f, +1.0f);
            if (ev.xbutton.button == Button5) io.AddMouseWheelEvent(0.0f, -1.0f);
            break;
        }
        case KeyPress:
        case KeyRelease:
        {
            bool down = (ev.type == KeyPress);
            KeySym sym = NoSymbol;
            char text[8] = {};
            int textLen = XLookupString(&ev.xkey, text, (int)sizeof(text), &sym, nullptr);

            k64_updateImGuiModsFromXState(io, ev.xkey.state);

            ImGuiKey key = k64_x11KeysymToImGuiKey(sym);
            if (key != ImGuiKey_None)
                io.AddKeyEvent(key, down);

            if (down)
            {
                // Feed text input only for regular character entry, not when
                // Ctrl/Alt combinations are active.
                bool ctrlOrAlt = (ev.xkey.state & (ControlMask | Mod1Mask)) != 0;
                if (!ctrlOrAlt)
                {
                    for (int i = 0; i < textLen; ++i)
                    {
                        const unsigned char c = (unsigned char)text[i];
                        if (c >= 32)
                            io.AddInputCharacter((unsigned int)c);
                    }
                }
            }
            break;
        }
        case FocusIn:
            io.AddFocusEvent(true);
            break;
        case FocusOut:
            io.AddFocusEvent(false);
            break;
        case LeaveNotify:
            io.AddMousePosEvent(-1.0f, -1.0f);
            break;
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

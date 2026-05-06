#include "PluginView.h"
#include "pluginterfaces/base/fstrdefs.h"

#ifdef _WIN32
#include <windows.h>
#include <GL/gl.h>
#include <vector>
#include <algorithm>
#include <mutex>
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
#include <vector>
#include <algorithm>
#include <mutex>
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
#if defined(_WIN32) || (defined(__linux__) && !defined(__APPLE__))
// Common singleton view state shared by Windows and Linux implementations.
static ImGuiContext*  s_imguiCtx      = nullptr;
static void*          s_nativeHandle  = nullptr;   // current host handle hosting renderer
static int            s_viewWidth     = K64PluginView::kDefaultWidth;
static int            s_viewHeight    = K64PluginView::kDefaultHeight;
static volatile bool  s_renderRunning = false;
static std::mutex     s_renderMutex;
static int            s_viewCount     = 0;  // # of live K64PluginView instances
static std::vector<K64PluginView*> s_allViews;
static K64PluginView* s_activeView    = nullptr;
#endif

#ifdef _WIN32
// Session-static window geometry — position and total HWND size (including host
// chrome) captured from GetWindowRect so SetWindowPos round-trips cleanly.
static POINT s_savedPos      = { -1, -1 };  // -1,-1 = not yet saved
static int   s_savedWinW     = 0;
static int   s_savedWinH     = 0;
static bool  s_pendingRestore = false;
// Forward declaration — definition at the bottom of the WIN32 section
static DWORD WINAPI renderThreadEntryWin(LPVOID arg);

// ── Singleton rendering state ─────────────────────────────────────────────────
// The ImGui context, GL context, and render thread are process-wide singletons.
// Multiple K64PluginView instances (VST alias instruments) share this state so
// there is always exactly one ImGui context and one render thread at a time.
static void*          s_winGLCtx         = nullptr;   // HGLRC
static void*          s_winDC            = nullptr;   // HDC
static void*          s_winRenderThread  = nullptr;   // HANDLE

#elif defined(__linux__) && !defined(__APPLE__)
// Linux singleton render state (mirrors the Windows singleton model)
static Display*        s_linuxDisplay       = nullptr;
static Window          s_linuxWindow        = 0;
static GLXContext      s_linuxGLCtx         = nullptr;
static pthread_t       s_linuxRenderThread  = 0;

// Linux: forward declaration — definition is at the bottom of this file
static void* renderThreadEntryLinux(void* arg);
#endif

K64PluginView::K64PluginView()
    : CPluginView(nullptr)
{
    rect = { 0, 0, kDefaultWidth, kDefaultHeight };
#if defined(_WIN32) || (defined(__linux__) && !defined(__APPLE__))
    ++s_viewCount;
    s_allViews.push_back(this);
#endif
}

K64PluginView::~K64PluginView()
{
#if defined(_WIN32) || (defined(__linux__) && !defined(__APPLE__))
    // Unregister from the view list.
    auto it = std::find(s_allViews.begin(), s_allViews.end(), this);
    if (it != s_allViews.end())
        s_allViews.erase(it);

    // Destroy the singleton ImGui context when the very last view instance is
    // released (plugin unload).
    if (--s_viewCount <= 0)
    {
        s_viewCount = 0;
        if (s_imguiCtx)
        {
            ImGui::DestroyContext(s_imguiCtx);
            s_imguiCtx = nullptr;
        }
    }
#endif
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
    // ── Singleton migration ───────────────────────────────────────────────────
    // If another view is currently rendering (e.g. a second alias editor just
    // opened), stop that render and tear down its backends before re-initialising
    // on the new HWND.  The ImGui context is kept alive to preserve canvas state.
    if (s_imguiCtx && s_renderRunning)
    {
        s_renderRunning = false;
        if (s_winRenderThread)
        {
            WaitForSingleObject((HANDLE)s_winRenderThread, 3000);
            CloseHandle((HANDLE)s_winRenderThread);
            s_winRenderThread = nullptr;
        }
        if (s_winDC && s_winGLCtx)
            wglMakeCurrent((HDC)s_winDC, (HGLRC)s_winGLCtx);

        if (g_originalWndProc && s_nativeHandle)
        {
            SetWindowLongPtr((HWND)s_nativeHandle, GWLP_WNDPROC, (LONG_PTR)g_originalWndProc);
            g_originalWndProc = nullptr;
        }
        K64GUI::setWindowHandle(nullptr);
        ImGui::SetCurrentContext(s_imguiCtx);
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplWin32_Shutdown();
        destroyWGLContext();   // releases s_winDC, s_winGLCtx (uses s_nativeHandle)
        s_nativeHandle = nullptr;
    }

    // ── Create WGL context for the new HWND ──────────────────────────────────
    if (!createWGLContext(parent))
        return kResultFalse;

    s_nativeHandle = parent;

    // ── Create or reuse the singleton ImGui context ───────────────────────────
    if (!s_imguiCtx)
    {
        IMGUI_CHECKVERSION();
        s_imguiCtx = ImGui::CreateContext();
    }
    ImGui::SetCurrentContext(s_imguiCtx);

    // Reset style from scratch so ScaleAllSizes always starts from defaults.
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding    = 0.0f;
    style.FrameRounding     = 2.0f;
    style.ScrollbarRounding = 2.0f;

    // ── Backends ─────────────────────────────────────────────────────────────
    ImGui_ImplWin32_Init((HWND)parent);
    ImGui_ImplOpenGL3_Init("#version 130");

    // ── Fonts + DPI ───────────────────────────────────────────────────────────
    // We do NOT call ImGui_ImplWin32_EnableDpiAwareness() — that is process-wide
    // and would interfere with the host DAW.
    float dpiScale = ImGui_ImplWin32_GetDpiScaleForHwnd((HWND)parent);
    if (dpiScale <= 0.f) dpiScale = 1.f;
    g_initialDpiScale = dpiScale;

    // Rebuild font atlas for the new HWND's DPI (two sizes: default + high-zoom).
    io.Fonts->Clear();
    io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 14.0f * dpiScale);
    io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 32.0f * dpiScale);
    io.Fonts->Build();

    ImGui::GetStyle().ScaleAllSizes(dpiScale);

    // ── Subclass host HWND for input events ──────────────────────────────────
    g_originalWndProc = (WNDPROC)SetWindowLongPtr((HWND)parent, GWLP_WNDPROC, (LONG_PTR)editorWndProc);

    // ── K64GUI ────────────────────────────────────────────────────────────────
    K64GUI::init();
    K64GUI::setWindowHandle(parent);

    // Release WGL context from this thread so the render thread can own it.
    wglMakeCurrent(nullptr, nullptr);

    // ── Start render thread ───────────────────────────────────────────────────
    s_renderRunning = true;
    s_winRenderThread  = CreateThread(nullptr, 0, renderThreadEntryWin, this, 0, nullptr);

    s_pendingRestore = true;

#elif defined(__APPLE__)

    int w = rect.right  - rect.left;
    int h = rect.bottom - rect.top;
    if (w <= 0) w = kDefaultWidth;
    if (h <= 0) h = kDefaultHeight;
    if (!k64_macOS_createView(parent, w, h, this))
        return kResultFalse;

#else // Linux — X11 + OpenGL3

    s_viewWidth  = rect.right  - rect.left;
    s_viewHeight = rect.bottom - rect.top;
    if (s_viewWidth  <= 0) s_viewWidth  = kDefaultWidth;
    if (s_viewHeight <= 0) s_viewHeight = kDefaultHeight;

    // ── Singleton migration ─────────────────────────────────────────────────
    // If another view currently owns rendering, stop and tear down Linux GL/X11
    // backends before recreating on the new host parent. Keep ImGui context.
    if (s_linuxDisplay && s_renderRunning)
    {
        s_renderRunning = false;
        if (s_linuxRenderThread)
        {
            pthread_join(s_linuxRenderThread, nullptr);
            s_linuxRenderThread = 0;
        }

        if (s_linuxDisplay && s_linuxWindow && s_linuxGLCtx)
            glXMakeCurrent(s_linuxDisplay, s_linuxWindow, s_linuxGLCtx);

        K64GUI::setWindowHandle(nullptr);

        if (s_imguiCtx)
        {
            ImGui::SetCurrentContext(s_imguiCtx);
            ImGui_ImplOpenGL3_Shutdown();
        }

        if (s_linuxDisplay)
        {
            glXMakeCurrent(s_linuxDisplay, None, nullptr);
            if (s_linuxGLCtx)
            {
                glXDestroyContext(s_linuxDisplay, s_linuxGLCtx);
                s_linuxGLCtx = nullptr;
            }
            if (s_linuxWindow)
            {
                XDestroyWindow(s_linuxDisplay, s_linuxWindow);
                s_linuxWindow = 0;
            }
            XCloseDisplay(s_linuxDisplay);
            s_linuxDisplay = nullptr;
        }

        s_nativeHandle = nullptr;
        s_activeView = nullptr;
    }

    // Open connection to the X server
    XInitThreads();
    s_linuxDisplay = XOpenDisplay(nullptr);
    if (!s_linuxDisplay)
    {
        fprintf(stderr, "64klang3: XOpenDisplay failed\n");
        return kResultFalse;
    }

    int screen = DefaultScreen(s_linuxDisplay);

    // Choose a visual with double-buffered OpenGL
    static const int visualAttribs[] = {
        GLX_RGBA, GLX_DOUBLEBUFFER,
        GLX_RED_SIZE, 8, GLX_GREEN_SIZE, 8, GLX_BLUE_SIZE, 8,
        GLX_DEPTH_SIZE, 0,
        None
    };
    XVisualInfo* vi = glXChooseVisual(s_linuxDisplay, screen, const_cast<int*>(visualAttribs));
    if (!vi)
    {
        fprintf(stderr, "64klang3: glXChooseVisual failed\n");
        XCloseDisplay(s_linuxDisplay);
        s_linuxDisplay = nullptr;
        return kResultFalse;
    }

    // Create a child X11 window embedded in the host's window
    XSetWindowAttributes swa = {};
    swa.colormap   = XCreateColormap(s_linuxDisplay,
                                     RootWindow(s_linuxDisplay, vi->screen),
                                     vi->visual, AllocNone);
    swa.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask
                   | ButtonPressMask | ButtonReleaseMask | PointerMotionMask
                   | StructureNotifyMask | FocusChangeMask | EnterWindowMask | LeaveWindowMask;

    s_linuxWindow = XCreateWindow(
        s_linuxDisplay,
        (Window)(uintptr_t)parent,   // parent from host
        0, 0,
        (unsigned)s_viewWidth, (unsigned)s_viewHeight,
        0,
        vi->depth, InputOutput, vi->visual,
        CWColormap | CWEventMask, &swa);

    XMapWindow(s_linuxDisplay, s_linuxWindow);
    XFlush(s_linuxDisplay);

    // Create GLX context
    s_linuxGLCtx = glXCreateContext(s_linuxDisplay, vi, nullptr, GL_TRUE);
    XFree(vi);
    if (!s_linuxGLCtx)
    {
        fprintf(stderr, "64klang3: glXCreateContext failed\n");
        XDestroyWindow(s_linuxDisplay, s_linuxWindow);
        XCloseDisplay(s_linuxDisplay);
        s_linuxDisplay = nullptr;
        s_linuxWindow  = 0;
        return kResultFalse;
    }

    // Make current on this thread briefly to initialise ImGui, then release —
    // the render thread will own the context for the rest of the session.
    glXMakeCurrent(s_linuxDisplay, s_linuxWindow, s_linuxGLCtx);

    if (!s_imguiCtx)
    {
        IMGUI_CHECKVERSION();
        s_imguiCtx = ImGui::CreateContext();
    }
    ImGui::SetCurrentContext(s_imguiCtx);
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
    K64GUI::setWindowHandle(parent);

    glXMakeCurrent(s_linuxDisplay, None, nullptr); // release; render thread takes over

    // Start background render thread
    s_nativeHandle = parent;
    s_activeView = this;
    s_renderRunning = true;
    pthread_create(&s_linuxRenderThread, nullptr, renderThreadEntryLinux, this);

#endif

    guiInitialized = true;
    return kResultOk;
}

tresult PLUGIN_API K64PluginView::removed()
{
#ifdef _WIN32
    if (nativeHandle)
    {
        // Only interact with the singleton render state if this view is the one
        // currently driving it.  A non-active view (migrated away) just clears
        // its instance pointers without touching the shared rendering stack.
        if (nativeHandle == s_nativeHandle)
        {
            // Save full HWND rect for session-static restore on next open.
            RECT wr = {};
            ::GetWindowRect((HWND)nativeHandle, &wr);
            s_savedPos  = { wr.left, wr.top };
            s_savedWinW = wr.right  - wr.left;
            s_savedWinH = wr.bottom - wr.top;

            // Signal the render thread to stop and wait for it to exit.
            s_renderRunning = false;
            if (s_winRenderThread)
            {
                WaitForSingleObject((HANDLE)s_winRenderThread, 3000);
                CloseHandle((HANDLE)s_winRenderThread);
                s_winRenderThread = nullptr;
            }

            // The render thread released the WGL context before exiting;
            // re-acquire it here so we can call the ImGui/GL shutdown functions.
            if (s_winDC && s_winGLCtx)
                wglMakeCurrent((HDC)s_winDC, (HGLRC)s_winGLCtx);

            // Restore original wndproc
            if (g_originalWndProc)
            {
                SetWindowLongPtr((HWND)nativeHandle, GWLP_WNDPROC, (LONG_PTR)g_originalWndProc);
                g_originalWndProc = nullptr;
            }

            K64GUI::setWindowHandle(nullptr);
            K64GUI::shutdown();

            // Shut down the backends; the ImGui context itself is kept alive so
            // canvas state (zoom, pan, open panels) survives the window closing.
            // It is destroyed in ~K64PluginView() when the last instance is gone.
            if (s_imguiCtx)
            {
                ImGui::SetCurrentContext(s_imguiCtx);
                ImGui_ImplOpenGL3_Shutdown();
                ImGui_ImplWin32_Shutdown();
            }

            destroyWGLContext();
            s_nativeHandle = nullptr;

            // If another view's editor window is still open, restart the singleton
            // renderer on it.  This handles the case where a second alias editor
            // was the active renderer and has just been removed while the first
            // instance's editor window is still visible.
            for (auto* v : s_allViews)
            {
                if (v != this && v->nativeHandle != nullptr)
                {
                    // Don't forcibly reposition the host-owned HWND.
                    s_savedPos = { -1, -1 };
                    v->attached(v->nativeHandle, kPlatformTypeHWND);
                    break;
                }
            }
        }
    } // if (nativeHandle)

#elif defined(__APPLE__)

    k64_macOS_destroyView();

#else // Linux

    if (nativeHandle && nativeHandle == s_nativeHandle)
    {
        s_renderRunning = false;
        if (s_linuxRenderThread)
        {
            pthread_join(s_linuxRenderThread, nullptr);
            s_linuxRenderThread = 0;
        }

        if (s_linuxDisplay && s_linuxWindow && s_linuxGLCtx)
            glXMakeCurrent(s_linuxDisplay, s_linuxWindow, s_linuxGLCtx);

        K64GUI::setWindowHandle(nullptr);
        K64GUI::shutdown();

        if (s_imguiCtx)
        {
            ImGui::SetCurrentContext(s_imguiCtx);
            ImGui_ImplOpenGL3_Shutdown();
        }

        if (s_linuxDisplay)
        {
            glXMakeCurrent(s_linuxDisplay, None, nullptr);
            if (s_linuxGLCtx)
            {
                glXDestroyContext(s_linuxDisplay, s_linuxGLCtx);
                s_linuxGLCtx = nullptr;
            }

            if (s_linuxWindow)
            {
                XDestroyWindow(s_linuxDisplay, s_linuxWindow);
                s_linuxWindow = 0;
            }

            XCloseDisplay(s_linuxDisplay);
            s_linuxDisplay = nullptr;
        }

        s_nativeHandle = nullptr;
        s_activeView = nullptr;

        // Reattach singleton renderer to another still-open view if available.
        for (auto* v : s_allViews)
        {
            if (v != this && v->nativeHandle != nullptr)
            {
                v->attached(v->nativeHandle, kPlatformTypeX11EmbedWindowID);
                break;
            }
        }
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
    // Only resize the render surface when this view is the active renderer.
    if (s_winGLCtx && nativeHandle && nativeHandle == s_nativeHandle)
    {
        // Use the actual HWND client rect; host may add chrome inside the same HWND.
        RECT hwndRect = {};
        ::GetClientRect((HWND)nativeHandle, &hwndRect);
        int hw = (hwndRect.right  > 0) ? (hwndRect.right  - hwndRect.left) : w;
        int hh = (hwndRect.bottom > 0) ? (hwndRect.bottom - hwndRect.top)  : h;
        std::lock_guard<std::mutex> lock(s_renderMutex);
        s_viewWidth  = hw;
        s_viewHeight = hh;
    }

#elif defined(__APPLE__)

    if (guiInitialized && w > 0 && h > 0)
        k64_macOS_resizeView(w, h);

#else // Linux

    if (guiInitialized && nativeHandle && nativeHandle == s_nativeHandle &&
        s_linuxDisplay && s_linuxWindow && w > 0 && h > 0)
    {
        // Wait for any in-progress renderFrameLinux() to finish before
        // modifying the dimensions the render thread reads each frame.
        std::lock_guard<std::mutex> lock(s_renderMutex);
        s_viewWidth  = w;
        s_viewHeight = h;
        XResizeWindow(s_linuxDisplay, s_linuxWindow, (unsigned)w, (unsigned)h);
        XFlush(s_linuxDisplay);
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
    s_winDC = GetDC((HWND)hwnd);
    if (!s_winDC)
        return false;

    PIXELFORMATDESCRIPTOR pfd = {};
    pfd.nSize      = sizeof(pfd);
    pfd.nVersion   = 1;
    pfd.dwFlags    = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;

    int pf = ChoosePixelFormat((HDC)s_winDC, &pfd);
    if (!pf || !SetPixelFormat((HDC)s_winDC, pf, &pfd))
    {
        ReleaseDC((HWND)hwnd, (HDC)s_winDC);
        s_winDC = nullptr;
        return false;
    }

    s_winGLCtx = wglCreateContext((HDC)s_winDC);
    if (!s_winGLCtx)
    {
        ReleaseDC((HWND)hwnd, (HDC)s_winDC);
        s_winDC = nullptr;
        return false;
    }

    // Make current briefly on this (attach) thread so ImGui init functions work.
    // attached() releases the context before starting the render thread.
    wglMakeCurrent((HDC)s_winDC, (HGLRC)s_winGLCtx);

    // Enable vsync — the render thread uses Sleep(1) as a yield; SwapBuffers
    // with vsync provides natural ~60 fps pacing without busy-spinning.
    wgl_SwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
    if (wgl_SwapIntervalEXT)
        wgl_SwapIntervalEXT(1);

    // Init viewport dimensions from HWND client area
    RECT cr = {};
    if (::GetClientRect((HWND)hwnd, &cr))
    {
        s_viewWidth  = (cr.right  > 0) ? (cr.right  - cr.left) : kDefaultWidth;
        s_viewHeight = (cr.bottom > 0) ? (cr.bottom - cr.top)  : kDefaultHeight;
    }

    return true;
}

void K64PluginView::destroyWGLContext()
{
    if (s_winGLCtx)
    {
        wglMakeCurrent(nullptr, nullptr);
        wglDeleteContext((HGLRC)s_winGLCtx);
        s_winGLCtx = nullptr;
    }
    if (s_winDC && s_nativeHandle)
    {
        ReleaseDC((HWND)s_nativeHandle, (HDC)s_winDC);
        s_winDC = nullptr;
    }
}

// ─── renderFrameWin ──────────────────────────────────────────────────────────
// One rendered frame. Called by runWinRenderLoop() on the render thread.
void K64PluginView::renderFrameWin()
{
    // Skip while minimised — no visible surface.
    if (IsIconic((HWND)s_nativeHandle))
        return;

    // Hold s_renderMutex for the whole frame so onSize() (host thread) can safely
    // update s_viewWidth/s_viewHeight between frames.
    std::lock_guard<std::mutex> lock(s_renderMutex);

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

    SwapBuffers((HDC)s_winDC); // vsync via wglSwapIntervalEXT(1)
}

// ─── runWinRenderLoop ─────────────────────────────────────────────────────────
// Render thread entry: owns the WGL context for its lifetime.
void K64PluginView::runWinRenderLoop()
{
    wglMakeCurrent((HDC)s_winDC, (HGLRC)s_winGLCtx);

    while (s_renderRunning)
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
    if (!s_linuxDisplay || !s_linuxWindow || !s_linuxGLCtx)
        return;

    // Hold the mutex for the entire frame so that onSize() (host thread) and
    // removed() (after pthread_join) cannot race with resource access.
    std::lock_guard<std::mutex> lock(s_renderMutex);

    // Handle X11 events (key/mouse) — forward to ImGui
    while (XPending(s_linuxDisplay))
    {
        XEvent ev;
        XNextEvent(s_linuxDisplay, &ev);
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
    io.DisplaySize = ImVec2((float)s_viewWidth, (float)s_viewHeight);
    io.DeltaTime   = 1.0f / 60.0f;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    K64GUI::render();

    ImGui::Render();

    glClearColor(0.12f, 0.12f, 0.14f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glXSwapBuffers(s_linuxDisplay, s_linuxWindow);
}

void K64PluginView::runLinuxRenderLoop()
{
    glXMakeCurrent(s_linuxDisplay, s_linuxWindow, s_linuxGLCtx);
    while (s_renderRunning)
    {
        renderFrameLinux();
        usleep(16000); // ~60 fps
    }
    glXMakeCurrent(s_linuxDisplay, None, nullptr);
}

static void* renderThreadEntryLinux(void* arg)
{
    static_cast<K64PluginView*>(arg)->runLinuxRenderLoop();
    return nullptr;
}

#endif // Linux

} // namespace Vst
} // namespace Steinberg

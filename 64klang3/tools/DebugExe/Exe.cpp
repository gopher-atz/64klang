// 64klang3 DebugExe — standalone test host
// No C++/CLI. Direct SynthController calls + OpenGL/WGL ImGui window.

#include <windows.h>
#include <GL/gl.h>
#include <chrono>

#include "core/SynthController.h"
#include "gui/ImGuiPlugin.h"

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_opengl3.h"

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

    if (!SynthController::DataAccessMutex.try_lock_for(std::chrono::milliseconds(0)))
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
// WGL/OpenGL resources
// ---------------------------------------------------------------------------

typedef BOOL (WINAPI * PFNWGLSWAPINTERVALEXTPROC)(int);
static HDC   g_hdc    = nullptr;
static HGLRC g_glCtx  = nullptr;
static int   g_winW   = 1280;
static int   g_winH   = 800;

static bool createWGLContext(HWND hwnd)
{
    g_hdc = GetDC(hwnd);
    if (!g_hdc) return false;

    PIXELFORMATDESCRIPTOR pfd = {};
    pfd.nSize      = sizeof(pfd);
    pfd.nVersion   = 1;
    pfd.dwFlags    = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;

    int pf = ChoosePixelFormat(g_hdc, &pfd);
    if (!pf || !SetPixelFormat(g_hdc, pf, &pfd))
    {
        ReleaseDC(hwnd, g_hdc); g_hdc = nullptr;
        return false;
    }

    g_glCtx = wglCreateContext(g_hdc);
    if (!g_glCtx)
    {
        ReleaseDC(hwnd, g_hdc); g_hdc = nullptr;
        return false;
    }

    wglMakeCurrent(g_hdc, g_glCtx);

    // Disable vsync so the 16 ms timer controls pacing
    auto wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
    if (wglSwapIntervalEXT) wglSwapIntervalEXT(0);

    RECT cr = {};
    if (GetClientRect(hwnd, &cr))
    {
        g_winW = (cr.right  > 0) ? cr.right  - cr.left : 1280;
        g_winH = (cr.bottom > 0) ? cr.bottom - cr.top  : 800;
    }
    return true;
}

static void destroyWGLContext(HWND hwnd)
{
    if (g_glCtx) { wglMakeCurrent(nullptr, nullptr); wglDeleteContext(g_glCtx); g_glCtx = nullptr; }
    if (g_hdc)   { ReleaseDC(hwnd, g_hdc); g_hdc = nullptr; }
}

// ---------------------------------------------------------------------------
// Render frame
// ---------------------------------------------------------------------------

static void renderFrame()
{
    if (!g_glCtx || !g_hdc)
        return;

    // Guard against re-entrant calls: file dialogs pump the message loop,
    // which would fire WM_TIMER again before the current frame finishes.
    static bool s_inFrame = false;
    if (s_inFrame)
        return;
    s_inFrame = true;

    wglMakeCurrent(g_hdc, g_glCtx);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    K64GUI::render();

    ImGui::Render();

    glViewport(0, 0, g_winW, g_winH);
    glClearColor(0.12f, 0.12f, 0.14f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    SwapBuffers(g_hdc);
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
        if (g_glCtx && wParam != SIZE_MINIMIZED)
        {
            g_winW = LOWORD(lParam);
            g_winH = HIWORD(lParam);
        }
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

    if (!createWGLContext(hwnd))
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
    ImGui_ImplOpenGL3_Init("#version 130");

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

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    destroyWGLContext(hwnd);
    CoUninitialize();

    return (int)msg.wParam;
}

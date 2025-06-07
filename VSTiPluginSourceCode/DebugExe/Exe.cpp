// Exe.cpp : Defines the entry point for the application.
//

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

#include "resource.h"
#include "../64klang2Core/SynthController.h"
#include "../64klang2Wrapper/64klang2Wrapper.h"
#include "AudioOut.h"
#include "MidiIn.h"

#define MAX_LOADSTRING 100

// MIDI input device name goes here (empty: disable MIDI in)
static const wchar_t* MidiInName = L"A-PRO 1";

// stolen from vstxsynthproc.cpp
static void ApplyEvent(MidiEvent midiData)
{
    unsigned char status = midiData.cmd & 0xf0;		// status
    unsigned char channel = midiData.cmd & 0x0f;		// channel

    // note on/off events
    if (status == 0x90 || status == 0x80)
    {
        unsigned char note = midiData.d1 & 0x7f;
        unsigned char velocity = midiData.d2 & 0x7f;
        // note off
        if (status == 0x80 || ((status == 0x90) && (velocity == 0)))
        {
            SynthController::instance()->noteOff(channel, note, velocity);
        }
        // note on
        else if (status == 0x90)
        {
            SynthController::instance()->noteOn(channel, note, velocity);
        }
    }
    // polyphonic aftertouch
    else if (status == 0xA)
    {
        byte note = midiData.d1 & 0x7f;
        byte pressure = midiData.d2 & 0x7f;
        SynthController::instance()->noteAftertouch(channel, note, pressure);
    }
    /*	// channel aftertouch
        else if (status == 0xD)
        {
            byte pressure = midiData[1] & 0x7f;
            Go4kVSTi_ChannelAftertouch(channel, pressure);
        }
    */	// Controller Change
    else if (status == 0xB0)
    {
        unsigned char number = midiData.d1 & 0x7f;
        unsigned char value = midiData.d2 & 0x7f;
        SynthController::instance()->midiSignal(channel, value, number);
    }
    // Pitch Bend
    else if (status == 0xE0)
    {
        unsigned char lsb = midiData.d1 & 0x7f;
        unsigned char msb = midiData.d2 & 0x7f;
        int value = (((int)(msb)) << 7) + lsb;
        // pitch bend precision is 0 - 16383, center 8192
        // we dont use full precision for the sake of equally sized streams
        SynthController::instance()->midiSignal(channel, (value >> 6), 0); // 0 - 255, center 128
    }
}

// the LO in YOLO stands for "lockless"
static MidiEvent midiQueue[256];
static int mqRead = 0;
static int mqWrite = 0;

// let's have audio out
static void audioCallback(float* buffer, int samples)
{
    const int TEMPSIZE = 16384;
    static float tempL[TEMPSIZE];
    static float tempR[TEMPSIZE];

    auto instance = SynthController::instance();
    if (!instance->isInitialized())
        goto silence;

    // try to acquire the mutex, if gui requests update after 1ms dont process the synth but return 0 values
    DWORD dwWaitResult = WaitForSingleObject(instance->DataAccessMutex, 1);
    if (dwWaitResult == WAIT_TIMEOUT)
        goto silence;

    while (samples > 0)
    {
        while ((mqWrite - mqRead) > 0)
            ApplyEvent(midiQueue[(mqRead++ & 0xff)]);

        int todo = Min(samples, TEMPSIZE);
        instance->tick(tempL, tempR, todo);

        // convert separate stereo to interleaved
        // and while we're at it, clip and fix NaNs, we all know how it goes.
        for (int i = 0; i < samples; i++)
        {
            volatile float l = tempL[i];
            volatile float r = tempR[i];
            *buffer++ = Clamp(l == l ? l : 0, -0.999f, 0.999f);
            *buffer++ = Clamp(r == r ? r : 0, -0.999f, 0.999f);
        }

        samples -= todo;
    }

    // unset data access mutex
    ReleaseMutex(instance->DataAccessMutex);
    return;

silence:
    memset(buffer, 0, 8 * samples);
    return;  
}

// .. and MIDI in 
static void midiCallback(MidiEvent midiData)
{
    midiQueue[mqWrite++ & 0xff] = midiData;
}

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY _tWinMain(HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPTSTR    lpCmdLine,
    int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.
    MSG msg;
    HACCEL hAccelTable;

    // Initialize global strings
    LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadString(hInstance, IDC_EXE, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance(hInstance, nCmdShow))
    {
        return FALSE;
    }

    hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_EXE));

    StartAudio(44100, audioCallback);
    StartMidi(MidiInName, midiCallback);

    // Main message loop:
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        if (GetAsyncKeyState(VK_ESCAPE) != 0)
            break;
    }

    StopMidi();
    StopAudio();

    return (int)msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_EXE));
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCE(IDC_EXE);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
float left[441000];
float right[441000];


BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    // set the appartment model to STA before initializing the .NET wrapper. seems to fix the STA exception
    CoInitialize(NULL);

    hInst = hInstance; // Store instance handle in our global variable

    SynthWrapper::instance()->openWindow();
    SynthController::instance()->setBPM(120);
    return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int wmId, wmEvent;
    PAINTSTRUCT ps;
    HDC hdc;

    switch (message)
    {
    case WM_COMMAND:
        wmId = LOWORD(wParam);
        wmEvent = HIWORD(wParam);
        // Parse the menu selections:
        switch (wmId)
        {
        case IDM_ABOUT:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;
    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        // TODO: Add any drawing code here...
        EndPaint(hWnd, &ps);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

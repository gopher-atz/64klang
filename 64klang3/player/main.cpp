#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <windows.h>
#include <mmsystem.h>
#include <mmreg.h>

// Synth public API (player mode — COMPILE_VSTI not defined)
#include "Synth.h"

// Song data exported from the 64klang3 plugin (File → Export → Song / Patch)
// Place 64k2Patch.h and 64k2Song.h in this directory (or set PLAYER_SONG_DIR in CMake).
#define INCLUDE_NODES
#include "64k2Patch.h"
#include "64k2Song.h"

#define SAMPLE_RATE   44100
#define SAMPLE_TYPE   float

// Output buffer: song length + 60 s safety tail
static SAMPLE_TYPE lpSoundBuffer[MAX_SAMPLES * 2 + SAMPLE_RATE * 60 * 2];
static HWAVEOUT    hWaveOut;

static WAVEFORMATEX WaveFMT =
{
    WAVE_FORMAT_IEEE_FLOAT,
    2,                                      // channels
    SAMPLE_RATE,                            // samples per second
    SAMPLE_RATE * sizeof(SAMPLE_TYPE) * 2,  // bytes per second
    sizeof(SAMPLE_TYPE) * 2,               // block align
    sizeof(SAMPLE_TYPE) * 8,               // bits per sample
    0                                       // no extension
};

static WAVEHDR WaveHDR =
{
    (LPSTR)lpSoundBuffer,
    MAX_SAMPLES * sizeof(SAMPLE_TYPE) * 2,
    0, 0, 0, 0, 0, 0
};

static MMTIME MMTime = { TIME_SAMPLES, 0 };

// Required when linking without the CRT (/NODEFAULTLIB) to suppress the
// "floating-point support not loaded" linker warning.
extern "C" { int _fltused = 1; }

// Debug or 64-bit Release: link CRT normally, use standard main().
// 32-bit Release: no-CRT build (/NODEFAULTLIB /ENTRY:mainCRTStartup), use raw entry point.
#if defined(_DEBUG) || defined(_M_X64)
int main()
#else
void mainCRTStartup()
#endif
{
    // Initialise the synth with the exported patch and song data
    _64klang_Init(SynthStream, SynthNodes,
                  SynthMonoConstantOffset, SynthStereoConstantOffset, SynthMaxOffset);

    // Start rendering on a background thread; give it a 5 s head-start
    CreateThread(0, 0, (LPTHREAD_START_ROUTINE)_64klang_Render, lpSoundBuffer, 0, 0);
    Sleep(5000);

    // Open waveOut device and begin playback
    waveOutOpen(&hWaveOut, WAVE_MAPPER, &WaveFMT, NULL, 0, CALLBACK_NULL);
    waveOutPrepareHeader(hWaveOut, &WaveHDR, sizeof(WaveHDR));
    waveOutWrite(hWaveOut, &WaveHDR, sizeof(WaveHDR));

    // Wait until playback completes or user presses Escape
    do
    {
        waveOutGetPosition(hWaveOut, &MMTime, sizeof(MMTIME));
        Sleep(128);
    } while (MMTime.u.sample < (DWORD)MAX_SAMPLES && !GetAsyncKeyState(VK_ESCAPE));

    ExitProcess(0);
}

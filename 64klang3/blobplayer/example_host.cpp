///////////////////////////////////////////////////////////////////////////////
// BlobHostExample — minimal Win32 waveOut host demonstrating runtime blob loading
//
// Usage:
//   BlobHostExample [patch.blob] [song.blob]
//
// Defaults to "64k2Patch.h.blob" and "64k2Song.h.blob" in the working
// directory — the exact filenames produced by the 64klang3 VST3 plugin's
// File → Export Song / Export Patch commands.
//
// Unlike the compiled-in Player, this executable does not need to be
// recompiled when you export a new song: just drop in the new .blob files
// and run again.
///////////////////////////////////////////////////////////////////////////////

#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <windows.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <stdio.h>
#include <stdlib.h>

#include "64klang_blob_api.h"

#define SAMPLE_RATE 44100

static HWAVEOUT   hWaveOut;
static MMTIME     MMTime = { TIME_SAMPLES, 0 };

static WAVEFORMATEX WaveFMT =
{
    WAVE_FORMAT_IEEE_FLOAT,
    2,                                      // channels
    SAMPLE_RATE,                            // samples per second
    SAMPLE_RATE * sizeof(float) * 2,        // bytes per second
    sizeof(float) * 2,                      // block align
    sizeof(float) * 8,                      // bits per sample
    0                                       // no extension
};

int main(int argc, char* argv[])
{
    const char* patchPath = argc > 1 ? argv[1] : "64k2Patch.h.blob";
    const char* songPath  = argc > 2 ? argv[2] : "64k2Song.h.blob";

    printf("Loading patch: %s\n", patchPath);
    printf("Loading song:  %s\n", songPath);

    if (k64_blob_init_from_files(patchPath, songPath) != 0)
    {
        fprintf(stderr, "Error: failed to load blob files.\n"
                        "Export song and patch from the 64klang3 VST3 plugin,\n"
                        "then place the .blob files next to this executable.\n");
        return 1;
    }

    int   songSamples = k64_blob_song_length();
    float bpm         = k64_blob_bpm();
    printf("BPM: %.2f  Song length: %d samples (%.2fs)\n",
           bpm, songSamples, (double)songSamples / SAMPLE_RATE);

    // Allocate render buffer: interleaved L/R floats
    float* renderBuf = (float*)malloc((size_t)songSamples * 2 * sizeof(float));
    if (!renderBuf)
    {
        fprintf(stderr, "Error: out of memory.\n");
        return 1;
    }

    printf("Rendering...\n");

    // Render on a background thread; give it a 5-second head-start before
    // opening waveOut (matches the pattern used by the compiled-in Player).
    HANDLE hRender = CreateThread(NULL, 0,
        (LPTHREAD_START_ROUTINE)k64_blob_render, renderBuf, 0, NULL);
    Sleep(5000);

    // Open waveOut and begin playback
    WAVEHDR waveHdr;
    memset(&waveHdr, 0, sizeof(waveHdr));
    waveHdr.lpData         = (LPSTR)renderBuf;
    waveHdr.dwBufferLength = (DWORD)((size_t)songSamples * 2 * sizeof(float));

    waveOutOpen(&hWaveOut, WAVE_MAPPER, &WaveFMT, NULL, 0, CALLBACK_NULL);
    waveOutPrepareHeader(hWaveOut, &waveHdr, sizeof(waveHdr));
    waveOutWrite(hWaveOut, &waveHdr, sizeof(waveHdr));

    printf("Playing — press Escape to stop.\n");
    do
    {
        waveOutGetPosition(hWaveOut, &MMTime, sizeof(MMTIME));
        Sleep(128);
    } while (MMTime.u.sample < (DWORD)songSamples && !GetAsyncKeyState(VK_ESCAPE));

    WaitForSingleObject(hRender, INFINITE);
    CloseHandle(hRender);
    free(renderBuf);
    return 0;
}

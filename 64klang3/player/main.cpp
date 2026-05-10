#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <windows.h>
#include <mmsystem.h>
#include <mmreg.h>

#define WAV_EXPORT

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

#ifdef WAV_EXPORT
char ExportWaveHeader[44] =
{
	'R', 'I', 'F', 'F',
	0, 0, 0, 0,				// filled below
	'W', 'A', 'V', 'E',
	'f', 'm', 't', ' ',
	16, 0, 0, 0,
	3, 0,
	2, 0,
	0x44, 0xac, 0, 0,
	0x20, 0x62, 0x05, 0,
	8, 0,
	32, 0,
	'd', 'a', 't', 'a',
	0, 0, 0, 0				// filled below
};
#endif

char infotext[] =
	"64klang3 Synthesizer and Player by Gopher/Alcatraz (2026)\n"
	"Precalc is 5s. Wait for termination to get a exemusic.wav file written.\n\n"
	"Title  : ...\n"
	"Author : ...\n"
	"Length : 13:37\n\n"
	"        |-------------------------------------------------------------------|\n"
    "PlayPos: ";

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
    // write info text to console
	WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), infotext, sizeof(infotext), NULL, NULL);
    
    // Initialise the synth with the exported patch and song data
    _64klang_Init(SynthStream, SynthNodes,
                  SynthMonoConstantOffset, SynthStereoConstantOffset, SynthMaxOffset);
    int SongLength = *(((int*)SynthStream) + 1);

    // Start rendering on a background thread; give it a 5 s head-start
    CreateThread(0, 0, (LPTHREAD_START_ROUTINE)_64klang_Render, lpSoundBuffer, 0, 0);
    Sleep(5000);

    // Open waveOut device and begin playback
    waveOutOpen(&hWaveOut, WAVE_MAPPER, &WaveFMT, NULL, 0, CALLBACK_NULL);
    waveOutPrepareHeader(hWaveOut, &WaveHDR, sizeof(WaveHDR));
    waveOutWrite(hWaveOut, &WaveHDR, sizeof(WaveHDR));
    DWORD lastUpdate = 0;
    // Wait until playback completes or user presses Escape
    do
    {
        waveOutGetPosition(hWaveOut, &MMTime, sizeof(MMTIME));
        // update song progress bar
        if ((MMTime.u.sample - lastUpdate) >= SongLength / 70)
		{
			lastUpdate = MMTime.u.sample;
			WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), "#", 1, NULL, NULL);
		}
        Sleep(128);
    } while (MMTime.u.sample < SongLength && !GetAsyncKeyState(VK_ESCAPE));
    WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), "\n", 1, NULL, NULL);

    // write a wav file in the end
#ifdef WAV_EXPORT
#ifdef WAIT_FOR_IT
	do
	{
		Sleep(128);
	} while (!_64klang_RenderDone());
#endif
	HANDLE hFile = CreateFileA("exemusic.wav", // name of the write
                       GENERIC_WRITE,          // open for writing
                       0,                      // do not share
                       NULL,                   // default security
                       CREATE_ALWAYS,          // create new file only
                       FILE_ATTRIBUTE_NORMAL,  // normal file
                       NULL);                  // no attr. template
	
	// init wave header	
	*((DWORD*)(&ExportWaveHeader[4])) = SongLength*2*sizeof(SAMPLE_TYPE)+36;	// size of the rest of the file in bytes
	*((DWORD*)(&ExportWaveHeader[40])) = SongLength*2*sizeof(SAMPLE_TYPE);		// size of raw sample data to come
	// write wave header
	WriteFile(	hFile,			// open file handle
				ExportWaveHeader,		// start of data to write
				44,				// number of bytes to write
				&lastUpdate,			// number of bytes that were written
				NULL);			// no overlapped structure
	WriteFile(	hFile,			// open file handle
				lpSoundBuffer,		// start of data to write
				SongLength*2*sizeof(SAMPLE_TYPE),				// number of bytes to write
				&lastUpdate,			// number of bytes that were written
				NULL);			// no overlapped structure

	CloseHandle(hFile);
#endif

    ExitProcess(0);
}

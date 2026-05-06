#pragma once

#include <windows.h>
#include <mmsystem.h>
#pragma comment (lib, "winmm.lib")

typedef struct MidiEvent { byte cmd, d1, d2; };
typedef void (*MidiCallback)(MidiEvent midiData);

namespace MidiIn 
{
    static int device;
    static MidiCallback cb;
    static HMIDIIN handle{};
    
    static void CALLBACK MidiInProc(HMIDIIN hMidiIn, UINT wMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
    {
        if (wMsg == MIM_DATA)
        {
            MidiEvent md{};
            md.cmd = dwParam1 & 0xFF;
            md.d1 = (dwParam1 >> 8) & 0xFF;
            md.d2 = (dwParam1 >> 16) & 0xFF;
            cb(md);
        }
    }
}

static void StartMidi(const wchar_t *devName, MidiCallback callback)
{
    if (!devName || !devName[0])
        return;

    MidiIn::cb = callback;
    UINT numDevices = midiInGetNumDevs();
    if (!numDevices)
    {
        MessageBox(0, L"No MIDI input devices found", 0, MB_ICONERROR);
        return;
    }

    MidiIn::device = -1;
    for (int i = 0; i < numDevices; i++)
    {
        MIDIINCAPS caps{};
        midiInGetDevCaps(i, &caps, sizeof(caps));
        if (wcsstr(caps.szPname, devName))
        {
            MidiIn::device = i;
            break;
        }
    }
    if (MidiIn::device < 0)
    {
        wchar_t msg[256];
        wsprintf(msg, L"MIDI input '%s' not found, using default", devName);
        MessageBox(0, msg, 0, MB_ICONWARNING);
        MidiIn::device = 0;
    }

    CHECK(midiInOpen(&MidiIn::handle, MidiIn::device, (DWORD_PTR)MidiIn::MidiInProc, 0, CALLBACK_FUNCTION));
    CHECK(midiInStart(MidiIn::handle));
}

static void StopMidi()
{
    midiInStop(MidiIn::handle);
    midiInClose(MidiIn::handle);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 64klang core interface functions for playback, plugin and authoring
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef __SYNTH_H__
#define __SYNTH_H__

#include <windows.h>

#ifdef USE_BLOBS
void _64klang_Init			(BYTE* songStream, void* patchData);
#else
void _64klang_Init			(BYTE* songStream, void* patchData, DWORD const1Offset, DWORD const2Offset, DWORD maxoffset);
#endif
int  _64klang_ACMConvert	(void* srcFormat, void* dstFormat, LPBYTE srcBuffer, DWORD srcBufferSize, LPBYTE& dstBuffer, DWORD& dstBufferSize);
#ifdef COMPILE_VSTI
void _64klang_NoteOn		(DWORD channel, DWORD note, DWORD velocity);
void _64klang_NoteOff		(DWORD channel, DWORD note, DWORD velocity);
void _64klang_NoteAftertouch(DWORD channel, DWORD note, DWORD value);
void _64klang_MidiSignal	(DWORD channel, int value, DWORD cc);
void _64klang_SetBPM		(float bpm);
void _64klang_Tick			(float* left, float* right, DWORD samples);
#else
void _64klang_Render(float* dstbuffer);
bool _64klang_RenderDone();
#ifdef AUTHORING
int _64klang_CurrentBufferSample();
#endif
#endif

#endif
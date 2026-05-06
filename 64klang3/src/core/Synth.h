///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 64klang core interface functions for playback, plugin and authoring
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef __SYNTH_H__
#define __SYNTH_H__

#include "platform.h"

#ifdef USE_BLOBS
void _64klang_Init			(uint8_t* songStream, void* patchData);
#else
void _64klang_Init			(uint8_t* songStream, void* patchData, uint32_t const1Offset, uint32_t const2Offset, uint32_t maxoffset);
#endif
int  _64klang_ACMConvert	(void* srcFormat, void* dstFormat, uint8_t* srcBuffer, uint32_t srcBufferSize, uint8_t*& dstBuffer, uint32_t& dstBufferSize);
#ifdef COMPILE_VSTI
void _64klang_NoteOn		(uint32_t channel, uint32_t note, uint32_t velocity);
void _64klang_NoteOff		(uint32_t channel, uint32_t note, uint32_t velocity);
void _64klang_NoteAftertouch(uint32_t channel, uint32_t note, uint32_t value);
void _64klang_MidiSignal	(uint32_t channel, int value, uint32_t cc);
void _64klang_SetBPM		(float bpm);
void _64klang_Tick			(float* left, float* right, uint32_t samples);
#else
void _64klang_Render(float* dstbuffer);
bool _64klang_RenderDone();
#ifdef AUTHORING
int _64klang_CurrentBufferSample();
#endif
#endif

#endif

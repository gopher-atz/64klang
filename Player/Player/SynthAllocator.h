///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// custom memory allocation for 64klang's SSE based data. takes care of memory alignment
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _SYNTH_ALLOCATOR_H_
#define _SYNTH_ALLOCATOR_H_

#include "windows.h"

void* __fastcall SynthMalloc(int size);
void __fastcall SynthFree(void* ptr);
void __fastcall SynthDeferredFree();
void __fastcall SynthMemSet(void* ptr, int size, int value);
void __fastcall SynthMemCopy(void* dst, void* src, int s);

#endif
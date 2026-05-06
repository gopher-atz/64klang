///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// custom memory allocation for 64klang's SSE based data. takes care of memory alignment
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _SYNTH_ALLOCATOR_H_
#define _SYNTH_ALLOCATOR_H_

#include "platform.h"

void* SYNTHCALL SynthMalloc(int size);
void SYNTHCALL SynthFree(void* ptr);
void SYNTHCALL SynthDeferredFree();
void SYNTHCALL SynthMemSet(void* ptr, int size, int value);
void SYNTHCALL SynthMemCopy(void* dst, void* src, int s);

#endif

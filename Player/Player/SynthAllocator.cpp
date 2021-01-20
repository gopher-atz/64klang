#include "SynthAllocator.h"
#ifdef COMPILE_VSTI
	#include "SynthController.h"	
#endif 

#ifndef COMPILE_VSTI
void* SynthFreeList[65536];
int SynthFreeCounter = 0;
#endif

void* __fastcall SynthMalloc(int size)
{
	// always allocate 16 bytes more than requested
	int* ptr = (int*)GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT, size + 16);
	// step forward 16 bytes
	ptr += 4;
	// not 16byte aligned? go back 8 bytes for the pointer to align it and mark the int before that with a 1 to indicate initial 8 byte alignment
	if ((uintptr_t)ptr & 0x08)
	{		
		ptr -= 2;
		*(ptr - 1) = (uintptr_t)ptr;
	}
	return ptr;
}

void __fastcall SynthFree(void* ptr)
{
	return;
	int* fptr = (int*)ptr;
	// check if initial malloc returned an 8 byte aligned address by checking for a 1 the int before (see above)
	if (*(fptr - 1) == (uintptr_t)fptr)
	{
		fptr -= 2;
	}
	// initial malloc was 16 bytes aligned, so just remove the 16bytes forward step
	else
	{
		fptr -= 4;
	}

#ifndef COMPILE_VSTI
	SynthFreeList[SynthFreeCounter++] = fptr;
#else
	SynthController::instance()->AddDeferredFreeNode(fptr);
#endif
}

void __fastcall SynthDeferredFree()
{
#ifndef COMPILE_VSTI
	while (SynthFreeCounter)
		GlobalFree(SynthFreeList[--SynthFreeCounter]);
#endif
}

#pragma function(memset)
void* __cdecl memset(void* pTarget, int value, size_t cbTarget) {
	char* p = reinterpret_cast<char*>(pTarget);
	while (cbTarget-- > 0) {
		*p++ = static_cast<char>(value);
	}
	return pTarget;
}


void __fastcall SynthMemSet(void* mptr, int s, int v)
{
#ifdef _M_X64
	memset(mptr, v, s);
#else
	__asm
	{
		mov eax, v
		mov	ecx, s
		mov edi, dword ptr [mptr]		
		rep stosb
	}
#endif
}

void __fastcall SynthMemCopy(void* dst, void* src, int s)
{
#ifdef _M_X64
	char* psrc = (char*)src, * pdst = (char*)dst;
	for (int i = 0; i < s; ++i)
		pdst[i] = psrc[i];
#else
	__asm
	{
		mov	ecx, s
		mov esi, dword ptr [src]		
		mov edi, dword ptr [dst]		
		rep movsb
	}
#endif
}
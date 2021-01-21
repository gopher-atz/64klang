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
	// not 16byte aligned?
	if ((uintptr_t)ptr & 0x08)
	{
		// go back 8 bytes for the pointer to be 16 byte aligned
		ptr -= 2;
		// to know we had originally unaligned mem, store the pointer as marker in the 8 bytes before (start of original memory block)
		*((uintptr_t*)(ptr-2)) = (uintptr_t)ptr;
	}
	return ptr;
}

void __fastcall SynthFree(void* ptr)
{
	int* fptr = (int*)ptr;
	// check if initial malloc returned an 8 byte aligned address by checking for pointer to self at the beginning of the (see above)
	if (*((uintptr_t*)(fptr-2)) == (uintptr_t)fptr)
	{
		fptr -= 2;
	}
	// initial malloc was 16 bytes aligned
	else
	{
		// step back 16 bytes
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

void __fastcall SynthMemSet(void* mptr, int s, int v)
{
	char* pdst = (char*)mptr;
	while(s--)
		pdst[s] = v;
}

void __fastcall SynthMemCopy(void* dst, void* src, int s)
{
	char* psrc = (char*)src, * pdst = (char*)dst;
	while (s--)
		pdst[s] = psrc[s];
}
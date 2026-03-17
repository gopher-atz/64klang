///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// platform.h — Portability macros for 64klang3
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _64KLANG_PLATFORM_H_
#define _64KLANG_PLATFORM_H_

#include <cstdint>
#include <cstddef>
#include <cstring>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Calling convention macro
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if defined(_MSC_VER) && defined(_M_IX86)
  #define SYNTHCALL __fastcall
#elif defined(__GNUC__) && defined(__i386__)
  #define SYNTHCALL __attribute__((fastcall))
#else
  #define SYNTHCALL  // no-op on x64 and ARM
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Alignment macro (replaces _MM_ALIGN16)
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if defined(_MSC_VER)
  #define K64_ALIGN16 __declspec(align(16))
#elif defined(__GNUC__) || defined(__clang__)
  #define K64_ALIGN16 __attribute__((aligned(16)))
#else
  #define K64_ALIGN16 alignas(16)
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Code section macro (replaces #pragma code_seg)
// On MSVC this places code in named sections for Crinkler/kkrunchy packing.
// On other compilers it's a no-op.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if defined(_MSC_VER)
  #define K64_CODE_SECTION(name) __pragma(code_seg(name))
#else
  #define K64_CODE_SECTION(name)
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DLL export/import macro (replaces MY64KLANG2CORE_API)
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if defined(_MSC_VER)
  #ifdef MY64KLANG2CORE_EXPORTS
    #define K64_API __declspec(dllexport)
  #else
    #define K64_API
  #endif
#elif defined(__GNUC__) || defined(__clang__)
  #define K64_API __attribute__((visibility("default")))
#else
  #define K64_API
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Win32 type fallbacks
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef _WIN32
  // On Windows, include the real headers so all Win32 types are available.
  // This avoids conflicts when later headers (sapi.h, mmreg.h, etc.) pull
  // in windows.h and try to redefine these types.
  // Note: do NOT define WIN32_LEAN_AND_MEAN — the codebase uses multimedia
  // APIs (ACM, SAPI, mmreg) that are excluded by LEAN_AND_MEAN.
  #ifndef NOMINMAX
    #define NOMINMAX
  #endif
  #include <windows.h>
#else
  // Non-Windows: provide minimal type aliases matching Win32 signatures
  typedef uint32_t DWORD;
  typedef uint16_t WORD;
  typedef uint8_t  BYTE;
  typedef BYTE*    LPBYTE;
  typedef int      BOOL;

  #ifndef TRUE
    #define TRUE 1
  #endif
  #ifndef FALSE
    #define FALSE 0
  #endif
  #ifndef WAVE_FORMAT_PCM
    #define WAVE_FORMAT_PCM 1
  #endif
#endif

#ifndef _WIN32
  #ifndef MAKEFOURCC
    #define MAKEFOURCC(ch0, ch1, ch2, ch3) \
      ((uint32_t)(uint8_t)(ch0)        | ((uint32_t)(uint8_t)(ch1) << 8) | \
       ((uint32_t)(uint8_t)(ch2) << 16) | ((uint32_t)(uint8_t)(ch3) << 24))
  #endif
  #ifndef WAVE_FORMAT_PCM
    #define WAVE_FORMAT_PCM 0x0001
  #endif
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SIMD: SSE (x86/x64) or NEON (ARM64)
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if defined(__aarch64__) || (defined(__ARM_NEON) && defined(__ARM_NEON_FP))
  #define K64_USE_NEON 1
#endif

#if defined(K64_USE_NEON)
  #include <arm_neon.h>
#else
  #if defined(_MSC_VER)
    #include <intrin.h>
  #else
    #include <immintrin.h>
  #endif
  #include <smmintrin.h>
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CPUID wrapper (x86 only; no-op on ARM)
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static inline void k64_cpuid(int info[4], int function_id)
{
#if defined(K64_USE_NEON)
  (void)function_id;
  info[0] = info[1] = info[2] = info[3] = 0;
#elif defined(_MSC_VER)
  __cpuid(info, function_id);
#elif defined(__GNUC__) || defined(__clang__)
  __asm__ __volatile__(
    "cpuid"
    : "=a"(info[0]), "=b"(info[1]), "=c"(info[2]), "=d"(info[3])
    : "a"(function_id)
  );
#endif
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Aligned allocation (replaces GlobalAlloc for aligned memory)
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <cstdlib>

static inline void* k64_aligned_alloc(size_t size, size_t alignment)
{
#if defined(K64_USE_NEON)
  void* ptr = nullptr;
  if (posix_memalign(&ptr, alignment, size) == 0) {
    if (ptr) memset(ptr, 0, size);
    return ptr;
  }
  return nullptr;
#else
  void* ptr = _mm_malloc(size, alignment);
  if (ptr) memset(ptr, 0, size);
  return ptr;
#endif
}

static inline void k64_aligned_free(void* ptr)
{
#if defined(K64_USE_NEON)
  free(ptr);
#else
  _mm_free(ptr);
#endif
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SSE control register wrappers
// _mm_getcsr / _mm_setcsr are only valid on x86/x64 targets.
// On ARM (AArch64, ARM32) and other non-x86 architectures they don't exist.
// The denormals-are-zero and flush-to-zero behaviour is still desirable on
// AArch64 via FPCR, but for now we leave the FP mode unchanged on non-x86.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if defined(__i386__) || defined(_M_IX86) || defined(__x86_64__) || defined(_M_X64)
  static inline unsigned int k64_getcsr()              { return _mm_getcsr(); }
  static inline void         k64_setcsr(unsigned int v){ _mm_setcsr(v); }
  #define K64_HAS_MXCSR 1
#else
  static inline unsigned int k64_getcsr()              { return 0u; }
  static inline void         k64_setcsr(unsigned int)  {}
  #define K64_HAS_MXCSR 0
#endif

#endif // _64KLANG_PLATFORM_H_

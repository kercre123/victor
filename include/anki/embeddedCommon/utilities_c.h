#ifndef _ANKICORETECHEMBEDDED_COMMON_UTILITIES_C_H_
#define _ANKICORETECHEMBEDDED_COMMON_UTILITIES_C_H_

#include "anki/embeddedCommon/config.h"

#ifndef MAX
#define MAX( a, b ) ( ((a) > (b)) ? (a) : (b) )
#endif

#ifndef MIN
#define MIN( a, b ) ( ((a) < (b)) ? (a) : (b) )
#endif

#ifndef CLIP
#define CLIP(n, min, max) ( MIN(MAX(min, n), max ) )
#endif

#ifndef ABS
#define ABS(a) ((a) < 0 ? -(a) : (a))
#endif

#ifndef SIGN
#define SIGN(a) ((a) >= 0)
#endif

#define SWAP(type, a, b) { type t = a; a = b; b = t; }

// ct_assert is a compile time assertion, useful for checking sizeof() and other compile time knowledge
#define ASSERT_CONCAT_(a, b) a##b
#define ASSERT_CONCAT(a, b) ASSERT_CONCAT_(a, b)
#define ct_assert(e) enum { ASSERT_CONCAT(assert_line_, __LINE__) = 1/(!!(e)) }

#ifndef ALIGNVARIABLE
#if defined(_MSC_VER)
#define ALIGNVARIABLE __declspec(align(16))
#elif defined(__APPLE_CC__)
#define ALIGNVARIABLE __attribute__ ((aligned (16)))
#elif defined(__GNUC__)
#define ALIGNVARIABLE __attribute__ ((aligned (16)))
#endif
#endif // #ifndef ALIGNVARIABLE

#ifdef __cplusplus
extern "C" {
#endif

  void explicitPrintf(int reverseWords, const char *format, ...);

#if defined(USING_MOVIDIUS_GCC_COMPILER)

#define memset explicitMemset
#define powf powF32S32
#define pow powF64S32

  void* explicitMemset(void * dst, int value, size_t size);
  f32 powF32S32(const f32 x, const s32 y);
  f64 powF64S32(const f64 x, const s32 y);
#endif

#ifdef __cplusplus
}
#endif

#endif // _ANKICORETECHEMBEDDED_COMMON_UTILITIES_C_H_
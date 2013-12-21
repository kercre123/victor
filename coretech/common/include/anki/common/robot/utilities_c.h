/**
File: utilities_c.h
Author: Various
Created: 201X - 2013

Various simple macros and utilities. Everything in this file should be usable with C99, and not require any C++.

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _ANKICORETECHEMBEDDED_COMMON_UTILITIES_C_H_
#define _ANKICORETECHEMBEDDED_COMMON_UTILITIES_C_H_

#include "anki/common/robot/config.h"

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

  f32 Roundf(const f32 number);
  f64 Round(const f64 number);

  // Returns 1 if it is a power of two, zero otherwise
  s32 IsPowerOfTwo(u32 x);

  u32 Log2u32(u32 x);
  u64 Log2u64(u64 x);

  // Returns 1 if it is odd, zero otherwise
  s32 IsOdd(const s32 x);

  // Get the current system time. Currently only implemented for MSVC and generic linux
  double GetTime();

  void explicitPrintf(int (*writeChar)(int), int reverseEachFourCharacters, const char *format, ...);
  // void explicitPrintfWithExplicitBuffer(int reverseEachFourCharacters, int * buffer, const char *format, ...);

  void PrintInt(int (*writeChar)(int), s32 value); // Print a single number
  void PrintHex(int (*writeChar)(int), u32 value); // Print a single unsigned int in hex format 0xabcd0123
  void PrintFloat(int (*writeChar)(int), f64 value); // Print a single float
  void PrintFloatWithExponent(int (*writeChar)(int), f64 value); // Print a single float in format "significand * 2^exponent"

#if defined(USING_MOVIDIUS_GCC_COMPILER)

  //#define memset explicitMemset
#define powf powF32S32
#define pow powF64S32

  //void* explicitMemset(void * dst, int value, size_t size);
  f32 powF32S32(const f32 x, const s32 y);
  f64 powF64S32(const f64 x, const s32 y);
#endif

#ifdef __cplusplus
}
#endif

#endif // _ANKICORETECHEMBEDDED_COMMON_UTILITIES_C_H_

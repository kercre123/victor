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

#include "coretech/common/shared/types.h"
#include "coretech/common/robot/config.h"

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
#elif defined(__EDG__)  // MDK-ARM
#define ALIGNVARIABLE __attribute__ ((aligned (16)))
#endif
#endif // #ifndef ALIGNVARIABLE

#ifdef __cplusplus
extern "C" {
#endif

#define SwapEndianU32(value) \
  ((((u32)((value) & 0x000000FF)) << 24) | \
  ( ((u32)((value) & 0x0000FF00)) <<  8) | \
  ( ((u32)((value) & 0x00FF0000)) >>  8) | \
  ( ((u32)((value) & 0xFF000000)) >> 24))

  // Returns 1 if it is a power of two, zero otherwise
  s32 IsPowerOfTwo(u32 x);

  u32 Log2u32(u32 x);
  u64 Log2u64(u64 x);

  // Returns 1 if it is odd, zero otherwise
  s32 IsOdd(const s32 x);

  void explicitPrintf(int (*writeChar)(int), int reverseEachFourCharacters, const char *format, ...);

  // Print an int like 1234567 as 1,234,567
  // Returns the number of characters (not counting final '\0')
  s32 SnprintfCommasS32(char *buffer, const s32 bufferLength, const s32 value);

  void PrintS32(int (*writeChar)(int), s32 value); // Print a single number
  void PrintU32Hex(int (*writeChar)(int), u32 value); // Print a single unsigned int in hex format 0xabcd0123
  void PrintF64(int (*writeChar)(int), f64 value); // Print a single float
  void PrintF64WithExponent(int (*writeChar)(int), f64 value); // Print a single float in format "significand * 2^exponent"

  // Data is the data to compute the CRC code of
  // dataLength is the number of bytes of data
  // Use initialCRC=0xFFFFFFFF for the first pass
  u32 ComputeCRC32(const void * data, const s32 dataLength, const u32 initialCRC);

#ifdef __cplusplus
}
#endif

#endif // _ANKICORETECHEMBEDDED_COMMON_UTILITIES_C_H_

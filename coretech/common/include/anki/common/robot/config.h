/**
File: config.h
Author: Peter Barnum
Created: 2013

Basic configuation file for all common files, as well as all other coretech libraries.
Everything in this file should be compatible with plain C, as well as C++

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _ANKICORETECHEMBEDDED_COMMON_CONFIG_H_
#define _ANKICORETECHEMBEDDED_COMMON_CONFIG_H_

#define ANKICORETECHEMBEDDED_VERSION_MAJOR 0
#define ANKICORETECHEMBEDDED_VERSION_MINOR 1
#define ANKICORETECHEMBEDDED_VERSION_REVISION 0

#if defined(__MOVICOMPILE__)
//#warning Using MoviCompile
#define USING_MOVIDIUS_SHAVE_COMPILER
#define USING_MOVIDIUS_COMPILER

#elif defined(ROBOT_HARDWARE)
//#warning Using GNUC 4.4.2
#define USING_MOVIDIUS_GCC_COMPILER
#define USING_MOVIDIUS_COMPILER
#endif

#ifndef NULL
#define NULL (0)
#endif

// To support 128-bit SIMD loads and stores
#define MEMORY_ALIGNMENT_RAW 16 // Sometimes the preprocesor can't handle the safer version MEMORY_ALIGNMENT
#define MEMORY_ALIGNMENT ( (size_t)(MEMORY_ALIGNMENT_RAW) )

// To make processing faster, some kernels require image widths that are a multiple of ANKI_VISION_IMAGE_WIDTH_MULTIPLE
#define ANKI_VISION_IMAGE_WIDTH_SHIFT 4
#define ANKI_VISION_IMAGE_WIDTH_MULTIPLE (1<<ANKI_VISION_IMAGE_WIDTH_SHIFT)

// Which errors will be checked and reported?
#define ANKI_DEBUG_MINIMAL 0 // Only check and output issue with explicit unit tests
#define ANKI_DEBUG_ERRORS 10 // Check and output AnkiErrors and explicit unit tests
#define ANKI_DEBUG_ERRORS_AND_WARNS 20 // Check and output AnkiErrors, AnkiWarns, and explicit unit tests
#define ANKI_DEBUG_ERRORS_AND_WARNS_AND_ASSERTS 30 // Check and output AnkiErrors, AnkiWarns, AnkiAsserts, and explicit unit tests
#define ANKI_DEBUG_ALL 40 // Check and output AnkiErrors, AnkiWarns, and explicit unit tests, plus run any additional extensive tests

// How will errors be reported?
#define ANKI_OUTPUT_DEBUG_NONE 0
#define ANKI_OUTPUT_DEBUG_PRINTF 10

#define ANKI_OUTPUT_DEBUG_LEVEL ANKI_OUTPUT_DEBUG_PRINTF

// Various defines that make different compilers work on the same code
#if defined(_MSC_VER) // We're using the MSVC compiler
#pragma warning(disable: 4068) // Unknown pragma
#pragma warning(disable: 4127) // Conditional expression is constant
//#pragma warning(2: 4100) // Unused variable
#pragma warning(disable: 4800) // 'BOOL' : forcing value to bool 'true' or 'false' (performance warning)

#ifndef __PRETTY_FUNCTION__
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

#ifndef restrict
#define restrict
#endif

#ifndef snprintf
#define snprintf sprintf_s
#endif

#ifndef vsprintf_s
#define vsnprintf vsprintf_s
#endif

#ifndef staticInline
#define staticInline static __inline
#endif

#ifdef _DEBUG
#define ANKI_DEBUG_LEVEL ANKI_DEBUG_ERRORS_AND_WARNS_AND_ASSERTS
#else
#define ANKI_DEBUG_LEVEL ANKI_DEBUG_ERRORS
#endif

// Hack
#undef __EDG__

#endif // #if defined(_MSC_VER)

#if defined(__APPLE_CC__) // Apple Xcode

#ifndef restrict
#define restrict
#endif

#ifndef _strcmpi
#define _strcmpi strcasecmp
#endif

#ifndef staticInline
#define staticInline static inline
#endif

#define ANKI_DEBUG_LEVEL ANKI_DEBUG_ERRORS_AND_WARNS_AND_ASSERTS

#endif // #if defined(__APPLE_CC__)

#if defined(__EDG__)  // MDK-ARM

#ifndef restrict
#define restrict __restrict
#endif

#ifndef staticInline
#define staticInline static __inline
#endif

#define ANKI_DEBUG_LEVEL ANKI_DEBUG_ERRORS

#endif

#if defined(USING_MOVIDIUS_COMPILER) // If using any movidius compiler
//#warning USING MOVIDIUS COMPILER!!
#undef ANKICORETECH_EMBEDDED_USE_MATLAB
#undef ANKICORETECH_EMBEDDED_USE_OPENCV
#undef ANKICORETECH_EMBEDDED_USE_GTEST

// Make it easy to detect usages of iostream
#define iostream IOSTREAM_DOESNT_WORK

//#ifdef EMULATE_SHAVE_ON_LEON
//#warning Emulating the Shave using natural c code on the Leon
//#endif

#ifndef restrict
#define restrict __restrict
#endif

#ifndef staticInline
#define staticInline static inline
#endif

#define ANKI_DEBUG_LEVEL ANKI_DEBUG_ERRORS

#endif // #if defined(USING_MOVIDIUS_COMPILER)

#include "anki/common/types.h"

//#ifdef USING_MOVIDIUS_GCC_COMPILER // If using the movidius gcc compiler for Leon
#ifdef USING_MOVIDIUS_COMPILER // If using the movidius compiler for Leon or Shave

#ifdef __cplusplus
extern "C" {
#endif

#include "mv_types.h"

#define CHECK_ANKI_ASSERTS 0

#ifdef USING_MOVIDIUS_GCC_COMPILER // If using the movidius gcc compiler for Leon

#define EXPLICIT_PRINTF_FLIP_CHARACTERS 0

#include "DrvUart.h"
#include "swcTestUtils.h"
#include "swcLeonUtils.h"
#include "DrvL2Cache.h"
#include <registersMyriad.h>
#include <DrvSvu.h>
#include <swcShaveLoader.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <float.h>
#include <stdarg.h>

  //#undef printf
  //#define printf(...) explicitPrintf(0, EXPLICIT_PRINTF_FLIP_CHARACTERS, __VA_ARGS__)

  //#define printf(...) (_xprintf(SYSTEM_PUTCHAR_FUNCTION , 0, __VA_ARGS__ ) )
#define xprintf(...) (_xprintf(SYSTEM_PUTCHAR_FUNCTION , 0, __VA_ARGS__ ) )

#else // If using the movidius moviCompile for Shave

#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <float.h>
#include <stdarg.h>
#include <moviVectorUtils.h>

#endif // #ifdef USING_MOVIDIUS_GCC_COMPILER ... #else

#ifdef __cplusplus
}
#endif

#define f32 float
#define f64 double

#else // If not using the movidius gcc compiler for Leon

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <float.h>
#include <stdarg.h>

#ifdef __cplusplus
}
#endif

// If we're not building mex (which will replace printf w/ mexPrintf),
// then we want to swap printf for explicitPrintf
#ifndef ANKI_MEX_BUILD
#define EXPLICIT_PRINTF_FLIP_CHARACTERS 0
//#undef printf
//#define printf(...) explicitPrintf(0, EXPLICIT_PRINTF_FLIP_CHARACTERS, __VA_ARGS__)
#endif

#ifdef _DEBUG
#define CHECK_ANKI_ASSERTS 1
#else
#define CHECK_ANKI_ASSERTS 0
#endif

#include "anki/common/types.h"

#endif // #ifdef USING_MOVIDIUS_GCC_COMPILER

#include "anki/common/constantsAndMacros.h"

#endif // _ANKICORETECHEMBEDDED_COMMON_CONFIG_H_

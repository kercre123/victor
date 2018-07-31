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

// 
// Compiler-specific configuration, with various defines that make different compilers work on the same code
//

// Section directives to allocate memory in specific places
#ifdef STM32F429_439xx  // Early version with external memory
#define ONCHIP __attribute__((section("ONCHIP")))
#else
#define OFFCHIP
#define ONCHIP
#define CCM
#endif

#if defined(_MSC_VER) // We're using the MSVC compiler
#pragma warning(disable: 4068) // Disable warning for unknown pragma
#pragma warning(disable: 4127) // Disable warning for conditional expression is constant
//#pragma warning(2: 4100) // Enable warning for unused variable warning
#pragma warning(disable: 4800) // Disable warning for 'BOOL' : forcing value to bool 'true' or 'false' (performance warning)

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

#ifndef NO_INLINE
#define NO_INLINE
#endif

#ifndef USE_M4_HOST_INTRINSICS
#define USE_M4_HOST_INTRINSICS 1
#endif

// Warning: High levels of compiler optimization could cause this test to not work
#define isnan(a) ((a) != (a))

// Hack, because __EDG__ is used to detect the ARM compiler
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

#ifndef NO_INLINE
#define NO_INLINE
#endif

#ifndef USE_M4_HOST_INTRINSICS
#define USE_M4_HOST_INTRINSICS 1
#endif

#endif // #if defined(__APPLE_CC__)

#if defined(__GNUC__) // GCC

#ifndef restrict
#define restrict __restrict
#endif

#ifndef staticInline
#define staticInline static __inline
#endif

#ifndef NO_INLINE
#define NO_INLINE __attribute__ ((noinline))
#endif

#ifndef USE_M4_HOST_INTRINSICS
#define USE_M4_HOST_INTRINSICS 1
#endif

#endif // #if defined(__GNUC__) // GCC

// WARNING: Visual Studio also defines __EDG__
#if defined(__EDG__)  // MDK-ARM

#ifndef restrict
#define restrict __restrict
#endif

#ifndef staticInline
#define staticInline static __inline
#endif

#ifndef NO_INLINE
#define NO_INLINE __declspec(noinline)
#endif

#ifndef USE_M4_HOST_INTRINSICS
#define USE_M4_HOST_INTRINSICS 0
#endif

// Allow anonymous unions in Keil
#pragma anon_unions

//#define ARM_MATH_CM4
//#define ARM_MATH_ROUNDING
//#define __FPU_PRESENT 1

//#include "ARMCM4.h"
//#include "arm_math.h"

#endif // #if defined(__EDG__)  // MDK-ARM

//
// Non-compiler-specific configuration
//

// TODO: For the love of all that is holy, use our normal error/logging macros! (VIC-4941)
#if !defined(ANKI_DEBUG_LEVEL)
#ifdef NDEBUG
#define ANKI_DEBUG_LEVEL ANKI_DEBUG_ERRORS
#else
#define ANKI_DEBUG_LEVEL ANKI_DEBUG_ERRORS_AND_WARNS_AND_ASSERTS
#endif
#endif // ANKI_DEBUG_LEVEL

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

#include "coretech/common/shared/types.h"

#ifdef __cplusplus
}
#endif

// To support 128-bit SIMD loads and stores. This should always be 16.
#define MEMORY_ALIGNMENT_RAW 16 // Sometimes the preprocesor can't handle the safer version MEMORY_ALIGNMENT
#define MEMORY_ALIGNMENT ( (size_t)(MEMORY_ALIGNMENT_RAW) )

// To make processing faster, some kernels require image widths that are a multiple of ANKI_VISION_IMAGE_WIDTH_MULTIPLE
#define ANKI_VISION_IMAGE_WIDTH_SHIFT 4
#define ANKI_VISION_IMAGE_WIDTH_MULTIPLE (1<<ANKI_VISION_IMAGE_WIDTH_SHIFT)

// Which errors will be checked and reported?
#define ANKI_DEBUG_MINIMAL 0 // Only check and output issues with explicit unit tests
#define ANKI_DEBUG_ERRORS 10 // Check and output AnkiErrors and explicit unit tests
#define ANKI_DEBUG_ERRORS_AND_WARNS 20 // Check and output AnkiErrors, AnkiWarns, and explicit unit tests
#define ANKI_DEBUG_ERRORS_AND_WARNS_AND_ASSERTS 30 // Check and output AnkiErrors, AnkiWarns, AnkiAsserts, and explicit unit tests
#define ANKI_DEBUG_ALL 40 // Check and output AnkiErrors, AnkiWarns, and explicit unit tests, plus run any additional extensive tests

// How will errors be reported?
#define ANKI_OUTPUT_DEBUG_NONE 0
#define ANKI_OUTPUT_DEBUG_PRINTF 10

#endif // _ANKICORETECHEMBEDDED_COMMON_CONFIG_H_

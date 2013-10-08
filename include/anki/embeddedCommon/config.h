/*
Basic configuation file for all common files, as well as all other coretech libraries.
Everything in this file should be compatible with plain C, as well as C++
*/

#ifndef _ANKICORETECHEMBEDDED_COMMON_CONFIG_H_
#define _ANKICORETECHEMBEDDED_COMMON_CONFIG_H_

#define ANKICORETECHEMBEDDED_VERSION_MAJOR 0
#define ANKICORETECHEMBEDDED_VERSION_MINOR 1
#define ANKICORETECHEMBEDDED_VERSION_REVISION 0

#if !defined(__APPLE_CC__) && defined(__GNUC__) && __GNUC__==4 && __GNUC_MINOR__==2 && __GNUC_PATCHLEVEL__==1 //hack to detect the movidius compiler
#warning Using GNUC 4.2.1
#define USING_MOVIDIUS_SHAVE_COMPILER
#define USING_MOVIDIUS_COMPILER
#endif

#if !defined(__APPLE_CC__) && defined(__GNUC__) && __GNUC__==4 && __GNUC_MINOR__==4 && __GNUC_PATCHLEVEL__==2 //hack to detect the movidius compiler
#warning Using GNUC 4.4.2
#define USING_MOVIDIUS_GCC_COMPILER
#define USING_MOVIDIUS_COMPILER
#endif

#ifndef NULL
#define NULL (0)
#endif

// Various defines that make different compilers work on the same code
#if defined(_MSC_VER) // We're using the MSVC compiler
#pragma warning(disable: 4068) // Unknown pragma
#pragma warning(disable: 4127) // Conditional expression is constant

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

#ifndef IN_DDR
#define IN_DDR
#endif

#endif // #if defined(_MSC_VER)

#if defined(__APPLE_CC__) // Apple Xcode

#ifndef IN_DDR
#define IN_DDR
#endif

#ifndef restrict
#define restrict
#endif

#ifndef _strcmpi
#define _strcmpi strcasecmp
#endif

#endif // #if defined(__APPLE_CC__)

#if defined(USING_MOVIDIUS_COMPILER) // If using any movidius compiler
#warning USING MOVIDIUS COMPILER!!
#undef ANKICORETECH_EMBEDDED_USE_MATLAB
#undef ANKICORETECH_EMBEDDED_USE_OPENCV
#undef ANKICORETECH_EMBEDDED_USE_GTEST

// Make it easy to detect usages of iostream
#define iostream IOSTREAM_DOESNT_WORK

#ifndef restrict
#define restrict __restrict
#endif

#ifndef IN_DDR
#define IN_DDR __attribute__((section(".ddr_direct.text")))
//#define IN_DDR __attribute__((section(".ddr.text")))
#endif

#endif // #if defined(USING_MOVIDIUS_COMPILER)

#ifdef USING_MOVIDIUS_GCC_COMPILER // If using the movidius gcc compiler for Leon

#ifdef __cplusplus
extern "C" {
#endif

#include "mv_types.h"
#include "DrvUart.h"
#include "swcTestUtils.h"
#include "swcLeonUtils.h"
#include "DrvL2Cache.h"
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <float.h>
#include <stdarg.h>

#undef printf
#define printf(...) explicitPrintf(1, __VA_ARGS__)
  
#define xprintf(...) (_xprintf(SYSTEM_PUTCHAR_FUNCTION , 0, __VA_ARGS__ ) )

#ifdef __cplusplus
}
#endif

#define f32 float
#define f64 double

#else // If not using the movidius gcc compiler for Leon

#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <float.h>
#include <stdarg.h>

// If we're not building mex (which will replace printf w/ mexPrintf),
// then we want to swap printf for explicitPrintf
#ifndef ANKI_MEX_BUILD
#undef printf
#define printf(...) explicitPrintf(0, __VA_ARGS__)
#endif

#include "anki/common/types.h"

#endif // #ifdef USING_MOVIDIUS_GCC_COMPILER

#include "anki/common/constantsAndMacros.h"

// If we're using c++, Result is in a namespace. In c, it's not.
#ifdef __cplusplus
namespace Anki
{
  namespace Embedded
  {
#endif
    // Return values:
    typedef enum {
      RESULT_OK = 0,
      RESULT_FAIL = 1,
      RESULT_FAIL_MEMORY = 10000,
      RESULT_FAIL_OUT_OF_MEMORY = 10001,
      RESULT_FAIL_IO = 20000,
      RESULT_FAIL_INVALID_PARAMETERS = 30000
    } Result;
#ifdef __cplusplus
  }
}
#endif

#define MEMORY_ALIGNMENT ( (size_t)(16) ) // To support 128-bit SIMD loads and stores

#define ANKI_DEBUG_MINIMAL 0 // Only check and output issue with explicit unit tests
#define ANKI_DEBUG_ERRORS 10 // Check and output AnkiErrors and explicit unit tests
#define ANKI_DEBUG_ERRORS_AND_WARNS 20 // Check and output AnkiErrors, AnkiWarns, and explicit unit tests
#define ANKI_DEBUG_ALL 30 // Check and output AnkiErrors, AnkiWarns, and explicit unit tests, plus run any additional extensive tests

#define ANKI_DEBUG_LEVEL ANKI_DEBUG_ERRORS

#define ANKI_OUTPUT_DEBUG_NONE 0
#define ANKI_OUTPUT_DEBUG_PRINTF 10

#define ANKI_OUTPUT_DEBUG_LEVEL ANKI_OUTPUT_DEBUG_PRINTF

#endif // _ANKICORETECHEMBEDDED_COMMON_CONFIG_H_

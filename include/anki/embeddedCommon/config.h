#ifndef _ANKICORETECHEMBEDDED_COMMON_CONFIG_H_
#define _ANKICORETECHEMBEDDED_COMMON_CONFIG_H_

#define ANKICORETECHEMBEDDED_VERSION_MAJOR 0
#define ANKICORETECHEMBEDDED_VERSION_MINOR 1
#define ANKICORETECHEMBEDDED_VERSION_REVISION 0

//#define ANKICORETECHEMBEDDED_USE_MATLAB
//#define ANKICORETECHEMBEDDED_USE_OPENCV
//#define ANKICORETECHEMBEDDED_USE_GTEST

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

#if defined(USING_MOVIDIUS_COMPILER)
#warning USING MOVIDIUS COMPILER!!
#undef ANKICORETECHEMBEDDED_USE_MATLAB
#undef ANKICORETECHEMBEDDED_USE_OPENCV
#undef ANKICORETECHEMBEDDED_USE_GTEST

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

#include <stddef.h>

#ifdef USING_MOVIDIUS_GCC_COMPILER
// We specify types according to their sign and bits. We should use these in
// our code instead of the normal 'int','short', etc. because different
// compilers on different architectures treat these differently.
#ifndef u8
#define u8 unsigned char
#define s8 char
#define u16 unsigned short
#define s16 short
#define u32 unsigned int
#define s32 int
#define u64 unsigned long long
#define s64 long long
#endif

#define f32 float
#define f64 double
#else // #ifdef USING_MOVIDIUS_GCC_COMPILER
#include <stdint.h>
// We specify types according to their sign and bits. We should use these in
// our code instead of the normal 'int','short', etc. because different
// compilers on different architectures treat these differently.
typedef uint8_t  u8;
typedef int8_t   s8;
typedef uint16_t u16;
typedef int16_t  s16;
typedef uint32_t u32;
typedef int32_t  s32;
typedef uint64_t u64;
typedef int64_t  s64;
typedef float    f32;
typedef double   f64;
#endif // #ifdef USING_MOVIDIUS_GCC_COMPILER

// Maximum and minimum values
const u8 u8_MAX =  static_cast<u8>(0xFF);
const u16 u16_MAX = static_cast<u16>(0xFFFF);
const u32 u32_MAX = static_cast<u32>(0xFFFFFFFF);

const s8 s8_MAX = static_cast<s8>(0x7F);
const s16 s16_MAX = static_cast<s16>(0x7FFF);
const s32 s32_MAX = static_cast<s32>(0x7FFFFFFF);

const s8 s8_MIN = static_cast<s8>(-1 - 0x7F);
const s16 s16_MIN = static_cast<s16>(-1 - 0x7FFF);
const s32 s32_MIN = static_cast<s32>(-1 - 0x7FFFFFFF);

#ifndef PI
#define PI 3.1415926535897932384626433832795028841971693993751058209749445923078164062862089986280348253421170679
#endif

namespace Anki
{
  namespace Embedded
  {
#define MEMORY_ALIGNMENT static_cast<size_t>(16) // To support 128-bit SIMD loads and stores

    // Return values:
    typedef enum Result_ {
      RESULT_OK = 0,
      RESULT_FAIL = 1,
      RESULT_FAIL_MEMORY = 10000,
      RESULT_FAIL_OUT_OF_MEMORY = 10001,
      RESULT_FAIL_IO = 20000,
      RESULT_FAIL_INVALID_PARAMETERS = 30000
    } Result;
  }
}

#define ANKI_DEBUG_MINIMAL 0 // Only check and output issue with explicit unit tests
#define ANKI_DEBUG_ERRORS 10 // Check and output AnkiErrors and explicit unit tests
#define ANKI_DEBUG_ERRORS_AND_WARNS 20 // Check and output AnkiErrors, AnkiWarns, and explicit unit tests
#define ANKI_DEBUG_ALL 30 // Check and output AnkiErrors, AnkiWarns, and explicit unit tests, plus run any additional extensive tests

#define ANKI_DEBUG_LEVEL ANKI_DEBUG_ERRORS

#define ANKI_OUTPUT_DEBUG_NONE 0
#define ANKI_OUTPUT_DEBUG_PRINTF 10

#define ANKI_OUTPUT_DEBUG_LEVEL ANKI_OUTPUT_DEBUG_PRINTF

#endif // _ANKICORETECHEMBEDDED_COMMON_CONFIG_H_

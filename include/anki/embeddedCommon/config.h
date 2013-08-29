#ifndef _ANKICORETECHEMBEDDED_COMMON_CONFIG_H_
#define _ANKICORETECHEMBEDDED_COMMON_CONFIG_H_

#define ANKICORETECHEMBEDDED_VERSION_MAJOR 0
#define ANKICORETECHEMBEDDED_VERSION_MINOR 1
#define ANKICORETECHEMBEDDED_VERSION_REVISION 0

//#define ANKICORETECHEMBEDDED_USE_MATLAB
//#define ANKICORETECHEMBEDDED_USE_OPENCV
//#define ANKICORETECHEMBEDDED_USE_GTEST

#if defined(__GNUC__) && __GNUC__==4 && __GNUC_MINOR__==2 && __GNUC_PATCHLEVEL__==1 //hack to detect the movidius compiler
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

#endif // #if defined(_MSC_VER)

#if defined(__APPLE_CC__) // Apple Xcode

#endif // #if defined(__APPLE_CC__)

#if defined(USING_MOVIDIUS_COMPILER)
#undef ANKICORETECHEMBEDDED_USE_MATLAB
#undef ANKICORETECHEMBEDDED_USE_OPENCV
#undef ANKICORETECHEMBEDDED_USE_GTEST

// Make it easy to detect usages of iostream
#define iostream IOSTREAM_DOESNT_WORK

#ifndef restrict
#define restrict __restrict
#endif
#endif

#include <stdint.h>
#include <stddef.h>

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

// Maximum and minimum values
#define u8_MAX  ((u8)0xFF)
#define u16_MAX ((u16)0xFFFF)
#define u32_MAX ((u32)0xFFFFFFFF)

#define s8_MAX  ((s8)0x7F)
#define s16_MAX ((s16)0x7FFF)
#define s32_MAX ((s32)0x7FFFFFFF)

#define s8_MIN  ((s8)0x80)
#define s16_MIN ((s16)0x8000)
#define s32_MIN ((s32)0x80000000)

namespace Anki
{
  namespace Embedded
  {
    const size_t MEMORY_ALIGNMENT = 16; // To support 128-bit SIMD loads and stores
  }
}

#define ANKI_DEBUG_OFF  0
#define ANKI_DEBUG_LOW  1000
#define ANKI_DEBUG_HIGH 2000

#define ANKI_DEBUG_LEVEL ANKI_DEBUG_HIGH

#endif // _ANKICORETECHEMBEDDED_COMMON_CONFIG_H_

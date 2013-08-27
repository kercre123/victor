#ifndef _ANKICORETECHEMBEDDED_COMMON_UTILITIES_H_
#define _ANKICORETECHEMBEDDED_COMMON_UTILITIES_H_

#include "anki/common/config.h"

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

#ifndef NULL
#define NULL (0)
#endif

#define SWAP(type, a, b) { type t; t = a; a = b; b = t; }

// ct_assert is a compile time assertion, useful for checking sizeof() and other compile time knowledge
#define ASSERT_CONCAT_(a, b) a##b
#define ASSERT_CONCAT(a, b) ASSERT_CONCAT_(a, b)
#define ct_assert(e) enum { ASSERT_CONCAT(assert_line_, __LINE__) = 1/(!!(e)) }

namespace Anki
{
  namespace Embedded
  {
    template<typename T> inline T RoundUp(T number, T multiple)
    {
      return multiple*( (number-1)/multiple + 1 );

      // For unsigned only?
      // return (number + (multiple-1)) & ~(multiple-1);
    }

    template<typename T> inline T RoundDown(T number, T multiple)
    {
      return multiple * (number/multiple);
    }

    bool IsPowerOfTwo(u32 x);

    u32 Log2(u32 x);
    u64 Log2(u64 x);

    double GetTime();

#if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
    int ConvertToOpenCvType(const char *typeName, size_t byteDepth); // Converts from typeid names to openCV types
#endif // #if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
  } // namespace Embedded
} // namespace Anki

#endif // _ANKICORETECHEMBEDDED_COMMON_UTILITIES_H_

#include "anki/embeddedCommon/utilities.h"
#include "anki/embeddedCommon/errorHandling.h"
#include "anki/embeddedCommon/array2d.h"

#if defined(_MSC_VER)
#include <windows.h >
#elif defined(USING_MOVIDIUS_COMPILER)

#else
#include <sys/time.h>
#endif

#include <math.h>

#if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
#include "opencv2/core/core.hpp"
#endif

// If using movidius, add some extra spaces, due to the byte-reordering
#if defined(USING_MOVIDIUS_COMPILER)
#define PrintfOneArray_FORMAT_STRING_2 "%d %d   "
#define PrintfOneArray_FORMAT_STRING_1 "%d %d "
#define PrintfOneArray_EXTRA_SPACES "   "
#else
#define PrintfOneArray_FORMAT_STRING_2 "%d   "
#define PrintfOneArray_FORMAT_STRING_1 "%d "
#define PrintfOneArray_EXTRA_SPACES ""
#endif

namespace Anki
{
  namespace Embedded
  {
#if !defined(USING_MOVIDIUS_COMPILER)
    IN_DDR double GetTime()
    {
#if defined(_MSC_VER)
      LARGE_INTEGER frequency, counter;
      QueryPerformanceCounter(&counter);
      QueryPerformanceFrequency(&frequency);
      return double(counter.QuadPart)/double(frequency.QuadPart);
#elif defined(__APPLE_CC__)
      return 0.0;
#else
      timespec ts;
      clock_gettime(CLOCK_MONOTONIC, &ts);
      return double(ts.tv_sec) + double(ts.tv_nsec)/1000000000.0;
#endif
    }
#endif // #if !defined(USING_MOVIDIUS_COMPILER)

    f32 Round(const f32 number)
    {
      if(number > 0)
        return floorf(number + 0.5f);
      else
        return floorf(number - 0.5f);
    }

    f64 Round(const f64 number)
    {
      // This casting wierdness is because the myriad doesn't have a double-precision floor function.
      if(number > 0)
        return static_cast<f64>(floorf(static_cast<f32>(number) + 0.5f));
      else
        return static_cast<f64>(floorf(static_cast<f32>(number) - 0.5f));
    }

    IN_DDR bool IsPowerOfTwo(u32 x)
    {
      // While x is even and greater than 1, keep dividing by two
      while (((x & 1) == 0) && x > 1)
        x >>= 1;

      return static_cast<bool>(x == 1);
    }

    bool IsOdd(const s32 x)
    {
      if(x | 1)
        return true;
      else
        return false;
    }

    s32 Determinant2x2(const s32 a, const s32 b, const s32 c, const s32 d)
    {
      return a*d - b*c;
    }

    IN_DDR u32 Log2(u32 x)
    {
      u32 powerCount = 0;
      // While x is even and greater than 1, keep dividing by two
      while (x >>= 1) {
        powerCount++;
      }

      return powerCount;
    }

    IN_DDR u64 Log2(u64 x)
    {
      u64 powerCount = 0;
      // While x is even and greater than 1, keep dividing by two
      while (x >>= 1) {
        powerCount++;
      }

      return powerCount;
    }

    IN_DDR void PrintfOneArray_f32(const Array<f32> &array, const char * variableName)
    {
      printf(variableName);
      printf(":\n" PrintfOneArray_EXTRA_SPACES);
      for(s32 y=0; y<array.get_size(0); y++) {
        const f32 * const rowPointer = array.Pointer(y, 0);

        // This goes every two, because the output on movidius looks more reasonable
        for(s32 x=0; x<array.get_size(1)-1; x+=2) {
          const f32 value1 = rowPointer[x];
          const f32 value2 = rowPointer[x+1];
          const f32 mulitipliedValue1 = 10000.0f * value1;
          const f32 mulitipliedValue2 = 10000.0f * value2;

          printf(PrintfOneArray_FORMAT_STRING_2, static_cast<s32>(mulitipliedValue1), static_cast<s32>(mulitipliedValue2));
        }

        for(s32 x=array.get_size(1)-2; x<array.get_size(1); x++) {
          const f32 value1 = rowPointer[x];
          const f32 mulitipliedValue1 = 10000.0f * value1;

          printf(PrintfOneArray_FORMAT_STRING_1, static_cast<s32>(mulitipliedValue1));
        }
        printf("\n" PrintfOneArray_EXTRA_SPACES);
      }
      printf("\n" PrintfOneArray_EXTRA_SPACES);
    }

    IN_DDR void PrintfOneArray_f64(const Array<f64> &array, const char * variableName)
    {
      printf(variableName);
      printf(":\n" PrintfOneArray_EXTRA_SPACES);
      for(s32 y=0; y<array.get_size(0); y++) {
        const f64 * const rowPointer = array.Pointer(y, 0);

        // This goes every two, because the output on movidius looks more reasonable
        for(s32 x=0; x<array.get_size(1)-1; x+=2) {
          const f64 value1 = rowPointer[x];
          const f64 value2 = rowPointer[x+1];
          const f64 mulitipliedValue1 = 10000.0 * value1;
          const f64 mulitipliedValue2 = 10000.0 * value2;

          printf(PrintfOneArray_FORMAT_STRING_2, static_cast<s32>(mulitipliedValue1), static_cast<s32>(mulitipliedValue2));
        }

        for(s32 x=array.get_size(1)-2; x<array.get_size(1); x++) {
          const f64 value1 = rowPointer[x];
          const f64 mulitipliedValue1 = 10000.0 * value1;

          printf(PrintfOneArray_FORMAT_STRING_1, static_cast<s32>(mulitipliedValue1));
        }
        printf("\n" PrintfOneArray_EXTRA_SPACES);
      }
      printf("\n" PrintfOneArray_EXTRA_SPACES);
    }

#if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
    IN_DDR int ConvertToOpenCvType(const char *typeName, size_t byteDepth)
    {
      if(typeName[0] == 'u') { //unsigned
        if(byteDepth == 1) {
          return CV_8U;
        } else if(byteDepth == 2) {
          return CV_16U;
        }
      } else if(typeName[0] == 'f' && byteDepth == 4) { //float
        return CV_32F;
      } else if(typeName[0] == 'd' && byteDepth == 8) { //double
        return CV_64F;
      } else { // signed
        if(byteDepth == 1) {
          return CV_8S;
        } else if(byteDepth == 2) {
          return CV_16S;
        } else if(byteDepth == 4) {
          return CV_32S;
        }
      }

      return -1;
    }
#endif //#if defined(ANKICORETECHEMBEDDED_USE_OPENCV)

#if defined(USING_MOVIDIUS_GCC_COMPILER)
    IN_DDR void memset(void * dst, int value, size_t size)
    {
      size_t i;
      for(i=0; i<size; i++)
      {
        ((char*)dst)[i] = value;
      }
    }
#endif // #if defined(USING_MOVIDIUS_GCC_COMPILER)
  } // namespace Embedded
} // namespace Anki
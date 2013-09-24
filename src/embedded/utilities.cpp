#include "anki/embeddedCommon/utilities.h"
#include "anki/embeddedCommon/errorHandling.h"

#if defined(_MSC_VER)
#include <windows.h >
#elif defined(USING_MOVIDIUS_COMPILER)
#else
#include <sys/time.h>
#endif

#include <math.h>

#if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
#include "opencv2/opencv.hpp"
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

    //// Perform the matrix multiplication "matOut = mat1 * mat2"
    //// Note that this is the naive O(n^3) implementation
    //Result MultiplyMatrices(const Array<f64> &mat1, const Array<f64> &mat2, Array<f64> &matOut)
    //{
    //  AnkiConditionalErrorAndReturnValue(mat1.get_size(1) == mat2.get_size(0),
    //    RESULT_FAIL, "MultiplyMatrices", "Input matrices are incompatible sizes");

    //  AnkiConditionalErrorAndReturnValue(matOut.get_size(0) == mat1.get_size(0),
    //    RESULT_FAIL, "MultiplyMatrices", "Input and Output matrices are incompatible sizes");

    //  AnkiConditionalErrorAndReturnValue(matOut.get_size(1) == mat2.get_size(1),
    //    RESULT_FAIL, "MultiplyMatrices", "Input and Output matrices are incompatible sizes");

    //  for(s32 y1=0; y1<mat1.get_size(0); y1++) {
    //    const f64 * restrict mat1_rowPointer = mat1.Pointer(y1, 0);
    //    f64 * restrict matOut_rowPointer = matOut.Pointer(y1, 0);

    //    for(s32 x1=0; x1<mat1.get_size(1); x1++) {
    //      matOut_rowPointer[x1] = 0;

    //      for(s32 y2=0; y2<mat1.get_size(0); y2++) {
    //        matOut_rowPointer[x1] += mat1_rowPointer[y2] * (*mat2.Pointer(y2, x1));
    //      }
    //    }
    //  }

    //  return RESULT_OK;
    //}

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
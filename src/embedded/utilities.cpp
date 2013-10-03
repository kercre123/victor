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
#include "opencv2/core/core.hpp"
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

    //#ifdef USING_MOVIDIUS_GCC_COMPILER
    //    void explicitPrintf(const char *format, ...)
    //    {
    //#define MAX_PRINTF_DIGITS 50
    //      int digits[MAX_PRINTF_DIGITS];
    //      int curChar = 0;
    //      int percentSignFound = 0;
    //
    //      int numArguments = 0;
    //      int previousChar = ' ';
    //
    //      const char * const formatStart = format;
    //      while(format != 0x00)
    //      {
    //        if(*format == '%') {
    //          numArguments++;
    //        }
    //
    //        format++;
    //      }
    //      format = formatStart;
    //
    //      va_list arguments;
    //      va_start(arguments, numArguments);
    //
    //      while(format != 0x00) {
    //        if(curChar >= 3) {
    //          curChar = 0;
    //          DrvApbUartPutChar(*format);
    //          DrvApbUartPutChar(charBuffer[2]);
    //          DrvApbUartPutChar(charBuffer[1]);
    //          DrvApbUartPutChar(charBuffer[0]);
    //          format++;
    //        } else { // if(curChar >= 3)
    //          if(percentSignFound) {
    //            int i;
    //            for(i=0; i<MAX_PRINTF_DIGITS; i++) {
    //              digits[i] = 0;
    //            }
    //
    //            int value = va_arg(arguments, int);
    //            if(value < 0) {
    //              DrvApbUartPutChar('-');
    //              value = -value;
    //            }
    //
    //            if(value == 0) {
    //              DrvApbUartPutChar('0');
    //              DrvApbUartPutChar(' ');
    //              format++;
    //              continue;
    //            }
    //
    //            i=0;
    //            while(value > 0) {
    //              const int curDigit = value - (10*(value/10));
    //
    //              digits[i++] = curDigit;
    //
    //              value /= 10;
    //            }
    //
    //            i--;
    //            for( ; i>=0; i--) {
    //              DrvApbUartPutChar(digits[i] + 48);
    //            }
    //
    //            DrvApbUartPutChar(' ');
    //            format++;
    //
    //            percentSignFound = 0;
    //          } else { // if(percentSignFound)
    //            if(*format == '%') {
    //              while(curChar <= 3) {
    //                charBuffer[curChar++] = ' ';
    //              }
    //              percentSignFound = 1;
    //              previousChar = *(format - 1);
    //              format++;
    //            } else {
    //              charBuffer[curChar++] = *format;
    //              format++;
    //            }
    //          } // if(percentSignFound) ... else
    //        } // if(curChar >= 3) ... else
    //      } // while(format != 0x00)
    //      va_end(arguments);
    //    } // void explicitPrintf(const char *format, ...)
    //#endif // #ifdef USING_MOVIDIUS_GCC_COMPILER
  } // namespace Embedded
} // namespace Anki
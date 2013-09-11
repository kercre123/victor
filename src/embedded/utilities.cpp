#include "anki/embeddedCommon.h"

#if defined(_MSC_VER)
#include <windows.h >
#elif defined(USING_MOVIDIUS_COMPILER)
#else
#include <sys/time.h>
#endif

#if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
#include "opencv2/opencv.hpp"
#endif

namespace Anki
{
  namespace Embedded
  {
#if !defined(USING_MOVIDIUS_COMPILER)
    double GetTime()
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

    f32 Round(f32 number)
    {
      if(number > 0)
        return floor(number + 0.5f);
      else
        return floor(number - 0.5f);
    }

    f64 Round(f64 number)
    {
      if(number > 0)
        return floor(number + 0.5);
      else
        return floor(number - 0.5);
    }

    bool IsPowerOfTwo(u32 x)
    {
      // While x is even and greater than 1, keep dividing by two
      while (((x & 1) == 0) && x > 1)
        x >>= 1;

      return static_cast<bool>(x == 1);
    }

    u32 Log2(u32 x)
    {
      u32 powerCount = 0;
      // While x is even and greater than 1, keep dividing by two
      while (x >>= 1) {
        powerCount++;
      }

      return powerCount;
    }

    u64 Log2(u64 x)
    {
      u64 powerCount = 0;
      // While x is even and greater than 1, keep dividing by two
      while (x >>= 1) {
        powerCount++;
      }

      return powerCount;
    }

#if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
    int ConvertToOpenCvType(const char *typeName, size_t byteDepth)
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
  } // namespace Embedded
} // namespace Anki
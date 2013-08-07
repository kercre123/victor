
#include "anki/common.h"

#if defined(_MSC_VER)
#include <windows.h >
#else
#include <sys/time.h>
#endif

#if defined(ANKICORETECH_USE_OPENCV)
#include "opencv2/opencv.hpp"
#endif


double Anki::GetTime()
{
#if defined(_MSC_VER)
  LARGE_INTEGER frequency, counter;
  QueryPerformanceCounter(&counter); 
  QueryPerformanceFrequency(&frequency);
  return 1000.0*double(counter.QuadPart)/double(frequency.QuadPart);
#elif defined(__APPLE_CC__)
  return 0.0;
#else
  timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return double(ts.tv_sec) + double(ts.tv_nsec)/1000000000.0;
#endif
}

#if defined(ANKICORETECH_USE_OPENCV)
int Anki::ConvertToOpenCvType(const char *typeName, size_t byteDepth)
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
#endif //#if defined(ANKICORETECH_USE_OPENCV)

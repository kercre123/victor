#define USING_MOVIDIUS_COMPILER

#include "anki/embeddedVision.h"

#if defined(ANKICORETECH_USE_OPENCV)
#include "opencv2/opencv.hpp"
#endif

#if defined(ANKICORETECH_USE_MATLAB)
Anki::Embedded::Matlab matlab(false);
#endif

#define CHECK_FOR_ERRORS

#define USE_L2_CACHE
#define MAX_BYTES 5000000

#ifdef _MSC_VER
char buffer[MAX_BYTES];
#else

#ifdef USE_L2_CACHE
char buffer[MAX_BYTES] __attribute__((section(".ddr.bss"))); // With L2 cache
#else
char buffer[MAX_BYTES] __attribute__((section(".ddr_direct.bss"))); // No L2 cache
#endif

#endif // #ifdef USING_MOVIDIUS_COMPILER

s32 BenchmarkBinomialFilter()
{
  const s32 width = 640;
  const s32 height = 480;
  const s32 numBytes = MIN(MAX_BYTES, 5000000);

  Anki::Embedded::MemoryStack ms(buffer, numBytes);

#ifdef CHECK_FOR_ERRORS
  DASConditionalErrorAndReturnValue(ms.IsValid(), -1, "ms.IsValid()", "");
#endif

  Anki::Embedded::Array_u8 img(height, width, ms, false);
  Anki::Embedded::Array_u8 imgFiltered(height, width, ms);

#ifdef CHECK_FOR_ERRORS
  DASConditionalErrorAndReturnValue(img.get_rawDataPointer()!= NULL, -2, "(img.get_rawDataPointer()!= NULL", "");
  DASConditionalErrorAndReturnValue(imgFiltered.get_rawDataPointer()!= NULL, -3, "(imgFiltered.get_rawDataPointer()!= NULL", "");
#endif

  Anki::Embedded::Result result = Anki::Embedded::BinomialFilter(img, imgFiltered, ms);

#ifdef CHECK_FOR_ERRORS
  DASConditionalErrorAndReturnValue(result == Anki::Embedded::RESULT_OK, -4, "result == Anki::Embedded::RESULT_OK", "");
#endif

  return 0;
}

#ifdef _MSC_VER
void RUN_ALL_BENCHMARKS()
{
  double times[20];

  times[0] = Anki::Embedded::GetTime();
  BenchmarkBinomialFilter();
  times[1] = Anki::Embedded::GetTime();

  printf("BenchmarkBinomialFilter: %f ms\n", 1000.0*(times[1]-times[0]));
} // void RUN_ALL_BENCHMARKS()
#else // #ifdef _MSC_VER

#endif // #ifdef _MSC_VER ... #else
#ifdef _MSC_VER

#include "visionBenchmarks.h"

//#define USING_MOVIDIUS_COMPILER
#include "anki/embeddedVision.h"

#include <stdio.h>

using namespace Anki::Embedded;

void RUN_ALL_BENCHMARKS()
{
  /*double times[20];

  times[0] = GetTime();*/

  BenchmarkSimpleDetector_Steps12345_realImage(50);

  /*//times[1] = GetTime();
  for(s32 i=0; i<48; i++) {
  BenchmarkBinomialFilter();
  }
  times[2] = GetTime();
  for(s32 i=0; i<48; i++) {
  BenchmarkDownsampleByFactor();
  }
  times[3] = GetTime();
  for(s32 i=0; i<48; i++) {
  BenchmarkComputeCharacteristicScale();
  }
  times[4] = GetTime();
  BenchmarkTraceInteriorBoundary();
  times[5] = GetTime();

  printf(
  "BenchmarkSimpleDetector_Steps12345_realImage: %f ms\n"
  "BenchmarkBinomialFilter: %f ms\n"
  "BenchmarkDownsampleByFactor: %f ms\n"
  "BenchmarkComputeCharacteristicScale: %f ms\n"
  "BenchmarkTraceInteriorBoundary: %f ms\n",
  1000.0*(times[1]-times[0]),
  1000.0*(times[2]-times[1]),
  1000.0*(times[3]-times[2]),
  1000.0*(times[4]-times[3]),
  1000.0*(times[5]-times[4]));*/
} // void RUN_ALL_BENCHMARKS()

int main()
{
  RUN_ALL_BENCHMARKS();

  //while(true) {}

  return 0;
}

#else

#warning pc_visionBenchmarks is a big no-op without MSC!
int main()
{
  return 0;
}

#endif // #ifdef _MSC_VER ... #else
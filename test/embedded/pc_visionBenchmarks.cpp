#ifdef _MSC_VER

#include "visionBenchmarks.h"

//#define USING_MOVIDIUS_COMPILER
#include "anki/embeddedVision.h"

#include <stdio.h>

void RUN_ALL_BENCHMARKS()
{
  double times[20];

  times[0] = Anki::Embedded::GetTime();
  for(s32 i=0; i<48; i++) {
    BenchmarkBinomialFilter();
  }
  times[1] = Anki::Embedded::GetTime();
  for(s32 i=0; i<48; i++) {
    BenchmarkDownsampleByFactor();
  }
  times[2] = Anki::Embedded::GetTime();
  for(s32 i=0; i<48; i++) {
    BenchmarkComputeCharacteristicScale();
  }
  times[3] = Anki::Embedded::GetTime();
  BenchmarkTraceBoundary();
  times[4] = Anki::Embedded::GetTime();

  printf(
    "BenchmarkBinomialFilter: %f ms\n"
    "BenchmarkDownsampleByFactor: %f ms\n"
    "BenchmarkComputeCharacteristicScale: %f ms\n"
    "BenchmarkTraceBoundary: %f ms\n",
    1000.0*(times[1]-times[0]),
    1000.0*(times[2]-times[1]),
    1000.0*(times[3]-times[2]),
    1000.0*(times[4]-times[3]));
} // void RUN_ALL_BENCHMARKS()

int main()
{
  RUN_ALL_BENCHMARKS();

  return 0;
}

#else

#warning pc_visionBenchmarks is a big no-op without MSC!
int main()
{
  return 0;
}

#endif // #ifdef _MSC_VER ... #else
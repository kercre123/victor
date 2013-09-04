#ifdef _MSC_VER

#include "visionBenchmarks.h"

#define USING_MOVIDIUS_COMPILER
#include "anki/embeddedVision.h"

#include <stdio.h>

void RUN_ALL_BENCHMARKS()
{
  double times[20];

  times[0] = Anki::Embedded::GetTime();
  BenchmarkBinomialFilter();
  times[1] = Anki::Embedded::GetTime();

  printf("BenchmarkBinomialFilter: %f ms\n", 1000.0*(times[1]-times[0]));
} // void RUN_ALL_BENCHMARKS()

int main()
{
  RUN_ALL_BENCHMARKS();

  return 0;
}

#endif // #ifdef _MSC_VER ... #else
#ifndef _MSC_VER
#include "swcTestUtils.h"
#include "visionBenchmarks.h"

#include <stdio.h>

#define NUM_PERFORMANCE_STRUCTS 20

//#define OUTPUT_CYCLES

void PrintTime(const performanceStruct * const perfStr, const char * const benchmarkName)
{
#ifdef OUTPUT_CYCLES
  printf(
    "\n%s:\n%d cycles in %d ms\n",
    benchmarkName,
    (u32)(perfStr->perfCounterTimer),
    (u32)(DrvTimerTicksToMs(perfStr->perfCounterTimer)));
#else
  printf(
    "\n%s: %d.0 ms",
    benchmarkName,
    (u32)(DrvTimerTicksToMs(perfStr->perfCounterTimer)));
#endif
}

void RUN_ALL_BENCHMARKS()
{
  int i;

  performanceStruct perfStr[NUM_PERFORMANCE_STRUCTS];

  for(i=0; i<NUM_PERFORMANCE_STRUCTS; i++) {
    swcShaveProfInit(&perfStr[i]);
    perfStr[i].countShCodeRun = 2;
  }

  swcShaveProfStartGathering(0, &perfStr[0]);
  BenchmarkBinomialFilter();
  swcShaveProfStopGathering(0, &perfStr[0]);

  swcShaveProfStartGathering(0, &perfStr[1]);
  BenchmarkDownsampleByFactor();
  swcShaveProfStopGathering(0, &perfStr[1]);

  swcShaveProfStartGathering(0, &perfStr[2]);
  BenchmarkComputeCharacteristicScale();
  swcShaveProfStopGathering(0, &perfStr[2]);

  swcShaveProfStartGathering(0, &perfStr[3]);
  BenchmarkTraceBoundary();
  swcShaveProfStopGathering(0, &perfStr[3]);

  PrintTime(&perfStr[0], "BenchmarkBinomialFilter");
  PrintTime(&perfStr[1], "BenchmarkDownsampleByFactor");
  PrintTime(&perfStr[2], "BenchmarkComputeCharacteristicScale");
  PrintTime(&perfStr[3], "BenchmarkTraceBoundary");
} // void RUN_ALL_BENCHMARKS()

#endif // #ifndef _MSC_VER
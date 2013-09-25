#include "visionBenchmarks.h"

#ifndef _MSC_VER
#include "swcTestUtils.h"

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
    "\n%s: %d ms",
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
  for(i=0; i<48; i++) {
    BenchmarkBinomialFilter();
  }
  swcShaveProfStopGathering(0, &perfStr[0]);

  swcShaveProfStartGathering(0, &perfStr[1]);
  for(i=0; i<48; i++) {
    BenchmarkDownsampleByFactor();
  }
  swcShaveProfStopGathering(0, &perfStr[1]);

  swcShaveProfStartGathering(0, &perfStr[2]);
  for(i=0; i<48; i++) {
    BenchmarkComputeCharacteristicScale();
  }
  swcShaveProfStopGathering(0, &perfStr[2]);

  swcShaveProfStartGathering(0, &perfStr[3]);
  for(i=0; i<48; i++) {
    BenchmarkTraceInteriorBoundary();
  }
  swcShaveProfStopGathering(0, &perfStr[3]);

  PrintTime(&perfStr[0], "BenchmarkBinomialFilter");
  PrintTime(&perfStr[1], "BenchmarkDownsampleByFactor");
  PrintTime(&perfStr[2], "BenchmarkComputeCharacteristicScale");
  PrintTime(&perfStr[3], "BenchmarkTraceInteriorBoundary");
} // void RUN_ALL_BENCHMARKS()

#endif // #ifndef _MSC_VER
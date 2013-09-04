#ifndef _MSC_VER
#include "swcTestUtils.h"
#include "visionBenchmarks.h"

#include <stdio.h>

#define NUM_PERFORMANCE_STRUCTS 20

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

  printf("\nBenchmarkBinomialFilter: %d cycles in %06d micro seconds ([%d ms])\n",(u32)(perfStr[0].perfCounterTimer), (u32)(DrvTimerTicksToMs(perfStr[0].perfCounterTimer)*1000), (u32)(DrvTimerTicksToMs(perfStr[0].perfCounterTimer)));
} // void RUN_ALL_BENCHMARKS()

#endif // #ifndef _MSC_VER

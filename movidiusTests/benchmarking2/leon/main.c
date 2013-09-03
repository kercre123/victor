///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief     main leon file 
///

#include <stdio.h>
#include "isaac_registers.h"
#include "mv_types.h"
#include "DrvSvu.h"
#include <DrvSvuDebug.h>
#include "DrvCpr.h"
#include "DrvTimer.h"
#include <DrvL2Cache.h>

//#include <sys/time.h>
//#include <ctime>

#include "swcShaveLoader.h"
#include "app_config.h"
#include "swcTestUtils.h"

#include "testCpp.h"

performanceStruct perfStr;

/*
double GetTime()
{
  timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return double(ts.tv_sec) + double(ts.tv_nsec)/1000000000.0;
}*/

int main(void)
{
    u32          i;
    u32          *fl;
    u32          test_pass = 1;
    int dogResult;

    initClocksAndMemory();


    printf("Starting profiling\n");

    swcShaveProfInit(&perfStr);
 
    perfStr.countShCodeRun = 2;

    swcShaveProfStartGathering(0, &perfStr);

    dogResult = helloDog(2);

    runTests();

    swcShaveProfStopGathering(0, &perfStr);

    printf("%d\n", dogResult);

//    swcShaveProfPrint(0, &perfStr);
    printf("\nLeon executed %d cycles in %06d micro seconds ([%d ms])\n",(u32)(perfStr.perfCounterTimer), (u32)(DrvTimerTicksToMs(perfStr.perfCounterTimer)*1000), (u32)(DrvTimerTicksToMs(perfStr.perfCounterTimer)));

    printf("Finished profiling\n");

    return 0;
}

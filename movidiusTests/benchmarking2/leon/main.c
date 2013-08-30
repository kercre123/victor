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

    initClocksAndMemory();


    printf("Starting profiling\n");

    swcShaveProfInit(&perfStr);

    printf("%d\n", helloDog(2));

    swcShaveProfPrint(0, &perfStr);

    printf("Finished profiling\n");

    return 0;
}

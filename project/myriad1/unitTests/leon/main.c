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

#include "cppInterface.h"

performanceStruct perfStr;

/*
double GetTime()
{
  timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return double(ts.tv_sec) + double(ts.tv_nsec)/1000000000.0;
}*/

// Copied from leon3.h
__inline__ void sparc_leon3_disable_cache(void) {
  /*asi 2*/
  __asm__ volatile ("lda [%%g0] 2, %%l1\n\t"  \
                    "set 0x00000f, %%l2\n\t"  \
                    "andn  %%l2, %%l1, %%l2\n\t" \
                    "sta %%l2, [%%g0] 2\n\t"  \
                    :  : : "l1", "l2");	
};

int main(void)
{
    u32          i;
    u32          *fl;
    u32          test_pass = 1;
    int dogResult;

    initClocksAndMemory();

    sparc_leon3_disable_cache();

    printf("Starting unit tests\n");

    swcShaveProfInit(&perfStr);
 
    perfStr.countShCodeRun = 2;

    swcShaveProfStartGathering(0, &perfStr);

    runTests();

    swcShaveProfStopGathering(0, &perfStr);

//    The below printf is modified from swcShaveProfPrint(0, &perfStr);
    printf("\nLeon executed %d cycles in %06d micro seconds ([%d ms])\n",(u32)(perfStr.perfCounterTimer), (u32)(DrvTimerTicksToMs(perfStr.perfCounterTimer)*1000), (u32)(DrvTimerTicksToMs(perfStr.perfCounterTimer)));

    printf("Finished unit tests\n");

    return 0;
}

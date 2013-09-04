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

#include "swcShaveLoader.h"
#include "app_config.h"
#include "swcTestUtils.h"

#include "visionBenchmarks.h"

void RUN_ALL_BENCHMARKS();

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

    initClocksAndMemory();

    sparc_leon3_disable_cache();

    printf("Starting benchmarking\n");
 
    RUN_ALL_BENCHMARKS();

    printf("Finished benchmarking\n");

    return 0;
}

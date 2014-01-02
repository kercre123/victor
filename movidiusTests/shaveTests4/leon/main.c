///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief     main leon file 
///

// 1: Includes
// ----------------------------------------------------------------------------
#include <stdio.h>
#include <mv_types.h>
#include <registersMyriad.h>
#include <DrvSvu.h>
#include <swcShaveLoader.h>
#include "app_config.h"

// 2:  Source Specific #defines and types  (typedef, enum, struct)
// ----------------------------------------------------------------------------
#define SHAVE_USED 0

// 3: Global Data (Only if absolutely necessary)
// ----------------------------------------------------------------------------
extern u32 helloShave0_main;

// 4: Static Local Data
// ----------------------------------------------------------------------------
// 5: Static Function Prototypes
// ----------------------------------------------------------------------------
// 6: Functions Implementation
// ----------------------------------------------------------------------------

extern char helloShave0_helloShave[250];

int main(void)
{
    initClocksAndMemory();

    swcResetShave(SHAVE_USED);
    swcSetAbsoluteDefaultStack(SHAVE_USED);

    swcStartShave(0,(u32)&helloShave0_main);
    swcWaitShave(SHAVE_USED);

#ifdef MYRIAD1
    //no to do on MYRIAD1 here
#else //MYRIAD2
    //Print hello from shave by Leon
    printf("%s", helloShave0_helloShave);
#endif

    return 0;
}

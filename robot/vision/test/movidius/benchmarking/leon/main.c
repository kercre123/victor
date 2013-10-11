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

// include here for convenience
// for complex shave modules split to separate shave_export.h file or similar
extern void*  (SVE0_main);

//extern u32 SVE0_myint1;
//extern u32 SVE0_myint2;
//extern u32 SVE0_myintrez;

performanceStruct perfStr;

// This is a trivial test where we pass in two vectors to a shave that adds them together
// the results is checked using the RISC processor.
//
int main(void)
{
    u32          i;
    u32          *fl;
    u32          test_pass = 1;


    initClocksAndMemory();

    // Set input parameters Shaves
    printf("Start Shave at addr:0x%X \n", (unsigned int)&SVE0_main);

/*
    u32* int1_in = (u32 *)swcSolveShaveRelAddrAHB(((unsigned int)&SVE0_myint1),0);
    u32* int2_in = (u32 *)swcSolveShaveRelAddrAHB(((unsigned int)&SVE0_myint2),0);

    int1_in[0] = 4;
    int1_in[1] = 25;
    int1_in[2] = 313;
    int1_in[3] = 13;

    int2_in[0] = 4;
    int2_in[1] = 30;
    int2_in[2] = 400;
    int2_in[3] = 500;

    printf("Set 1: %d %d %d %d\n",int1_in[0], int1_in[1], int1_in[2], int1_in[3]);
    printf("Set 2: %d %d %d %d\n",int2_in[0], int2_in[1], int2_in[2], int2_in[3]);
*/
    printf("Starting profiling\n");

    swcShaveProfInit(&perfStr);

    while(!swcShaveProfGatheringDone(&perfStr))
    {
        swcResetShave(0);

        swcShaveProfStartGathering(0, &perfStr);
        swcStartShave(0,(u32)&SVE0_main);
        swcWaitShave(0);
        swcShaveProfStopGathering(0, &perfStr);
    }
    swcShaveProfPrint(0, &perfStr);

    // check the result

/*
    fl = (u32 *)swcSolveShaveRelAddrAHB((unsigned int)&SVE0_myintrez, 0);
    printf("res  : %d %d %d %d\n",fl[0], fl[1], fl[2], fl[3]);
    for (i = 0; i < 3; i++)
        if (fl[i] != (int1_in[i] + int2_in[i]))
            test_pass = 0;

    if (test_pass)
        printf("\n\nShaveHelloWorld Executed Successfully\n\n");
    else
        printf("ShaveHelloWorld failed\n");
*/

    printf("Finished profiling\n");

    return 0;
}

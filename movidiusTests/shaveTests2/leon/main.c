///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief     Simple Example showing non blocking shave call
///

// common includes
#include <stdio.h>
#include "mv_types.h"
#include "swcShaveLoader.h"
#include "DrvSvuDebug.h"
#include "DrvCpr.h"
#include "DrvTimer.h"
#include <DrvL2Cache.h>

// project includes
#include "app_config.h"
#include "../shave/shaveShared.h"

// include here for convenience
// for complex shave modules split to separate shave_export.h file or similar
extern int shaveTests7_main;

// Declare Shave halted interrupt handler
void ShaveHaltedISR(u32 unused);

const s32 expectedFilterOutput[IMAGE_WIDTH] = {62, 70, 77, 86, 94, 103, 111, 119, 127, 136, 143, 152, 162, 170, 177, 186, 194, 203, 211, 219, 227, 236, 243, 252, 262, 270, 277, 286, 294, 303, 311, 319, 327, 336, 343, 352, 362, 370, 377, 386, 394, 403, 411, 419, 427, 436, 443, 452, 462, 470, 477, 486, 494, 503, 511, 519, 527, 536, 543, 0, 0, 0, 0, 0};

const s32 expectedIntegralImage[IMAGE_WIDTH] = {2, 5, 9, 14, 20, 28, 37, 47, 58, 70, 83, 97, 112, 128, 145, 163, 182, 203, 225, 248, 272, 297, 323, 350, 378, 407, 437, 468, 500, 534, 569, 605, 642, 680, 719, 759, 800, 842, 885, 929, 974, 1021, 1069, 1118, 1168, 1219, 1271, 1324, 1378, 1433, 1489, 1546, 1604, 1664, 1725, 1787, 1850, 1914, 1979, 2045, 2112, 2180, 2249, 2319};

// Global event variable
volatile u32 g_shaveFinishedEvent = 0;

extern volatile int shaveTests7_whichAlgorithm;
extern volatile int shaveTests7_integralImage[IMAGE_WIDTH];
extern volatile int shaveTests7_outputLine[IMAGE_WIDTH];

extern volatile int shaveTests7_pOut[NUM_COORDINATES];

// Leon Entry point
int __attribute__((section(".sys.text.start"))) main(void)
{
  s32 i;
  u32 running   = 1;

  initClocksAndMemory();

  //*(volatile u32*)MXI_CMX_CTRL_BASE_ADR |= (1 << 24);
  //    DrvL2CacheSetupPartition(PART128KB);
  //    DrvL2CacheAllocateSetPartitions();
  //    SET_REG_WORD(L2C_MXITID_ADR, 0x0);

  shaveTests7_whichAlgorithm = 2;

  // Start the Shave but do not wait for stop
  printf("Start Shave at addr:0x%X \n", (unsigned int)&shaveTests7_main);
  swcResetShave(7);
  swcSetAbsoluteDefaultStack(7);
  swcStartShaveAsync(7,(u32)&shaveTests7_main, ShaveHaltedISR);

  // Main loop that handles events
  while (running)
  {
    int localSum = 0;
    int otherLocalSum = 0;

    // potentially do other stuff here

    // Handle a Shave Finished Event in main loop when it occurs
    if (g_shaveFinishedEvent)
    {
      u32 i;
      u32 test_pass = 1;

      if(shaveTests7_whichAlgorithm == 1) {
        for(i=0; i<IMAGE_WIDTH; i+=5) {
          printf("%d) correct:%d shave:%d\n", i, expectedFilterOutput[i], shaveTests7_outputLine[i]);
        }

        for(i=0; i<IMAGE_WIDTH; i++) {
          if(shaveTests7_outputLine[i] != expectedFilterOutput[i]) {
            test_pass = 0;
            printf("Fail at %d\n", i);
            //break;
          }
        }
      } else if(shaveTests7_whichAlgorithm == 2) {
        for(i=0; i<IMAGE_WIDTH; i+=7) {
          printf("%d) correct:%d shave:%d\n", i, expectedIntegralImage[i], shaveTests7_integralImage[i+IMAGE_WIDTH]);
        }

        for(i=0; i<61; i++) {
          if(shaveTests7_integralImage[i+IMAGE_WIDTH] != expectedIntegralImage[i]) {
            test_pass = 0;
            printf("Fail at %d\n", i);
            //break;
          }
        }
      } else if(shaveTests7_whichAlgorithm == 3) {
        // TODO: fill in correct answers
      }

      if (test_pass)
        printf("\n\nShaveHelloWorld Executed Successfully\n\n");
      else
        printf("ShaveHelloWorld failed\n");

      // mark event as handled
      g_shaveFinishedEvent = 0;

      // in this case we just want to finish the application after a single shave event
      running = 0;
    }
  }

  return 0;
}

// Definition of Shave ISR
// Generally goal is to minimed code in ISRs in this case we'll just use a
// simple event that gets picked up in main loop.
void ShaveHaltedISR(u32 unused)
{
  g_shaveFinishedEvent = 1;
}
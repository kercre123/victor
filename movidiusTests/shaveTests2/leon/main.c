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

const expectedFilterOutput[IMAGE_WIDTH] = {43,52,58,64,71,78,84,90,95,101,107,112,118,127,133,139,146,153,159,165,170,176,182,187,193,202,208,214,221,228,234,240,245,251,257,262,268,277,283,289,296,303,309,315,320,326,332,337,343,352,358,364,371,378,384,390,395,401,407,412,0,0,0,0};

// Global event variable
volatile u32 g_shaveFinishedEvent = 0;

extern volatile int shaveTests7_whichAlgorithm;
extern volatile int shaveTests7_outputLine[IMAGE_WIDTH];

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

  shaveTests7_whichAlgorithm = 1;

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

      for(i=0; i<IMAGE_WIDTH; i++) {
        printf("%d) correct:%d shave:%d\n", i, expectedFilterOutput[i], shaveTests7_outputLine[i]);
      }

      for(i=0; i<IMAGE_WIDTH; i++) {
        if(shaveTests7_outputLine[i] != expectedFilterOutput[i]) {
          test_pass = 0;
          printf("Fail at %d\n", i);
          //break;
        }
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
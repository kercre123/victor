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

// include here for convenience
// for complex shave modules split to separate shave_export.h file or similar
extern int shaveTests7_main;

// Declare Shave halted interrupt handler
void ShaveHaltedISR(u32 unused);

// Global event variable
volatile u32 g_shaveFinishedEvent = 0;

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

  // Start the Shave but do not wait for stop
  printf("Start Shave at addr:0x%X \n", (unsigned int)&shaveTests7_main);
  swcResetShave(7);
  swcSetAbsoluteDefaultStack(7);
  swcStartShaveAsync(7,(u32)&shaveTests7_main, ShaveHaltedISR);

  // Main loop that handles events
  while (running)
  {
    // potentially do other stuff here

    // Handle a Shave Finished Event in main loop when it occurs
    if (g_shaveFinishedEvent)
    {
      printf("ShaveTests finished\n");

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
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
extern volatile int shaveTests7_whichAlgorithm;
extern volatile int shaveTests7_numPixelsToProcess;

//__attribute__((section(".imageData"))) __attribute__ ((aligned (16))) extern s32 shaveTests7_im1_ddr[IMAGE_SIZE];
//__attribute__((section(".imageData"))) __attribute__ ((aligned (16))) extern s32 shaveTests7_im2_ddr[IMAGE_SIZE];
//__attribute__((section(".imageData"))) __attribute__ ((aligned (16))) extern s32 shaveTests7_out_ddr[IMAGE_SIZE];

__attribute__((section(".imageData"))) __attribute__ ((aligned (16))) s32 shaveTests7_im1_ddr[IMAGE_SIZE];
__attribute__((section(".imageData"))) __attribute__ ((aligned (16))) s32 shaveTests7_im2_ddr[IMAGE_SIZE];
__attribute__((section(".imageData"))) __attribute__ ((aligned (16))) s32 shaveTests7_out_ddr[IMAGE_SIZE];

__attribute__ ((aligned (16))) extern s32 shaveTests7_im1[IMAGE_SIZE];
__attribute__ ((aligned (16))) extern s32 shaveTests7_im2[IMAGE_SIZE];
__attribute__ ((aligned (16))) extern s32 shaveTests7_out[IMAGE_SIZE];

// Declare Shave halted interrupt handler
void ShaveHaltedISR(u32 unused);

// Global event variable
volatile u32 g_shaveFinishedEvent = 0;

// Leon Entry point
int __attribute__((section(".sys.text.start"))) main(void)
{
  s32 i;
  u32 running   = 1;
  u64 times[20];

  tyTimeStamp  g_ticks;

  initClocksAndMemory();

  //*(volatile u32*)MXI_CMX_CTRL_BASE_ADR |= (1 << 24);
  //    DrvL2CacheSetupPartition(PART128KB);
  //    DrvL2CacheAllocateSetPartitions();
  //    SET_REG_WORD(L2C_MXITID_ADR, 0x0);

  shaveTests7_whichAlgorithm = 3;
  shaveTests7_numPixelsToProcess = 5000;

  //if(shaveTests7_whichAlgorithm == 1 || shaveTests7_whichAlgorithm == 3 || shaveTests7_whichAlgorithm == 4) {
  for(i=0; i<IMAGE_SIZE; i++) {
    shaveTests7_im1_ddr[i] = i;
    shaveTests7_im2_ddr[i] = 5;
  }

  if(shaveTests7_whichAlgorithm == 2) {
    for(i=0; i<IMAGE_SIZE; i++) {
      shaveTests7_im1[i] = i;
      shaveTests7_im2[i] = 5;
    }
  }

  // Start the Shave but do not wait for stop
  printf("Start Shave at addr:0x%X \n", (unsigned int)&shaveTests7_main);
  swcResetShave(7);
  swcSetAbsoluteDefaultStack(7);

  DrvTimerStart(&g_ticks);

  //if(shaveTests7_whichAlgorithm == 3 || shaveTests7_whichAlgorithm == 4) {
  //  scDmaSetup(DMA_TASK_0, &shaveTests7_im1_ddr[0], &shaveTests7_im1[0], shaveTests7_numPixelsToProcess*sizeof(s32));
  //  scDmaSetup(DMA_TASK_1, &shaveTests7_im2_ddr[0], &shaveTests7_im2[0], shaveTests7_numPixelsToProcess*sizeof(s32));
  //  scDmaStart(START_DMA01);
  //  scDmaWaitFinished();
  //}

  swcStartShaveAsync(7,(u32)&shaveTests7_main, ShaveHaltedISR);

  //times[0] = DrvTimerGetSysTicks64();

  // Main loop that handles events
  while (running)
  {
    int localSum = 0;
    int otherLocalSum = 0;

    // potentially do other stuff here

    // Handle a Shave Finished Event in main loop when it occurs
    if (g_shaveFinishedEvent)
    {
      s32 i;
      u32 test_pass = 1;

      //times[1] = DrvTimerGetSysTicks64();
      //printf("It took %llu ms\n", DrvTimerTicksToMs(times[1]-times[0])/10000);

      //if(shaveTests7_whichAlgorithm == 4) {
      //  scDmaSetup(DMA_TASK_0, &shaveTests7_out_ddr[0], &shaveTests7_out[0], shaveTests7_numPixelsToProcess*sizeof(s32));
      //  scDmaStart(START_DMA0);
      //  scDmaWaitFinished();
      //}

      {
        const u32 passed_cycles = DrvTimerElapsedTicks(&g_ticks);

        printf("Operation took %d micro seconds\n", (u32)(DrvTimerTicksToMs(passed_cycles)*1000));
      }

      if(shaveTests7_whichAlgorithm == 1) {
        for(i=0; i<shaveTests7_numPixelsToProcess; i++) {
          if(shaveTests7_out_ddr[i] != (shaveTests7_im1_ddr[i]+shaveTests7_im2_ddr[i]) ) {
            printf("fail at %d. (%d != %d+%d)\n", i, shaveTests7_out_ddr[i], shaveTests7_im1_ddr[i], shaveTests7_im2_ddr[i]);
            test_pass = 0;
          }
        }
      } else if(shaveTests7_whichAlgorithm == 3 || shaveTests7_whichAlgorithm == 4) {
        for(i=0; i<shaveTests7_numPixelsToProcess; i++) {
          if(shaveTests7_out_ddr[i] != (shaveTests7_im1[i]+shaveTests7_im2[i]) ) {
            printf("fail at %d. (%d != %d+%d)\n", i, shaveTests7_out_ddr[i], shaveTests7_im1[i], shaveTests7_im2[i]);
            test_pass = 0;
          }
        }
      } else {
        for(i=0; i<shaveTests7_numPixelsToProcess; i++) {
          if(shaveTests7_out[i] != (shaveTests7_im1_ddr[i]+shaveTests7_im2_ddr[i]) ) {
            printf("fail at %d. (%d != %d+%d)\n", i, shaveTests7_out[i], shaveTests7_im1_ddr[i], shaveTests7_im2_ddr[i]);
            test_pass = 0;
          }
        }
      }

      if (test_pass)
        printf("\nShaveDmaTests Executed Successfully\n\n");
      else
        printf("\nShaveDmaTests Failed\n\n");

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
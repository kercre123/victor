///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief     Shave hello world source code
///

#include <stdio.h>
#include <math.h>
#include <moviVectorUtils.h>
#include <svuCommonShave.h>

#include "shaveShared.h"

//__attribute__((section(".imageData"))) __attribute__ ((aligned (16))) s32 im1_ddr[IMAGE_SIZE];
//__attribute__((section(".imageData"))) __attribute__ ((aligned (16))) s32 im2_ddr[IMAGE_SIZE];
//__attribute__((section(".imageData"))) __attribute__ ((aligned (16))) s32 out_ddr[IMAGE_SIZE];

__attribute__((section(".imageData"))) __attribute__ ((aligned (16))) extern s32 im1_ddr[IMAGE_SIZE];
__attribute__((section(".imageData"))) __attribute__ ((aligned (16))) extern s32 im2_ddr[IMAGE_SIZE];
__attribute__((section(".imageData"))) __attribute__ ((aligned (16))) extern s32 out_ddr[IMAGE_SIZE];

__attribute__ ((aligned (16))) s32 im1[IMAGE_SIZE];
__attribute__ ((aligned (16))) s32 im2[IMAGE_SIZE];
__attribute__ ((aligned (16))) s32 out[IMAGE_SIZE];

volatile int whichAlgorithm;
volatile int numPixelsToProcess;

void simpleAdd(const s32 * restrict im1, const s32 * restrict im2, s32 * restrict out, int numElements);

int main( void )
{
  if(whichAlgorithm == 1) {
    simpleAdd(&im1_ddr[0], &im2_ddr[0], &out_ddr[0], numPixelsToProcess);
  } else if(whichAlgorithm == 2) {
    simpleAdd(&im1[0], &im2[0], &out[0], numPixelsToProcess);
  } else if(whichAlgorithm == 3) {
    scDmaSetup(DMA_TASK_0, &im1_ddr[0], &im1[0], numPixelsToProcess*sizeof(s32));
    scDmaSetup(DMA_TASK_1, &im2_ddr[0], &im2[0], numPixelsToProcess*sizeof(s32));
    scDmaStart(START_DMA01);
    scDmaWaitFinished();
    simpleAdd(&im1[0], &im2[0], &out_ddr[0], numPixelsToProcess);
  } else if(whichAlgorithm == 4) {
    scDmaSetup(DMA_TASK_0, &im1_ddr[0], &im1[0], numPixelsToProcess*sizeof(s32));
    scDmaSetup(DMA_TASK_1, &im2_ddr[0], &im2[0], numPixelsToProcess*sizeof(s32));
    scDmaStart(START_DMA01);
    scDmaWaitFinished();
    simpleAdd(&im1[0], &im2[0], &out[0], numPixelsToProcess);
    scDmaSetup(DMA_TASK_0, &out[0], &out_ddr[0], numPixelsToProcess*sizeof(s32));
    scDmaStart(START_DMA0);
    scDmaWaitFinished();
  }

  return 0;
}
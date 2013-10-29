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

void simpleAdd(const u8 * restrict im1, const u8 * restrict im2, u8 * restrict out, int numElements);

int main( void )
{
  if(whichAlgorithm == 1) {
    simpleAdd(im1_ddr, im2_ddr, out_ddr, numPixelsToProcess);
  } else if(whichAlgorithm == 2) {
    simpleAdd(im1, im2, out, numPixelsToProcess);
  }

  return 0;
}
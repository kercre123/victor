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

volatile int whichAlgorithm;

__attribute__ ((aligned (16))) volatile int input1[DATA_LENGTH];
__attribute__ ((aligned (16))) volatile int input2[DATA_LENGTH];

__attribute__ ((aligned (16))) volatile int output[DATA_LENGTH];

int main( void )
{
  if(whichAlgorithm == 1) {
    simpleLoop();
  } else if(whichAlgorithm == 2) {
    simpleVectorLoop();
  } else if(whichAlgorithm == 3) {
    intrinsicsLoop();
  } else if(whichAlgorithm == 4) {
    assemblyLoop();
  }

  return 0;
}
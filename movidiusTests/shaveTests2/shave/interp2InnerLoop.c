#include <stdio.h>
#include <math.h>
#include <moviVectorUtils.h>
#include "shaveShared.h"

#define INNER_LOOP_VERSION 1

void interp2_shaveInnerLoop(
  const float * restrict pXCoordinates, const float * restrict pYCoordinates,
  const float bufferWidth,
  const float * restrict reference, const int referenceStride,
  const float xReferenceMax, const float yReferenceMax,
  float * restrict pOut)
{
  __asm(
  ""
    : //Output registers
  : //Input registers
  : //Clobbered registers
  );

#if INNER_LOOP_VERSION == 1
  int x;
  for(x=0; x<bufferWidth; x++) {
    __asm(
    ""
      : //Output registers
    : //Input registers
    : //Clobbered registers
    );
  }

#endif // INNER_LOOP_VERSION
}
#include <stdio.h>
#include <math.h>
#include <moviVectorUtils.h>
#include "shaveShared.h"

#define INNER_LOOP_VERSION 1

void interp2_shaveInnerLoop(
  const f32 * restrict pXCoordinates, const f32 * restrict pYCoordinates,
  const f32 bufferWidth,
  const f32 * restrict reference, const int referenceStride,
  const f32 xReferenceMax, const f32 yReferenceMax,
  f32 * restrict pOut)
{
  __asm(
  ""
    : //Output registers
  : //Input registers
  : //Clobbered registers
  );

#if INNER_LOOP_VERSION == 1
  s32 x;
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
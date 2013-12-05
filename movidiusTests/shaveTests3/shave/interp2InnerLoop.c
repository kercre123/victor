#include <stdio.h>
#include <math.h>
#include <moviVectorUtils.h>
#include "shaveKernels.h"

#define INNER_LOOP_VERSION 1

#define InterpolateBilinear2d(pixelTL, pixelTR, pixelBL, pixelBR, alphaY, alphaYinverse, alphaX, alphaXinverse, interpolatedPixel)\
{\
  const f32 interpolatedTop = alphaXinverse*pixelTL + alphaX*pixelTR;\
  const f32 interpolatedBottom = alphaXinverse*pixelBL + alphaX*pixelBR;\
  interpolatedPixel = alphaYinverse*interpolatedTop + alphaY*interpolatedBottom;\
}

void interp2_shaveInnerLoop(
  const f32 * restrict pXCoordinates, const f32 * restrict pYCoordinates,
  const s32 numCoordinates,
  const u8 * restrict pReference, const int referenceStride,
  const f32 xReferenceMax, const f32 yReferenceMax,
  f32 * restrict pOut)
{
#if INNER_LOOP_VERSION == 1
  const f32 invalidValue = -2.0f;

  s32 x;
  for(x=0; x<numCoordinates; x++) {
    const f32 curX = pXCoordinates[x];
    const f32 curY = pYCoordinates[x];

    const f32 x0 = floorf(curX);
    const f32 x1 = ceilf(curX); // x0 + 1.0f;

    const f32 y0 = floorf(curY);
    const f32 y1 = ceilf(curY); // y0 + 1.0f;

    // If out of bounds, set as invalid and continue
    if(x0 < 0.0f || x1 > xReferenceMax || y0 < 0.0f || y1 > yReferenceMax) {
      pOut[x] = invalidValue;
      continue;
    }

    {
      const f32 alphaX = curX - x0;
      const f32 alphaXinverse = 1 - alphaX;

      const f32 alphaY = curY - y0;
      const f32 alphaYinverse = 1.0f - alphaY;

      const s32 y0S32 = (s32)(floorf(y0));
      const s32 y1S32 = (s32)(floorf(y1));
      const s32 x0S32 = (s32)(floorf(x0));

      const u8 * restrict pReference_y0 = &pReference[y0S32*referenceStride + x0S32];
      const u8 * restrict pReference_y1 = &pReference[y1S32*referenceStride + x0S32];

      const f32 pixelTL = *pReference_y0;
      const f32 pixelTR = *(pReference_y0+1);
      const f32 pixelBL = *pReference_y1;
      const f32 pixelBR = *(pReference_y1+1);

      f32 interpolatedPixel;
      InterpolateBilinear2d(pixelTL, pixelTR, pixelBL, pixelBR, alphaY, alphaYinverse, alphaX, alphaXinverse, interpolatedPixel);

      pOut[x] = interpolatedPixel;
    }
  } // for(s32 x=0; x<xIterationMax; x++)

#endif // INNER_LOOP_VERSION
}
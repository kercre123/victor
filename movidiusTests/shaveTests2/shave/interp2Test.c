#include "shaveShared.h"
#include <math.h>

extern int xIterationMax;
extern int referenceStride;
extern float xReferenceMax;
extern float yReferenceMax;

#define InterpolateBilinear2d(pixelTL, pixelTR, pixelBL, pixelBR, alphaY, alphaYinverse, alphaX, alphaXinverse, interpolatedPixel)\
{\
  const float interpolatedTop = alphaXinverse*pixelTL + alphaX*pixelTR;\
  const float interpolatedBottom = alphaXinverse*pixelBL + alphaX*pixelBR;\
  interpolatedPixel = alphaYinverse*interpolatedTop + alphaY*interpolatedBottom;\
}

void testInterp2_reference()
{
  const int stride4 = referenceStride >> 2;
  const float invalidValue = -2.0f;

  int x;
  for(x=0; x<xIterationMax; x++) {
    const float curX = pXCoordinates[x];
    const float curY = pYCoordinates[x];

    const float x0 = floorf(curX);
    const float x1 = ceilf(curX); // x0 + 1.0f;

    const float y0 = floorf(curY);
    const float y1 = ceilf(curY); // y0 + 1.0f;

    // If out of bounds, set as invalid and continue
    if(x0 < 0.0f || x1 > xReferenceMax || y0 < 0.0f || y1 > yReferenceMax) {
      pOut[x] = invalidValue;
      continue;
    }

    {
      const float alphaX = curX - x0;
      const float alphaXinverse = 1 - alphaX;

      const float alphaY = curY - y0;
      const float alphaYinverse = 1.0f - alphaY;

      const int y0S32 = (int)(floorf(y0));
      const int y1S32 = (int)(floorf(y1));
      const int x0S32 = (int)(floorf(x0));

      const unsigned char * restrict pReference_y0 = &interpolationReferenceImage[y0S32*stride4 + x0S32];
      const unsigned char * restrict pReference_y1 = &interpolationReferenceImage[y1S32*stride4 + x0S32];

      const float pixelTL = *pReference_y0;
      const float pixelTR = *(pReference_y0+1);
      const float pixelBL = *pReference_y1;
      const float pixelBR = *(pReference_y1+1);

      float interpolatedPixel;
      InterpolateBilinear2d(pixelTL, pixelTR, pixelBL, pixelBR, alphaY, alphaYinverse, alphaX, alphaXinverse, interpolatedPixel);

      pOut[x] = interpolatedPixel;
    }
  } // for(int x=0; x<xIterationMax; x++)
}

void testInterp2_shave()
{
  //interp2_shaveInnerLoop(
  //  const float * restrict pXCoordinates, const float * restrict pYCoordinates,
  //  const float bufferWidth,
  //  const float * restrict reference, const int referenceStride,
  //  const float xReferenceMax, const float yReferenceMax,
  //  float * restrict pOut);
}
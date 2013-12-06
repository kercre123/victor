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
  const s32 xReferenceMax, const s32 yReferenceMax,
  f32 * restrict pOut)
{
  const f32 invalidValue = -2.0f;

#if INNER_LOOP_VERSION == 1
  const f32 xReferenceMaxF32 = (f32)xReferenceMax;
  const f32 yReferenceMaxF32 = (f32)yReferenceMax;

  s32 x;
  for(x=0; x<numCoordinates; x++) {
    const f32 curX = pXCoordinates[x];
    const f32 curY = pYCoordinates[x];

    const f32 x0 = floorf(curX);
    const f32 x1 = x0 + 1.0f; // ceilf(curX)

    const f32 y0 = floorf(curY);
    const f32 y1 = y0 + 1.0f; // ceilf(curY)

    // If out of bounds, set as invalid and continue
    if(x0 < 0.0f || x1 > xReferenceMaxF32 || y0 < 0.0f || y1 > yReferenceMaxF32) {
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

#elif INNER_LOOP_VERSION == 2
  __asm(
  ".set pXCoordinates_address i0 \n"
    ".set pYCoordinates_address i1 \n"
    ".set pOut_address i2 \n"
    ".set zero i3 \n"
    ".set xReferenceMax i4 \n"
    ".set yReferenceMax i5 \n"
    "nop 5 \n"
    "cmu.cpii pXCoordinates_address %0 \n"
    "cmu.cpii pYCoordinates_address %1 \n"
    "cmu.cpii pOut_address %2 \n"
    "cmu.cpii xReferenceMax %3 \n"
    "cmu.cpii yReferenceMax %4 \n"
    "iau.sub zero zero zero \n"
    : //Output registers
  :"r"(&pXCoordinates[0]), "r"(&pYCoordinates[0]), "r"(&pOut[0]), "r"(&xReferenceMax), "r"(&yReferenceMax) //Input registers
    :"i0", "i1", "i2", "i3", "i4" //Clobbered registers
    );

  s32 x;
  for(x=0; x<numCoordinates; x++) {
    const f32 curX = pXCoordinates[x];
    const f32 curY = pYCoordinates[x];

    __asm(
    ".set curX s5 \n"
      ".set curY s6 \n"
      ".set x0 i10 \n"
      ".set x1 i11 \n"
      ".set y0 i12 \n"
      ".set y1 i13 \n"
      "lsu0.ldi32 curX pXCoordinates_address || lsu1.ldi32 curY pYCoordinates_address \n"
      "nop 5 \n"
      "cmu.cpsi.f32.i32s x0 curX \n"
      "cmu.cpsi.f32.i32s y0 curY \n"
      "iau.add x1 x0 1 \n"
      "iau.add y1 y0 1 \n"

      "cmu.cmii.i32 x numPixelsToProcess "
      "peu.pc1c lt || bru.bra _filteringLoop \n"

      "cmu.cmii.i32 x0 zero \n"

      "cmu.clamp.i32 x0 xReferenceMax \n"
      "cmu.clamp.i32 x0 xReferenceMax \n"

      "peu.oracc || \n"

      if(x0 < 0.0f || x1 > xReferenceMax || y0 < 0.0f || y1 > yReferenceMax) {
        pOut[x] = invalidValue;
        continue;
      }

      "vau.alignvec integralImage_01 integralImage_01a integralImage_01b 4 \n"
        "vau.alignvec integralImage_11 integralImage_11a integralImage_11b 4 \n"
        "nop 1 \n"
        "vau.sub.i32 output integralImage_11 integralImage_10\n" //output[x] = integralImage_11[x] - integralImage_10[x] + integralImage_00[x] - integralImage_01[x];
        "nop 1 \n"
        "vau.add.i32 output output integralImage_00 \n"
        "nop 1 \n"
        "vau.sub.i32 output output integralImage_01 \n"
        "nop 1 \n"
        "lsu0.stxv output output_address \n"
        "iau.add output_address output_address 16 \n \n"
        : //Output registers
      ://Input registers
      :"i5", "i6", "i7", "i10", "i11", "i12", "i13", "s5", "s6"  //Clobbered registers
        );

      {
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
      }
  } // for(s32 x=0; x<xIterationMax; x++)

#endif // INNER_LOOP_VERSION
}
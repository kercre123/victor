#ifndef _SHAVE_KERNELS_H_
#define _SHAVE_KERNELS_H_

#include "mv_types.h"

#ifndef f32
#define f32 float
#endif

#ifndef f64
#define f64 double
#endif

void ScrollingIntegralImage_u8_s32_FilterRow_shaveInnerLoop(
  const s32 * restrict integralImage_00,
  const s32 * restrict integralImage_01,
  const s32 * restrict integralImage_10,
  const s32 * restrict integralImage_11,
  const s32 numPixelsToProcess,
  s32 * restrict output);

void ScrollingIntegralImage_u8_s32_ComputeIntegralImageRow_nthRow(
  const u8 * restrict paddedImage_currentRow,
  const s32 * restrict integralImage_previousRow,
  s32 * restrict integralImage_currentRow,
  const s32 integralImageWidth);

void interp2_shaveInnerLoop(
  const f32 * restrict pXCoordinates, const f32 * restrict pYCoordinates,
  const s32 numCoordinates,
  const u8 * restrict pReference, const int referenceStride,
  const s32 xReferenceMax, const s32 yReferenceMax,
  f32 * restrict pOut);

void predicateTests();

#endif // #ifndef _SHAVE_KERNELS_H_
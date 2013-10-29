#include "shaveShared.h"

void testIntegralImage() {
  const int filterHalfWidth = 2;

  const int * restrict integralImage_00 = &integralImage[0];
  const int * restrict integralImage_01 = &integralImage[5];
  const int * restrict integralImage_10 = &integralImage[5*IMAGE_HEIGHT + 0];
  const int * restrict integralImage_11 = &integralImage[5*IMAGE_HEIGHT + 5];

  const int minXSimd = 0;
  const int maxXSimd = IMAGE_WIDTH - 2*filterHalfWidth - 1;

  int * restrict output = &outputLine[0];

  ScrollingIntegralImage_u8_s32_FilterRow_shaveInnerLoop(integralImage_00, integralImage_01, integralImage_10, integralImage_11, maxXSimd-minXSimd, output);
}
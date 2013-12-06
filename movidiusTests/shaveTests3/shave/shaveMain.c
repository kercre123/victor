#include <stdio.h>
#include <math.h>
#include <moviVectorUtils.h>
#include "shaveKernels.h"
#include "shaveData.h"

s32 xIterationMax = NUM_COORDINATES;
s32 referenceStride = IMAGE_WIDTH << 2;
f32 xReferenceMax = (f32)IMAGE_WIDTH;
f32 yReferenceMax = (f32)IMAGE_HEIGHT;

s32 testIntegralImageFiltering()
{
  s32 i;
  s32 testPassed = 1;

  const int filterHalfWidth = 2;

  const s32 * restrict integralImage_00 = &integralImage[0];
  const s32 * restrict integralImage_01 = &integralImage[5];
  const s32 * restrict integralImage_10 = &integralImage[5*IMAGE_HEIGHT + 0];
  const s32 * restrict integralImage_11 = &integralImage[5*IMAGE_HEIGHT + 5];

  const s32 minXSimd = 0;
  const s32 maxXSimd = IMAGE_WIDTH - 2*filterHalfWidth - 1;

  s32 * restrict output = &outputLine[0];

  ScrollingIntegralImage_u8_s32_FilterRow_shaveInnerLoop(integralImage_00, integralImage_01, integralImage_10, integralImage_11, maxXSimd-minXSimd, output);

  printf("Starting tests...\n");
  for(i=0; i<59; i++) {
    if(outputLine[i] != filterOutput_groundTruth[i]) {
      testPassed = 0;
      printf("Fail at %d (%d != %d)\n", (int)i, (int)outputLine[i], (int)filterOutput_groundTruth[i]);
      //break;
    }
  }

  // This should fail at i==59, because the shave is performing 4-way simd, and writing too far
  for(i=59; i<60; i++) {
    if(outputLine[i] == filterOutput_groundTruth[i]) {
      testPassed = 0;
      printf("Fail at %d (%d != %d)\n", (int)i, (int)outputLine[i], (int)filterOutput_groundTruth[i]);
      //break;
    }
  }
  printf("Tests completed\n");

  for(i=0; i<IMAGE_WIDTH; i+=5) {
    printf("%d) correct:%d shave:%d\n", (int)i, (int)filterOutput_groundTruth[i], (int)outputLine[i]);
  }

  return testPassed;
}

int testIntegralImageGeneration()
{
  s32 testPassed = 1;
  s32 x, i;

  for(x=0; x<IMAGE_WIDTH; x++) {
    paddedImageRow[x] = x+1;
    integralImage[x+IMAGE_WIDTH] = 1;
  }

  ScrollingIntegralImage_u8_s32_ComputeIntegralImageRow_nthRow(paddedImageRow, integralImage, integralImage+IMAGE_WIDTH, 61);

  printf("Starting tests...\n");
  for(i=0; i<61; i++) {
    if(integralImage[i+IMAGE_WIDTH] != integralImage_groundTruth[i]) {
      testPassed = 0;
      printf("Fail at %d (%d != %d)\n", (int)i, (int)integralImage[i+IMAGE_WIDTH], (int)integralImage_groundTruth[i]);
      //break;
    }
  }
  printf("Tests completed\n");

  for(i=0; i<IMAGE_WIDTH; i+=7) {
    printf("%d) correct:%d shave:%d\n", (int)i, (int)integralImage_groundTruth[i], (int)integralImage[i+IMAGE_WIDTH]);
  }

  return testPassed;
}

s32 testsInterp2()
{
  s32 i;
  s32 testPassed = 1;

  const f32 xReferenceMax = (f32)(IMAGE_WIDTH) - 1.0f;
  const f32 yReferenceMax = (f32)(IMAGE_HEIGHT) - 1.0f;

  for(i=0; i<NUM_COORDINATES; i++) {
    pOut[i] = -1.0f;
  }

  interp2_shaveInnerLoop(
    pXCoordinates, pYCoordinates,
    NUM_COORDINATES,
    &interpolationReferenceImage[0], IMAGE_WIDTH*sizeof(u8),
    xReferenceMax,
    yReferenceMax,
    pOut);

  printf("Starting tests...\n");
  for(i=0; i<NUM_COORDINATES; i++) {
    if(fabs(pOut_groundTruth[i]-pOut[i]) > 1e-3) {
      testPassed = 0;
      printf("Fail at %d (%f != %f)\n", (int)i, pOut_groundTruth[i], pOut[i]);
      //break;
    }
  }
  printf("Tests completed\n");

  for(i=0; i<NUM_COORDINATES; i+=3) {
    printf("%d) correct:%f shave:%f\n", (int)i, pOut_groundTruth[i], pOut[i]);
  }

  return testPassed;
}

int main( void )
{
  s32 testPassed;
  const s32 whichAlgorithm = 3;

  if(whichAlgorithm == 1) {
    testPassed = testIntegralImageFiltering();
  } else if(whichAlgorithm == 2) {
    testPassed = testIntegralImageGeneration();
  } else if (whichAlgorithm == 3) {
    testPassed = testsInterp2();
  } else if (whichAlgorithm == 4) {
    predicateTests();
    testPassed = 1;
  }

  if(testPassed)
    printf("\n\nShaveTests Executed Successfully\n\n");
  else
    printf("nShaveTests failed\n");

  return testPassed;
}
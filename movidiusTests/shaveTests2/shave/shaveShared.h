#ifndef _SHAVE_SHARED_H_
#define _SHAVE_SHARED_H_

#define IMAGE_HEIGHT 64
#define IMAGE_WIDTH 64

__attribute__ ((aligned (16))) extern volatile int outputLine[IMAGE_WIDTH];
__attribute__ ((aligned (16))) extern volatile unsigned char paddedImageRow[IMAGE_WIDTH];
__attribute__ ((aligned (16))) extern int integralImage[IMAGE_HEIGHT*IMAGE_WIDTH];

void testIntegralImageFiltering();

void testIntegralImageGeneration();

#endif // _SHAVE_SHARED_H_